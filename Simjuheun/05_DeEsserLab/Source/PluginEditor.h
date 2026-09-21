#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class DetectorView final : public juce::Component,
                           private juce::Timer
{
public:
    explicit DetectorView (DeEsserLabAudioProcessor& owner);
    void paint (juce::Graphics& graphics) override;

private:
    void timerCallback() override;
    DeEsserLabAudioProcessor& processor_;
};

class DeEsserLabAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit DeEsserLabAudioProcessorEditor (DeEsserLabAudioProcessor& owner);
    ~DeEsserLabAudioProcessorEditor() override = default;
    void paint (juce::Graphics& graphics) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void configureKnob (juce::Slider& slider, juce::Label& label,
                        const juce::String& text);
    void layoutKnob (juce::Rectangle<int> bounds, juce::Slider& slider,
                     juce::Label& label);

    DeEsserLabAudioProcessor& processor_;
    DetectorView detectorView_;
    juce::Slider frequency_, q_, threshold_, ratio_, range_, attack_, release_;
    juce::Label frequencyLabel_, qLabel_, thresholdLabel_, ratioLabel_, rangeLabel_;
    juce::Label attackLabel_, releaseLabel_;
    juce::ToggleButton listen_ { "DETECTOR LISTEN" };
    std::unique_ptr<SliderAttachment> frequencyAttachment_, qAttachment_;
    std::unique_ptr<SliderAttachment> thresholdAttachment_, ratioAttachment_, rangeAttachment_;
    std::unique_ptr<SliderAttachment> attackAttachment_, releaseAttachment_;
    std::unique_ptr<ButtonAttachment> listenAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeEsserLabAudioProcessorEditor)
};
