#include "PluginEditor.h"

#include <array>
#include <cmath>

FrequencyResponseComponent::FrequencyResponseComponent (EqLabAudioProcessor& processor)
    : processor_ (processor)
{
    // 노브를 움직일 때 곡선이 자연스럽게 따라오도록 초당 30번 다시 그린다.
    startTimerHz (30);
}

void FrequencyResponseComponent::timerCallback()
{
    repaint();
}

void FrequencyResponseComponent::paint (juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();
    graphics.setColour (juce::Colour (0xff18212b));
    graphics.fillRoundedRectangle (bounds, 10.0f);
    graphics.setColour (juce::Colour (0xff35404c));
    graphics.drawRoundedRectangle (bounds, 10.0f, 1.0f);

    auto plot = bounds.reduced (42.0f, 18.0f);
    plot.removeFromBottom (12.0f);

    constexpr double minimumFrequency = 20.0;
    constexpr double maximumFrequency = 20000.0;
    constexpr float minimumDb = -36.0f;
    constexpr float maximumDb = 18.0f;

    const auto frequencyToX = [&plot] (double frequency)
    {
        const auto proportion = (std::log10 (frequency) - std::log10 (minimumFrequency))
                                / (std::log10 (maximumFrequency)
                                   - std::log10 (minimumFrequency));
        return plot.getX() + static_cast<float> (proportion) * plot.getWidth();
    };

    const auto decibelsToY = [&plot] (double decibels)
    {
        const auto limited = juce::jlimit (static_cast<double> (minimumDb),
                                           static_cast<double> (maximumDb), decibels);
        return juce::jmap (static_cast<float> (limited),
                           minimumDb, maximumDb,
                           plot.getBottom(), plot.getY());
    };

    graphics.setFont (11.0f);
    graphics.setColour (juce::Colour (0xff667382));

    for (const auto db : std::array<float, 5> { -36.0f, -24.0f, -12.0f, 0.0f, 12.0f })
    {
        const auto y = decibelsToY (db);
        graphics.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        graphics.drawText (juce::String (static_cast<int> (db)) + " dB",
                           2, juce::roundToInt (y - 8.0f), 38, 16,
                           juce::Justification::centredRight);
    }

    struct FrequencyMark { double frequency; const char* label; };
    for (const auto mark : std::array<FrequencyMark, 5> {
             FrequencyMark { 20.0, "20" },
             FrequencyMark { 100.0, "100" },
             FrequencyMark { 1000.0, "1k" },
             FrequencyMark { 10000.0, "10k" },
             FrequencyMark { 20000.0, "20k" } })
    {
        const auto x = frequencyToX (mark.frequency);
        graphics.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        graphics.drawText (mark.label,
                           juce::roundToInt (x - 18.0f),
                           juce::roundToInt (plot.getBottom() + 2.0f),
                           36, 14, juce::Justification::centred);
    }

    const auto* typeValue = processor_.parameters().getRawParameterValue (
        EqLabAudioProcessor::filterTypeParameterId);
    const auto* frequencyValue = processor_.parameters().getRawParameterValue (
        EqLabAudioProcessor::frequencyParameterId);
    const auto* gainValue = processor_.parameters().getRawParameterValue (
        EqLabAudioProcessor::gainParameterId);
    const auto* qValue = processor_.parameters().getRawParameterValue (
        EqLabAudioProcessor::qParameterId);
    const auto* slopeValue = processor_.parameters().getRawParameterValue (
        EqLabAudioProcessor::slopeParameterId);

    const auto typeIndex = juce::jlimit (0, 5, static_cast<int> (typeValue->load()));
    const auto type = static_cast<eqlab::FilterType> (typeIndex);
    const auto frequency = frequencyValue->load();
    const auto sampleRate = processor_.getSampleRate() > 0.0
                                ? processor_.getSampleRate()
                                : 44100.0;
    const auto coefficients = eqlab::BiquadDsp::makeCoefficients (
        type, sampleRate, frequency, gainValue->load(), qValue->load(),
        slopeValue->load());

    // 선택한 중심/차단 주파수를 세로선으로 표시한다.
    const auto selectedX = frequencyToX (frequency);
    graphics.setColour (juce::Colour (0x6655d6a8));
    graphics.drawVerticalLine (juce::roundToInt (selectedX), plot.getY(), plot.getBottom());

    juce::Path responsePath;
    for (int pixel = 0; pixel <= juce::roundToInt (plot.getWidth()); ++pixel)
    {
        const auto proportion = static_cast<double> (pixel) / plot.getWidth();
        const auto graphFrequency = minimumFrequency
                                    * std::pow (maximumFrequency / minimumFrequency,
                                                proportion);
        const auto responseDb = eqlab::BiquadDsp::magnitudeDbAt (
            coefficients, sampleRate, graphFrequency);
        const auto x = plot.getX() + static_cast<float> (pixel);
        const auto y = decibelsToY (responseDb);

        if (pixel == 0)
            responsePath.startNewSubPath (x, y);
        else
            responsePath.lineTo (x, y);
    }

    graphics.setColour (juce::Colour (0xff4da3ff));
    graphics.strokePath (responsePath, juce::PathStrokeType (2.5f));

    graphics.setColour (juce::Colour (0xffb9c5d2));
    graphics.drawText (juce::String (frequency, frequency >= 1000.0f ? 0 : 1) + " Hz",
                       juce::roundToInt (selectedX - 38.0f), 2, 76, 16,
                       juce::Justification::centred);
}

