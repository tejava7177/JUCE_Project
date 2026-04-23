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
    
    juce::Slider gainSlider;
    juce::Label rmsLabel;
    juce::Label peakLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleGainPluginAudioProcessorEditor)
};
