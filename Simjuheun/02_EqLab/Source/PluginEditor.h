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
    juce::Label titleLabel_;
    juce::Label subtitleLabel_;
    juce::Label firLabel_;
    juce::Label iirLabel_;
    juce::Label statusLabel_;
};
