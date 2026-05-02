#pragma once

#include <JuceHeader.h>

class VoltaAgentPluginAudioProcessor;

class AgentView : public juce::Component
{
public:
    explicit AgentView (VoltaAgentPluginAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshState();

private:
    VoltaAgentPluginAudioProcessor& audioProcessor;

    juce::Label sectionLabel;
    juce::Label volumeLabel;
    juce::Label lowCutLabel;
    juce::Label presenceLabel;
    juce::Label compressionLabel;
    juce::Label warmthLabel;
    juce::Label lastCommandTitle;
    juce::Label lastCommandValue;
    juce::Label lastAppliedTitle;
    juce::Label lastAppliedValue;

    juce::Slider volumeSlider;
    juce::ToggleButton lowCutToggle { "Enable" };
    juce::Slider lowCutFrequencySlider;
    juce::Slider presenceSlider;
    juce::Slider compressionSlider;
    juce::Slider warmthSlider;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> volumeAttachment;
    std::unique_ptr<ButtonAttachment> lowCutToggleAttachment;
    std::unique_ptr<SliderAttachment> lowCutFrequencyAttachment;
    std::unique_ptr<SliderAttachment> presenceAttachment;
    std::unique_ptr<SliderAttachment> compressionAttachment;
    std::unique_ptr<SliderAttachment> warmthAttachment;
};
