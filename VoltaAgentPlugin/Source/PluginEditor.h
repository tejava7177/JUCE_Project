#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/AgentView.h"
#include "UI/ControllerView.h"
#include "UI/DebugPanel.h"

class VoltaAgentPluginAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit VoltaAgentPluginAudioProcessorEditor (VoltaAgentPluginAudioProcessor&);
    ~VoltaAgentPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshLayoutAndState();

    VoltaAgentPluginAudioProcessor& audioProcessor;

    juce::Label titleLabel;
    juce::Label trackTypeLabel;
    juce::Label statusTitleLabel;
    juce::Label statusValueLabel;
    juce::ComboBox trackTypeBox;

    AgentView agentView;
    ControllerView controllerView;
    DebugPanel debugPanel;

    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboAttachment> trackTypeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoltaAgentPluginAudioProcessorEditor)
};
