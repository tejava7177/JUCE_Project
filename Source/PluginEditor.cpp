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
    gainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    gainSlider.setTextValueSuffix (" dB");
    addAndMakeVisible (gainSlider);

    rmsLabel.setJustificationType (juce::Justification::centred);
    rmsLabel.setText ("RMS: -60.0 dB", juce::dontSendNotification);
    addAndMakeVisible (rmsLabel);

    peakLabel.setJustificationType (juce::Justification::centred);
    peakLabel.setText ("Peak: -60.0 dB", juce::dontSendNotification);
    addAndMakeVisible (peakLabel);

    // SliderAttachment가 슬라이더와 APVTS 파라미터를 양방향으로 동기화합니다.
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts,
        "gain_db",
        gainSlider);

    setSize (400, 300);
    startTimerHz (30);
}

SimpleGainPluginAudioProcessorEditor::~SimpleGainPluginAudioProcessorEditor()
{
}

//==============================================================================
void SimpleGainPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("Gain", 0, 20, getWidth(), 24, juce::Justification::centred, 1);
}

void SimpleGainPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (40);

    gainSlider.setBounds (bounds.removeFromTop (160).reduced (60, 0));
    rmsLabel.setBounds (bounds.removeFromTop (30));
    peakLabel.setBounds (bounds.removeFromTop (30));
}

void SimpleGainPluginAudioProcessorEditor::timerCallback()
{
    rmsLabel.setText ("RMS: "
                          + juce::String (audioProcessor.getRmsLevelDb(), 1)
                          + " dB",
                      juce::dontSendNotification);

    peakLabel.setText ("Peak: "
                           + juce::String (audioProcessor.getPeakLevelDb(), 1)
                           + " dB",
                       juce::dontSendNotification);
}
