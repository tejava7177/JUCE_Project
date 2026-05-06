#include "DebugPanel.h"
#include "../PluginProcessor.h"

namespace
{
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

    endpointLabel.setText ("Server URL", juce::dontSendNotification);
    endpointEditor.addListener (this);

    configureSectionLabel (serverLabel, "Server / Session");
    configureSectionLabel (projectLabel, "Project Session");
    configureSectionLabel (tracksLabel, "Tracks");
    configureSectionLabel (explanationLabel, "Latest Explanation / Preview");
    configureSectionLabel (activityLabel, "Activity Log");

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
    sessionLabel.setText ("Server: " + audioProcessor.getServerStatusText()
                              + " | Session: " + audioProcessor.getSessionStatusText(),
                          juce::dontSendNotification);
    projectValueLabel.setText ("Project: " + audioProcessor.getProjectSessionText()
                                   + " | Stem folder: "
                                   + (audioProcessor.getStemFolderText().isNotEmpty() ? audioProcessor.getStemFolderText() : "Not selected")
                                   + " | Analysis: " + audioProcessor.getAnalysisStatusText(),
                               juce::dontSendNotification);
    tracksEditor.setText (audioProcessor.getTrackListText(), juce::dontSendNotification);
    explanationEditor.setText ("Explanation:\n" + audioProcessor.getExplanationText()
                                   + "\n\nPreview:\n" + audioProcessor.getPlannedChangesText(),
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
        expandButton.setButtonText (expanded ? "Hide Advanced Debug" : "Advanced Debug");
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
