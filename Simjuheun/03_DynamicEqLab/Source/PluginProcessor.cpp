#include "PluginProcessor.h"
#include "PluginEditor.h"

DynamicEqLabAudioProcessor::DynamicEqLabAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    frequency_ = parameters.getRawParameterValue (frequencyId);
    q_ = parameters.getRawParameterValue (qId);
    threshold_ = parameters.getRawParameterValue (thresholdId);
    ratio_ = parameters.getRawParameterValue (ratioId);
    range_ = parameters.getRawParameterValue (rangeId);
    attack_ = parameters.getRawParameterValue (attackId);
    release_ = parameters.getRawParameterValue (releaseId);
}

juce::AudioProcessorValueTreeState::ParameterLayout
DynamicEqLabAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::NormalisableRange<float> frequencyRange { 20.0f, 20000.0f, 1.0f };
    frequencyRange.setSkewForCentre (1000.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { frequencyId, 1 }, "Frequency", frequencyRange, 1000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { qId, 1 }, "Q",
        juce::NormalisableRange<float> { 0.2f, 12.0f, 0.01f }, 2.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { thresholdId, 1 }, "Threshold",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f }, -24.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dBFS")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ratioId, 1 }, "Ratio",
        juce::NormalisableRange<float> { 1.0f, 10.0f, 0.1f }, 4.0f,
        juce::AudioParameterFloatAttributes().withLabel (":1")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { rangeId, 1 }, "Range",
        juce::NormalisableRange<float> { 0.0f, 18.0f, 0.1f }, 6.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    juce::NormalisableRange<float> attackRange { 0.1f, 200.0f, 0.1f };
    attackRange.setSkewForCentre (10.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { attackId, 1 }, "Attack", attackRange, 10.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    juce::NormalisableRange<float> releaseRange { 10.0f, 1000.0f, 1.0f };
    releaseRange.setSkewForCentre (120.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { releaseId, 1 }, "Release", releaseRange, 120.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    return layout;
}

const juce::String DynamicEqLabAudioProcessor::getName() const { return JucePlugin_Name; }

void DynamicEqLabAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    sampleRate_ = sampleRate;
    dynamicEq_.prepare (sampleRate, getTotalNumInputChannels());
}

void DynamicEqLabAudioProcessor::releaseResources() {}

bool DynamicEqLabAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono()
        && output != juce::AudioChannelSet::stereo())
        return false;

    return output == layouts.getMainInputChannelSet();
}

void DynamicEqLabAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto inputChannels = getTotalNumInputChannels();
    const auto outputChannels = getTotalNumOutputChannels();
    for (auto channel = inputChannels; channel < outputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    dynamic_eq_lab::DynamicBellParameters p;
    p.frequencyHz = frequency_->load (std::memory_order_relaxed);
    p.q = q_->load (std::memory_order_relaxed);
    p.thresholdDb = threshold_->load (std::memory_order_relaxed);
    p.ratio = ratio_->load (std::memory_order_relaxed);
    p.maxReductionDb = range_->load (std::memory_order_relaxed);
    p.attackMs = attack_->load (std::memory_order_relaxed);
    p.releaseMs = release_->load (std::memory_order_relaxed);
    dynamicEq_.setParameters (p);

    std::array<float, dynamic_eq_lab::DynamicBellDsp::maxChannels> frame {};
    const auto channels = std::min (inputChannels,
                                    dynamic_eq_lab::DynamicBellDsp::maxChannels);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        for (int channel = 0; channel < channels; ++channel)
            frame[static_cast<std::size_t> (channel)] = buffer.getSample (channel, sample);

        dynamicEq_.processSampleFrame (frame.data(), channels);

        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample (channel, sample, frame[static_cast<std::size_t> (channel)]);
    }

    detectorDb_.store (dynamicEq_.detectorDb(), std::memory_order_relaxed);
    reductionDb_.store (dynamicEq_.reductionDb(), std::memory_order_relaxed);
}

bool DynamicEqLabAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* DynamicEqLabAudioProcessor::createEditor()
{
    return new DynamicEqLabAudioProcessorEditor (*this);
}

bool DynamicEqLabAudioProcessor::acceptsMidi() const { return false; }
bool DynamicEqLabAudioProcessor::producesMidi() const { return false; }
bool DynamicEqLabAudioProcessor::isMidiEffect() const { return false; }
double DynamicEqLabAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int DynamicEqLabAudioProcessor::getNumPrograms() { return 1; }
int DynamicEqLabAudioProcessor::getCurrentProgram() { return 0; }
void DynamicEqLabAudioProcessor::setCurrentProgram (int) {}
const juce::String DynamicEqLabAudioProcessor::getProgramName (int) { return {}; }
void DynamicEqLabAudioProcessor::changeProgramName (int, const juce::String&) {}

void DynamicEqLabAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    if (const auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destinationData);
}

void DynamicEqLabAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DynamicEqLabAudioProcessor();
}
