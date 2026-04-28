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
    void handleApplyCommand();
    void handleSendPrompt();
    void setPromptStatus (const juce::String& newStatus, juce::Colour colour = juce::Colours::white);
    void setPromptExplanation (const juce::String& newExplanation);

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
    juce::TextEditor commandEditor;
    juce::TextButton applyButton;
    juce::Label commandStatusLabel;
    juce::Label promptLabel;
    juce::TextEditor promptEditor;
    juce::TextButton sendButton;
    juce::Label promptStatusLabel;
    juce::Label promptExplanationLabel;
    SensorBar rmsBar;
    SensorBar peakBar;
    SensorBar silenceRatioBar;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::atomic<bool> promptRequestInFlight { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleGainPluginAudioProcessorEditor)
};
