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
     // APVTS를 생성하면서 이 프로세서의 파라미터 레이아웃도 함께 등록합니다.
     , apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

SimpleGainPluginAudioProcessor::~SimpleGainPluginAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleGainPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // gain 파라미터를 추가합니다.
    // ID는 코드에서 사용할 내부 이름이고, "Gain"은 호스트/UI에 보일 이름입니다.
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        "gain",
        "Gain",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f),
        1.0f));

    return { parameters.begin(), parameters.end() };
}

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
    // 오디오 스레드에서 현재 gain 값을 읽습니다.
    auto gain = apvts.getRawParameterValue ("gain")->load();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // 각 샘플에 gain을 곱해서 실제 볼륨 변화를 만듭니다.
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            channelData[sample] *= gain;
    }
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
    // 현재 파라미터 상태를 저장해서,
    // DAW 프로젝트를 다시 열었을 때 같은 값으로 복원할 수 있게 합니다.
    if (auto state = apvts.copyState(); auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SimpleGainPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // 저장해 둔 파라미터 상태를 다시 읽어서 복원합니다.
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
