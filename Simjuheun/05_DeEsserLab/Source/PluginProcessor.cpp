#include "PluginProcessor.h"
#include "PluginEditor.h"

DeEsserLabAudioProcessor::DeEsserLabAudioProcessor()
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
    listen_ = parameters.getRawParameterValue (listenId);
}

juce::AudioProcessorValueTreeState::ParameterLayout
DeEsserLabAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    juce::NormalisableRange<float> frequencyRange { 3000.0f, 12000.0f, 1.0f };
    frequencyRange.setSkewForCentre (7000.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { frequencyId, 1 }, "Detector Frequency",
        frequencyRange, 7000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { qId, 1 }, "Detector Q",
        juce::NormalisableRange<float> { 0.3f, 8.0f, 0.01f }, 1.2f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { thresholdId, 1 }, "Threshold",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f }, -30.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dBFS")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ratioId, 1 }, "Ratio",
        juce::NormalisableRange<float> { 1.0f, 20.0f, 0.1f }, 4.0f,
        juce::AudioParameterFloatAttributes().withLabel (":1")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { rangeId, 1 }, "Range",
        juce::NormalisableRange<float> { 0.0f, 18.0f, 0.1f }, 10.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    juce::NormalisableRange<float> attackRange { 0.1f, 50.0f, 0.1f };
    attackRange.setSkewForCentre (1.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { attackId, 1 }, "Attack", attackRange, 1.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));
    juce::NormalisableRange<float> releaseRange { 10.0f, 500.0f, 1.0f };
    releaseRange.setSkewForCentre (80.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { releaseId, 1 }, "Release", releaseRange, 80.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { listenId, 1 }, "Detector Listen", false));
    return layout;
}

const juce::String DeEsserLabAudioProcessor::getName() const { return JucePlugin_Name; }
void DeEsserLabAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    deEsser_.prepare (sampleRate, getTotalNumInputChannels());
}
void DeEsserLabAudioProcessor::releaseResources() {}
bool DeEsserLabAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono()
        && output != juce::AudioChannelSet::stereo())
        return false;
    return output == layouts.getMainInputChannelSet();
}

void DeEsserLabAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);
    const auto inputs = getTotalNumInputChannels();
    const auto outputs = getTotalNumOutputChannels();
    for (auto channel = inputs; channel < outputs; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    deesser_lab::WideBandDeEsserParameters p;
    p.detectorFrequencyHz = frequency_->load (std::memory_order_relaxed);
    p.detectorQ = q_->load (std::memory_order_relaxed);
    p.thresholdDb = threshold_->load (std::memory_order_relaxed);
    p.ratio = ratio_->load (std::memory_order_relaxed);
    p.maxReductionDb = range_->load (std::memory_order_relaxed);
    p.attackMs = attack_->load (std::memory_order_relaxed);
    p.releaseMs = release_->load (std::memory_order_relaxed);
    p.detectorListen = listen_->load (std::memory_order_relaxed) >= 0.5f;
    deEsser_.setParameters (p);

    std::array<float, deesser_lab::WideBandDeEsserDsp::maxChannels> frame {};
    const auto channels = std::min (inputs, deesser_lab::WideBandDeEsserDsp::maxChannels);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        for (int channel = 0; channel < channels; ++channel)
            frame[static_cast<std::size_t> (channel)] = buffer.getSample (channel, sample);
        deEsser_.processSampleFrame (frame.data(), channels);
        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample (channel, sample, frame[static_cast<std::size_t> (channel)]);
    }

    detectorDb_.store (deEsser_.detectorDb(), std::memory_order_relaxed);
    reductionDb_.store (deEsser_.reductionDb(), std::memory_order_relaxed);
}

bool DeEsserLabAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* DeEsserLabAudioProcessor::createEditor()
{
    return new DeEsserLabAudioProcessorEditor (*this);
}
bool DeEsserLabAudioProcessor::acceptsMidi() const { return false; }
bool DeEsserLabAudioProcessor::producesMidi() const { return false; }
bool DeEsserLabAudioProcessor::isMidiEffect() const { return false; }
double DeEsserLabAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int DeEsserLabAudioProcessor::getNumPrograms() { return 1; }
int DeEsserLabAudioProcessor::getCurrentProgram() { return 0; }
void DeEsserLabAudioProcessor::setCurrentProgram (int) {}
const juce::String DeEsserLabAudioProcessor::getProgramName (int) { return {}; }
void DeEsserLabAudioProcessor::changeProgramName (int, const juce::String&) {}
void DeEsserLabAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    if (const auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destinationData);
}
void DeEsserLabAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DeEsserLabAudioProcessor();
}
