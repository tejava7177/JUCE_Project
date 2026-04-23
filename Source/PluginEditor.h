/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class SensorBar  : public juce::Component
{
public:
    enum class Mode
    {
        Decibel,
        Ratio
    };

    void setDecibelValue (float newValueDb);
    void setRatioValue (float newRatio);
    void setMode (Mode newMode);

    void paint (juce::Graphics& g) override;

private:
    Mode mode { Mode::Decibel };
    float value { 0.0f };
};

class SimpleGainPluginAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    SimpleGainPluginAudioProcessorEditor (SimpleGainPluginAudioProcessor&);
    ~SimpleGainPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleGainPluginAudioProcessor& audioProcessor;
    
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label rmsLabel;
    juce::Label peakLabel;
    juce::Label crestFactorLabel;
    juce::Label clipCountLabel;
    juce::Label silenceRatioLabel;
    juce::Label gainLabel;
    juce::Slider gainSlider;
    SensorBar rmsBar;
    SensorBar peakBar;
    SensorBar silenceRatioBar;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleGainPluginAudioProcessorEditor)
};
