#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class EqLabAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit EqLabAudioProcessorEditor (EqLabAudioProcessor& audioProcessor);

    void paint (juce::Graphics& graphics) override;
    void resized() override;

private:
    EqLabAudioProcessor& processor_;

    juce::Label titleLabel_;
    juce::Label subtitleLabel_;
    juce::Label frequencyLabel_;
    juce::Label gainLabel_;
    juce::Label qLabel_;
    juce::Label statusLabel_;
    juce::Slider frequencyKnob_;
    juce::Slider gainKnob_;
    juce::Slider qKnob_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> frequencyAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttachment_;
};
