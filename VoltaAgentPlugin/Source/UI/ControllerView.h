#pragma once

#include <JuceHeader.h>

class VoltaAgentPluginAudioProcessor;

class ControllerView : public juce::Component,
                       private juce::Button::Listener
{
public:
    explicit ControllerView (VoltaAgentPluginAudioProcessor&);
    ~ControllerView() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshState();

private:
    void buttonClicked (juce::Button* button) override;

    VoltaAgentPluginAudioProcessor& audioProcessor;

    juce::Label commandLabel;
    juce::TextEditor commandEditor;
    juce::TextButton sendButton { "Send Command" };
    juce::TextButton analyzeButton { "Analyze Mix" };
    juce::Label agentsLabel;
    juce::TextEditor agentsList;
    juce::Label commandStatusTitle;
    juce::Label commandStatusValue;
    juce::Label analyzeStatusTitle;
    juce::Label analyzeStatusValue;
    juce::Label analyzeSummaryTitle;
    juce::TextEditor analyzeSummaryValue;
    juce::Label analyzeSuggestionsTitle;
    juce::TextEditor analyzeSuggestionsValue;
};
