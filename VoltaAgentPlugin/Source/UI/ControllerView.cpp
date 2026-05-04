#include "ControllerView.h"
#include "../PluginProcessor.h"

namespace
{
void configureReadOnlyArea (juce::TextEditor& editor)
{
    editor.setMultiLine (true);
    editor.setReadOnly (true);
    editor.setScrollbarsShown (true);
    editor.setCaretVisible (false);
}
}

ControllerView::ControllerView (VoltaAgentPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    sessionStatusTitle.setText ("Session", juce::dontSendNotification);
    tracksTitle.setText ("Tracks", juce::dontSendNotification);
    promptTitle.setText ("AI Prompt", juce::dontSendNotification);
    explanationTitle.setText ("Explanation", juce::dontSendNotification);
    activityTitle.setText ("Activity Log", juce::dontSendNotification);
    plannedChangesTitle.setText ("Planned Changes", juce::dontSendNotification);

    sessionStatusTitle.setJustificationType (juce::Justification::centredLeft);
    sessionStatusLabel.setJustificationType (juce::Justification::centredLeft);

    promptEditor.setMultiLine (true);
    promptEditor.setReturnKeyStartsNewLine (true);
    promptEditor.addListener (this);

    configureReadOnlyArea (tracksList);
    configureReadOnlyArea (explanationValue);
    configureReadOnlyArea (activityLogValue);
    configureReadOnlyArea (plannedChangesValue);

    refreshSessionButton.addListener (this);
    planButton.addListener (this);
    applyButton.addListener (this);

    addAndMakeVisible (sessionStatusTitle);
    addAndMakeVisible (sessionStatusLabel);
    addAndMakeVisible (refreshSessionButton);
    addAndMakeVisible (tracksTitle);
    addAndMakeVisible (tracksList);
    addAndMakeVisible (promptTitle);
    addAndMakeVisible (promptEditor);
    addAndMakeVisible (planButton);
    addAndMakeVisible (applyButton);
    addAndMakeVisible (explanationTitle);
    addAndMakeVisible (explanationValue);
    addAndMakeVisible (activityTitle);
    addAndMakeVisible (activityLogValue);
    addAndMakeVisible (plannedChangesTitle);
    addAndMakeVisible (plannedChangesValue);

    refreshState();
}

ControllerView::~ControllerView()
{
    refreshSessionButton.removeListener (this);
    planButton.removeListener (this);
    applyButton.removeListener (this);
    promptEditor.removeListener (this);
}

void ControllerView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 18));
    g.fillRoundedRectangle (bounds, 10.0f);
    g.setColour (juce::Colour::fromRGBA (87, 225, 193, 90));
    g.drawRoundedRectangle (bounds, 10.0f, 1.0f);
}

void ControllerView::resized()
{
    auto area = getLocalBounds().reduced (16);
    auto topRow = area.removeFromTop (32);
    sessionStatusTitle.setBounds (topRow.removeFromLeft (64));
    sessionStatusLabel.setBounds (topRow.removeFromLeft (356));
    refreshSessionButton.setBounds (topRow.removeFromRight (160));

    area.removeFromTop (14);

    auto leftColumn = area.removeFromLeft (290);
    auto rightColumn = area;

    tracksTitle.setBounds (leftColumn.removeFromTop (20));
    tracksList.setBounds (leftColumn.removeFromTop (220));
    leftColumn.removeFromTop (14);
    activityTitle.setBounds (leftColumn.removeFromTop (20));
    activityLogValue.setBounds (leftColumn);

    promptTitle.setBounds (rightColumn.removeFromTop (20));
    promptEditor.setBounds (rightColumn.removeFromTop (72));
    rightColumn.removeFromTop (10);

    auto buttonRow = rightColumn.removeFromTop (30);
    planButton.setBounds (buttonRow.removeFromLeft (120));
    buttonRow.removeFromLeft (10);
    applyButton.setBounds (buttonRow.removeFromLeft (120));

    rightColumn.removeFromTop (14);
    explanationTitle.setBounds (rightColumn.removeFromTop (20));
    explanationValue.setBounds (rightColumn.removeFromTop (80));
    rightColumn.removeFromTop (14);
    plannedChangesTitle.setBounds (rightColumn.removeFromTop (20));
    plannedChangesValue.setBounds (rightColumn);
}

void ControllerView::refreshState()
{
    sessionStatusLabel.setText (audioProcessor.getSessionStatusText(), juce::dontSendNotification);
    tracksList.setText (audioProcessor.getTrackListText(), juce::dontSendNotification);
    explanationValue.setText (audioProcessor.getExplanationText(), juce::dontSendNotification);
    plannedChangesValue.setText (audioProcessor.getPlannedChangesText(), juce::dontSendNotification);
    activityLogValue.setText (audioProcessor.getActivityLogText(), juce::dontSendNotification);

    if (promptEditor.getText() != audioProcessor.getPromptText())
        promptEditor.setText (audioProcessor.getPromptText(), juce::dontSendNotification);

    auto busy = audioProcessor.isRequestInFlight();
    refreshSessionButton.setEnabled (! busy);
    planButton.setEnabled (! busy);
    applyButton.setEnabled (! busy && audioProcessor.canApplyPlan());
}

void ControllerView::buttonClicked (juce::Button* button)
{
    if (button == &refreshSessionButton)
    {
        audioProcessor.refreshSession();
    }
    else if (button == &planButton)
    {
        audioProcessor.setCurrentPrompt (promptEditor.getText());
        audioProcessor.planActions();
    }
    else if (button == &applyButton)
    {
        audioProcessor.applyPlannedActions();
    }

    refreshState();
}

void ControllerView::textEditorTextChanged (juce::TextEditor& editor)
{
    if (&editor == &promptEditor)
        audioProcessor.setCurrentPrompt (promptEditor.getText());
}
