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

juce::String korean (const char* escapedUnicode)
{
    return juce::String::fromUTF8 (juce::CharPointer_UTF8 (escapedUnicode));
}

juce::String greetingLine()
{
    return korean ("\xEC\x95\x88\xEB\x85\x95\xED\x95\x98\xEC\x84\xB8\xEC\x9A\x94. \xEB\xA8\xBC\xEC\xA0\x80 \xEA\xB3\xA1\xEC\x9D\x98 \xEC\x9E\xA5\xEB\xA5\xB4\xEB\xA5\xBC \xEC\x95\x8C\xEB\xA0\xA4\xEC\xA3\xBC\xEC\x84\xB8\xEC\x9A\x94.");
}

juce::String greetingExample()
{
    return korean ("\xEC\x98\x88: Hip-hop, K-pop, Rock, R&B, EDM \xEB\x93\xB1");
}

juce::String syncingTracksLine()
{
    return korean ("\xED\x94\x84\xEB\xA1\x9C\xEC\xA0\x9D\xED\x8A\xB8 \xED\x8A\xB8\xEB\x9E\x99\xEC\x9D\x84 \xEB\xB0\xB1\xEA\xB7\xB8\xEB\x9D\xBC\xEC\x9A\xB4\xEB\x93\x9C\xEC\x97\x90\xEC\x84\x9C \xEB\x8F\x99\xEA\xB8\xB0\xED\x99\x94\xED\x95\x98\xEA\xB3\xA0 \xEC\x9E\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String sessionReadyLine()
{
    return korean ("\xEC\x84\xB8\xEC\x85\x98 \xEC\x8A\xA4\xEC\xBA\x94\xEC\x9D\xB4 \xEC\x99\x84\xEB\xA3\x8C\xEB\x90\x98\xEC\x97\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4. \xEC\x9E\xA5\xEB\xA5\xB4\xEB\xA5\xBC \xEC\x9E\x85\xEB\xA0\xA5\xED\x95\x98\xEB\xA9\xB4 \xEB\x8B\xA4\xEC\x9D\x8C \xEB\xB6\x84\xEC\x84\x9D \xEB\x8B\xA8\xEA\xB3\x84\xEB\xA1\x9C \xEB\x84\x98\xEC\x96\xB4\xEA\xB0\x91\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String analysisPreparingLine()
{
    return korean ("\xED\x94\x84\xEB\xA1\x9C\xEC\xA0\x9D\xED\x8A\xB8 \xEB\xB6\x84\xEC\x84\x9D\xEC\x9D\x84 \xEC\xA4\x80\xEB\xB9\x84 \xEC\xA4\x91\xEC\x9E\x85\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String syncStatusLine()
{
    return korean ("\xED\x8A\xB8\xEB\x9E\x99 \xEC\xA0\x95\xEB\xB3\xB4\xEB\xA5\xBC \xEC\x84\x9C\xEB\xB2\x84\xEB\xA1\x9C \xEB\x8F\x99\xEA\xB8\xB0\xED\x99\x94\xED\x95\x98\xEB\x8A\x94 \xEC\xA4\x91\xEC\x9E\x85\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String waitingGenreLine()
{
    return korean ("\xEC\x9E\xA5\xEB\xA5\xB4 \xEC\x9E\x85\xEB\xA0\xA5 \xEB\x8C\x80\xEA\xB8\xB0 \xEC\xA4\x91");
}

juce::String sessionReadyShortLine()
{
    return korean ("\xEC\x84\xB8\xEC\x85\x98 \xEC\xA4\x80\xEB\xB9\x84 \xEC\x99\x84\xEB\xA3\x8C");
}

juce::String genrePromptTitle()
{
    return korean ("\xEC\x9E\xA5\xEB\xA5\xB4\xEC\x99\x80 \xEC\xB2\xAB \xEC\x9A\x94\xEC\xB2\xAD");
}
}

ControllerView::ControllerView (VoltaAgentPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    configureCardLabel (chatTitle, "Assistant");
    configureCardLabel (analysisTitle, "Analysis Status");
    configureCardLabel (actionTitle, "Genre Shortcuts");
    configureCardLabel (promptTitle, genrePromptTitle());
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
    chatLines.add ("AI: " + greetingLine());
    chatLines.add ("AI: " + greetingExample());
    if (sessionStatus.contains ("tracks loaded"))
        chatLines.add ("AI: " + sessionReadyLine());
    else
        chatLines.add ("AI: " + syncingTracksLine());
    if (promptText.isNotEmpty())
        chatLines.add ("User: " + promptText);

    juce::StringArray analysisLines;
    analysisLines.add (analysisPreparingLine());
    analysisLines.add (syncStatusLine());
    analysisLines.add ("Server: " + serverStatus);
    analysisLines.add ("Session: " + sessionStatus);
    analysisLines.add (sessionStatus.contains ("tracks loaded") ? sessionReadyShortLine() : waitingGenreLine());
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
        setSuggestedPrompt ("K-pop");
    }
    else if (button == &groupingButton)
    {
        setSuggestedPrompt ("Hip-hop");
    }
    else if (button == &gainBalanceButton)
    {
        setSuggestedPrompt ("Rock");
    }
    else if (button == &eqPrepButton)
    {
        setSuggestedPrompt ("R&B");
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
