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

double valueOf (juce::AudioProcessorValueTreeState& state, const char* id)
{
    return state.getRawParameterValue (id)->load (std::memory_order_relaxed);
}
}

DetectorView::DetectorView (DeEsserLabAudioProcessor& owner) : processor_ (owner)
{
    startTimerHz (30);
}

void DetectorView::timerCallback() { repaint(); }

void DetectorView::paint (juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();
    graphics.setColour (panel);
    graphics.fillRoundedRectangle (bounds, 14.0f);
    graphics.setColour (grid.withAlpha (0.65f));
    graphics.drawRoundedRectangle (bounds.reduced (0.5f), 14.0f, 1.0f);

    auto plot = bounds.reduced (24.0f);
    auto meters = plot.removeFromBottom (70.0f);
    plot.removeFromBottom (10.0f);

    constexpr double minHz = 1000.0;
    constexpr double maxHz = 16000.0;
    const auto xForFrequency = [&plot] (double hz)
    {
        const auto proportion = std::log (hz / minHz) / std::log (maxHz / minHz);
        return plot.getX() + static_cast<float> (proportion) * plot.getWidth();
    };
    const auto yForDb = [&plot] (double db)
    {
        return plot.getBottom() - static_cast<float> ((db + 36.0) / 36.0) * plot.getHeight();
    };

    graphics.setFont (11.0f);
    for (const auto hz : { 1000.0, 2000.0, 4000.0, 7000.0, 10000.0, 16000.0 })
    {
        const auto x = xForFrequency (hz);
        graphics.setColour (grid.withAlpha (0.35f));
        graphics.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        graphics.setColour (muted);
        graphics.drawText (juce::String (hz / 1000.0, 0) + "k",
                           juce::Rectangle<float> (x - 18.0f, plot.getBottom() - 15.0f,
                                                   36.0f, 15.0f),
                           juce::Justification::centred);
    }
    for (const auto db : { -36.0, -24.0, -12.0, 0.0 })
    {
        const auto y = yForDb (db);
        graphics.setColour (grid.withAlpha (db == 0.0 ? 0.75f : 0.35f));
        graphics.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    const auto frequency = valueOf (processor_.parameters, DeEsserLabAudioProcessor::frequencyId);
    const auto q = valueOf (processor_.parameters, DeEsserLabAudioProcessor::qId);
    const auto coefficients = deesser_lab::makeBandPass (48000.0, frequency, q);
    juce::Path response;
    for (int pixel = 0; pixel <= juce::roundToInt (plot.getWidth()); ++pixel)
    {
        const auto proportion = static_cast<double> (pixel) / std::max (plot.getWidth(), 1.0f);
        const auto hz = minHz * std::pow (maxHz / minHz, proportion);
        const auto db = std::max (-36.0, deesser_lab::magnitudeDbAt (coefficients, 48000.0, hz));
        const auto x = plot.getX() + static_cast<float> (pixel);
        if (pixel == 0)
            response.startNewSubPath (x, yForDb (db));
        else
            response.lineTo (x, yForDb (db));
    }
    graphics.setColour (blue);
    graphics.strokePath (response, juce::PathStrokeType (3.0f));
    graphics.setColour (coral);
    graphics.drawVerticalLine (juce::roundToInt (xForFrequency (frequency)),
                               plot.getY(), plot.getBottom());

    const auto threshold = valueOf (processor_.parameters, DeEsserLabAudioProcessor::thresholdId);
    const auto detectorDb = juce::jlimit (-60.0, 0.0, processor_.currentDetectorDb());
    const auto reductionDb = juce::jlimit (0.0, 18.0, processor_.currentReductionDb());
    auto detectorArea = meters.removeFromLeft (meters.getWidth() * 2.0f / 3.0f).reduced (4.0f, 20.0f);
    meters.removeFromLeft (12.0f);
    auto reductionArea = meters.reduced (4.0f, 20.0f);

    graphics.setColour (juce::Colour::fromRGB (12, 18, 25));
    graphics.fillRoundedRectangle (detectorArea, 5.0f);
    const auto detectorWidth = detectorArea.getWidth()
                               * static_cast<float> ((detectorDb + 60.0) / 60.0);
    graphics.setColour (detectorDb > threshold ? coral : blue);
    graphics.fillRoundedRectangle (detectorArea.withWidth (detectorWidth), 5.0f);
    const auto thresholdX = detectorArea.getX()
                            + detectorArea.getWidth() * static_cast<float> ((threshold + 60.0) / 60.0);
    graphics.setColour (text);
    graphics.drawVerticalLine (juce::roundToInt (thresholdX),
                               detectorArea.getY() - 4.0f, detectorArea.getBottom() + 4.0f);

    graphics.setColour (juce::Colour::fromRGB (12, 18, 25));
    graphics.fillRoundedRectangle (reductionArea, 5.0f);
    graphics.setColour (coral);
    graphics.fillRoundedRectangle (
        reductionArea.withWidth (reductionArea.getWidth()
                                 * static_cast<float> (reductionDb / 18.0)), 5.0f);
    graphics.setFont (12.0f);
    graphics.setColour (text);
    graphics.drawText ("DETECTOR " + juce::String (processor_.currentDetectorDb(), 1) + " dBFS",
                       detectorArea.translated (0.0f, -21.0f), juce::Justification::centredLeft);
    graphics.drawText ("GR -" + juce::String (reductionDb, 1) + " dB",
                       reductionArea.translated (0.0f, -21.0f), juce::Justification::centredLeft);
}

DeEsserLabAudioProcessorEditor::DeEsserLabAudioProcessorEditor (
    DeEsserLabAudioProcessor& owner)
    : AudioProcessorEditor (&owner), processor_ (owner), detectorView_ (owner)
{
    addAndMakeVisible (detectorView_);
    configureKnob (frequency_, frequencyLabel_, "DETECTOR FREQ");
    configureKnob (q_, qLabel_, "DETECTOR Q");
    configureKnob (threshold_, thresholdLabel_, "THRESHOLD");
    configureKnob (ratio_, ratioLabel_, "RATIO");
    configureKnob (range_, rangeLabel_, "RANGE");
    configureKnob (attack_, attackLabel_, "ATTACK");
    configureKnob (release_, releaseLabel_, "RELEASE");

    frequencyAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DeEsserLabAudioProcessor::frequencyId, frequency_);
    qAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DeEsserLabAudioProcessor::qId, q_);
    thresholdAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DeEsserLabAudioProcessor::thresholdId, threshold_);
    ratioAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DeEsserLabAudioProcessor::ratioId, ratio_);
    rangeAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DeEsserLabAudioProcessor::rangeId, range_);
    attackAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DeEsserLabAudioProcessor::attackId, attack_);
    releaseAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, DeEsserLabAudioProcessor::releaseId, release_);

    listen_.setColour (juce::ToggleButton::textColourId, text);
    listen_.setColour (juce::ToggleButton::tickColourId, coral);
    addAndMakeVisible (listen_);
    listenAttachment_ = std::make_unique<ButtonAttachment> (
        processor_.parameters, DeEsserLabAudioProcessor::listenId, listen_);

    setSize (940, 740);
    setResizable (true, true);
    setResizeLimits (780, 650, 1200, 920);
}

