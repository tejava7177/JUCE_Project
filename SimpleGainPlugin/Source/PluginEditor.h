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
    juce::Label statusLabel_;
    juce::Slider gainKnob_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment_;
};
