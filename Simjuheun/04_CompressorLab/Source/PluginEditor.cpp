#include "PluginEditor.h"

#include <algorithm>

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

CompressionGraph::CompressionGraph (CompressorLabAudioProcessor& owner)
    : processor_ (owner)
{
    startTimerHz (30);
}

void CompressionGraph::timerCallback() { repaint(); }

void CompressionGraph::paint (juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();
    graphics.setColour (panel);
    graphics.fillRoundedRectangle (bounds, 14.0f);
    graphics.setColour (grid.withAlpha (0.65f));
    graphics.drawRoundedRectangle (bounds.reduced (0.5f), 14.0f, 1.0f);

    auto plot = bounds.reduced (28.0f);
    auto meterArea = plot.removeFromRight (145.0f);
    plot.removeFromRight (24.0f);

    const auto xForDb = [&plot] (double db)
    {
        return plot.getX() + static_cast<float> ((db + 60.0) / 60.0) * plot.getWidth();
    };
    const auto yForDb = [&plot] (double db)
    {
        return plot.getBottom() - static_cast<float> ((db + 60.0) / 60.0) * plot.getHeight();
    };

    graphics.setFont (11.0f);
    for (const auto db : { -60.0, -48.0, -36.0, -24.0, -12.0, 0.0 })
    {
        const auto x = xForDb (db);
        const auto y = yForDb (db);
        graphics.setColour (grid.withAlpha (0.35f));
        graphics.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        graphics.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        graphics.setColour (muted);
        graphics.drawText (juce::String (juce::roundToInt (db)),
                           juce::Rectangle<float> (x - 18.0f, plot.getBottom() - 15.0f,
                                                   36.0f, 15.0f),
                           juce::Justification::centred);
    }

    graphics.setColour (grid);
    graphics.drawLine (xForDb (-60.0), yForDb (-60.0),
                       xForDb (0.0), yForDb (0.0), 1.5f);

    const auto threshold = valueOf (processor_.parameters,
                                    CompressorLabAudioProcessor::thresholdId);
    const auto ratio = valueOf (processor_.parameters, CompressorLabAudioProcessor::ratioId);
    const auto makeup = valueOf (processor_.parameters, CompressorLabAudioProcessor::makeupId);

    juce::Path curve;
    for (int i = 0; i <= 240; ++i)
    {
        const auto inputDb = -60.0 + 60.0 * static_cast<double> (i) / 240.0;
        const auto reduction = compressor_lab::CompressorDsp::calculateReductionDb (
            inputDb, threshold, ratio);
        const auto outputDb = std::clamp (inputDb - reduction + makeup, -60.0, 0.0);
        if (i == 0)
            curve.startNewSubPath (xForDb (inputDb), yForDb (outputDb));
        else
            curve.lineTo (xForDb (inputDb), yForDb (outputDb));
    }
    graphics.setColour (blue);
    graphics.strokePath (curve, juce::PathStrokeType (3.0f));

    graphics.setColour (coral.withAlpha (0.7f));
    graphics.drawVerticalLine (juce::roundToInt (xForDb (threshold)),
                               plot.getY(), plot.getBottom());

    const auto inputDb = juce::jlimit (-60.0, 0.0, processor_.currentInputDb());
    const auto reductionDb = processor_.currentReductionDb();
    const auto outputDb = juce::jlimit (-60.0, 0.0, inputDb - reductionDb + makeup);
    graphics.setColour (coral);
    graphics.fillEllipse (xForDb (inputDb) - 5.0f, yForDb (outputDb) - 5.0f,
                          10.0f, 10.0f);

    graphics.setColour (muted);
    graphics.drawText ("INPUT dBFS",
                       juce::Rectangle<float> (plot.getX(), plot.getBottom() + 4.0f,
                                               plot.getWidth(), 18.0f),
                       juce::Justification::centred);
    graphics.saveState();
    graphics.addTransform (juce::AffineTransform::rotation (
        -juce::MathConstants<float>::halfPi,
        plot.getX() - 16.0f, plot.getCentreY()));
    graphics.drawText ("OUTPUT dBFS",
                       juce::Rectangle<float> (plot.getX() - 65.0f,
                                               plot.getCentreY() - 9.0f,
                                               130.0f, 18.0f),
                       juce::Justification::centred);
    graphics.restoreState();

    auto meter = meterArea.reduced (44.0f, 8.0f);
    graphics.setColour (juce::Colour::fromRGB (12, 18, 25));
    graphics.fillRoundedRectangle (meter, 7.0f);
    const auto amount = static_cast<float> (juce::jlimit (0.0, 24.0, reductionDb) / 24.0);
    auto fill = meter;
    fill.setHeight (meter.getHeight() * amount);
    graphics.setColour (coral);
    graphics.fillRoundedRectangle (fill, 7.0f);

    graphics.setColour (text);
    graphics.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    graphics.drawText ("GR",
                       juce::Rectangle<float> (meterArea.getX(), meterArea.getY(),
                                               meterArea.getWidth(), 20.0f),
                       juce::Justification::centredTop);
    graphics.setFont (13.0f);
    graphics.drawText ("-" + juce::String (reductionDb, 1) + " dB",
                       juce::Rectangle<float> (meterArea.getX(), meterArea.getBottom() - 22.0f,
                                               meterArea.getWidth(), 20.0f),
                       juce::Justification::centredBottom);
}

