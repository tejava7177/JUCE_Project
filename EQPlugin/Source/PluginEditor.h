#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MeterBar  : public juce::Component
{
public:
    void setValue (float newValue);
    void setAccentColour (juce::Colour newColour);
    void paint (juce::Graphics& g) override;

private:
    float value { 0.0f };
    juce::Colour accentColour { juce::Colours::skyblue };
};

class ResponseCurveComponent  : public juce::Component,
                                private juce::Timer
{
public:
    explicit ResponseCurveComponent (EQPluginAudioProcessor& processorToUse);
    ~ResponseCurveComponent() override;

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;
    float xToFrequency (float x) const;
    float frequencyToX (float frequency) const;

    EQPluginAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResponseCurveComponent)
};

class EQPluginAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    EQPluginAudioProcessorEditor (EQPluginAudioProcessor&);
    ~EQPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void timerCallback() override;
    void configureDial (juce::Slider& slider, const juce::String& suffix);
    void configureSlopeBox (juce::ComboBox& comboBox);
    void handleSendPrompt();
    void setPromptStatus (const juce::String& newStatus, juce::Colour colour = juce::Colours::white);
    void setPromptExplanation (const juce::String& newExplanation);
    juce::Label& addValueLabel (juce::Label& label, const juce::String& text);

    EQPluginAudioProcessor& audioProcessor;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    ResponseCurveComponent responseCurve;

    juce::GroupComponent controlsGroup;
    juce::Slider lowCutFreqSlider;
    juce::Slider peakFreqSlider;
    juce::Slider peakGainSlider;
    juce::Slider peakQSlider;
    juce::Slider highCutFreqSlider;
    juce::ComboBox lowCutSlopeBox;
    juce::ComboBox highCutSlopeBox;
    juce::Label lowCutFreqLabel;
    juce::Label lowCutSlopeLabel;
    juce::Label peakFreqLabel;
    juce::Label peakGainLabel;
    juce::Label peakQLabel;
    juce::Label highCutFreqLabel;
    juce::Label highCutSlopeLabel;

    juce::GroupComponent monitorGroup;
    juce::Label rmsLabel;
    juce::Label peakLabel;
    juce::Label balanceTitleLabel;
    juce::Label lowBalanceLabel;
    juce::Label midBalanceLabel;
    juce::Label highBalanceLabel;
    MeterBar lowBalanceBar;
    MeterBar midBalanceBar;
    MeterBar highBalanceBar;

    juce::GroupComponent promptGroup;
    juce::Label promptLabel;
    juce::TextEditor promptEditor;
    juce::TextButton sendButton;
    juce::Label promptStatusLabel;
    juce::Label promptExplanationLabel;

    std::unique_ptr<SliderAttachment> lowCutFreqAttachment;
    std::unique_ptr<ComboAttachment> lowCutSlopeAttachment;
    std::unique_ptr<SliderAttachment> peakFreqAttachment;
    std::unique_ptr<SliderAttachment> peakGainAttachment;
    std::unique_ptr<SliderAttachment> peakQAttachment;
    std::unique_ptr<SliderAttachment> highCutFreqAttachment;
    std::unique_ptr<ComboAttachment> highCutSlopeAttachment;

    std::atomic<bool> promptRequestInFlight { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQPluginAudioProcessorEditor)
};
