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
    void setSuggestedPrompt (const juce::String& promptText);

    VoltaAgentPluginAudioProcessor& audioProcessor;

    juce::Label chatTitle;
    juce::TextEditor chatTranscript;

    juce::Label analysisTitle;
    juce::TextEditor analysisStatusValue;

    juce::Label actionTitle;
    juce::TextButton trackLengthButton { "Track Lengths" };
    juce::TextButton groupingButton { "Grouping" };
    juce::TextButton gainBalanceButton { "Gain Balance" };
    juce::TextButton eqPrepButton { "EQ Prep" };

    juce::Label promptTitle;
    juce::TextEditor promptEditor;
    juce::TextButton planButton { "Send to AI" };
    juce::TextButton applyButton { "Apply Staged" };

    juce::Label sessionStatusTitle;
    juce::Label sessionStatusLabel;
    juce::TextButton refreshSessionButton { "Refresh Session" };

    juce::Label tracksTitle;
    juce::TextEditor tracksList;

    juce::Label explanationTitle;
    juce::TextEditor explanationValue;

    juce::Label plannedChangesTitle;
    juce::TextEditor plannedChangesValue;

    juce::Label activityTitle;
    juce::TextEditor activityLogValue;
};
