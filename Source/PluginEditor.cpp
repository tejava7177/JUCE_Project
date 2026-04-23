/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleGainPluginAudioProcessorEditor::SimpleGainPluginAudioProcessorEditor (SimpleGainPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    titleLabel.setText ("Track Sensor", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

    for (auto* label : { &rmsLabel, &peakLabel, &crestFactorLabel, &clipCountLabel, &silenceRatioLabel })
    {
        label->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*label);
    }

    setSize (360, 220);
    startTimerHz (20);
}

SimpleGainPluginAudioProcessorEditor::~SimpleGainPluginAudioProcessorEditor()
{
}

//==============================================================================
void SimpleGainPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
}

void SimpleGainPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (20);

    titleLabel.setBounds (bounds.removeFromTop (32));
    rmsLabel.setBounds (bounds.removeFromTop (28));
    peakLabel.setBounds (bounds.removeFromTop (28));
    crestFactorLabel.setBounds (bounds.removeFromTop (28));
    clipCountLabel.setBounds (bounds.removeFromTop (28));
    silenceRatioLabel.setBounds (bounds.removeFromTop (28));
}

void SimpleGainPluginAudioProcessorEditor::timerCallback()
{
    rmsLabel.setText ("RMS Level: "
                          + juce::String (audioProcessor.getRmsDb(), 1)
                          + " dB",
                      juce::dontSendNotification);

    peakLabel.setText ("Peak Level: "
                           + juce::String (audioProcessor.getPeakDb(), 1)
                           + " dB",
                       juce::dontSendNotification);

    crestFactorLabel.setText ("Crest Factor: "
                                  + juce::String (audioProcessor.getCrestFactorDb(), 1)
                                  + " dB",
                              juce::dontSendNotification);

    clipCountLabel.setText ("Clip Count: "
                                + juce::String (audioProcessor.getClipCount()),
                            juce::dontSendNotification);

    silenceRatioLabel.setText ("Silence Ratio: "
                                   + juce::String (audioProcessor.getSilenceRatio() * 100.0f, 1)
                                   + " %",
                               juce::dontSendNotification);
}
