#include "PluginProcessor.h"
#include "PluginEditor.h"

CompressorLabAudioProcessor::CompressorLabAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    threshold_ = parameters.getRawParameterValue (thresholdId);
    ratio_ = parameters.getRawParameterValue (ratioId);
    attack_ = parameters.getRawParameterValue (attackId);
    release_ = parameters.getRawParameterValue (releaseId);
    makeup_ = parameters.getRawParameterValue (makeupId);
}

juce::AudioProcessorValueTreeState::ParameterLayout
CompressorLabAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { thresholdId, 1 }, "Threshold",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f }, -18.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dBFS")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ratioId, 1 }, "Ratio",
        juce::NormalisableRange<float> { 1.0f, 20.0f, 0.1f }, 4.0f,
        juce::AudioParameterFloatAttributes().withLabel (":1")));

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

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { makeupId, 1 }, "Makeup Gain",
        juce::NormalisableRange<float> { -12.0f, 18.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return layout;
}

const juce::String CompressorLabAudioProcessor::getName() const { return JucePlugin_Name; }

void CompressorLabAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    compressor_.prepare (sampleRate, getTotalNumInputChannels());
}

void CompressorLabAudioProcessor::releaseResources() {}

bool CompressorLabAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono()
        && output != juce::AudioChannelSet::stereo())
        return false;
    return output == layouts.getMainInputChannelSet();
}

void CompressorLabAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto inputs = getTotalNumInputChannels();
    const auto outputs = getTotalNumOutputChannels();
    for (auto channel = inputs; channel < outputs; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    compressor_lab::CompressorParameters p;
    p.thresholdDb = threshold_->load (std::memory_order_relaxed);
    p.ratio = ratio_->load (std::memory_order_relaxed);
    p.attackMs = attack_->load (std::memory_order_relaxed);
    p.releaseMs = release_->load (std::memory_order_relaxed);
    p.makeupDb = makeup_->load (std::memory_order_relaxed);
    compressor_.setParameters (p);

    std::array<float, compressor_lab::CompressorDsp::maxChannels> frame {};
    const auto channels = std::min (inputs, compressor_lab::CompressorDsp::maxChannels);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        for (int channel = 0; channel < channels; ++channel)
            frame[static_cast<std::size_t> (channel)] = buffer.getSample (channel, sample);

        compressor_.processSampleFrame (frame.data(), channels);

        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample (channel, sample, frame[static_cast<std::size_t> (channel)]);
    }

    inputDb_.store (compressor_.inputDb(), std::memory_order_relaxed);
    reductionDb_.store (compressor_.reductionDb(), std::memory_order_relaxed);
}

bool CompressorLabAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* CompressorLabAudioProcessor::createEditor()
{
    return new CompressorLabAudioProcessorEditor (*this);
}
bool CompressorLabAudioProcessor::acceptsMidi() const { return false; }
bool CompressorLabAudioProcessor::producesMidi() const { return false; }
bool CompressorLabAudioProcessor::isMidiEffect() const { return false; }
double CompressorLabAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int CompressorLabAudioProcessor::getNumPrograms() { return 1; }
int CompressorLabAudioProcessor::getCurrentProgram() { return 0; }
void CompressorLabAudioProcessor::setCurrentProgram (int) {}
const juce::String CompressorLabAudioProcessor::getProgramName (int) { return {}; }
void CompressorLabAudioProcessor::changeProgramName (int, const juce::String&) {}

void CompressorLabAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    if (const auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destinationData);
}

void CompressorLabAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorLabAudioProcessor();
}
