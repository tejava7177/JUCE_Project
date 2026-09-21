#include "PluginEditor.h"

#include "DSP/Biquad.h"

#include <array>
#include <cmath>

namespace
{
const auto background = juce::Colour::fromRGB (16, 23, 31);
const auto panel = juce::Colour::fromRGB (28, 39, 51);
const auto grid = juce::Colour::fromRGB (77, 94, 110);
const auto blue = juce::Colour::fromRGB (70, 156, 255);
const auto coral = juce::Colour::fromRGB (255, 112, 99);
const auto text = juce::Colour::fromRGB (235, 240, 247);
const auto muted = juce::Colour::fromRGB (151, 166, 184);

double parameterValue (juce::AudioProcessorValueTreeState& state, const char* id)
{
    return static_cast<double> (state.getRawParameterValue (id)->load (
        std::memory_order_relaxed));
}
}

DynamicResponseView::DynamicResponseView (DynamicEqLabAudioProcessor& processor)
    : processor_ (processor)
{
    startTimerHz (30);
}

void DynamicResponseView::timerCallback()
{
    repaint();
}

void DynamicResponseView::paint (juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();
    graphics.setColour (panel);
    graphics.fillRoundedRectangle (bounds, 14.0f);
    graphics.setColour (grid.withAlpha (0.65f));
    graphics.drawRoundedRectangle (bounds.reduced (0.5f), 14.0f, 1.0f);

    auto plot = bounds.reduced (20.0f);
    auto meterArea = plot.removeFromBottom (54.0f);
    plot.removeFromBottom (10.0f);

    const auto xForFrequency = [&plot] (double frequency)
    {
        const auto proportion = std::log10 (frequency / 20.0) / std::log10 (1000.0);
        return plot.getX() + static_cast<float> (proportion) * plot.getWidth();
    };
    const auto yForDb = [&plot] (double db)
    {
        constexpr double topDb = 3.0;
        constexpr double bottomDb = -18.0;
        return plot.getY() + static_cast<float> ((topDb - db) / (topDb - bottomDb))
                             * plot.getHeight();
    };

    const std::array<double, 10> frequencyTicks {
        20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0
    };
    graphics.setFont (11.0f);
    for (const auto frequency : frequencyTicks)
    {
        const auto x = xForFrequency (frequency);
        graphics.setColour (grid.withAlpha (0.35f));
        graphics.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        graphics.setColour (muted);
        const auto label = frequency >= 1000.0
                               ? juce::String (frequency / 1000.0, frequency < 10000.0 ? 1 : 0) + "k"
                               : juce::String (juce::roundToInt (frequency));
        graphics.drawText (label, juce::Rectangle<float> (x - 20.0f, plot.getBottom() - 16.0f,
                                                           40.0f, 15.0f),
                           juce::Justification::centred);
    }

    for (const auto db : { 0.0, -6.0, -12.0, -18.0 })
    {
        const auto y = yForDb (db);
        graphics.setColour (grid.withAlpha (db == 0.0 ? 0.8f : 0.35f));
        graphics.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        graphics.setColour (muted);
        graphics.drawText (juce::String (juce::roundToInt (db)) + " dB",
                           juce::Rectangle<float> (plot.getX(), y - 15.0f, 45.0f, 14.0f),
                           juce::Justification::left);
    }

    const auto frequency = parameterValue (processor_.parameters,
                                            DynamicEqLabAudioProcessor::frequencyId);
    const auto q = parameterValue (processor_.parameters, DynamicEqLabAudioProcessor::qId);
    const auto reduction = processor_.currentReductionDb();
    const auto sampleRate = processor_.currentSampleRate();
    const auto coefficients = dynamic_eq_lab::BiquadDesigner::makeBell (
        sampleRate, frequency, q, -reduction);

    juce::Path responsePath;
    for (int pixel = 0; pixel <= juce::roundToInt (plot.getWidth()); ++pixel)
    {
        const auto proportion = static_cast<double> (pixel) / std::max (plot.getWidth(), 1.0f);
        const auto hz = 20.0 * std::pow (1000.0, proportion);
        const auto responseDb = dynamic_eq_lab::BiquadDesigner::magnitudeDbAt (
            coefficients, sampleRate, hz);
        const auto x = plot.getX() + static_cast<float> (pixel);
        const auto y = yForDb (responseDb);
        if (pixel == 0)
            responsePath.startNewSubPath (x, y);
        else
            responsePath.lineTo (x, y);
    }

    graphics.setColour (blue);
    graphics.strokePath (responsePath, juce::PathStrokeType (3.0f));

    const auto centreX = xForFrequency (frequency);
    graphics.setColour (coral.withAlpha (0.7f));
    graphics.drawVerticalLine (juce::roundToInt (centreX), plot.getY(), plot.getBottom());
    graphics.fillEllipse (centreX - 5.0f, yForDb (-reduction) - 5.0f, 10.0f, 10.0f);

    const auto detectorDb = juce::jlimit (-60.0, 0.0, processor_.currentDetectorDb());
    const auto threshold = parameterValue (processor_.parameters,
                                            DynamicEqLabAudioProcessor::thresholdId);
    const auto meter = meterArea.reduced (2.0f, 17.0f);
    const auto xForLevel = [&meter] (double db)
    {
        return meter.getX() + static_cast<float> ((db + 60.0) / 60.0) * meter.getWidth();
    };

    graphics.setColour (juce::Colour::fromRGB (12, 18, 25));
    graphics.fillRoundedRectangle (meter, 5.0f);
    graphics.setColour (detectorDb > threshold ? coral : blue);
    graphics.fillRoundedRectangle (meter.withWidth (xForLevel (detectorDb) - meter.getX()), 5.0f);
    graphics.setColour (text);
    graphics.drawVerticalLine (juce::roundToInt (xForLevel (threshold)), meter.getY() - 4.0f,
                               meter.getBottom() + 4.0f);

    graphics.setFont (12.0f);
    graphics.drawText ("DETECTOR " + juce::String (processor_.currentDetectorDb(), 1) + " dBFS",
                       meterArea.removeFromLeft (meterArea.getWidth() / 2),
                       juce::Justification::centredLeft);
    graphics.setColour (coral);
    graphics.drawText ("GAIN REDUCTION  -" + juce::String (reduction, 1) + " dB",
                       meterArea, juce::Justification::centredRight);
}

