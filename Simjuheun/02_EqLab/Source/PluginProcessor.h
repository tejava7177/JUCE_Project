#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/BiquadBellDsp.h"

#include <array>

class EqLabAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr auto frequencyParameterId = "frequency";
    static constexpr auto gainParameterId = "gain";
    static constexpr auto qParameterId = "q";

    EqLabAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& midiMessages) override;

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

    juce::AudioProcessorValueTreeState& parameters() { return parameters_; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState parameters_;
    std::atomic<float>* frequencyHz_ = nullptr;
    std::atomic<float>* gainDb_ = nullptr;
    std::atomic<float>* q_ = nullptr;

    double sampleRate_ = 44100.0;

    // 왼쪽과 오른쪽 채널은 서로 다른 과거 샘플을 기억해야 하므로 필터도 따로 둔다.
    std::array<eqlab::BiquadBellDsp, 2> channelFilters_;
};