CompressorLabAudioProcessorEditor::CompressorLabAudioProcessorEditor (
    CompressorLabAudioProcessor& owner)
    : AudioProcessorEditor (&owner), processor_ (owner), graph_ (owner)
{
    addAndMakeVisible (graph_);
    configureKnob (threshold_, thresholdLabel_, "THRESHOLD");
    configureKnob (ratio_, ratioLabel_, "RATIO");
    configureKnob (attack_, attackLabel_, "ATTACK");
    configureKnob (release_, releaseLabel_, "RELEASE");
    configureKnob (makeup_, makeupLabel_, "MAKEUP GAIN");

    thresholdAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, CompressorLabAudioProcessor::thresholdId, threshold_);
    ratioAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, CompressorLabAudioProcessor::ratioId, ratio_);
    attackAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, CompressorLabAudioProcessor::attackId, attack_);
    releaseAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, CompressorLabAudioProcessor::releaseId, release_);
    makeupAttachment_ = std::make_unique<SliderAttachment> (
        processor_.parameters, CompressorLabAudioProcessor::makeupId, makeup_);

    setSize (920, 610);
    setResizable (true, true);
    setResizeLimits (760, 540, 1200, 820);
}

void CompressorLabAudioProcessorEditor::configureKnob (juce::Slider& slider,
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

void CompressorLabAudioProcessorEditor::layoutKnob (juce::Rectangle<int> bounds,
                                                     juce::Slider& slider,
                                                     juce::Label& label)
{
    label.setBounds (bounds.removeFromTop (24));
    slider.setBounds (bounds.reduced (8, 0));
}

void CompressorLabAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (background);
    graphics.setColour (text);
    graphics.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    graphics.drawText ("CompressorLab", 0, 15, getWidth(), 42,
                       juce::Justification::centred);
    graphics.setColour (muted);
    graphics.setFont (14.0f);
    graphics.drawText ("ENVELOPE  →  GAIN COMPUTER  →  MAKEUP GAIN",
                       0, 54, getWidth(), 24, juce::Justification::centred);
}

void CompressorLabAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (22);
    area.removeFromTop (72);
    graph_.setBounds (area.removeFromTop (310));
    area.removeFromTop (14);

    const auto width = area.getWidth() / 5;
    layoutKnob (area.removeFromLeft (width), threshold_, thresholdLabel_);
    layoutKnob (area.removeFromLeft (width), ratio_, ratioLabel_);
    layoutKnob (area.removeFromLeft (width), attack_, attackLabel_);
    layoutKnob (area.removeFromLeft (width), release_, releaseLabel_);
    layoutKnob (area, makeup_, makeupLabel_);
}
