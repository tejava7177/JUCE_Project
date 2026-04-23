/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void SensorBar::setDecibelValue (float newValueDb)
{
    value = juce::jlimit (-60.0f, 0.0f, newValueDb);
    mode = Mode::Decibel;
    repaint();
}

void SensorBar::setRatioValue (float newRatio)
{
    value = juce::jlimit (0.0f, 1.0f, newRatio);
    mode = Mode::Ratio;
    repaint();
}

void SensorBar::setMode (Mode newMode)
{
    mode = newMode;
    repaint();
}

void SensorBar::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colours::darkgrey);
    g.fillRoundedRectangle (bounds, 3.0f);

    auto normalisedValue = mode == Mode::Decibel
        ? juce::jmap (value, -60.0f, 0.0f, 0.0f, 1.0f)
        : value;

    auto fillBounds = bounds.reduced (2.0f);
    fillBounds.setWidth (fillBounds.getWidth() * normalisedValue);

    if (mode == Mode::Ratio)
    {
        g.setColour (juce::Colours::cornflowerblue);
    }
    else if (value >= -1.0f)
    {
        g.setColour (juce::Colours::red);
    }
    else if (value >= -6.0f)
    {
        g.setColour (juce::Colours::orange);
    }
    else
    {
        g.setColour (juce::Colours::green);
    }

    g.fillRoundedRectangle (fillBounds, 2.0f);
}

//==============================================================================
SimpleGainPluginAudioProcessorEditor::SimpleGainPluginAudioProcessorEditor (SimpleGainPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    titleLabel.setText ("Track Sensor", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

    statusLabel.setText ("Healthy", juce::dontSendNotification);
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);

    for (auto* label : { &rmsLabel, &peakLabel, &crestFactorLabel, &clipCountLabel, &silenceRatioLabel })
    {
        label->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*label);
    }

    rmsBar.setMode (SensorBar::Mode::Decibel);
    peakBar.setMode (SensorBar::Mode::Decibel);
    silenceRatioBar.setMode (SensorBar::Mode::Ratio);
    addAndMakeVisible (rmsBar);
    addAndMakeVisible (peakBar);
    addAndMakeVisible (silenceRatioBar);

    setSize (420, 300);
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
    statusLabel.setBounds (bounds.removeFromTop (30));
    bounds.removeFromTop (8);

    rmsLabel.setBounds (bounds.removeFromTop (24));
    rmsBar.setBounds (bounds.removeFromTop (16));
    bounds.removeFromTop (8);

    peakLabel.setBounds (bounds.removeFromTop (24));
    peakBar.setBounds (bounds.removeFromTop (16));
    bounds.removeFromTop (8);

    crestFactorLabel.setBounds (bounds.removeFromTop (28));
    clipCountLabel.setBounds (bounds.removeFromTop (28));
    silenceRatioLabel.setBounds (bounds.removeFromTop (28));
    silenceRatioBar.setBounds (bounds.removeFromTop (16));
}

void SimpleGainPluginAudioProcessorEditor::timerCallback()
{
    auto rms = audioProcessor.getRmsDb();
    auto peak = audioProcessor.getPeakDb();
    auto crestFactor = audioProcessor.getCrestFactorDb();
    auto clips = audioProcessor.getClipCount();
    auto silence = audioProcessor.getSilenceRatio();

    rmsLabel.setText ("RMS Level: "
                          + juce::String (rms, 1)
                          + " dB",
                      juce::dontSendNotification);

    peakLabel.setText ("Peak Level: "
                           + juce::String (peak, 1)
                           + " dB",
                       juce::dontSendNotification);

    crestFactorLabel.setText ("Crest Factor: "
                                  + juce::String (crestFactor, 1)
                                  + " dB",
                              juce::dontSendNotification);

    clipCountLabel.setText ("Clip Count: "
                                + juce::String (clips),
                            juce::dontSendNotification);
    clipCountLabel.setColour (juce::Label::textColourId, clips > 0 ? juce::Colours::red : juce::Colours::white);

    silenceRatioLabel.setText ("Silence Ratio: "
                                   + juce::String (silence * 100.0f, 1)
                                   + " %",
                               juce::dontSendNotification);

    rmsBar.setDecibelValue (rms);
    peakBar.setDecibelValue (peak);
    silenceRatioBar.setRatioValue (silence);

    juce::String status = "Healthy";

    if (clips > 0)
        status = "Clipping";
    else if (peak > -1.0f)
        status = "Hot";
    else if (silence > 0.7f)
        status = "Mostly Silent";
    else if (crestFactor > 10.0f)
        status = "Dynamic";

    statusLabel.setText ("Status: " + status, juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, status == "Clipping" ? juce::Colours::red : juce::Colours::white);
}
