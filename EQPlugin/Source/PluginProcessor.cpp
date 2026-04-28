#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr auto outputFloorDb = -100.0f;

float mapEnergyToDisplay (float value)
{
    return juce::jlimit (0.0f, 1.0f, value);
}
}

EQPluginAudioProcessor::EQPluginAudioProcessor()
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

EQPluginAudioProcessor::~EQPluginAudioProcessor() = default;

juce::StringArray EQPluginAudioProcessor::getSlopeChoices()
{
    return { "12 dB/Oct", "24 dB/Oct", "36 dB/Oct", "48 dB/Oct" };
}

juce::AudioProcessorValueTreeState::ParameterLayout EQPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    auto frequencyRange = juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f);
    auto lowCutRange = juce::NormalisableRange<float> (20.0f, 2000.0f, 1.0f, 0.35f);
    auto highCutRange = juce::NormalisableRange<float> (200.0f, 20000.0f, 1.0f, 0.35f);
    auto gainRange = juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f);
    auto qRange = juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.4f);

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("lowcut_freq", "Low Cut Freq", lowCutRange, 30.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> ("lowcut_slope", "Low Cut Slope", getSlopeChoices(), 0));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("peak_freq", "Peak Freq", frequencyRange, 750.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("peak_gain", "Peak Gain", gainRange, 0.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("peak_q", "Peak Q", qRange, 1.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("highcut_freq", "High Cut Freq", highCutRange, 18000.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> ("highcut_slope", "High Cut Slope", getSlopeChoices(), 0));

    return { parameters.begin(), parameters.end() };
}

float EQPluginAudioProcessor::getOutputRmsDb() const      { return outputRmsDb.load(); }
float EQPluginAudioProcessor::getOutputPeakDb() const     { return outputPeakDb.load(); }
float EQPluginAudioProcessor::getLowBandBalance() const   { return lowBandBalance.load(); }
float EQPluginAudioProcessor::getMidBandBalance() const   { return midBandBalance.load(); }
float EQPluginAudioProcessor::getHighBandBalance() const  { return highBandBalance.load(); }

const juce::String EQPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EQPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EQPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EQPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EQPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EQPluginAudioProcessor::getNumPrograms()                         { return 1; }
int EQPluginAudioProcessor::getCurrentProgram()                      { return 0; }
void EQPluginAudioProcessor::setCurrentProgram (int)                 {}
const juce::String EQPluginAudioProcessor::getProgramName (int)      { return {}; }
void EQPluginAudioProcessor::changeProgramName (int, const juce::String&) {}

void EQPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 1;

    leftChain.prepare (spec);
    rightChain.prepare (spec);

    leftChain.reset();
    rightChain.reset();

    updateFilters();
}

void EQPluginAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EQPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

template <typename ChainType, typename CoefficientArray>
void EQPluginAudioProcessor::updateCutFilter (ChainType& chain, const CoefficientArray& coefficients, int slopeIndex)
{
    chain.template setBypassed<0> (true);
    chain.template setBypassed<1> (true);
    chain.template setBypassed<2> (true);
    chain.template setBypassed<3> (true);

    switch (slopeIndex)
    {
        case slope48:
            *chain.template get<3>().coefficients = *coefficients[3];
            chain.template setBypassed<3> (false);
            [[fallthrough]];
        case slope36:
            *chain.template get<2>().coefficients = *coefficients[2];
            chain.template setBypassed<2> (false);
            [[fallthrough]];
        case slope24:
            *chain.template get<1>().coefficients = *coefficients[1];
            chain.template setBypassed<1> (false);
            [[fallthrough]];
        case slope12:
            *chain.template get<0>().coefficients = *coefficients[0];
            chain.template setBypassed<0> (false);
            break;
        default:
            break;
    }
}

EQPluginAudioProcessor::CoefficientsPtr EQPluginAudioProcessor::makePeakCoefficients() const
{
    auto peakFreq = apvts.getRawParameterValue ("peak_freq")->load();
    auto peakGain = apvts.getRawParameterValue ("peak_gain")->load();
    auto peakQ = apvts.getRawParameterValue ("peak_q")->load();

    return Coefficients::makePeakFilter (getSampleRate(), peakFreq, peakQ, juce::Decibels::decibelsToGain (peakGain));
}

juce::ReferenceCountedArray<EQPluginAudioProcessor::Coefficients> EQPluginAudioProcessor::makeLowCutCoefficients() const
{
    auto frequency = apvts.getRawParameterValue ("lowcut_freq")->load();
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod (
        frequency,
        getSampleRate(),
        2 * (apvts.getRawParameterValue ("lowcut_slope")->load() + 1));
}

juce::ReferenceCountedArray<EQPluginAudioProcessor::Coefficients> EQPluginAudioProcessor::makeHighCutCoefficients() const
{
    auto frequency = apvts.getRawParameterValue ("highcut_freq")->load();
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod (
        frequency,
        getSampleRate(),
        2 * (apvts.getRawParameterValue ("highcut_slope")->load() + 1));
}

void EQPluginAudioProcessor::updatePeakFilter()
{
    auto peakCoefficients = makePeakCoefficients();
    *leftChain.get<peak>().coefficients = *peakCoefficients;
    *rightChain.get<peak>().coefficients = *peakCoefficients;
}

void EQPluginAudioProcessor::updateLowCutFilters()
{
    auto coefficients = makeLowCutCoefficients();
    auto slopeIndex = static_cast<int> (apvts.getRawParameterValue ("lowcut_slope")->load());
    updateCutFilter (leftChain.get<lowCut>(), coefficients, slopeIndex);
    updateCutFilter (rightChain.get<lowCut>(), coefficients, slopeIndex);
}

void EQPluginAudioProcessor::updateHighCutFilters()
{
    auto coefficients = makeHighCutCoefficients();
    auto slopeIndex = static_cast<int> (apvts.getRawParameterValue ("highcut_slope")->load());
    updateCutFilter (leftChain.get<highCut>(), coefficients, slopeIndex);
    updateCutFilter (rightChain.get<highCut>(), coefficients, slopeIndex);
}

void EQPluginAudioProcessor::updateFilters()
{
    if (getSampleRate() <= 0.0)
        return;

    updatePeakFilter();
    updateLowCutFilters();
    updateHighCutFilters();
}

float EQPluginAudioProcessor::getMagnitudeForFrequency (double frequency) const
{
    if (getSampleRate() <= 0.0)
        return 1.0f;

    auto magnitude = 1.0;

    auto lowCutCoefficients = makeLowCutCoefficients();
    auto highCutCoefficients = makeHighCutCoefficients();
    auto peakCoefficients = makePeakCoefficients();

    auto lowCutStages = static_cast<int> (apvts.getRawParameterValue ("lowcut_slope")->load()) + 1;
    auto highCutStages = static_cast<int> (apvts.getRawParameterValue ("highcut_slope")->load()) + 1;

    for (int stage = 0; stage < lowCutStages; ++stage)
        magnitude *= lowCutCoefficients[static_cast<size_t> (stage)]->getMagnitudeForFrequency (frequency, getSampleRate());

    magnitude *= peakCoefficients->getMagnitudeForFrequency (frequency, getSampleRate());

    for (int stage = 0; stage < highCutStages; ++stage)
        magnitude *= highCutCoefficients[static_cast<size_t> (stage)]->getMagnitudeForFrequency (frequency, getSampleRate());

    return static_cast<float> (magnitude);
}

void EQPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    updateFilters();

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    for (auto channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);

    juce::dsp::AudioBlock<float> block (buffer);

    if (totalNumInputChannels > 0)
    {
        auto leftBlock = block.getSingleChannelBlock (0);
        juce::dsp::ProcessContextReplacing<float> leftContext (leftBlock);
        leftChain.process (leftContext);
    }

    if (totalNumInputChannels > 1)
    {
        auto rightBlock = block.getSingleChannelBlock (1);
        juce::dsp::ProcessContextReplacing<float> rightContext (rightBlock);
        rightChain.process (rightContext);
    }

    auto sumSquares = 0.0f;
    auto peak = 0.0f;
    auto lowEnergy = 0.0f;
    auto midEnergy = 0.0f;
    auto highEnergy = 0.0f;

    const auto lowCutFreq = apvts.getRawParameterValue ("lowcut_freq")->load();
    const auto highCutFreq = apvts.getRawParameterValue ("highcut_freq")->load();
    const auto peakFreq = apvts.getRawParameterValue ("peak_freq")->load();
    const auto peakGain = apvts.getRawParameterValue ("peak_gain")->load();

    auto lowWeight = juce::jlimit (0.05f, 1.0f, 200.0f / juce::jmax (40.0f, lowCutFreq));
    auto highWeight = juce::jlimit (0.05f, 1.0f, juce::jmax (1000.0f, highCutFreq) / 12000.0f);
    auto midWeight = juce::jlimit (0.05f, 1.0f, 0.45f + std::abs (peakGain) / 24.0f * 0.55f);
    juce::ignoreUnused (peakFreq);

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getReadPointer (channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            auto sampleValue = channelData[sample];
            auto absoluteSampleValue = std::abs (sampleValue);

            sumSquares += sampleValue * sampleValue;
            peak = juce::jmax (peak, absoluteSampleValue);
            lowEnergy += absoluteSampleValue * lowWeight;
            midEnergy += absoluteSampleValue * midWeight;
            highEnergy += absoluteSampleValue * highWeight;
        }
    }

    auto numberOfSamples = juce::jmax (1, totalNumInputChannels * numSamples);
    auto rms = std::sqrt (sumSquares / static_cast<float> (numberOfSamples));
    auto combinedEnergy = juce::jmax (0.0001f, lowEnergy + midEnergy + highEnergy);

    outputRmsDb.store (juce::Decibels::gainToDecibels (rms, outputFloorDb));
    outputPeakDb.store (juce::Decibels::gainToDecibels (peak, outputFloorDb));
    lowBandBalance.store (mapEnergyToDisplay (lowEnergy / combinedEnergy));
    midBandBalance.store (mapEnergyToDisplay (midEnergy / combinedEnergy));
    highBandBalance.store (mapEnergyToDisplay (highEnergy / combinedEnergy));
}

bool EQPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* EQPluginAudioProcessor::createEditor()
{
    return new EQPluginAudioProcessorEditor (*this);
}

void EQPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void EQPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xmlState);

        if (state.isValid())
            apvts.replaceState (state);
    }

    updateFilters();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EQPluginAudioProcessor();
}
