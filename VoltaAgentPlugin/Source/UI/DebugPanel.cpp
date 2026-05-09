#include "DebugPanel.h"
#include "../PluginProcessor.h"

namespace
{
juce::String debugKorean (const char* escapedUnicode)
{
    return juce::String::fromUTF8 (juce::CharPointer_UTF8 (escapedUnicode));
}

void configureDebugReadOnlyArea (juce::TextEditor& editor)
{
    editor.setMultiLine (true);
    editor.setReadOnly (true);
    editor.setScrollbarsShown (true);
    editor.setCaretVisible (false);
    editor.setPopupMenuEnabled (false);
}

void configureSectionLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (juce::FontOptions (14.0f, juce::Font::bold));
}
}

DebugPanel::DebugPanel (VoltaAgentPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    addAndMakeVisible (expandButton);
    expandButton.addListener (this);
    expandButton.setButtonText (debugKorean ("\xEA\xB3\xA0\xEA\xB8\x89 \xEB\x94\x94\xEB\xB2\x84\xEA\xB7\xB8"));
    refreshSessionButton.setButtonText (debugKorean ("\xEC\x97\x90\xEC\x9D\xB4\xEB\xB8\x94\xED\x86\xA4 \xEC\xA0\x95\xEB\xB3\xB4 \xEB\xB6\x88\xEB\x9F\xAC\xEC\x98\xA4\xEA\xB8\xB0"));

    endpointLabel.setText (debugKorean ("\xEC\x84\x9C\xEB\xB2\x84 \xEC\xA3\xBC\xEC\x86\x8C"), juce::dontSendNotification);
    endpointEditor.addListener (this);

    configureSectionLabel (serverLabel, debugKorean ("\xEC\x84\x9C\xEB\xB2\x84 / \xEC\x84\xB8\xEC\x85\x98"));
    configureSectionLabel (projectLabel, debugKorean ("\xED\x94\x84\xEB\xA1\x9C\xEC\xA0\x9D\xED\x8A\xB8 \xEC\x84\xB8\xEC\x85\x98"));
    configureSectionLabel (tracksLabel, debugKorean ("\xED\x8A\xB8\xEB\x9E\x99 \xEC\xA0\x95\xEB\xB3\xB4"));
    configureSectionLabel (explanationLabel, debugKorean ("\xEC\xB5\x9C\xEA\xB7\xBC \xEC\x84\xA4\xEB\xAA\x85 / \xEB\xAF\xB8\xEB\xA6\xAC\xEB\xB3\xB4\xEA\xB8\xB0"));
    configureSectionLabel (activityLabel, debugKorean ("\xED\x99\x9C\xEB\x8F\x99 \xEB\xA1\x9C\xEA\xB7\xB8"));

    configureDebugReadOnlyArea (tracksEditor);
    configureDebugReadOnlyArea (explanationEditor);
    configureDebugReadOnlyArea (activityEditor);

    addChildComponent (endpointLabel);
    addChildComponent (endpointEditor);
    addChildComponent (serverLabel);
    addChildComponent (sessionLabel);
    addChildComponent (projectLabel);
    addChildComponent (projectValueLabel);
    addChildComponent (refreshSessionButton);
    addChildComponent (tracksLabel);
    addChildComponent (tracksEditor);
    addChildComponent (explanationLabel);
    addChildComponent (explanationEditor);
    addChildComponent (activityLabel);
    addChildComponent (activityEditor);

    refreshSessionButton.addListener (this);

    updateVisibility();
    refreshState();
}

DebugPanel::~DebugPanel()
{
    expandButton.removeListener (this);
    endpointEditor.removeListener (this);
    refreshSessionButton.removeListener (this);
}

void DebugPanel::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 20));
    g.drawRoundedRectangle (getLocalBounds().toFloat(), 8.0f, 1.0f);
}

void DebugPanel::resized()
{
    auto area = getLocalBounds().reduced (12);
    expandButton.setBounds (area.removeFromTop (28));

    if (! expanded)
        return;

    area.removeFromTop (10);
    constexpr int rowHeight = 28;
    constexpr int labelWidth = 92;

    auto endpointRow = area.removeFromTop (rowHeight);
    endpointLabel.setBounds (endpointRow.removeFromLeft (labelWidth));
    endpointEditor.setBounds (endpointRow);

    area.removeFromTop (10);
    serverLabel.setBounds (area.removeFromTop (22));
    area.removeFromTop (6);

    auto statusRow = area.removeFromTop (30);
    sessionLabel.setBounds (statusRow.removeFromLeft (juce::jmax (320, statusRow.getWidth() - 170)));
    statusRow.removeFromLeft (10);
    refreshSessionButton.setBounds (statusRow.removeFromLeft (160));

    area.removeFromTop (10);
    projectLabel.setBounds (area.removeFromTop (22));
    area.removeFromTop (6);
    projectValueLabel.setBounds (area.removeFromTop (32));
    area.removeFromTop (10);

    auto upperHeight = juce::jmin (132, juce::jmax (96, area.getHeight() / 2 - 12));
    auto upper = area.removeFromTop (upperHeight);
    auto lower = area;

    auto upperLeft = upper.removeFromLeft (juce::jmax (280, upper.getWidth() / 2 - 6));
    upper.removeFromLeft (12);
    auto upperRight = upper;

    tracksLabel.setBounds (upperLeft.removeFromTop (22));
    upperLeft.removeFromTop (6);
    tracksEditor.setBounds (upperLeft);

    explanationLabel.setBounds (upperRight.removeFromTop (22));
    upperRight.removeFromTop (6);
    explanationEditor.setBounds (upperRight);

    activityLabel.setBounds (lower.removeFromTop (22));
    lower.removeFromTop (6);
    activityEditor.setBounds (lower);
}

