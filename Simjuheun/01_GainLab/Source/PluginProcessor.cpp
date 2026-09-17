#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/DryWetDsp.h"
#include "DSP/GainDsp.h"

GainLabAudioProcessor::GainLabAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters_ (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // DAW나 UI가 바꾸는 Gain 값을 오디오 스레드에서 안전하게 읽기 위한 포인터다.
    gainDb_ = parameters_.getRawParameterValue (gainParameterId);
    delayTimeMs_ = parameters_.getRawParameterValue (delayTimeParameterId);
    feedbackPercent_ = parameters_.getRawParameterValue (feedbackParameterId);
    mixPercent_ = parameters_.getRawParameterValue (mixParameterId);
}

juce::AudioProcessorValueTreeState::ParameterLayout
GainLabAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // 사용자가 보는 값은 dB다. 0 dB는 원래 크기, 음수는 감쇄, 양수는 증폭이다.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { gainParameterId, 1 },
        "Gain",
        juce::NormalisableRange<float> { -60.0f, 12.0f, 0.1f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    // Delay Time은 원본을 몇 ms 뒤에 다시 들려줄지 결정한다.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { delayTimeParameterId, 1 },
        "Delay Time",
        juce::NormalisableRange<float> { 1.0f, maximumDelayMilliseconds, 1.0f },
        250.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    // Feedback은 지연된 소리를 버퍼에 얼마나 다시 넣을지 결정한다.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { feedbackParameterId, 1 },
        "Feedback",
        juce::NormalisableRange<float> { 0.0f, 95.0f, 1.0f },
        35.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // 0%는 원본만, 100%는 Gain과 Delay 처리가 끝난 Wet만 출력한다.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { mixParameterId, 1 },
        "Mix",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f },
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    return layout;
}

const juce::String GainLabAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

void GainLabAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    // ms 단위의 smoothing 시간을 현재 sample rate에 맞는 샘플 개수로 변환한다.
    gainSmoother_.prepare (sampleRate, parameterSmoothingSeconds);
    delayTimeSmoother_.prepare (sampleRate, parameterSmoothingSeconds);
    feedbackSmoother_.prepare (sampleRate, parameterSmoothingSeconds);
    mixSmoother_.prepare (sampleRate, parameterSmoothingSeconds);

    // 오디오 처리 중에는 메모리를 만들지 않도록 재생 시작 전에 최대 1초 버퍼를 준비한다.
    delayDsp_.prepare (sampleRate,
                       maximumDelayMilliseconds,
                       getTotalNumInputChannels());

    // 플러그인을 처음 켰을 때 1.0에서 저장된 Gain까지 불필요하게 움직이지 않도록
    // 현재 파라미터 값으로 smoother의 시작점과 목표점을 함께 초기화한다.
    const auto initialGainDb = gainDb_->load (std::memory_order_relaxed);
    const auto initialLinearGain = gainlab::GainDsp::decibelsToLinear (initialGainDb);
    gainSmoother_.setCurrentAndTargetValue (initialLinearGain);

    const auto initialDelayTime = delayTimeMs_->load (std::memory_order_relaxed);
    delayTimeSmoother_.setCurrentAndTargetValue (initialDelayTime);

    const auto initialFeedbackPercent = feedbackPercent_->load (std::memory_order_relaxed);
    feedbackSmoother_.setCurrentAndTargetValue (initialFeedbackPercent / 100.0f);

    const auto initialMixPercent = mixPercent_->load (std::memory_order_relaxed);
    const auto initialMix = gainlab::DryWetDsp::percentToProportion (initialMixPercent);
    mixSmoother_.setCurrentAndTargetValue (initialMix);
}

void GainLabAudioProcessor::releaseResources()
{
    delayDsp_.reset();
}

bool GainLabAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();

    if (output != juce::AudioChannelSet::mono()
        && output != juce::AudioChannelSet::stereo())
        return false;

    return output == layouts.getMainInputChannelSet();
}

void GainLabAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto inputChannels = getTotalNumInputChannels();
    const auto outputChannels = getTotalNumOutputChannels();

    for (auto channel = inputChannels; channel < outputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    // 1. 노브에서 전달된 dB 값을 읽는다.
    //    예: -6.0206 dB는 진폭을 정확히 절반으로 만드는 값이다.
    const auto gainDb = gainDb_->load (std::memory_order_relaxed);

    // 2. 샘플에는 dB를 직접 곱할 수 없으므로 선형 배수로 변환한다.
    //    공식: linearGain = 10 ^ (dB / 20)
    const auto linearGain = gainlab::GainDsp::decibelsToLinear (gainDb);

    // UI의 0~100% 값을 DSP에서 사용하는 0.0~1.0 비율로 변환한다.
    const auto mixPercent = mixPercent_->load (std::memory_order_relaxed);
    const auto mix = gainlab::DryWetDsp::percentToProportion (mixPercent);

    const auto delayTime = delayTimeMs_->load (std::memory_order_relaxed);
    const auto feedbackPercent = feedbackPercent_->load (std::memory_order_relaxed);
    const auto feedback = feedbackPercent / 100.0f;

    // 새 값으로 즉시 점프하지 않고 20 ms 동안 이동하도록 목표만 전달한다.
    // smoother 자체는 멤버이므로 이전 processBlock()에서 진행한 위치를 기억한다.
    gainSmoother_.setTargetValue (linearGain);
    delayTimeSmoother_.setTargetValue (delayTime);
    feedbackSmoother_.setTargetValue (feedback);
    mixSmoother_.setTargetValue (mix);

    // 3. 한 샘플마다 smoother를 한 번 진행하고, 그 값을 모든 채널에 똑같이 적용한다.
    //    샘플 루프가 바깥에 있어야 좌우 채널의 Gain이 정확히 동일하게 움직인다.
    auto* channels = buffer.getArrayOfWritePointers();

    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto smoothedGain = gainSmoother_.getNextValue();
        const auto smoothedDelayTime = delayTimeSmoother_.getNextValue();
        const auto smoothedFeedback = feedbackSmoother_.getNextValue();
        const auto smoothedMix = mixSmoother_.getNextValue();
        const auto delayInSamples = delayDsp_.millisecondsToSamples (smoothedDelayTime);

        for (auto channel = 0; channel < inputChannels; ++channel)
        {
            // Gain을 적용하기 전에 원본 샘플을 보존해야 Dry 신호로 사용할 수 있다.
            const auto drySample = channels[channel][sample];
            // Gain으로 Wet 입력 레벨을 정한 뒤 Delay 버퍼에 넣는다.
            const auto delayInput = gainlab::GainDsp::processSample (drySample,
                                                                     smoothedGain);
            const auto wetSample = delayDsp_.processSample (channel,
                                                             delayInput,
                                                             delayInSamples,
                                                             smoothedFeedback);

            channels[channel][sample] = gainlab::DryWetDsp::mixSample (
                drySample, wetSample, smoothedMix);
        }

        // 모든 채널이 같은 시간 위치를 사용한 다음에 쓰기 위치를 한 샘플 진행한다.
        delayDsp_.advance();
    }
}

bool GainLabAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GainLabAudioProcessor::createEditor()
{
    return new GainLabAudioProcessorEditor (*this);
}

bool GainLabAudioProcessor::acceptsMidi() const  { return false; }
bool GainLabAudioProcessor::producesMidi() const { return false; }
bool GainLabAudioProcessor::isMidiEffect() const { return false; }
double GainLabAudioProcessor::getTailLengthSeconds() const { return 30.0; }

int GainLabAudioProcessor::getNumPrograms() { return 1; }
int GainLabAudioProcessor::getCurrentProgram() { return 0; }
void GainLabAudioProcessor::setCurrentProgram (int) {}
const juce::String GainLabAudioProcessor::getProgramName (int) { return {}; }
void GainLabAudioProcessor::changeProgramName (int, const juce::String&) {}

void GainLabAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    // Logic 프로젝트를 저장할 때 현재 Gain 값도 함께 저장한다.
    const auto state = parameters_.copyState();
    const auto xml = state.createXml();
    copyXmlToBinary (*xml, destinationData);
}

void GainLabAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Logic 프로젝트를 다시 열 때 저장했던 Gain 값을 복원한다.
    const auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml != nullptr && xml->hasTagName (parameters_.state.getType()))
        parameters_.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GainLabAudioProcessor();
}
