#pragma once

#include <JuceHeader.h>

class VoltaAgentPluginAudioProcessor;

class DebugPanel : public juce::Component,
                   private juce::Button::Listener,
                   private juce::TextEditor::Listener
{
public:
    explicit DebugPanel (VoltaAgentPluginAudioProcessor&);
    ~DebugPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshState();
    int getPreferredHeight() const;

    std::function<void()> onLayoutChange;

private:
    void buttonClicked (juce::Button* button) override;
    void textEditorFocusLost (juce::TextEditor& editor) override;
    void textEditorReturnKeyPressed (juce::TextEditor& editor) override;
    void commitTextEditors();
    void updateVisibility();

    VoltaAgentPluginAudioProcessor& audioProcessor;
    bool expanded = false;

    juce::TextButton expandButton { "Advanced Debug" };
    juce::Label endpointLabel;
    juce::TextEditor endpointEditor;
    juce::Label serverLabel;
    juce::Label sessionLabel;
    juce::TextButton refreshSessionButton { "Refresh Session" };
    juce::Label tracksLabel;
    juce::TextEditor tracksEditor;
    juce::Label explanationLabel;
    juce::TextEditor explanationEditor;
    juce::Label activityLabel;
    juce::TextEditor activityEditor;
};
