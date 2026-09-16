#include "PluginEditor.h"

GainLabAudioProcessorEditor::GainLabAudioProcessorEditor (GainLabAudioProcessor& audioProcessor)
    : AudioProcessorEditor (&audioProcessor), processor_ (audioProcessor)
{
    juce::ignoreUnused (processor_);

    titleLabel_.setText ("GainLab", juce::dontSendNotification);
    titleLabel_.setJustificationType (juce::Justification::centred);
    titleLabel_.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    titleLabel_.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel_);

    gainLabel_.setText ("GAIN", juce::dontSendNotification);
    gainLabel_.setJustificationType (juce::Justification::centred);
    gainLabel_.setColour (juce::Label::textColourId, juce::Colour (0xffaeb8c4));
    addAndMakeVisible (gainLabel_);

    gainKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gainKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 88, 24);
    gainKnob_.setRange (-60.0, 12.0, 0.1);
    gainKnob_.setValue (0.0);
    gainKnob_.setTextValueSuffix (" dB");
    gainKnob_.setColour (juce::Slider::rotarySliderFillColourId,
                         juce::Colour (0xff4da3ff));
    gainKnob_.setColour (juce::Slider::rotarySliderOutlineColourId,
                         juce::Colour (0xff36404c));
    gainKnob_.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible (gainKnob_);

    // SliderAttachment가 UI 노브와 Processor의 Gain 파라미터를 양방향으로 연결한다.
    // 노브를 돌리면 DSP 값이 바뀌고, DAW 자동화가 값을 바꾸면 노브도 따라 움직인다.
    gainAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor_.parameters(), GainLabAudioProcessor::gainParameterId, gainKnob_);

    statusLabel_.setText ("GAIN DSP CONNECTED",
                          juce::dontSendNotification);
    statusLabel_.setJustificationType (juce::Justification::centred);
    statusLabel_.setFont (juce::FontOptions (11.0f));
    statusLabel_.setColour (juce::Label::textColourId,
                            juce::Colour (0xff7f8b99));
    addAndMakeVisible (statusLabel_);

    setSize (320, 300);
}

void GainLabAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour (0xff14191f));

    auto panel = getLocalBounds().toFloat().reduced (14.0f);
    graphics.setColour (juce::Colour (0xff202832));
    graphics.fillRoundedRectangle (panel, 14.0f);

    graphics.setColour (juce::Colour (0xff35404c));
    graphics.drawRoundedRectangle (panel, 14.0f, 1.0f);
}

void GainLabAudioProcessorEditor::resized()
{
    titleLabel_.setBounds (30, 26, getWidth() - 60, 38);
    gainLabel_.setBounds (30, 76, getWidth() - 60, 22);
    gainKnob_.setBounds ((getWidth() - 150) / 2, 94, 150, 150);
    statusLabel_.setBounds (30, getHeight() - 38, getWidth() - 60, 18);
}
