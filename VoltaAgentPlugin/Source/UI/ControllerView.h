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
    juce::String buildProgressText (const juce::String& serverStatus,
                                    const juce::String& sessionStatus,
                                    const juce::String& analysisStatus) const;
    juce::String buildAssistantText (const juce::String& explanation,
                                     const juce::String& sessionStatus,
                                     const juce::String& analysisStatus) const;
    bool isWaitingState (const juce::String& analysisStatus) const;
    void sendCurrentPrompt();

    VoltaAgentPluginAudioProcessor& audioProcessor;

    juce::Label progressTitle;
    juce::Label progressValue;
    juce::Label stemFolderTitle;
    juce::Label stemFolderValue;
    juce::TextButton chooseStemFolderButton { "Choose Stem Folder" };
    juce::TextButton analyzeStemsButton { "Analyze WAV Stems" };

    juce::Label assistantTitle;
    juce::TextEditor assistantBubble;
    juce::Label userTitle;
    juce::TextEditor userBubble;
    juce::Label composerTitle;
    juce::TextEditor promptEditor;
    juce::TextButton planButton { "Send" };

    juce::Label plannedChangesTitle;
    juce::TextEditor plannedChangesValue;
};
