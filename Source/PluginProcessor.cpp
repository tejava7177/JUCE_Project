/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleGainPluginAudioProcessor::SimpleGainPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
     , apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

SimpleGainPluginAudioProcessor::~SimpleGainPluginAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleGainPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        "gain_db",
        "Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f));

    return { parameters.begin(), parameters.end() };
}

float SimpleGainPluginAudioProcessor::getRmsDb() const          { return rmsDb.load(); }
float SimpleGainPluginAudioProcessor::getPeakDb() const         { return peakDb.load(); }
float SimpleGainPluginAudioProcessor::getCrestFactorDb() const  { return crestFactorDb.load(); }
int SimpleGainPluginAudioProcessor::getClipCount() const        { return clipCount.load(); }
float SimpleGainPluginAudioProcessor::getSilenceRatio() const   { return silenceRatio.load(); }

//==============================================================================
const juce::String SimpleGainPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleGainPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleGainPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleGainPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleGainPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleGainPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleGainPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleGainPluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleGainPluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleGainPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleGainPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void SimpleGainPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleGainPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleGainPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    auto gainDb = apvts.getRawParameterValue ("gain_db")->load();
    auto gain = juce::Decibels::decibelsToGain (gainDb);

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        buffer.applyGain (channel, 0, numSamples, gain);

    auto sumOfSquares = 0.0f;
    auto peak = 0.0f;
    auto clips = 0;
    auto silentSamples = 0;

    constexpr auto clipThreshold = 1.0f;
    constexpr auto silenceThreshold = 0.001f;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getReadPointer (channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            auto sampleValue = channelData[sample];
            auto absoluteSampleValue = std::abs (sampleValue);

            sumOfSquares += sampleValue * sampleValue;
            peak = juce::jmax (peak, absoluteSampleValue);

            if (absoluteSampleValue >= clipThreshold)
                ++clips;

            if (absoluteSampleValue < silenceThreshold)
                ++silentSamples;
        }
    }

    auto numberOfSamples = totalNumInputChannels * numSamples;
    auto rms = numberOfSamples > 0 ? std::sqrt (sumOfSquares / static_cast<float> (numberOfSamples)) : 0.0f;
    auto newRmsDb = juce::Decibels::gainToDecibels (rms, -100.0f);
    auto newPeakDb = juce::Decibels::gainToDecibels (peak, -100.0f);
    auto newSilenceRatio = numberOfSamples > 0
        ? static_cast<float> (silentSamples) / static_cast<float> (numberOfSamples)
        : 1.0f;

    rmsDb.store (newRmsDb);
    peakDb.store (newPeakDb);
    crestFactorDb.store (newPeakDb - newRmsDb);
    clipCount.store (clips);
    silenceRatio.store (newSilenceRatio);
}

//==============================================================================
bool SimpleGainPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleGainPluginAudioProcessor::createEditor()
{
    return new SimpleGainPluginAudioProcessorEditor (*this);
}

//==============================================================================
void SimpleGainPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SimpleGainPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xmlState);

        if (state.isValid())
            apvts.replaceState (state);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleGainPluginAudioProcessor();
}
