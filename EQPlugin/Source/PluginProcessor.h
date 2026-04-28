#pragma once

#include <JuceHeader.h>

class EQPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    EQPluginAudioProcessor();
    ~EQPluginAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static juce::StringArray getSlopeChoices();

    float getMagnitudeForFrequency (double frequency) const;
    float getOutputRmsDb() const;
    float getOutputPeakDb() const;
    float getLowBandBalance() const;
    float getMidBandBalance() const;
    float getHighBandBalance() const;

    juce::AudioProcessorValueTreeState apvts;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    using CoefficientsPtr = Coefficients::Ptr;
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

    enum ChainPosition
    {
        lowCut,
        peak,
        highCut
    };

    enum Slope
    {
        slope12,
        slope24,
        slope36,
        slope48
    };

    template <typename ChainType, typename CoefficientArray>
    void updateCutFilter (ChainType& chain, const CoefficientArray& coefficients, int slopeIndex);

    void updateFilters();
    void updatePeakFilter();
    void updateLowCutFilters();
    void updateHighCutFilters();

    CoefficientsPtr makePeakCoefficients() const;
    juce::ReferenceCountedArray<Coefficients> makeLowCutCoefficients() const;
    juce::ReferenceCountedArray<Coefficients> makeHighCutCoefficients() const;

    MonoChain leftChain;
    MonoChain rightChain;

    std::atomic<float> outputRmsDb { -100.0f };
    std::atomic<float> outputPeakDb { -100.0f };
    std::atomic<float> lowBandBalance { 0.0f };
    std::atomic<float> midBandBalance { 0.0f };
    std::atomic<float> highBandBalance { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQPluginAudioProcessor)
};
