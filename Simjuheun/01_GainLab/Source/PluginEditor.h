#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class GainLabAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit GainLabAudioProcessorEditor (GainLabAudioProcessor& audioProcessor);

    void paint (juce::Graphics& graphics) override;
    void resized() override;

private:
    GainLabAudioProcessor& processor_;

    juce::Label titleLabel_;
    juce::Label gainLabel_;
    juce::Label delayTimeLabel_;
    juce::Label feedbackLabel_;
    juce::Label mixLabel_;
    juce::Label statusLabel_;
    juce::Slider gainKnob_;
    juce::Slider delayTimeKnob_;
    juce::Slider feedbackKnob_;
    juce::Slider mixKnob_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment_;
};
