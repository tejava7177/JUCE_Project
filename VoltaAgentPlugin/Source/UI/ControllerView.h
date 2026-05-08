#pragma once

#include <JuceHeader.h>

class VoltaAgentPluginAudioProcessor;

class ControllerView : public juce::Component,
                       public juce::FileDragAndDropTarget,
                       private juce::Button::Listener,
                       private juce::TextEditor::Listener
{
public:
    explicit ControllerView (VoltaAgentPluginAudioProcessor&);
    ~ControllerView() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshState();
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragMove (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

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
    juce::File resolveDroppedStemFolder (const juce::StringArray& files) const;
    static int countWavFilesInFolder (const juce::File& folder);
    void sendCurrentPrompt();

    VoltaAgentPluginAudioProcessor& audioProcessor;
    juce::Rectangle<int> chatUploadBounds;
    bool dragOverUploadZone = false;
    juce::String attachedFolderHeadline { "No WAV folder attached" };
    juce::String attachedFolderMeta { "Drop a folder here or choose one manually." };
    juce::String analysisUploadStateMeta { "0 wav files" };

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
    juce::Label chatTitle;

    juce::Label plannedChangesTitle;
    juce::TextEditor plannedChangesValue;
};
