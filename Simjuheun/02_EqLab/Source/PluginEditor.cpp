#include "PluginEditor.h"

EqLabAudioProcessorEditor::EqLabAudioProcessorEditor (EqLabAudioProcessor& audioProcessor)
    : AudioProcessorEditor (&audioProcessor), processor_ (audioProcessor)
{
    titleLabel_.setText ("EqLab", juce::dontSendNotification);
    titleLabel_.setJustificationType (juce::Justification::centred);
    titleLabel_.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    titleLabel_.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel_);

    subtitleLabel_.setText ("IIR BIQUAD / BELL EQ", juce::dontSendNotification);
    subtitleLabel_.setJustificationType (juce::Justification::centred);
    subtitleLabel_.setColour (juce::Label::textColourId, juce::Colour (0xff9aa8b8));
    addAndMakeVisible (subtitleLabel_);

    auto prepareLabel = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, juce::Colour (0xffaeb8c4));
        addAndMakeVisible (label);
    };

    auto prepareKnob = [this] (juce::Slider& knob, juce::Colour colour)
    {
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 94, 24);
        knob.setColour (juce::Slider::rotarySliderFillColourId, colour);
        knob.setColour (juce::Slider::rotarySliderOutlineColourId,
                        juce::Colour (0xff36404c));
        knob.setColour (juce::Slider::thumbColourId, juce::Colours::white);
        addAndMakeVisible (knob);
    };

    prepareLabel (frequencyLabel_, "FREQUENCY");
    prepareLabel (gainLabel_, "GAIN");
    prepareLabel (qLabel_, "Q");

    prepareKnob (frequencyKnob_, juce::Colour (0xff4da3ff));
    frequencyKnob_.setRange (20.0, 20000.0, 1.0);
    frequencyKnob_.setSkewFactorFromMidPoint (1000.0);
    frequencyKnob_.setTextValueSuffix (" Hz");

    prepareKnob (gainKnob_, juce::Colour (0xffffad5c));
    gainKnob_.setRange (-12.0, 12.0, 0.1);
    gainKnob_.setTextValueSuffix (" dB");

    prepareKnob (qKnob_, juce::Colour (0xff55d6a8));
    qKnob_.setRange (0.1, 10.0, 0.01);

    frequencyAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor_.parameters(), EqLabAudioProcessor::frequencyParameterId, frequencyKnob_);
    gainAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor_.parameters(), EqLabAudioProcessor::gainParameterId, gainKnob_);
    qAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor_.parameters(), EqLabAudioProcessor::qParameterId, qKnob_);

    statusLabel_.setText ("BELL EQ DSP CONNECTED",
                          juce::dontSendNotification);
    statusLabel_.setJustificationType (juce::Justification::centred);
    statusLabel_.setColour (juce::Label::textColourId, juce::Colour (0xff7f8b99));
    addAndMakeVisible (statusLabel_);

    setSize (560, 300);
}

void EqLabAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour (0xff14191f));

    const auto panel = getLocalBounds().toFloat().reduced (14.0f);
    graphics.setColour (juce::Colour (0xff202832));
    graphics.fillRoundedRectangle (panel, 14.0f);
    graphics.setColour (juce::Colour (0xff35404c));
    graphics.drawRoundedRectangle (panel, 14.0f, 1.0f);

}

void EqLabAudioProcessorEditor::resized()
{
    titleLabel_.setBounds (30, 26, getWidth() - 60, 40);
    subtitleLabel_.setBounds (30, 70, getWidth() - 60, 22);
    frequencyLabel_.setBounds (20, 102, 160, 22);
    gainLabel_.setBounds (200, 102, 160, 22);
    qLabel_.setBounds (380, 102, 160, 22);
    frequencyKnob_.setBounds (20, 120, 160, 125);
    gainKnob_.setBounds (200, 120, 160, 125);
    qKnob_.setBounds (380, 120, 160, 125);
    statusLabel_.setBounds (30, getHeight() - 48, getWidth() - 60, 22);
}
