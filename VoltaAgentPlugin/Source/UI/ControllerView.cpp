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
    editor.setPopupMenuEnabled (false);
}

void configureCardLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
}
}

ControllerView::ControllerView (VoltaAgentPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    configureCardLabel (chatTitle, "Assistant");
    configureCardLabel (analysisTitle, "Analysis Status");
    configureCardLabel (actionTitle, "Suggested Actions");
    configureCardLabel (promptTitle, "Prompt");
    configureCardLabel (sessionStatusTitle, "Session");
    configureCardLabel (tracksTitle, "Tracks");
    configureCardLabel (explanationTitle, "Latest Explanation");
    configureCardLabel (plannedChangesTitle, "Result Preview");
    configureCardLabel (activityTitle, "Activity Log");

    sessionStatusLabel.setJustificationType (juce::Justification::centredLeft);
    sessionStatusLabel.setFont (juce::FontOptions (15.0f, juce::Font::plain));

    promptEditor.setMultiLine (true);
    promptEditor.setReturnKeyStartsNewLine (true);
    promptEditor.addListener (this);

    configureReadOnlyArea (chatTranscript);
    configureReadOnlyArea (analysisStatusValue);
    configureReadOnlyArea (tracksList);
    configureReadOnlyArea (explanationValue);
    configureReadOnlyArea (plannedChangesValue);
    configureReadOnlyArea (activityLogValue);

    refreshSessionButton.addListener (this);
    planButton.addListener (this);
    applyButton.addListener (this);
    trackLengthButton.addListener (this);
    groupingButton.addListener (this);
    gainBalanceButton.addListener (this);
    eqPrepButton.addListener (this);

    addAndMakeVisible (chatTitle);
    addAndMakeVisible (chatTranscript);
    addAndMakeVisible (analysisTitle);
    addAndMakeVisible (analysisStatusValue);
    addAndMakeVisible (actionTitle);
    addAndMakeVisible (trackLengthButton);
    addAndMakeVisible (groupingButton);
    addAndMakeVisible (gainBalanceButton);
    addAndMakeVisible (eqPrepButton);
    addAndMakeVisible (promptTitle);
    addAndMakeVisible (promptEditor);
    addAndMakeVisible (planButton);
    addAndMakeVisible (applyButton);
    addAndMakeVisible (sessionStatusTitle);
    addAndMakeVisible (sessionStatusLabel);
    addAndMakeVisible (refreshSessionButton);
    addAndMakeVisible (tracksTitle);
    addAndMakeVisible (tracksList);
    addAndMakeVisible (explanationTitle);
    addAndMakeVisible (explanationValue);
    addAndMakeVisible (plannedChangesTitle);
    addAndMakeVisible (plannedChangesValue);
    addAndMakeVisible (activityTitle);
    addAndMakeVisible (activityLogValue);

    refreshState();
}

ControllerView::~ControllerView()
{
    refreshSessionButton.removeListener (this);
    planButton.removeListener (this);
    applyButton.removeListener (this);
    trackLengthButton.removeListener (this);
    groupingButton.removeListener (this);
    gainBalanceButton.removeListener (this);
    eqPrepButton.removeListener (this);
    promptEditor.removeListener (this);
}

void ControllerView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 16));
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (juce::Colour::fromRGBA (87, 225, 193, 70));
    g.drawRoundedRectangle (bounds, 12.0f, 1.0f);

    auto area = getLocalBounds().reduced (14);
    auto top = area.removeFromTop (268);
    auto bottom = area;

    auto leftTop = top.removeFromLeft (420);
    auto rightTop = top;

    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 10));
    g.fillRoundedRectangle (leftTop.toFloat(), 10.0f);
    g.fillRoundedRectangle (rightTop.toFloat(), 10.0f);

    auto leftBottom = bottom.removeFromLeft (420);
    auto rightBottom = bottom;
    g.fillRoundedRectangle (leftBottom.toFloat(), 10.0f);
    g.fillRoundedRectangle (rightBottom.toFloat(), 10.0f);
}

