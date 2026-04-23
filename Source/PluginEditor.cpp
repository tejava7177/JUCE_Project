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

    // SliderAttachment가 슬라이더와 APVTS 파라미터를 양방향으로 동기화합니다.
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts,
        "gain_db",
        gainSlider);

    setSize (400, 300);
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
    gainSlider.setBounds (getLocalBounds().reduced (100, 60));
}
