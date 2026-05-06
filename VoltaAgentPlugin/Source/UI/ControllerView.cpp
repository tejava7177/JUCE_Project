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

void configureBubbleArea (juce::TextEditor& editor)
{
    configureReadOnlyArea (editor);
    editor.setColour (juce::TextEditor::backgroundColourId, juce::Colour::fromRGB (34, 46, 52));
    editor.setColour (juce::TextEditor::outlineColourId, juce::Colour::fromRGBA (255, 255, 255, 0));
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
    return korean ("\xED\x94\x84\xEB\xA1\x9C\xEC\xA0\x9D\xED\x8A\xB8 \xED\x8A\xB8\xEB\x9E\x99 \xEC\xA0\x95\xEB\xB3\xB4\xEB\xA5\xBC \xEB\xB0\xB1\xEA\xB7\xB8\xEB\x9D\xBC\xEC\x9A\xB4\xEB\x93\x9C\xEC\x97\x90\xEC\x84\x9C \xEC\x84\x9C\xEB\xB2\x84\xEB\xA1\x9C \xEB\xB3\xB4\xEB\x82\xB4\xEA\xB3\xA0 \xEC\x9E\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String sessionReadyLine()
{
    return korean ("\xEC\x84\xB8\xEC\x85\x98 \xEC\x8A\xA4\xEC\xBA\x94\xEC\x9D\xB4 \xEC\x99\x84\xEB\xA3\x8C\xEB\x90\x98\xEC\x97\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4. \xEC\x9E\xA5\xEB\xA5\xB4\xEB\xA5\xBC \xEC\x9E\x85\xEB\xA0\xA5\xED\x95\x98\xEB\xA9\xB4 \xEB\x8B\xA4\xEC\x9D\x8C \xEB\xB6\x84\xEC\x84\x9D \xEB\x8B\xA8\xEA\xB3\x84\xEB\xA1\x9C \xEB\x84\x98\xEC\x96\xB4\xEA\xB0\x91\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String promptTitleText()
{
    return korean ("\xEC\x9E\xA5\xEB\xA5\xB4\xEC\x99\x80 \xEC\xB2\xAB \xEC\x9A\x94\xEC\xB2\xAD");
}

juce::String explanationPlaceholder()
{
    return korean ("\xEC\x9E\xA5\xEB\xA5\xB4\xEB\xA5\xBC \xEC\x9E\x85\xEB\xA0\xA5\xED\x95\x98\xEA\xB3\xA0 Start Session\xEC\x9D\x84 \xEB\x88\x84\xEB\xA5\xB4\xEB\xA9\xB4 \xEB\x8B\xA4\xEC\x9D\x8C \xEB\x8B\xA8\xEA\xB3\x84 \xEA\xB0\x80\xEC\x9D\xB4\xEB\x93\x9C\xEA\xB0\x80 \xEC\x97\xAC\xEA\xB8\xB0\xEC\x97\x90 \xEB\x82\x98\xEC\x98\xB5\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String previewPlaceholder()
{
    return korean ("\xEC\x95\x84\xEC\xA7\x81 \xEC\xA0\x81\xEC\x9A\xA9\xED\x95\xA0 \xEA\xB3\x84\xED\x9A\x8D\xEC\x9D\xB4 \xEC\x97\x86\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String analysisReadyLine()
{
    return korean ("\xEC\x8A\xA4\xED\x85\x9C \xEB\xB6\x84\xEC\x84\x9D\xEC\x9D\xB4 \xEC\xA4\x80\xEB\xB9\x84\xEB\x90\x98\xEC\x97\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4. WAV \xED\x8F\xB4\xEB\x8D\x94\xEB\xA5\xBC \xEC\x84\xA0\xED\x83\x9D\xED\x95\x98\xEA\xB3\xA0 Analyze WAV Stems\xEB\xA5\xBC \xEB\x88\x8C\xEB\x9F\xAC\xEC\xA3\xBC\xEC\x84\xB8\xEC\x9A\x94.");
}

juce::String analysisCompletedLine()
{
    return korean ("\xEC\x8A\xA4\xED\x85\x9C \xEB\xB6\x84\xEC\x84\x9D\xEC\x9D\xB4 \xEC\x99\x84\xEB\xA3\x8C\xEB\x90\x98\xEC\x97\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4. \xEC\x9D\xB4\xEC\xA0\x9C \xEC\x9E\xA5\xEB\xA5\xB4\xEC\x99\x80 \xEC\xB2\xAB \xEC\x9A\x94\xEC\xB2\xAD\xEC\x9D\x84 \xEC\x9E\x85\xEB\xA0\xA5\xED\x95\x98\xEB\xA9\xB4 \xEB\x8C\x80\xED\x99\x94\xEB\xA5\xBC \xEC\x8B\x9C\xEC\x9E\x91\xED\x95\xA0 \xEC\x88\x98 \xEC\x9E\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String uploadingLine()
{
    return korean ("\xEC\x8A\xA4\xED\x85\x9C WAV \xED\x8C\x8C\xEC\x9D\xBC\xEC\x9D\x84 \xEC\x84\x9C\xEB\xB2\x84\xEB\xA1\x9C \xEC\x97\x85\xEB\xA1\x9C\xEB\x93\x9C\xED\x95\x98\xEA\xB3\xA0 \xEB\xB6\x84\xEC\x84\x9D\xED\x95\x98\xEB\x8A\x94 \xEC\xA4\x91\xEC\x9E\x85\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String projectReadyLine()
{
    return korean ("\xED\x94\x84\xEB\xA1\x9C\xEC\xA0\x9D\xED\x8A\xB8 \xEC\x84\xB8\xEC\x85\x98\xEC\x9D\xB4 \xEC\x83\x9D\xEC\x84\xB1\xEB\x90\x98\xEC\x97\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4. \xEC\x9E\xA5\xEB\xA5\xB4\xEC\x99\x80 \xEC\xB2\xAB \xEC\x9A\x94\xEC\xB2\xAD\xEC\x9D\x84 \xEC\x9E\x85\xEB\xA0\xA5\xED\x95\x98\xEB\xA9\xB4 \xEC\x84\x9C\xEB\xB2\x84 \xEC\xB1\x84\xED\x8C\x85 \xEC\x84\xB8\xEC\x85\x98\xEC\x9D\x84 \xEC\x9D\xB4\xEC\x96\xB4\xEA\xB0\x88 \xEC\x88\x98 \xEC\x9E\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4.");
}
}

ControllerView::ControllerView (VoltaAgentPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    configureCardLabel (stepTitle, "Current Step");
    configureCardLabel (stemFolderTitle, "Stem Folder");
    configureCardLabel (chatTitle, "Chat");
    configureCardLabel (actionTitle, "Genre Shortcuts");
    configureCardLabel (promptTitle, promptTitleText());
    configureCardLabel (explanationTitle, "Assistant Reply");
    configureCardLabel (plannedChangesTitle, "Action Preview");

    stepValue.setJustificationType (juce::Justification::centredLeft);
    stepValue.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    stepValue.setColour (juce::Label::backgroundColourId, juce::Colour::fromRGB (34, 46, 52));
    stepValue.setColour (juce::Label::outlineColourId, juce::Colour::fromRGBA (255, 255, 255, 0));
    stepValue.setBorderSize (juce::BorderSize<int> (8, 12, 8, 12));
    stemFolderValue.setJustificationType (juce::Justification::centredLeft);
    stemFolderValue.setFont (juce::FontOptions (14.0f, juce::Font::plain));
    stemFolderValue.setColour (juce::Label::backgroundColourId, juce::Colour::fromRGB (34, 46, 52));
    stemFolderValue.setColour (juce::Label::outlineColourId, juce::Colour::fromRGBA (255, 255, 255, 0));
    stemFolderValue.setBorderSize (juce::BorderSize<int> (8, 12, 8, 12));

    promptEditor.setMultiLine (true);
    promptEditor.setReturnKeyStartsNewLine (false);
    promptEditor.addListener (this);

    configureBubbleArea (chatTranscript);
    configureBubbleArea (explanationValue);
    configureBubbleArea (plannedChangesValue);

    planButton.addListener (this);
    applyButton.addListener (this);
    chooseStemFolderButton.addListener (this);
    analyzeStemsButton.addListener (this);
    trackLengthButton.addListener (this);
    groupingButton.addListener (this);
    gainBalanceButton.addListener (this);
    eqPrepButton.addListener (this);

    addAndMakeVisible (stepTitle);
    addAndMakeVisible (stepValue);
    addAndMakeVisible (stemFolderTitle);
    addAndMakeVisible (stemFolderValue);
    addAndMakeVisible (chooseStemFolderButton);
    addAndMakeVisible (analyzeStemsButton);
    addAndMakeVisible (chatTitle);
    addAndMakeVisible (chatTranscript);
    addAndMakeVisible (actionTitle);
    addAndMakeVisible (trackLengthButton);
    addAndMakeVisible (groupingButton);
    addAndMakeVisible (gainBalanceButton);
    addAndMakeVisible (eqPrepButton);
    addAndMakeVisible (promptTitle);
    addAndMakeVisible (promptEditor);
    addAndMakeVisible (planButton);
    addAndMakeVisible (applyButton);
    addAndMakeVisible (explanationTitle);
    addAndMakeVisible (explanationValue);
    addAndMakeVisible (plannedChangesTitle);
    addAndMakeVisible (plannedChangesValue);

    refreshState();
}

ControllerView::~ControllerView()
{
    planButton.removeListener (this);
    applyButton.removeListener (this);
    chooseStemFolderButton.removeListener (this);
    analyzeStemsButton.removeListener (this);
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
    auto top = area.removeFromTop (260);
    auto middle = area.removeFromTop (150);
    area.removeFromTop (12);
    auto bottom = area;

    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 10));
    g.fillRoundedRectangle (top.toFloat(), 10.0f);
    g.fillRoundedRectangle (middle.toFloat(), 10.0f);
    g.fillRoundedRectangle (bottom.toFloat(), 10.0f);
}

void ControllerView::resized()
{
    auto area = getLocalBounds().reduced (18);

    stepTitle.setBounds (area.removeFromTop (24));
    area.removeFromTop (8);
    stepValue.setBounds (area.removeFromTop (40));
    area.removeFromTop (14);

    auto stemRow = area.removeFromTop (36);
    stemFolderTitle.setBounds (stemRow.removeFromLeft (100));
    stemFolderValue.setBounds (stemRow.removeFromLeft (juce::jmax (220, stemRow.getWidth() - 360)));
    stemRow.removeFromLeft (8);
    chooseStemFolderButton.setBounds (stemRow.removeFromLeft (160));
    stemRow.removeFromLeft (8);
    analyzeStemsButton.setBounds (stemRow);
    area.removeFromTop (14);

    auto top = area.removeFromTop (260);
    auto middle = area.removeFromTop (150);
    area.removeFromTop (12);
    auto bottom = area;

    chatTitle.setBounds (top.removeFromTop (24));
    top.removeFromTop (8);
    chatTranscript.setBounds (top.removeFromTop (106));
    top.removeFromTop (12);

    actionTitle.setBounds (top.removeFromTop (24));
    top.removeFromTop (8);
    auto actionRow = top.removeFromTop (40);
    auto buttonWidth = (actionRow.getWidth() - 24) / 4;
    trackLengthButton.setBounds (actionRow.removeFromLeft (buttonWidth));
    actionRow.removeFromLeft (8);
    groupingButton.setBounds (actionRow.removeFromLeft (buttonWidth));
    actionRow.removeFromLeft (8);
    gainBalanceButton.setBounds (actionRow.removeFromLeft (buttonWidth));
    actionRow.removeFromLeft (8);
    eqPrepButton.setBounds (actionRow);

    promptTitle.setBounds (middle.removeFromTop (24));
    middle.removeFromTop (8);
    promptEditor.setBounds (middle.removeFromTop (72));
    middle.removeFromTop (8);

    auto promptButtons = middle.removeFromTop (36);
    planButton.setBounds (promptButtons.removeFromLeft (180));
    promptButtons.removeFromLeft (10);
    applyButton.setBounds (promptButtons.removeFromLeft (180));

    auto bottomTop = bottom.removeFromTop ((bottom.getHeight() - 12) / 2);
    explanationTitle.setBounds (bottomTop.removeFromTop (24));
    bottomTop.removeFromTop (8);
    explanationValue.setBounds (bottomTop);

    bottom.removeFromTop (12);
    plannedChangesTitle.setBounds (bottom.removeFromTop (24));
    bottom.removeFromTop (8);
    plannedChangesValue.setBounds (bottom);
}

void ControllerView::refreshState()
{
    auto promptText = audioProcessor.getPromptText();
    auto sessionStatus = audioProcessor.getSessionStatusText();
    auto serverStatus = audioProcessor.getServerStatusText();
    auto analysisStatus = audioProcessor.getAnalysisStatusText();
    auto explanation = audioProcessor.getExplanationText();
    auto plannedChanges = audioProcessor.getPlannedChangesText();

    chatTranscript.setText (buildChatTranscript (promptText, sessionStatus, explanation), juce::dontSendNotification);
    stepValue.setText (buildStepStatus (serverStatus, sessionStatus, analysisStatus), juce::dontSendNotification);
    stemFolderValue.setText (audioProcessor.getStemFolderText().isNotEmpty() ? audioProcessor.getStemFolderText() : "No folder selected", juce::dontSendNotification);
    explanationValue.setText (explanation.isNotEmpty() ? explanation : explanationPlaceholder(), juce::dontSendNotification);
    plannedChangesValue.setText (plannedChanges.isNotEmpty() ? plannedChanges : previewPlaceholder(), juce::dontSendNotification);

    if (promptEditor.getText() != promptText)
        promptEditor.setText (promptText, juce::dontSendNotification);

    auto busy = audioProcessor.isRequestInFlight();
    planButton.setEnabled (! busy);
    applyButton.setEnabled (! busy && audioProcessor.canApplyPlan());
    chooseStemFolderButton.setEnabled (! busy);
    analyzeStemsButton.setEnabled (audioProcessor.canStartAnalysisUpload());
    trackLengthButton.setEnabled (! busy);
    groupingButton.setEnabled (! busy);
    gainBalanceButton.setEnabled (! busy);
    eqPrepButton.setEnabled (! busy);
}

void ControllerView::buttonClicked (juce::Button* button)
{
    if (button == &planButton)
    {
        audioProcessor.setCurrentPrompt (promptEditor.getText());
        audioProcessor.planActions();
    }
    else if (button == &applyButton)
    {
        audioProcessor.applyPlannedActions();
    }
    else if (button == &chooseStemFolderButton)
    {
        if (onChooseStemFolder != nullptr)
            onChooseStemFolder();
    }
    else if (button == &analyzeStemsButton)
    {
        audioProcessor.startAnalysisUpload();
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

void ControllerView::textEditorReturnKeyPressed (juce::TextEditor& editor)
{
    if (&editor != &promptEditor)
        return;

    if (juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown())
    {
        promptEditor.insertTextAtCaret ("\n");
        audioProcessor.setCurrentPrompt (promptEditor.getText());
        return;
    }

    if (audioProcessor.isRequestInFlight())
        return;

    audioProcessor.setCurrentPrompt (promptEditor.getText());
    audioProcessor.planActions();
    refreshState();
}

void ControllerView::setSuggestedPrompt (const juce::String& promptText)
{
    promptEditor.setText (promptText, juce::dontSendNotification);
    audioProcessor.setCurrentPrompt (promptText);
}

juce::String ControllerView::buildChatTranscript (const juce::String& promptText,
                                                  const juce::String& sessionStatus,
                                                  const juce::String& explanation) const
{
    auto serverStatus = audioProcessor.getServerStatusText();
    auto analysisStatus = audioProcessor.getAnalysisStatusText();
    juce::StringArray lines;
    lines.add ("AI  " + greetingLine());
    lines.add ("AI  " + greetingExample());
    lines.add ("AI  " + buildAssistantStatusLine (analysisStatus, sessionStatus));

    if (promptText.isNotEmpty())
        lines.add ("You  " + promptText);

    if (shouldShowAssistantReply (explanation, serverStatus))
        lines.add ("AI  " + explanation);

    return lines.joinIntoString ("\n\n");
}

juce::String ControllerView::buildStepStatus (const juce::String& serverStatus,
                                              const juce::String& sessionStatus,
                                              const juce::String& analysisStatus) const
{
    if (! serverStatus.startsWithIgnoreCase ("Server connected"))
        return "1. Connect to server";

    if (analysisStatus.containsIgnoreCase ("Ready to upload"))
        return "2. Choose a WAV stem folder";

    if (analysisStatus.containsIgnoreCase ("Uploading")
        || analysisStatus.containsIgnoreCase ("Creating")
        || analysisStatus.containsIgnoreCase ("Fetching"))
        return "2. Uploading and analyzing stems";

    if (analysisStatus.containsIgnoreCase ("completed"))
        return "3. Review stem analysis and start chat";

    if (! sessionStatus.contains ("tracks loaded"))
        return "2. Select a stem folder or sync project tracks";

    if (audioProcessor.isRequestInFlight())
        return "4. Waiting for AI response";

    if (audioProcessor.canApplyPlan())
        return "5. Review plan and apply";

    return "3. Analyze stems or enter genre and first request";
}

bool ControllerView::shouldShowAssistantReply (const juce::String& explanation,
                                               const juce::String& serverStatus) const
{
    auto trimmed = explanation.trim();

    if (trimmed.isEmpty())
        return false;

    if (trimmed == serverStatus.trim())
        return false;

    if (trimmed.equalsIgnoreCase ("Server connected")
        || trimmed.equalsIgnoreCase ("Connecting to server...")
        || trimmed.equalsIgnoreCase ("Refreshing session...")
        || trimmed.equalsIgnoreCase ("Planning changes...")
        || trimmed.equalsIgnoreCase ("Waiting for plan response..."))
        return false;

    if (trimmed == greetingLine()
        || trimmed == greetingExample()
        || trimmed == sessionReadyLine()
        || trimmed == syncingTracksLine()
        || trimmed == analysisReadyLine()
        || trimmed == analysisCompletedLine()
        || trimmed == projectReadyLine())
        return false;

    return true;
}

juce::String ControllerView::buildAssistantStatusLine (const juce::String& analysisStatus,
                                                       const juce::String& sessionStatus) const
{
    if (analysisStatus.containsIgnoreCase ("Uploading")
        || analysisStatus.containsIgnoreCase ("Creating")
        || analysisStatus.containsIgnoreCase ("Fetching"))
        return uploadingLine();

    if (analysisStatus.containsIgnoreCase ("completed"))
        return analysisCompletedLine();

    if (analysisStatus.containsIgnoreCase ("Ready to upload"))
        return analysisReadyLine();

    if (sessionStatus.contains ("tracks loaded"))
        return projectReadyLine();

    return syncingTracksLine();
}