void DeEsserLabAudioProcessorEditor::configureKnob (juce::Slider& slider,
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
    label.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addAndMakeVisible (label);
}

void DeEsserLabAudioProcessorEditor::layoutKnob (juce::Rectangle<int> bounds,
                                                  juce::Slider& slider,
                                                  juce::Label& label)
{
    label.setBounds (bounds.removeFromTop (22));
    slider.setBounds (bounds.reduced (8, 0));
}

void DeEsserLabAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (background);
    graphics.setColour (text);
    graphics.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    graphics.drawText ("DeEsserLab", 0, 14, getWidth(), 42,
                       juce::Justification::centred);
    graphics.setColour (muted);
    graphics.setFont (14.0f);
    graphics.drawText ("WIDE-BAND  |  SIBILANCE DETECTOR -> FULL SIGNAL GAIN",
                       0, 52, getWidth(), 24, juce::Justification::centred);
}

void DeEsserLabAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (22);
    area.removeFromTop (68);
    auto top = area.removeFromTop (34);
    listen_.setBounds (top.removeFromRight (175));
    detectorView_.setBounds (area.removeFromTop (270));
    area.removeFromTop (12);

    auto firstRow = area.removeFromTop (145);
    const auto firstWidth = firstRow.getWidth() / 4;
    layoutKnob (firstRow.removeFromLeft (firstWidth), frequency_, frequencyLabel_);
    layoutKnob (firstRow.removeFromLeft (firstWidth), q_, qLabel_);
    layoutKnob (firstRow.removeFromLeft (firstWidth), threshold_, thresholdLabel_);
    layoutKnob (firstRow, ratio_, ratioLabel_);

    area.removeFromTop (6);
    auto secondRow = area;
    const auto secondWidth = secondRow.getWidth() / 3;
    layoutKnob (secondRow.removeFromLeft (secondWidth), range_, rangeLabel_);
    layoutKnob (secondRow.removeFromLeft (secondWidth), attack_, attackLabel_);
    layoutKnob (secondRow, release_, releaseLabel_);
}