DynamicEqLabAudioProcessorEditor::DynamicEqLabAudioProcessorEditor (
    DynamicEqLabAudioProcessor& owner)
    : AudioProcessorEditor (&owner), processor_ (owner), response_ (owner)
{
    addAndMakeVisible (response_);

    configureKnob (frequency_, frequencyLabel_, "FREQUENCY");
    configureKnob (q_, qLabel_, "Q");
    configureKnob (threshold_, thresholdLabel_, "THRESHOLD");
    configureKnob (ratio_, ratioLabel_, "RATIO");
    configureKnob (range_, rangeLabel_, "RANGE");
    configureKnob (attack_, attackLabel_, "ATTACK");
    configureKnob (release_, releaseLabel_, "RELEASE");

    frequencyAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DynamicEqLabAudioProcessor::frequencyId, frequency_);
    qAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DynamicEqLabAudioProcessor::qId, q_);
    thresholdAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DynamicEqLabAudioProcessor::thresholdId, threshold_);
    ratioAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DynamicEqLabAudioProcessor::ratioId, ratio_);
    rangeAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DynamicEqLabAudioProcessor::rangeId, range_);
    attackAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DynamicEqLabAudioProcessor::attackId, attack_);
    releaseAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DynamicEqLabAudioProcessor::releaseId, release_);

    setSize (900, 700);
    setResizable (true, true);
    setResizeLimits (760, 620, 1200, 900);
}

void DynamicEqLabAudioProcessorEditor::configureKnob (juce::Slider& slider,
                                                       juce::Label& label,
                                                       const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 104, 24);
    slider.setColour (juce::Slider::rotarySliderFillColourId, blue);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId,
                      juce::Colour::fromRGB (54, 67, 82));
    slider.setColour (juce::Slider::thumbColourId, text);
    slider.setColour (juce::Slider::textBoxTextColourId, text);
    slider.setColour (juce::Slider::textBoxOutlineColourId, grid);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, panel);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, muted);
    label.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (label);
}

void DynamicEqLabAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (background);
    graphics.setColour (text);
    graphics.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    graphics.drawText ("DynamicEqLab", 0, 16, getWidth(), 42,
                       juce::Justification::centred);
    graphics.setColour (muted);
    graphics.setFont (14.0f);
    graphics.drawText ("BAND DETECTOR  →  ENVELOPE  →  DYNAMIC BELL GAIN",
                       0, 55, getWidth(), 24, juce::Justification::centred);
}

void DynamicEqLabAudioProcessorEditor::layoutKnob (juce::Rectangle<int> bounds,
                                                    juce::Slider& slider,
                                                    juce::Label& label)
{
    label.setBounds (bounds.removeFromTop (24));
    slider.setBounds (bounds.reduced (8, 0));
}

void DynamicEqLabAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (22);
    area.removeFromTop (72);
    response_.setBounds (area.removeFromTop (270));
    area.removeFromTop (16);

    auto firstRow = area.removeFromTop (150);
    const auto firstWidth = firstRow.getWidth() / 4;
    layoutKnob (firstRow.removeFromLeft (firstWidth), frequency_, frequencyLabel_);
    layoutKnob (firstRow.removeFromLeft (firstWidth), q_, qLabel_);
    layoutKnob (firstRow.removeFromLeft (firstWidth), threshold_, thresholdLabel_);
    layoutKnob (firstRow, ratio_, ratioLabel_);

    area.removeFromTop (8);
    auto secondRow = area;
    const auto secondWidth = secondRow.getWidth() / 3;
    layoutKnob (secondRow.removeFromLeft (secondWidth), range_, rangeLabel_);
    layoutKnob (secondRow.removeFromLeft (secondWidth), attack_, attackLabel_);
    layoutKnob (secondRow, release_, releaseLabel_);
}
