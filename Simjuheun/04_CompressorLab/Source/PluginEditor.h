#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class CompressionGraph final : public juce::Component,
                               private juce::Timer
{
public:
    explicit CompressionGraph (CompressorLabAudioProcessor& owner);
    void paint (juce::Graphics& graphics) override;

private:
    void timerCallback() override;
    CompressorLabAudioProcessor& processor_;
};

class CompressorLabAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit CompressorLabAudioProcessorEditor (CompressorLabAudioProcessor& owner);
    ~CompressorLabAudioProcessorEditor() override = default;

    void paint (juce::Graphics& graphics) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void configureKnob (juce::Slider& slider, juce::Label& label,
                        const juce::String& text);
    void layoutKnob (juce::Rectangle<int> bounds, juce::Slider& slider,
                     juce::Label& label);

    CompressorLabAudioProcessor& processor_;
    CompressionGraph graph_;
    juce::Slider threshold_, ratio_, attack_, release_, makeup_;
    juce::Label thresholdLabel_, ratioLabel_, attackLabel_, releaseLabel_, makeupLabel_;
    std::unique_ptr<SliderAttachment> thresholdAttachment_, ratioAttachment_;
    std::unique_ptr<SliderAttachment> attackAttachment_, releaseAttachment_, makeupAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorLabAudioProcessorEditor)
};
