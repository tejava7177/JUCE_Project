#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/LinearSmoother.h"

class GainLabAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr auto gainParameterId = "gain";
    static constexpr auto mixParameterId = "mix";

    GainLabAudioProcessor();

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

    static constexpr double parameterSmoothingSeconds = 0.020;

    juce::AudioProcessorValueTreeState parameters_;
    std::atomic<float>* gainDb_ = nullptr;
    std::atomic<float>* mixPercent_ = nullptr;

    // processBlock() 호출이 끝나도 현재 ramp 위치를 기억해야 하므로 멤버로 보관한다.
    // UI는 이 객체를 직접 만지지 않고, 오디오 스레드만 값을 진행시킨다.
    gainlab::LinearSmoother gainSmoother_;
    gainlab::LinearSmoother mixSmoother_;
};
