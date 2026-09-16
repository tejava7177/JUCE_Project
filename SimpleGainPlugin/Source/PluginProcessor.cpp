#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/GainDsp.h"

GainLabAudioProcessor::GainLabAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters_ (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // DAW나 UI가 바꾸는 Gain 값을 오디오 스레드에서 안전하게 읽기 위한 포인터다.
    gainDb_ = parameters_.getRawParameterValue (gainParameterId);
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

    return layout;
}

const juce::String GainLabAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

void GainLabAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void GainLabAudioProcessor::releaseResources()
{
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

    // 3. 각 채널의 모든 샘플에 같은 배수를 곱한다.
    //    getWritePointer()가 반환하는 주소는 DAW가 전달한 실제 샘플 메모리다.
    for (auto channel = 0; channel < inputChannels; ++channel)
    {
        auto* samples = buffer.getWritePointer (channel);
        gainlab::GainDsp::applyGain (samples, buffer.getNumSamples(), linearGain);
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
double GainLabAudioProcessor::getTailLengthSeconds() const { return 0.0; }

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
