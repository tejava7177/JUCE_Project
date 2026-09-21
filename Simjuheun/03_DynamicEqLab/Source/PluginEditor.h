#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class DynamicResponseView final : public juce::Component,
                                  private juce::Timer
{
public:
    explicit DynamicResponseView (DynamicEqLabAudioProcessor& processor);
    void paint (juce::Graphics& graphics) override;

private:
    void timerCallback() override;
    DynamicEqLabAudioProcessor& processor_;
};

class DynamicEqLabAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit DynamicEqLabAudioProcessorEditor (DynamicEqLabAudioProcessor& owner);
    ~DynamicEqLabAudioProcessorEditor() override = default;

    void paint (juce::Graphics& graphics) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void configureKnob (juce::Slider& slider, juce::Label& label,
                        const juce::String& labelText);
    void layoutKnob (juce::Rectangle<int> bounds, juce::Slider& slider,
                     juce::Label& label);

    DynamicEqLabAudioProcessor& processor_;
    DynamicResponseView response_;

    juce::Slider frequency_, q_, threshold_, ratio_, range_, attack_, release_;
    juce::Label frequencyLabel_, qLabel_, thresholdLabel_, ratioLabel_, rangeLabel_;
    juce::Label attackLabel_, releaseLabel_;

    std::unique_ptr<SliderAttachment> frequencyAttachment_;
    std::unique_ptr<SliderAttachment> qAttachment_;
    std::unique_ptr<SliderAttachment> thresholdAttachment_;
    std::unique_ptr<SliderAttachment> ratioAttachment_;
    std::unique_ptr<SliderAttachment> rangeAttachment_;
    std::unique_ptr<SliderAttachment> attackAttachment_;
    std::unique_ptr<SliderAttachment> releaseAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicEqLabAudioProcessorEditor)
};
