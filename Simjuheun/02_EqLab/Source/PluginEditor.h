#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

// 현재 필터 계수의 실제 주파수 응답을 그리는 UI 컴포넌트다.
// 오디오를 분석하는 그래프가 아니라, 설정된 필터가 각 주파수를 얼마나
// 키우거나 줄일지를 미리 계산해 보여주는 그래프다.
class FrequencyResponseComponent final : public juce::Component,
                                         private juce::Timer
{
public:
    explicit FrequencyResponseComponent (EqLabAudioProcessor& processor);
    void paint (juce::Graphics& graphics) override;

private:
    void timerCallback() override;

    EqLabAudioProcessor& processor_;
};

class EqLabAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit EqLabAudioProcessorEditor (EqLabAudioProcessor& audioProcessor);

    void paint (juce::Graphics& graphics) override;
    void resized() override;

private:
    void updateControlsForFilterType();

    EqLabAudioProcessor& processor_;
    FrequencyResponseComponent responseGraph_;

    juce::Label titleLabel_;
    juce::Label subtitleLabel_;
    juce::Label filterTypeLabel_;
    juce::Label frequencyLabel_;
    juce::Label gainLabel_;
    juce::Label qLabel_;
    juce::Label statusLabel_;
    juce::ComboBox filterTypeBox_;
    juce::Slider frequencyKnob_;
    juce::Slider gainKnob_;
    juce::Slider qKnob_;
    juce::Slider slopeKnob_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> frequencyAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> slopeAttachment_;
};
