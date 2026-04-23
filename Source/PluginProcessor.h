/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class SimpleGainPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleGainPluginAudioProcessor();
    ~SimpleGainPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getRmsLevelDb() const;
    float getPeakLevelDb() const;

    // 플러그인 파라미터를 호스트와 함께 관리하는 상태 객체입니다.
    // 이후 UI 슬라이더를 붙일 때도 이 객체를 기준으로 연결합니다.
    juce::AudioProcessorValueTreeState apvts;

private:
    // 이 플러그인이 가지는 파라미터 목록을 한 곳에서 정의합니다.
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // 오디오 스레드에서 계산한 meter 값을 UI 스레드가 안전하게 읽을 수 있게 합니다.
    std::atomic<float> rmsLevelDb { -60.0f };
    std::atomic<float> peakLevelDb { -60.0f };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleGainPluginAudioProcessor)
};
