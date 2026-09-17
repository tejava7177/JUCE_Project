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

    delayTimeLabel_.setText ("DELAY TIME", juce::dontSendNotification);
    delayTimeLabel_.setJustificationType (juce::Justification::centred);
    delayTimeLabel_.setColour (juce::Label::textColourId, juce::Colour (0xffaeb8c4));
    addAndMakeVisible (delayTimeLabel_);

    delayTimeKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    delayTimeKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 88, 24);
    delayTimeKnob_.setRange (1.0, 1000.0, 1.0);
    delayTimeKnob_.setValue (250.0);
    delayTimeKnob_.setTextValueSuffix (" ms");
    delayTimeKnob_.setColour (juce::Slider::rotarySliderFillColourId,
                              juce::Colour (0xffffad5c));
    delayTimeKnob_.setColour (juce::Slider::rotarySliderOutlineColourId,
                              juce::Colour (0xff36404c));
    delayTimeKnob_.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible (delayTimeKnob_);

    delayTimeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor_.parameters(), GainLabAudioProcessor::delayTimeParameterId, delayTimeKnob_);

    feedbackLabel_.setText ("FEEDBACK", juce::dontSendNotification);
    feedbackLabel_.setJustificationType (juce::Justification::centred);
    feedbackLabel_.setColour (juce::Label::textColourId, juce::Colour (0xffaeb8c4));
    addAndMakeVisible (feedbackLabel_);

    feedbackKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    feedbackKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 88, 24);
    feedbackKnob_.setRange (0.0, 95.0, 1.0);
    feedbackKnob_.setValue (35.0);
    feedbackKnob_.setTextValueSuffix (" %");
    feedbackKnob_.setColour (juce::Slider::rotarySliderFillColourId,
                             juce::Colour (0xffff6f91));
    feedbackKnob_.setColour (juce::Slider::rotarySliderOutlineColourId,
                             juce::Colour (0xff36404c));
    feedbackKnob_.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible (feedbackKnob_);

    feedbackAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor_.parameters(), GainLabAudioProcessor::feedbackParameterId, feedbackKnob_);

    mixLabel_.setText ("DRY / WET", juce::dontSendNotification);
    mixLabel_.setJustificationType (juce::Justification::centred);
    mixLabel_.setColour (juce::Label::textColourId, juce::Colour (0xffaeb8c4));
    addAndMakeVisible (mixLabel_);

    mixKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 88, 24);
    mixKnob_.setRange (0.0, 100.0, 1.0);
    mixKnob_.setValue (100.0);
    mixKnob_.setTextValueSuffix (" %");
    mixKnob_.setColour (juce::Slider::rotarySliderFillColourId,
                        juce::Colour (0xff55d6a8));
    mixKnob_.setColour (juce::Slider::rotarySliderOutlineColourId,
                        juce::Colour (0xff36404c));
    mixKnob_.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible (mixKnob_);

    mixAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor_.parameters(), GainLabAudioProcessor::mixParameterId, mixKnob_);

    statusLabel_.setText ("GAIN + DELAY + DRY/WET DSP CONNECTED",
                          juce::dontSendNotification);
    statusLabel_.setJustificationType (juce::Justification::centred);
    statusLabel_.setFont (juce::FontOptions (11.0f));
    statusLabel_.setColour (juce::Label::textColourId,
                            juce::Colour (0xff7f8b99));
    addAndMakeVisible (statusLabel_);

    setSize (620, 300);
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
    gainLabel_.setBounds (20, 76, 135, 22);
    delayTimeLabel_.setBounds (168, 76, 135, 22);
    feedbackLabel_.setBounds (316, 76, 135, 22);
    mixLabel_.setBounds (464, 76, 135, 22);
    gainKnob_.setBounds (20, 94, 135, 150);
    delayTimeKnob_.setBounds (168, 94, 135, 150);
    feedbackKnob_.setBounds (316, 94, 135, 150);
    mixKnob_.setBounds (464, 94, 135, 150);
    statusLabel_.setBounds (30, getHeight() - 38, getWidth() - 60, 18);
}
