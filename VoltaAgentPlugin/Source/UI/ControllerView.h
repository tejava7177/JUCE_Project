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

    std::function<void()> onChooseStemFolder;

private:
    void buttonClicked (juce::Button* button) override;
    void textEditorTextChanged (juce::TextEditor& editor) override;
    void textEditorReturnKeyPressed (juce::TextEditor& editor) override;
    void setSuggestedPrompt (const juce::String& promptText);
    juce::String buildChatTranscript (const juce::String& promptText,
                                      const juce::String& sessionStatus,
                                      const juce::String& explanation) const;
    juce::String buildStepStatus (const juce::String& serverStatus,
                                  const juce::String& sessionStatus,
                                  const juce::String& analysisStatus) const;
    bool shouldShowAssistantReply (const juce::String& explanation,
                                   const juce::String& serverStatus) const;
    juce::String buildAssistantStatusLine (const juce::String& analysisStatus,
                                           const juce::String& sessionStatus) const;

    VoltaAgentPluginAudioProcessor& audioProcessor;

    juce::Label stepTitle;
    juce::Label stepValue;
    juce::Label stemFolderTitle;
    juce::Label stemFolderValue;
    juce::TextButton chooseStemFolderButton { "Choose Stem Folder" };
    juce::TextButton analyzeStemsButton { "Analyze WAV Stems" };

    juce::Label chatTitle;
    juce::TextEditor chatTranscript;

    juce::Label actionTitle;
    juce::TextButton trackLengthButton { "K-pop" };
    juce::TextButton groupingButton { "Hip-hop" };
    juce::TextButton gainBalanceButton { "Rock" };
    juce::TextButton eqPrepButton { "R&B" };

    juce::Label promptTitle;
    juce::TextEditor promptEditor;
    juce::TextButton planButton { "Start Session" };
    juce::TextButton applyButton { "Apply Staged" };

    juce::Label explanationTitle;
    juce::TextEditor explanationValue;

    juce::Label plannedChangesTitle;
    juce::TextEditor plannedChangesValue;
};
