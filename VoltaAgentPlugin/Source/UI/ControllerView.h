#pragma once

#include <JuceHeader.h>

class VoltaAgentPluginAudioProcessor;

class ControllerView : public juce::Component,
                       private juce::Button::Listener,
                       private juce::TextEditor::Listener
{
public:
    explicit ControllerView (VoltaAgentPluginAudioProcessor&);
    ~ControllerView() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshState();

private:
    void buttonClicked (juce::Button* button) override;
    void textEditorTextChanged (juce::TextEditor& editor) override;

    VoltaAgentPluginAudioProcessor& audioProcessor;

    juce::Label sessionStatusTitle;
    juce::Label sessionStatusLabel;
    juce::TextButton refreshSessionButton { "Refresh Session" };

    juce::Label tracksTitle;
    juce::TextEditor tracksList;

    juce::Label promptTitle;
    juce::TextEditor promptEditor;
    juce::TextButton planButton { "Plan" };
    juce::TextButton applyButton { "Apply" };

    juce::Label explanationTitle;
    juce::TextEditor explanationValue;

    juce::Label activityTitle;
    juce::TextEditor activityLogValue;

    juce::Label plannedChangesTitle;
    juce::TextEditor plannedChangesValue;
};