void ControllerView::resized()
{
    auto area = getLocalBounds().reduced (18);

    auto topArea = area.removeFromTop (268);
    auto bottomArea = area;

    auto leftTop = topArea.removeFromLeft (420);
    topArea.removeFromLeft (12);
    auto rightTop = topArea;

    auto leftTopHeader = leftTop.removeFromTop (24);
    chatTitle.setBounds (leftTopHeader.removeFromLeft (120));
    leftTop.removeFromTop (8);
    chatTranscript.setBounds (leftTop.removeFromTop (116));
    leftTop.removeFromTop (12);
    analysisTitle.setBounds (leftTop.removeFromTop (24));
    leftTop.removeFromTop (8);
    analysisStatusValue.setBounds (leftTop.removeFromTop (88));

    auto sessionHeader = rightTop.removeFromTop (24);
    sessionStatusTitle.setBounds (sessionHeader.removeFromLeft (72));
    sessionStatusLabel.setBounds (sessionHeader.removeFromLeft (420));
    refreshSessionButton.setBounds (sessionHeader.removeFromRight (170));

    rightTop.removeFromTop (12);
    actionTitle.setBounds (rightTop.removeFromTop (24));
    rightTop.removeFromTop (8);

    auto actionRow = rightTop.removeFromTop (30);
    trackLengthButton.setBounds (actionRow.removeFromLeft (140));
    actionRow.removeFromLeft (8);
    groupingButton.setBounds (actionRow.removeFromLeft (110));
    actionRow.removeFromLeft (8);
    gainBalanceButton.setBounds (actionRow.removeFromLeft (128));
    actionRow.removeFromLeft (8);
    eqPrepButton.setBounds (actionRow.removeFromLeft (100));

    rightTop.removeFromTop (12);
    promptTitle.setBounds (rightTop.removeFromTop (24));
    rightTop.removeFromTop (8);
    promptEditor.setBounds (rightTop.removeFromTop (86));
    rightTop.removeFromTop (10);

    auto promptButtons = rightTop.removeFromTop (32);
    planButton.setBounds (promptButtons.removeFromLeft (140));
    promptButtons.removeFromLeft (10);
    applyButton.setBounds (promptButtons.removeFromLeft (140));

    auto leftBottom = bottomArea.removeFromLeft (420);
    bottomArea.removeFromLeft (12);
    auto rightBottom = bottomArea;

    tracksTitle.setBounds (leftBottom.removeFromTop (24));
    leftBottom.removeFromTop (8);
    tracksList.setBounds (leftBottom.removeFromTop (144));
    leftBottom.removeFromTop (12);
    activityTitle.setBounds (leftBottom.removeFromTop (24));
    leftBottom.removeFromTop (8);
    activityLogValue.setBounds (leftBottom);

    explanationTitle.setBounds (rightBottom.removeFromTop (24));
    rightBottom.removeFromTop (8);
    explanationValue.setBounds (rightBottom.removeFromTop (84));
    rightBottom.removeFromTop (12);
    plannedChangesTitle.setBounds (rightBottom.removeFromTop (24));
    rightBottom.removeFromTop (8);
    plannedChangesValue.setBounds (rightBottom);
}

void ControllerView::refreshState()
{
    auto promptText = audioProcessor.getPromptText();
    auto sessionStatus = audioProcessor.getSessionStatusText();
    auto serverStatus = audioProcessor.getServerStatusText();
    auto explanation = audioProcessor.getExplanationText();

    juce::StringArray chatLines;
    chatLines.add ("AI: Ready to start project analysis.");
    chatLines.add ("AI: Please enter the genre and the first task you want to try.");
    if (sessionStatus.contains ("tracks loaded"))
        chatLines.add ("AI: Session scan is complete. You can now review track lengths, grouping, and gain balance ideas.");
    else
        chatLines.add ("AI: If the session is not ready yet, use Refresh Session to load the latest project state.");
    if (promptText.isNotEmpty())
        chatLines.add ("User: " + promptText);

    juce::StringArray analysisLines;
    analysisLines.add ("Project session: pending scaffold");
    analysisLines.add ("Chat session: UI-first mockup");
    analysisLines.add ("Server: " + serverStatus);
    analysisLines.add ("Session: " + sessionStatus);
    analysisLines.add ("Latest result: " + explanation);

    chatTranscript.setText (chatLines.joinIntoString ("\n\n"), juce::dontSendNotification);
    analysisStatusValue.setText (analysisLines.joinIntoString ("\n"), juce::dontSendNotification);
    sessionStatusLabel.setText (sessionStatus, juce::dontSendNotification);
    tracksList.setText (audioProcessor.getTrackListText(), juce::dontSendNotification);
    explanationValue.setText (explanation, juce::dontSendNotification);
    plannedChangesValue.setText (audioProcessor.getPlannedChangesText(), juce::dontSendNotification);
    activityLogValue.setText (audioProcessor.getActivityLogText(), juce::dontSendNotification);

    if (promptEditor.getText() != promptText)
        promptEditor.setText (promptText, juce::dontSendNotification);

    auto busy = audioProcessor.isRequestInFlight();
    refreshSessionButton.setEnabled (! busy);
    planButton.setEnabled (! busy);
    applyButton.setEnabled (! busy && audioProcessor.canApplyPlan());
    trackLengthButton.setEnabled (! busy);
    groupingButton.setEnabled (! busy);
    gainBalanceButton.setEnabled (! busy);
    eqPrepButton.setEnabled (! busy);
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
    else if (button == &trackLengthButton)
    {
        setSuggestedPrompt ("Summarize the length and role of each track first.");
    }
    else if (button == &groupingButton)
    {
        setSuggestedPrompt ("Suggest drum, instrument, and vocal groupings for this session.");
    }
    else if (button == &gainBalanceButton)
    {
        setSuggestedPrompt ("Which tracks should I adjust first for basic gain balance?");
    }
    else if (button == &eqPrepButton)
    {
        setSuggestedPrompt ("Which tracks should I clean up first for low cut and high cut EQ?");
    }

    refreshState();
}

void ControllerView::textEditorTextChanged (juce::TextEditor& editor)
{
    if (&editor == &promptEditor)
        audioProcessor.setCurrentPrompt (promptEditor.getText());
}

void ControllerView::setSuggestedPrompt (const juce::String& promptText)
{
    promptEditor.setText (promptText, juce::dontSendNotification);
    audioProcessor.setCurrentPrompt (promptText);
}