void DebugPanel::refreshState()
{
    endpointEditor.setText (audioProcessor.getServerEndpointText(), juce::dontSendNotification);
    sessionLabel.setText (debugKorean ("\xEC\x84\x9C\xEB\xB2\x84: ") + audioProcessor.getServerStatusText()
                              + " | " + debugKorean ("\xEC\x84\xB8\xEC\x85\x98: ") + audioProcessor.getSessionStatusText(),
                          juce::dontSendNotification);
    projectValueLabel.setText (debugKorean ("\xED\x94\x84\xEB\xA1\x9C\xEC\xA0\x9D\xED\x8A\xB8: ") + audioProcessor.getProjectSessionText()
                                   + " | " + debugKorean ("WAV \xED\x8F\xB4\xEB\x8D\x94: ")
                                   + (audioProcessor.getStemFolderText().isNotEmpty() ? audioProcessor.getStemFolderText() : debugKorean ("\xEC\x84\xA0\xED\x83\x9D \xEC\x95\x88 \xEB\x90\xA8"))
                                   + " | " + debugKorean ("\xEB\xB6\x84\xEC\x84\x9D: ") + audioProcessor.getAnalysisStatusText(),
                               juce::dontSendNotification);
    tracksEditor.setText (audioProcessor.getTrackListText(), juce::dontSendNotification);
    explanationEditor.setText (debugKorean ("\xEC\x84\xA4\xEB\xAA\x85:\n") + audioProcessor.getExplanationText()
                                   + debugKorean ("\n\n\xEB\xAF\xB8\xEB\xA6\xAC\xEB\xB3\xB4\xEA\xB8\xB0:\n") + audioProcessor.getPlannedChangesText(),
                               juce::dontSendNotification);
    activityEditor.setText (audioProcessor.getActivityLogText(), juce::dontSendNotification);
    refreshSessionButton.setEnabled (! audioProcessor.isRequestInFlight());
}

int DebugPanel::getPreferredHeight() const
{
    return expanded ? 360 : 52;
}

void DebugPanel::buttonClicked (juce::Button* button)
{
    if (button == &expandButton)
    {
        expanded = ! expanded;
        expandButton.setButtonText (expanded
                                        ? debugKorean ("\xEA\xB3\xA0\xEA\xB8\x89 \xEB\x94\x94\xEB\xB2\x84\xEA\xB7\xB8 \xEC\x88\xA8\xEA\xB8\xB0\xEA\xB8\xB0")
                                        : debugKorean ("\xEA\xB3\xA0\xEA\xB8\x89 \xEB\x94\x94\xEB\xB2\x84\xEA\xB7\xB8"));
        updateVisibility();
        if (onLayoutChange)
            onLayoutChange();
        return;
    }

    if (button == &refreshSessionButton)
        audioProcessor.refreshSession();
}

void DebugPanel::textEditorFocusLost (juce::TextEditor& editor)
{
    juce::ignoreUnused (editor);
    commitTextEditors();
}

void DebugPanel::textEditorReturnKeyPressed (juce::TextEditor& editor)
{
    juce::ignoreUnused (editor);
    commitTextEditors();
}

void DebugPanel::commitTextEditors()
{
    audioProcessor.setServerEndpoint (endpointEditor.getText());
    audioProcessor.requestServerHealth();
    refreshState();
}

void DebugPanel::updateVisibility()
{
    endpointLabel.setVisible (expanded);
    endpointEditor.setVisible (expanded);
    serverLabel.setVisible (expanded);
    sessionLabel.setVisible (expanded);
    projectLabel.setVisible (expanded);
    projectValueLabel.setVisible (expanded);
    refreshSessionButton.setVisible (expanded);
    tracksLabel.setVisible (expanded);
    tracksEditor.setVisible (expanded);
    explanationLabel.setVisible (expanded);
    explanationEditor.setVisible (expanded);
    activityLabel.setVisible (expanded);
    activityEditor.setVisible (expanded);
}
