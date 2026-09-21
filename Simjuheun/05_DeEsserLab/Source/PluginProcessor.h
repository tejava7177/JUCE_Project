#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/WideBandDeEsserDsp.h"

#include <array>
#include <atomic>

class DeEsserLabAudioProcessor final : public juce::AudioProcessor
{
public:
    DeEsserLabAudioProcessor();
    ~DeEsserLabAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
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
    void getStateInformation (juce::MemoryBlock& destinationData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState parameters;

    double currentDetectorDb() const noexcept { return detectorDb_.load (std::memory_order_relaxed); }
    double currentReductionDb() const noexcept { return reductionDb_.load (std::memory_order_relaxed); }

    static constexpr const char* frequencyId = "frequency";
    static constexpr const char* qId = "q";
    static constexpr const char* thresholdId = "threshold";
    static constexpr const char* ratioId = "ratio";
    static constexpr const char* rangeId = "range";
    static constexpr const char* attackId = "attack";
    static constexpr const char* releaseId = "release";
    static constexpr const char* listenId = "listen";

private:
    deesser_lab::WideBandDeEsserDsp deEsser_;
    std::atomic<float>* frequency_ = nullptr;
    std::atomic<float>* q_ = nullptr;
    std::atomic<float>* threshold_ = nullptr;
    std::atomic<float>* ratio_ = nullptr;
    std::atomic<float>* range_ = nullptr;
    std::atomic<float>* attack_ = nullptr;
    std::atomic<float>* release_ = nullptr;
    std::atomic<float>* listen_ = nullptr;
    std::atomic<double> detectorDb_ { -180.0 };
    std::atomic<double> reductionDb_ { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeEsserLabAudioProcessor)
};
