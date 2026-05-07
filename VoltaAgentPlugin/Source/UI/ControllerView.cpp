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
    editor.setIndents (12, 12);
}

void configureCardLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
}

void configureBubbleArea (juce::TextEditor& editor, juce::Colour background)
{
    configureReadOnlyArea (editor);
    editor.setColour (juce::TextEditor::backgroundColourId, background);
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

juce::String analysisReadyLine()
{
    return korean ("\xEC\x8A\xA4\xED\x85\x9C \xEB\xB6\x84\xEC\x84\x9D\xEC\x9D\xB4 \xEC\xA4\x80\xEB\xB9\x84\xEB\x90\x98\xEC\x97\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4. WAV \xED\x8F\xB4\xEB\x8D\x94\xEB\xA5\xBC \xEC\x84\xA0\xED\x83\x9D\xED\x95\x98\xEA\xB3\xA0 Analyze WAV Stems\xEB\xA5\xBC \xEB\x88\x8C\xEB\x9F\xAC\xEC\xA3\xBC\xEC\x84\xB8\xEC\x9A\x94.");
}

juce::String analysisCompletedLine()
{
    return korean ("\xEC\x8A\xA4\xED\x85\x9C \xEB\xB6\x84\xEC\x84\x9D\xEC\x9D\xB4 \xEC\x99\x84\xEB\xA3\x8C\xEB\x90\x98\xEC\x97\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4. \xEC\x9D\xB4\xEC\xA0\x9C \xEC\x9E\xA5\xEB\xA5\xB4\xEC\x99\x80 \xEC\xB2\xAB \xEC\x9A\x94\xEC\xB2\xAD\xEC\x9D\x84 \xEC\x9E\x85\xEB\xA0\xA5\xED\x95\x98\xEB\xA9\xB4 \xEB\x8C\x80\xED\x99\x94\xEB\xA5\xBC \xEC\x8B\x9C\xEC\x9E\x91\xED\x95\xA0 \xEC\x88\x98 \xEC\x9E\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String waitingForModelLine()
{
    return korean ("\xEB\xAA\xA8\xEB\x8D\xB8\xEC\x9D\xB4 \xED\x8A\xB8\xEB\x9E\x99 \xEB\xB6\x84\xEC\x84\x9D \xEA\xB2\xB0\xEA\xB3\xBC\xEB\xA5\xBC \xED\x99\x95\xEC\x9D\xB8\xED\x95\x98\xEA\xB3\xA0 \xEC\x9D\x91\xEB\x8B\xB5\xEC\x9D\x84 \xEC\x9E\x91\xEC\x84\xB1\xED\x95\x98\xEB\x8A\x94 \xEC\xA4\x91\xEC\x9E\x85\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String noUserPromptLine()
{
    return korean ("\xEC\x95\x84\xEC\xA7\x81 \xEB\xB3\xB4\xEB\x82\xB8 \xEC\x9A\x94\xEC\xB2\xAD\xEC\x9D\xB4 \xEC\x97\x86\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4.");
}

juce::String previewPlaceholder()
{
    return korean ("\xEC\x95\x84\xEC\xA7\x81 \xEC\xA0\x81\xEC\x9A\xA9\xED\x95\xA0 \xEA\xB3\x84\xED\x9A\x8D\xEC\x9D\xB4 \xEC\x97\x86\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4.");
}
}

ControllerView::ControllerView (VoltaAgentPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    configureCardLabel (progressTitle, "Status");
    configureCardLabel (stemFolderTitle, "Stem Folder");
    configureCardLabel (assistantTitle, "Assistant");
    configureCardLabel (userTitle, "Your Request");
    configureCardLabel (composerTitle, korean ("\xEB\xA9\x94\xEC\x8B\x9C\xEC\xA7\x80"));
    configureCardLabel (plannedChangesTitle, "Action Preview");

    progressValue.setJustificationType (juce::Justification::centredLeft);
    progressValue.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    progressValue.setColour (juce::Label::backgroundColourId, juce::Colour::fromRGB (34, 46, 52));
    progressValue.setColour (juce::Label::outlineColourId, juce::Colour::fromRGBA (255, 255, 255, 0));
    progressValue.setBorderSize (juce::BorderSize<int> (8, 12, 8, 12));

    stemFolderValue.setJustificationType (juce::Justification::centredLeft);
    stemFolderValue.setFont (juce::FontOptions (14.0f, juce::Font::plain));
    stemFolderValue.setColour (juce::Label::backgroundColourId, juce::Colour::fromRGB (34, 46, 52));
    stemFolderValue.setColour (juce::Label::outlineColourId, juce::Colour::fromRGBA (255, 255, 255, 0));
    stemFolderValue.setBorderSize (juce::BorderSize<int> (8, 12, 8, 12));

    promptEditor.setMultiLine (true);
    promptEditor.setReturnKeyStartsNewLine (false);
    promptEditor.addListener (this);
    promptEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour::fromRGB (34, 46, 52));

    configureBubbleArea (assistantBubble, juce::Colour::fromRGB (34, 46, 52));
    configureBubbleArea (userBubble, juce::Colour::fromRGB (42, 58, 66));
    configureBubbleArea (plannedChangesValue, juce::Colour::fromRGB (34, 46, 52));

    planButton.addListener (this);
    chooseStemFolderButton.addListener (this);
    analyzeStemsButton.addListener (this);

    addAndMakeVisible (progressTitle);
    addAndMakeVisible (progressValue);
    addAndMakeVisible (stemFolderTitle);
    addAndMakeVisible (stemFolderValue);
    addAndMakeVisible (chooseStemFolderButton);
    addAndMakeVisible (analyzeStemsButton);
    addAndMakeVisible (assistantTitle);
    addAndMakeVisible (assistantBubble);
    addAndMakeVisible (userTitle);
    addAndMakeVisible (userBubble);
    addAndMakeVisible (composerTitle);
    addAndMakeVisible (promptEditor);
    addAndMakeVisible (planButton);
    addAndMakeVisible (plannedChangesTitle);
    addAndMakeVisible (plannedChangesValue);

    refreshState();
}

ControllerView::~ControllerView()
{
    planButton.removeListener (this);
    chooseStemFolderButton.removeListener (this);
    analyzeStemsButton.removeListener (this);
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
    auto header = area.removeFromTop (92);
    area.removeFromTop (12);
    auto chat = area.removeFromTop (270);
    area.removeFromTop (12);
    auto composer = area.removeFromTop (120);
    area.removeFromTop (12);
    auto preview = area;

    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 10));
    g.fillRoundedRectangle (header.toFloat(), 10.0f);
    g.fillRoundedRectangle (chat.toFloat(), 10.0f);
    g.fillRoundedRectangle (composer.toFloat(), 10.0f);
    g.fillRoundedRectangle (preview.toFloat(), 10.0f);
}

void ControllerView::resized()
{
    auto area = getLocalBounds().reduced (18);

    auto header = area.removeFromTop (92);
    area.removeFromTop (12);
    auto chat = area.removeFromTop (270);
    area.removeFromTop (12);
    auto composer = area.removeFromTop (120);
    area.removeFromTop (12);
    auto preview = area;

    progressTitle.setBounds (header.removeFromTop (24));
    header.removeFromTop (6);
    progressValue.setBounds (header.removeFromTop (34));
    header.removeFromTop (10);

    auto stemRow = header.removeFromTop (36);
    stemFolderTitle.setBounds (stemRow.removeFromLeft (100));
    stemFolderValue.setBounds (stemRow.removeFromLeft (juce::jmax (220, stemRow.getWidth() - 360)));
    stemRow.removeFromLeft (8);
    chooseStemFolderButton.setBounds (stemRow.removeFromLeft (160));
    stemRow.removeFromLeft (8);
    analyzeStemsButton.setBounds (stemRow);

    auto assistantArea = chat.removeFromLeft ((chat.getWidth() * 2) / 3);
    chat.removeFromLeft (12);
    auto userArea = chat;

    assistantTitle.setBounds (assistantArea.removeFromTop (24));
    assistantArea.removeFromTop (8);
    assistantBubble.setBounds (assistantArea);

    userTitle.setBounds (userArea.removeFromTop (24));
    userArea.removeFromTop (8);
    userBubble.setBounds (userArea);

    composerTitle.setBounds (composer.removeFromTop (24));
    composer.removeFromTop (8);
    promptEditor.setBounds (composer.removeFromTop (74));
    composer.removeFromTop (8);
    auto sendRow = composer.removeFromTop (36);
    auto sendButtonArea = sendRow.removeFromRight (160);
    planButton.setBounds (sendButtonArea.withSizeKeepingCentre (160, 32));

    plannedChangesTitle.setBounds (preview.removeFromTop (24));
    preview.removeFromTop (8);
    plannedChangesValue.setBounds (preview);
}

void ControllerView::refreshState()
{
    auto promptText = audioProcessor.getPromptText();
    auto sessionStatus = audioProcessor.getSessionStatusText();
    auto serverStatus = audioProcessor.getServerStatusText();
    auto analysisStatus = audioProcessor.getAnalysisStatusText();
    auto explanation = audioProcessor.getExplanationText();
    auto plannedChanges = audioProcessor.getPlannedChangesText();

    progressValue.setText (buildProgressText (serverStatus, sessionStatus, analysisStatus), juce::dontSendNotification);
    stemFolderValue.setText (audioProcessor.getStemFolderText().isNotEmpty() ? audioProcessor.getStemFolderText() : "No folder selected", juce::dontSendNotification);
    assistantBubble.setText (buildAssistantText (explanation, sessionStatus, analysisStatus), juce::dontSendNotification);
    userBubble.setText (audioProcessor.getLastSubmittedPromptText().isNotEmpty() ? audioProcessor.getLastSubmittedPromptText() : noUserPromptLine(),
                        juce::dontSendNotification);
    plannedChangesValue.setText (plannedChanges.isNotEmpty() ? plannedChanges : previewPlaceholder(), juce::dontSendNotification);

    if (promptEditor.getText() != promptText)
        promptEditor.setText (promptText, juce::dontSendNotification);

    auto busy = audioProcessor.isRequestInFlight();
    planButton.setEnabled (! busy);
    chooseStemFolderButton.setEnabled (! busy);
    analyzeStemsButton.setEnabled (audioProcessor.canStartAnalysisUpload());

    planButton.setButtonText (busy ? "Working..." : "Send");
}

void ControllerView::buttonClicked (juce::Button* button)
{
    if (button == &planButton)
    {
        sendCurrentPrompt();
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

    sendCurrentPrompt();
    refreshState();
}

juce::String ControllerView::buildProgressText (const juce::String& serverStatus,
                                                const juce::String& sessionStatus,
                                                const juce::String& analysisStatus) const
{
    if (! serverStatus.startsWithIgnoreCase ("Server connected"))
        return "Server offline. Check local server connection.";

    if (isWaitingState (analysisStatus))
        return waitingForModelLine();

    if (analysisStatus.containsIgnoreCase ("Uploading")
        || analysisStatus.containsIgnoreCase ("Creating")
        || analysisStatus.containsIgnoreCase ("Fetching"))
        return analysisStatus;

    if (analysisStatus.containsIgnoreCase ("completed"))
        return analysisCompletedLine();

    if (analysisStatus.containsIgnoreCase ("Ready to upload"))
        return analysisReadyLine();

    if (sessionStatus.contains ("tracks loaded"))
        return syncingTracksLine();

    return greetingLine();
}

juce::String ControllerView::buildAssistantText (const juce::String& explanation,
                                                 const juce::String& sessionStatus,
                                                 const juce::String& analysisStatus) const
{
    auto trimmed = explanation.trim();

    if (trimmed.isNotEmpty()
        && trimmed != "Server connected"
        && trimmed != "Connecting to server..."
        && trimmed != "Refreshing session..."
        && trimmed != "Planning changes..."
        && trimmed != "Waiting for plan response..."
        && trimmed != "Project session created"
        && trimmed != "Reply ready")
        return trimmed;

    juce::StringArray lines;
    lines.add (greetingLine());
    lines.add (greetingExample());

    if (analysisStatus.containsIgnoreCase ("completed"))
        lines.add (analysisCompletedLine());
    else if (analysisStatus.containsIgnoreCase ("Ready to upload"))
        lines.add (analysisReadyLine());
    else if (sessionStatus.contains ("tracks loaded"))
        lines.add (syncingTracksLine());

    return lines.joinIntoString ("\n\n");
}

bool ControllerView::isWaitingState (const juce::String& analysisStatus) const
{
    if (! audioProcessor.isRequestInFlight())
        return false;

    if (analysisStatus.containsIgnoreCase ("Uploading")
        || analysisStatus.containsIgnoreCase ("Creating")
        || analysisStatus.containsIgnoreCase ("Fetching"))
        return false;

    return true;
}

void ControllerView::sendCurrentPrompt()
{
    if (audioProcessor.isRequestInFlight())
        return;

    audioProcessor.setCurrentPrompt (promptEditor.getText());
    audioProcessor.planActions();
}
