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
    juce::Label modeLabel;
    juce::Label agentIdLabel;
    juce::Label endpointLabel;
    juce::ComboBox modeBox;
    juce::TextEditor agentIdEditor;
    juce::TextEditor endpointEditor;
    juce::ToggleButton pollingToggle { "Enable Polling" };

    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboAttachment> modeAttachment;
};
