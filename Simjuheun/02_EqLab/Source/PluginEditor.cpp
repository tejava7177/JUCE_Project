#include "PluginEditor.h"

EqLabAudioProcessorEditor::EqLabAudioProcessorEditor (EqLabAudioProcessor& audioProcessor)
    : AudioProcessorEditor (&audioProcessor)
{
    titleLabel_.setText ("EqLab", juce::dontSendNotification);
    titleLabel_.setJustificationType (juce::Justification::centred);
    titleLabel_.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    titleLabel_.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel_);

    subtitleLabel_.setText ("FIR / IIR LEARNING LAB", juce::dontSendNotification);
    subtitleLabel_.setJustificationType (juce::Justification::centred);
    subtitleLabel_.setColour (juce::Label::textColourId, juce::Colour (0xff9aa8b8));
    addAndMakeVisible (subtitleLabel_);

    // JUCE에 UTF-8 문자열임을 명시해야 한글 바이트가 깨지지 않는다.
    firLabel_.setText (juce::String::fromUTF8 (u8"FIR\n현재 + 과거 입력"),
                       juce::dontSendNotification);
    firLabel_.setJustificationType (juce::Justification::centred);
    firLabel_.setColour (juce::Label::textColourId, juce::Colour (0xff6fb4ff));
    addAndMakeVisible (firLabel_);

    iirLabel_.setText (juce::String::fromUTF8 (u8"IIR / BIQUAD\n입력 + 과거 출력"),
                       juce::dontSendNotification);
    iirLabel_.setJustificationType (juce::Justification::centred);
    iirLabel_.setColour (juce::Label::textColourId, juce::Colour (0xffffb467));
    addAndMakeVisible (iirLabel_);

    statusLabel_.setText ("PASS-THROUGH / EQ DSP NOT CONNECTED YET",
                          juce::dontSendNotification);
    statusLabel_.setJustificationType (juce::Justification::centred);
    statusLabel_.setColour (juce::Label::textColourId, juce::Colour (0xff7f8b99));
    addAndMakeVisible (statusLabel_);

    setSize (500, 300);
}

void EqLabAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour (0xff14191f));

    const auto panel = getLocalBounds().toFloat().reduced (14.0f);
    graphics.setColour (juce::Colour (0xff202832));
    graphics.fillRoundedRectangle (panel, 14.0f);
    graphics.setColour (juce::Colour (0xff35404c));
    graphics.drawRoundedRectangle (panel, 14.0f, 1.0f);

    const auto firPanel = juce::Rectangle<float> (45.0f, 118.0f, 190.0f, 82.0f);
    const auto iirPanel = juce::Rectangle<float> (265.0f, 118.0f, 190.0f, 82.0f);
    graphics.setColour (juce::Colour (0xff18212b));
    graphics.fillRoundedRectangle (firPanel, 10.0f);
    graphics.fillRoundedRectangle (iirPanel, 10.0f);
}

void EqLabAudioProcessorEditor::resized()
{
    titleLabel_.setBounds (30, 26, getWidth() - 60, 40);
    subtitleLabel_.setBounds (30, 70, getWidth() - 60, 22);
    firLabel_.setBounds (45, 118, 190, 82);
    iirLabel_.setBounds (265, 118, 190, 82);
    statusLabel_.setBounds (30, getHeight() - 48, getWidth() - 60, 22);
}
