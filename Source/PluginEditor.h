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
    juce::Label rmsLabel;
    juce::Label peakLabel;
    juce::Label crestFactorLabel;
    juce::Label clipCountLabel;
    juce::Label silenceRatioLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleGainPluginAudioProcessorEditor)
};
