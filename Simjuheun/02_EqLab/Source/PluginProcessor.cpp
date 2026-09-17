#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/PassthroughDsp.h"

EqLabAudioProcessor::EqLabAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

const juce::String EqLabAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

void EqLabAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // 다음 단계에서 IIR/FIR 필터의 sample rate와 내부 상태를 여기서 준비한다.
    juce::ignoreUnused (sampleRate, samplesPerBlock);
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

    // 현재 단계에서는 모든 샘플이 그대로 통과한다.
    // 다음 단계에서 이 한 줄이 IIR biquad EQ 호출로 바뀐다.
    for (auto channel = 0; channel < inputChannels; ++channel)
    {
        auto* samples = buffer.getWritePointer (channel);

        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
            samples[sample] = eqlab::PassthroughDsp::processSample (samples[sample]);
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
    juce::ignoreUnused (destinationData);
}

void EqLabAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EqLabAudioProcessor();
}