EqLabAudioProcessorEditor::EqLabAudioProcessorEditor (EqLabAudioProcessor& audioProcessor)
    : AudioProcessorEditor (&audioProcessor),
      processor_ (audioProcessor),
      responseGraph_ (audioProcessor)
{
    titleLabel_.setText ("EqLab", juce::dontSendNotification);
    titleLabel_.setJustificationType (juce::Justification::centred);
    titleLabel_.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    titleLabel_.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel_);

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

    prepareLabel (filterTypeLabel_, "FILTER TYPE");
    filterTypeBox_.addItem ("Bell", 1);
    filterTypeBox_.addItem ("Low-pass", 2);
    filterTypeBox_.addItem ("High-pass", 3);
    filterTypeBox_.addItem ("Low-shelf", 4);
    filterTypeBox_.addItem ("High-shelf", 5);
    filterTypeBox_.addItem ("Notch", 6);
    filterTypeBox_.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff18212b));
    filterTypeBox_.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    filterTypeBox_.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff465363));
    addAndMakeVisible (filterTypeBox_);

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

    prepareKnob (slopeKnob_, juce::Colour (0xff55d6a8));
    slopeKnob_.setRange (0.1, 1.0, 0.01);

    filterTypeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor_.parameters(), EqLabAudioProcessor::filterTypeParameterId, filterTypeBox_);
    frequencyAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor_.parameters(), EqLabAudioProcessor::frequencyParameterId, frequencyKnob_);
    gainAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor_.parameters(), EqLabAudioProcessor::gainParameterId, gainKnob_);
    qAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor_.parameters(), EqLabAudioProcessor::qParameterId, qKnob_);
    slopeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor_.parameters(), EqLabAudioProcessor::slopeParameterId, slopeKnob_);

    filterTypeBox_.onChange = [this] { updateControlsForFilterType(); };

    statusLabel_.setJustificationType (juce::Justification::centred);
    statusLabel_.setColour (juce::Label::textColourId, juce::Colour (0xff7f8b99));
    addAndMakeVisible (statusLabel_);
    addAndMakeVisible (responseGraph_);

    updateControlsForFilterType();
    setSize (680, 530);
}

void EqLabAudioProcessorEditor::updateControlsForFilterType()
{
    const auto selectedId = filterTypeBox_.getSelectedId();
    const auto isBell = selectedId <= 1;
    const auto isShelf = selectedId == 4 || selectedId == 5;
    const auto usesGain = isBell || isShelf;

    gainKnob_.setEnabled (usesGain);
    gainKnob_.setAlpha (usesGain ? 1.0f : 0.35f);
    gainLabel_.setAlpha (usesGain ? 1.0f : 0.35f);
    qKnob_.setVisible (! isShelf);
    slopeKnob_.setVisible (isShelf);
    qLabel_.setText (isShelf ? "SLOPE" : "Q", juce::dontSendNotification);
    frequencyLabel_.setText (isBell || isShelf || selectedId == 6
                                 ? "FREQUENCY" : "CUTOFF",
                             juce::dontSendNotification);

    if (selectedId == 2)
    {
        subtitleLabel_.setText ("IIR BIQUAD / LOW-PASS", juce::dontSendNotification);
        statusLabel_.setText ("LOW FREQUENCIES PASS / HIGH FREQUENCIES CUT",
                              juce::dontSendNotification);
    }
    else if (selectedId == 3)
    {
        subtitleLabel_.setText ("IIR BIQUAD / HIGH-PASS", juce::dontSendNotification);
        statusLabel_.setText ("HIGH FREQUENCIES PASS / LOW FREQUENCIES CUT",
                              juce::dontSendNotification);
    }
    else if (selectedId == 4)
    {
        subtitleLabel_.setText ("IIR BIQUAD / LOW-SHELF", juce::dontSendNotification);
        statusLabel_.setText ("LOW FREQUENCIES MOVE TO THE GAIN LEVEL",
                              juce::dontSendNotification);
    }
    else if (selectedId == 5)
    {
        subtitleLabel_.setText ("IIR BIQUAD / HIGH-SHELF", juce::dontSendNotification);
        statusLabel_.setText ("HIGH FREQUENCIES MOVE TO THE GAIN LEVEL",
                              juce::dontSendNotification);
    }
    else if (selectedId == 6)
    {
        subtitleLabel_.setText ("IIR BIQUAD / NOTCH", juce::dontSendNotification);
        statusLabel_.setText ("CENTRE FREQUENCY CANCELLED / Q CONTROLS WIDTH",
                              juce::dontSendNotification);
    }
    else
    {
        subtitleLabel_.setText ("IIR BIQUAD / BELL EQ", juce::dontSendNotification);
        statusLabel_.setText ("FREQUENCY + GAIN + Q", juce::dontSendNotification);
    }
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
    titleLabel_.setBounds (30, 22, getWidth() - 60, 38);
    subtitleLabel_.setBounds (30, 60, getWidth() - 60, 22);
    filterTypeLabel_.setBounds (32, 91, 110, 28);
    filterTypeBox_.setBounds (146, 91, 180, 28);
    responseGraph_.setBounds (30, 128, getWidth() - 60, 190);

    const auto knobWidth = (getWidth() - 60) / 3;
    frequencyLabel_.setBounds (30, 330, knobWidth, 22);
    gainLabel_.setBounds (30 + knobWidth, 330, knobWidth, 22);
    qLabel_.setBounds (30 + knobWidth * 2, 330, knobWidth, 22);
    frequencyKnob_.setBounds (30, 350, knobWidth, 130);
    gainKnob_.setBounds (30 + knobWidth, 350, knobWidth, 130);
    qKnob_.setBounds (30 + knobWidth * 2, 350, knobWidth, 130);
    slopeKnob_.setBounds (30 + knobWidth * 2, 350, knobWidth, 130);
    statusLabel_.setBounds (30, getHeight() - 42, getWidth() - 60, 20);
}
