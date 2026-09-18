#include "PluginProcessor.h"
#include "PluginEditor.h"

EqLabAudioProcessor::EqLabAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters_ (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    frequencyHz_ = parameters_.getRawParameterValue (frequencyParameterId);
    gainDb_ = parameters_.getRawParameterValue (gainParameterId);
    q_ = parameters_.getRawParameterValue (qParameterId);
}

juce::AudioProcessorValueTreeState::ParameterLayout
EqLabAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // 사람의 주파수 인지는 로그에 가까우므로 1 kHz가 노브 중앙에 오도록 skew를 준다.
    juce::NormalisableRange<float> frequencyRange { 20.0f, 20000.0f, 1.0f };
    frequencyRange.setSkewForCentre (1000.0f);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { frequencyParameterId, 1 },
        "Frequency",
        frequencyRange,
        1000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { gainParameterId, 1 },
        "Gain",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.1f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { qParameterId, 1 },
        "Q",
        juce::NormalisableRange<float> { 0.1f, 10.0f, 0.01f },
        1.0f));

    return layout;
}

const juce::String EqLabAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

void EqLabAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    sampleRate_ = sampleRate;

    for (auto& filter : channelFilters_)
        filter.reset();
}

void EqLabAudioProcessor::releaseResources()
{
}

bool EqLabAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();

    if (output != juce::AudioChannelSet::mono()
        && output != juce::AudioChannelSet::stereo())
        return false;

    return output == layouts.getMainInputChannelSet();
}

void EqLabAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto inputChannels = getTotalNumInputChannels();
    const auto outputChannels = getTotalNumOutputChannels();

    for (auto channel = inputChannels; channel < outputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    // 사용자가 조절한 세 값과 현재 Sample Rate로 Bell EQ 계수를 만든다.
    // 계수는 블록마다 한 번 계산하고, 블록 안의 모든 샘플에 재사용한다.
    const auto coefficients = eqlab::BiquadBellDsp::makeBellCoefficients (
        sampleRate_,
        frequencyHz_->load (std::memory_order_relaxed),
        gainDb_->load (std::memory_order_relaxed),
        q_->load (std::memory_order_relaxed));

    for (auto channel = 0; channel < inputChannels; ++channel)
    {
        auto& filter = channelFilters_[static_cast<std::size_t> (channel)];
        filter.setCoefficients (coefficients);

        auto* samples = buffer.getWritePointer (channel);

        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
            samples[sample] = filter.processSample (samples[sample]);
    }
}

bool EqLabAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* EqLabAudioProcessor::createEditor()
{
    return new EqLabAudioProcessorEditor (*this);
}

bool EqLabAudioProcessor::acceptsMidi() const { return false; }
bool EqLabAudioProcessor::producesMidi() const { return false; }
bool EqLabAudioProcessor::isMidiEffect() const { return false; }
double EqLabAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int EqLabAudioProcessor::getNumPrograms() { return 1; }
int EqLabAudioProcessor::getCurrentProgram() { return 0; }
void EqLabAudioProcessor::setCurrentProgram (int) {}
const juce::String EqLabAudioProcessor::getProgramName (int) { return {}; }
void EqLabAudioProcessor::changeProgramName (int, const juce::String&) {}

void EqLabAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    const auto state = parameters_.copyState();
    const auto xml = state.createXml();
    copyXmlToBinary (*xml, destinationData);
}

void EqLabAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml != nullptr && xml->hasTagName (parameters_.state.getType()))
        parameters_.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EqLabAudioProcessor();
}
