#include "ControllerView.h"
#include "../PluginProcessor.h"

namespace
{
const auto panelColour = juce::Colour::fromRGB (22, 26, 31);
const auto cardColour = juce::Colour::fromRGB (32, 36, 43);
const auto cardAltColour = juce::Colour::fromRGB (38, 43, 51);
const auto accentColour = juce::Colour::fromRGB (87, 225, 193);
const auto accentMutedColour = juce::Colour::fromRGB (43, 86, 83);
const auto outlineColour = juce::Colour::fromRGBA (255, 255, 255, 18);
const auto textColour = juce::Colour::fromRGB (235, 240, 244);
const auto subtleTextColour = juce::Colour::fromRGB (162, 173, 181);
const auto successColour = juce::Colour::fromRGB (92, 199, 120);

void configureReadOnlyArea (juce::TextEditor& editor)
{
    editor.setMultiLine (true);
    editor.setReadOnly (true);
    editor.setScrollbarsShown (true);
    editor.setCaretVisible (false);
    editor.setPopupMenuEnabled (false);
    editor.setIndents (12, 12);
    editor.setColour (juce::TextEditor::textColourId, textColour);
    editor.setColour (juce::TextEditor::highlightedTextColourId, textColour);
}

void configureCardLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, textColour);
}

void configureBubbleArea (juce::TextEditor& editor, juce::Colour background)
{
    configureReadOnlyArea (editor);
    editor.setColour (juce::TextEditor::backgroundColourId, background);
    editor.setColour (juce::TextEditor::outlineColourId, juce::Colour::fromRGBA (255, 255, 255, 0));
}

void configureEditorSurface (juce::TextEditor& editor, juce::Colour background)
{
    editor.setColour (juce::TextEditor::backgroundColourId, background);
    editor.setColour (juce::TextEditor::outlineColourId, juce::Colour::fromRGBA (255, 255, 255, 0));
    editor.setColour (juce::TextEditor::textColourId, textColour);
    editor.setColour (juce::CaretComponent::caretColourId, accentColour);
    editor.setIndents (12, 12);
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

juce::String doneMark()
{
    return "[OK]";
}

juce::String todoMark()
{
    return "[ ]";
}

void drawPanel (juce::Graphics& g, juce::Rectangle<int> area, juce::Colour fill, float cornerSize = 14.0f)
{
    g.setColour (fill);
    g.fillRoundedRectangle (area.toFloat(), cornerSize);
    g.setColour (outlineColour);
    g.drawRoundedRectangle (area.toFloat(), cornerSize, 1.0f);
}
}

ControllerView::ControllerView (VoltaAgentPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    configureCardLabel (progressTitle, korean ("\xED\x98\x84\xEC\x9E\xAC \xEB\x8B\xA8\xEA\xB3\x84"));
    configureCardLabel (stemFolderTitle, korean ("\xEC\xA4\x80\xEB\xB9\x84 \xEB\x90\x9C WAV \xED\x8F\xB4\xEB\x8D\x94"));
    configureCardLabel (assistantTitle, korean ("\xEC\x8B\x9C\xEC\x9E\x91 \xEC\x88\x9C\xEC\x84\x9C"));
    configureCardLabel (userTitle, korean ("\xEC\xA7\x81\xEC\xA0\x84 \xEC\x9A\x94\xEC\xB2\xAD"));
    configureCardLabel (composerTitle, korean ("\xEC\xB1\x84\xED\x8C\x85 \xEC\x9E\x85\xEB\xA0\xA5"));
    configureCardLabel (plannedChangesTitle, korean ("\xEB\xB6\x84\xEC\x84\x9D \xEC\x83\x81\xED\x83\x9C / \xEC\x95\x88\xEB\x82\xB4"));
    configureCardLabel (chatTitle, korean ("\xEC\xB1\x84\xED\x8C\x85 (\xEB\xB6\x84\xEC\x84\x9D \xED\x9B\x84)"));

    progressValue.setJustificationType (juce::Justification::topLeft);
    progressValue.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    progressValue.setColour (juce::Label::textColourId, textColour);
    progressValue.setBorderSize (juce::BorderSize<int> (4, 0, 0, 0));

    stemFolderValue.setJustificationType (juce::Justification::centredLeft);
    stemFolderValue.setFont (juce::FontOptions (14.0f, juce::Font::plain));
    stemFolderValue.setColour (juce::Label::textColourId, textColour);

    promptEditor.setMultiLine (true);
    promptEditor.setReturnKeyStartsNewLine (false);
    promptEditor.addListener (this);
    configureEditorSurface (promptEditor, cardAltColour);

    configureBubbleArea (assistantBubble, cardColour);
    configureBubbleArea (userBubble, cardAltColour);
    configureBubbleArea (plannedChangesValue, cardColour);

    planButton.addListener (this);
    refreshSessionButton.addListener (this);
    chooseStemFolderButton.addListener (this);
    analyzeStemsButton.addListener (this);
    refreshSessionButton.setButtonText (korean ("\xEC\x97\x90\xEC\x9D\xB4\xEB\xB8\x94\xED\x86\xA4 \xEC\xA0\x95\xEB\xB3\xB4 \xEB\xB6\x88\xEB\x9F\xAC\xEC\x98\xA4\xEA\xB8\xB0"));
    chooseStemFolderButton.setButtonText (korean ("\xED\x8F\xB4\xEB\x8D\x94 \xEC\xB6\x94\xEA\xB0\x80"));
    analyzeStemsButton.setButtonText (korean ("\xEB\xB6\x84\xEC\x84\x9D \xEC\x8B\x9C\xEC\x9E\x91"));
    planButton.setButtonText (korean ("\xEC\x9A\x94\xEC\xB2\xAD \xEB\xB3\xB4\xEB\x82\xB4\xEA\xB8\xB0"));
    promptEditor.setTextToShowWhenEmpty (korean ("\xEB\xB6\x84\xEC\x84\x9D\xEC\x9D\xB4 \xEC\x99\x84\xEB\xA3\x8C\xEB\x90\x98\xEB\xA9\xB4 \xEC\x97\xAC\xEA\xB8\xB0\xEC\x97\x90 \xEC\x88\x98\xEC\xA0\x95 \xEC\x9A\x94\xEC\xB2\xAD\xEC\x9D\x84 \xEC\x9E\x85\xEB\xA0\xA5\xED\x95\x98\xEC\x84\xB8\xEC\x9A\x94."), subtleTextColour);

    addAndMakeVisible (progressTitle);
    addAndMakeVisible (progressValue);
    addAndMakeVisible (stemFolderTitle);
    addAndMakeVisible (stemFolderValue);
    addAndMakeVisible (refreshSessionButton);
    addAndMakeVisible (chooseStemFolderButton);
    addAndMakeVisible (analyzeStemsButton);
    addAndMakeVisible (assistantTitle);
    addAndMakeVisible (assistantBubble);
    addAndMakeVisible (userTitle);
    addAndMakeVisible (userBubble);
    addAndMakeVisible (composerTitle);
    addAndMakeVisible (promptEditor);
    addAndMakeVisible (planButton);
    addAndMakeVisible (chatTitle);
    addAndMakeVisible (plannedChangesTitle);
    addAndMakeVisible (plannedChangesValue);

    refreshState();
}

ControllerView::~ControllerView()
{
    planButton.removeListener (this);
    refreshSessionButton.removeListener (this);
    chooseStemFolderButton.removeListener (this);
    analyzeStemsButton.removeListener (this);
    promptEditor.removeListener (this);
}

void ControllerView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (panelColour);
    g.fillRoundedRectangle (bounds, 18.0f);
    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 12));
    g.drawRoundedRectangle (bounds, 18.0f, 1.0f);

    auto area = getLocalBounds().reduced (18);
    auto content = area;
    auto chatColumn = content.removeFromRight (juce::jmax (270, content.getWidth() / 4));
    content.removeFromRight (14);
    auto workspaceColumn = content;

    auto heroCard = workspaceColumn.removeFromTop (juce::jmax (280, (workspaceColumn.getHeight() * 7) / 10));
    workspaceColumn.removeFromTop (14);
    auto bottomRow = workspaceColumn;
    auto stepCard = bottomRow.removeFromLeft (juce::jmax (140, bottomRow.getWidth() / 5));
    bottomRow.removeFromLeft (14);
    auto keepCard = bottomRow.removeFromLeft (juce::jmax (180, bottomRow.getWidth() / 3));
    bottomRow.removeFromLeft (14);
    auto alertCard = bottomRow;

    drawPanel (g, heroCard, cardColour, 16.0f);
    drawPanel (g, chatColumn, cardColour, 16.0f);
    drawPanel (g, stepCard, cardAltColour, 14.0f);
    drawPanel (g, keepCard, cardAltColour, 14.0f);
    drawPanel (g, alertCard, cardAltColour, 14.0f);

    auto uploadFill = dragOverUploadZone ? accentMutedColour.brighter (0.2f) : panelColour.brighter (0.04f);
    g.setColour (uploadFill);
    g.fillRoundedRectangle (chatUploadBounds.toFloat(), 12.0f);
    g.setColour (dragOverUploadZone ? accentColour : juce::Colour::fromRGBA (87, 225, 193, 90));
    g.drawRoundedRectangle (chatUploadBounds.toFloat(), 12.0f, dragOverUploadZone ? 2.0f : 1.4f);

    auto uploadTextArea = chatUploadBounds.reduced (16);
    g.setColour (textColour);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText (korean ("WAV \xED\x8F\xB4\xEB\x8D\x94 \xEB\x93\x9C\xEB\xA1\xAD \xEB\x98\x90\xEB\x8A\x94 \xEC\xB6\x94\xEA\xB0\x80"),
                uploadTextArea.removeFromTop (24),
                juce::Justification::centredLeft,
                true);
    g.setFont (juce::FontOptions (13.0f, juce::Font::plain));
    g.setColour (subtleTextColour);
    g.drawText (attachedFolderHeadline, uploadTextArea.removeFromTop (22), juce::Justification::centredLeft, true);
    g.setColour (dragOverUploadZone ? accentColour : subtleTextColour);
    g.drawText (attachedFolderMeta, uploadTextArea, juce::Justification::topLeft, true);

    auto chipBounds = chatUploadBounds.withTrimmedTop (chatUploadBounds.getHeight() - 28).reduced (14, 0).removeFromLeft (juce::jmin (chatUploadBounds.getWidth() - 28, 170));
    g.setColour (analysisUploadStateMeta.startsWith ("0 wav") ? cardAltColour.brighter (0.1f) : successColour.withAlpha (0.18f));
    g.fillRoundedRectangle (chipBounds.toFloat(), 10.0f);
    g.setColour (analysisUploadStateMeta.startsWith ("0 wav") ? subtleTextColour : successColour);
    g.drawRoundedRectangle (chipBounds.toFloat(), 10.0f, 1.0f);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (analysisUploadStateMeta, chipBounds.reduced (10, 0), juce::Justification::centredLeft, true);
}

void ControllerView::resized()
{
    auto area = getLocalBounds().reduced (18);
    auto content = area;
    auto chatColumn = content.removeFromRight (juce::jmax (270, content.getWidth() / 4));
    content.removeFromRight (14);
    auto workspaceColumn = content;

    auto heroCard = workspaceColumn.removeFromTop (juce::jmax (280, (workspaceColumn.getHeight() * 7) / 10));
    workspaceColumn.removeFromTop (14);
    auto bottomRow = workspaceColumn;
    auto stepCard = bottomRow.removeFromLeft (juce::jmax (140, bottomRow.getWidth() / 5));
    bottomRow.removeFromLeft (14);
    auto keepCard = bottomRow.removeFromLeft (juce::jmax (180, bottomRow.getWidth() / 3));
    bottomRow.removeFromLeft (14);
    auto alertCard = bottomRow;

    auto heroContent = heroCard.reduced (18);
    progressTitle.setBounds (heroContent.removeFromTop (26));
    heroContent.removeFromTop (8);
    progressValue.setBounds (heroContent.removeFromTop (58));
    heroContent.removeFromTop (12);
    assistantTitle.setBounds (heroContent.removeFromTop (24));
    heroContent.removeFromTop (8);
    assistantBubble.setBounds (heroContent);

    auto stepContent = stepCard.reduced (16);
    stemFolderTitle.setBounds (stepContent.removeFromTop (22));
    stepContent.removeFromTop (6);
    stemFolderValue.setBounds (stepContent);

    auto keepContent = keepCard.reduced (16);
    refreshSessionButton.setBounds (keepContent.removeFromTop (34));
    keepContent.removeFromTop (10);
    chooseStemFolderButton.setBounds (keepContent.removeFromTop (34));
    keepContent.removeFromTop (10);
    analyzeStemsButton.setBounds (keepContent.removeFromTop (34));

    auto alertContent = alertCard.reduced (16);
    plannedChangesTitle.setBounds (alertContent.removeFromTop (22));
    alertContent.removeFromTop (8);
    plannedChangesValue.setBounds (alertContent);

    auto chatContent = chatColumn.reduced (16);
    chatTitle.setBounds (chatContent.removeFromTop (26));
    chatContent.removeFromTop (10);
    chatUploadBounds = chatContent.removeFromTop (110);
    chatContent.removeFromTop (12);

    userTitle.setBounds (chatContent.removeFromTop (22));
    chatContent.removeFromTop (6);
    userBubble.setBounds (chatContent.removeFromTop (juce::jmax (90, chatContent.getHeight() / 4)));
    chatContent.removeFromTop (10);
    composerTitle.setBounds (chatContent.removeFromTop (22));
    chatContent.removeFromTop (6);
    auto inputArea = chatContent.removeFromTop (juce::jmax (120, chatContent.getHeight() - 44));
    promptEditor.setBounds (inputArea);
    chatContent.removeFromTop (10);
    planButton.setBounds (chatContent.removeFromTop (34).removeFromRight (120));
}

void ControllerView::refreshState()
{
    auto promptText = audioProcessor.getPromptText();
    auto sessionStatus = audioProcessor.getSessionStatusText();
    auto serverStatus = audioProcessor.getServerStatusText();
    auto analysisStatus = audioProcessor.getAnalysisStatusText();
    auto explanation = audioProcessor.getExplanationText();
    auto plannedChanges = audioProcessor.getPlannedChangesText();
    auto stemFolderPath = audioProcessor.getStemFolderText();
    auto hasStemFolder = stemFolderPath.isNotEmpty();
    auto sessionReady = isSessionReady (sessionStatus);
    auto analysisComplete = isAnalysisComplete (analysisStatus);

    progressValue.setText (buildNextActionText (sessionStatus, analysisStatus, hasStemFolder), juce::dontSendNotification);
    stemFolderValue.setText (hasStemFolder ? juce::File (stemFolderPath).getFileName() : korean ("\xEC\x95\x84\xEC\xA7\x81 \xED\x8F\xB4\xEB\x8D\x94\xEA\xB0\x80 \xEC\x97\x86\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4."),
                            juce::dontSendNotification);
    assistantBubble.setText (buildWorkflowGuide (sessionStatus, analysisStatus, hasStemFolder), juce::dontSendNotification);
    userBubble.setText (audioProcessor.getLastSubmittedPromptText().isNotEmpty() ? audioProcessor.getLastSubmittedPromptText() : noUserPromptLine(),
                        juce::dontSendNotification);
    plannedChangesValue.setText (analysisComplete ? (plannedChanges.isNotEmpty() ? plannedChanges : previewPlaceholder())
                                                  : buildAssistantText (explanation, sessionStatus, analysisStatus),
                                 juce::dontSendNotification);

    if (promptEditor.getText() != promptText)
        promptEditor.setText (promptText, juce::dontSendNotification);

    if (stemFolderPath.isNotEmpty())
    {
        auto folder = juce::File (stemFolderPath);
        auto wavCount = countWavFilesInFolder (folder);
        attachedFolderHeadline = folder.getFileName() + "/";
        attachedFolderMeta = folder.getFullPathName();
        analysisUploadStateMeta = juce::String (wavCount) + " wav files";
    }
    else
    {
        attachedFolderHeadline = korean ("\xEC\x95\x84\xEC\xA7\x81 WAV \xED\x8F\xB4\xEB\x8D\x94\xEA\xB0\x80 \xEC\x97\x86\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4.");
        attachedFolderMeta = korean ("\xEB\x93\x9C\xEB\x9E\x98\xEA\xB7\xB8\xED\x95\x98\xEA\xB1\xB0\xEB\x82\x98 \xED\x8F\xB4\xEB\x8D\x94 \xEC\xB6\x94\xEA\xB0\x80 \xEB\xB2\x84\xED\x8A\xBC\xEC\x9D\x84 \xEB\x88\x8C\xEB\x9F\xAC\xEC\xA3\xBC\xEC\x84\xB8\xEC\x9A\x94.");
        analysisUploadStateMeta = "0 wav files";
    }

    auto busy = audioProcessor.isRequestInFlight();
    planButton.setEnabled (! busy && analysisComplete);
    promptEditor.setEnabled (analysisComplete);
    refreshSessionButton.setEnabled (! busy);
    chooseStemFolderButton.setEnabled (! busy);
    analyzeStemsButton.setEnabled (! busy && sessionReady && hasStemFolder);
    analyzeStemsButton.setTooltip (sessionReady ? (hasStemFolder ? juce::String() : korean ("\xEB\xA8\xBC\xEC\xA0\x80 WAV \xED\x8F\xB4\xEB\x8D\x94\xEB\xA5\xBC \xEC\xB6\x94\xEA\xB0\x80\xED\x95\x98\xEC\x84\xB8\xEC\x9A\x94."))
                                               : korean ("\xEB\xA8\xBC\xEC\xA0\x80 Ableton \xEC\xA0\x95\xEB\xB3\xB4\xEB\xA5\xBC \xEB\xB6\x88\xEB\x9F\xAC\xEC\x98\xA4\xEC\x84\xB8\xEC\x9A\x94."));
    planButton.setButtonText (busy ? korean ("\xEC\x9E\x91\xEC\x97\x85 \xEC\xA4\x91...") : korean ("\xEC\x9A\x94\xEC\xB2\xAD \xEB\xB3\xB4\xEB\x82\xB4\xEA\xB8\xB0"));
    promptEditor.setTooltip (buildChatGuideText (analysisStatus));

    repaint();
}

bool ControllerView::isInterestedInFileDrag (const juce::StringArray& files)
{
    return resolveDroppedStemFolder (files).isDirectory();
}

void ControllerView::fileDragEnter (const juce::StringArray& files, int x, int y)
{
    fileDragMove (files, x, y);
}

void ControllerView::fileDragMove (const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused (files);
    auto shouldHighlight = chatUploadBounds.contains (x, y);

    if (dragOverUploadZone != shouldHighlight)
    {
        dragOverUploadZone = shouldHighlight;
        repaint (chatUploadBounds.expanded (4));
    }
}

void ControllerView::fileDragExit (const juce::StringArray& files)
{
    juce::ignoreUnused (files);

    if (dragOverUploadZone)
    {
        dragOverUploadZone = false;
        repaint (chatUploadBounds.expanded (4));
    }
}

void ControllerView::filesDropped (const juce::StringArray& files, int x, int y)
{
    auto folder = resolveDroppedStemFolder (files);
    auto dropInZone = chatUploadBounds.contains (x, y);
    dragOverUploadZone = false;

    if (dropInZone && folder.isDirectory())
    {
        audioProcessor.setStemFolder (folder);
        refreshState();
    }

    repaint (chatUploadBounds.expanded (4));
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
    else if (button == &refreshSessionButton)
    {
        if (onRefreshSession != nullptr)
            onRefreshSession();
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

juce::String ControllerView::buildWorkflowGuide (const juce::String& sessionStatus,
                                                 const juce::String& analysisStatus,
                                                 bool hasStemFolder) const
{
    juce::StringArray lines;
    lines.add ((isSessionReady (sessionStatus) ? doneMark() : todoMark()) + " 1. Ableton\xEC\x97\x90\xEC\x84\x9C Volta M4L\xEC\x9D\x98 Send\xEB\xA5\xBC \xEB\x88\x84\xEB\xA5\xB4\xEA\xB3\xA0 \xEC\x97\x90\xEC\x9D\xB4\xEB\xB8\x94\xED\x86\xA4 \xEC\xA0\x95\xEB\xB3\xB4\xEB\xA5\xBC \xEA\xB0\x80\xEC\xA0\xB8\xEC\x98\xA4\xEC\x84\xB8\xEC\x9A\x94.");
    lines.add ((hasStemFolder ? doneMark() : todoMark()) + " 2. \xEB\xB6\x84\xEC\x84\x9D\xED\x95\xA0 WAV \xED\x8F\xB4\xEB\x8D\x94\xEB\xA5\xBC \xEC\xB6\x94\xEA\xB0\x80\xED\x95\x98\xEC\x84\xB8\xEC\x9A\x94.");
    lines.add ((isAnalysisComplete (analysisStatus) ? doneMark() : todoMark()) + " 3. \xEB\xB6\x84\xEC\x84\x9D\xEC\x9D\x84 \xEC\x8B\x9C\xEC\x9E\x91\xED\x95\x98\xEA\xB3\xA0 AI \xEC\xA0\x9C\xEC\x95\x88\xEC\x9D\x84 \xED\x99\x95\xEC\x9D\xB8\xED\x95\x98\xEC\x84\xB8\xEC\x9A\x94.");
    lines.add ("");
    lines.add (buildNextActionText (sessionStatus, analysisStatus, hasStemFolder));
    return lines.joinIntoString ("\n");
}

juce::String ControllerView::buildNextActionText (const juce::String& sessionStatus,
                                                  const juce::String& analysisStatus,
                                                  bool hasStemFolder) const
{
    if (! isSessionReady (sessionStatus))
        return juce::String::fromUTF8 (u8"먼저 Ableton에서 Volta M4L의 Send를 누르고, '에이블톤 정보 불러오기'를 눌러주세요.");

    if (! hasStemFolder)
        return korean ("\xED\x8A\xB8\xEB\x9E\x99 \xEC\xA0\x95\xEB\xB3\xB4\xEB\xA5\xBC \xEB\xB6\x88\xEB\x9F\xAC\xEC\x99\x94\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4. \xEC\x9D\xB4\xEC\xA0\x9C \xEB\xB6\x84\xEC\x84\x9D\xED\x95\xA0 WAV \xED\x8F\xB4\xEB\x8D\x94\xEB\xA5\xBC \xEC\xB6\x94\xEA\xB0\x80\xED\x95\x98\xEC\x84\xB8\xEC\x9A\x94.");

    if (analysisStatus.containsIgnoreCase ("Uploading")
        || analysisStatus.containsIgnoreCase ("Creating")
        || analysisStatus.containsIgnoreCase ("Fetching"))
        return korean ("\xEC\xA7\x80\xEA\xB8\x88 AI\xEA\xB0\x80 \xEC\x8A\xA4\xED\x85\x9C\xEC\x9D\x84 \xEB\xB6\x84\xEC\x84\x9D\xED\x95\x98\xEA\xB3\xA0 \xEC\x9E\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4. \xEC\x99\x84\xEB\xA3\x8C\xEB\x90\x98\xEB\xA9\xB4 \xEC\xB1\x84\xED\x8C\x85\xEC\x9D\x84 \xEC\x8B\x9C\xEC\x9E\x91\xED\x95\xA0 \xEC\x88\x98 \xEC\x9E\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4.");

    if (isAnalysisComplete (analysisStatus))
        return korean ("\xEB\xB6\x84\xEC\x84\x9D\xEC\x9D\xB4 \xEC\x99\x84\xEB\xA3\x8C\xEB\x90\x98\xEC\x97\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4. \xEC\x9D\xB4\xEC\xA0\x9C \xEC\x98\xA4\xEB\xA5\xB8\xEC\xAA\xBD \xEC\xB1\x84\xED\x8C\x85\xEC\x97\x90 \xEC\x88\x98\xEC\xA0\x95 \xEC\x9A\x94\xEC\xB2\xAD\xEC\x9D\x84 \xEC\x9E\x85\xEB\xA0\xA5\xED\x95\x98\xEC\x84\xB8\xEC\x9A\x94.");

    return juce::String::fromUTF8 (u8"준비가 끝났습니다. 이제 '분석 시작'을 눌러주세요.");
}

juce::String ControllerView::buildChatGuideText (const juce::String& analysisStatus) const
{
    if (isAnalysisComplete (analysisStatus))
        return juce::String::fromUTF8 (u8"보컬을 조금 앞으로 빼줘, 기타 하이컷해줘 같은 요청을 입력하세요.");

    return korean ("\xEC\xB1\x84\xED\x8C\x85\xEC\x9D\x80 WAV \xEB\xB6\x84\xEC\x84\x9D\xEC\x9D\xB4 \xEC\x99\x84\xEB\xA3\x8C\xEB\x90\x9C \xEB\x92\xA4\xEC\x97\x90 \xEC\x82\xAC\xEC\x9A\xA9\xED\x95\xA0 \xEC\x88\x98 \xEC\x9E\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4.");
}

bool ControllerView::isSessionReady (const juce::String& sessionStatus) const
{
    return sessionStatus.containsIgnoreCase ("tracks loaded");
}

bool ControllerView::isAnalysisComplete (const juce::String& analysisStatus) const
{
    return analysisStatus.containsIgnoreCase ("completed");
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

juce::File ControllerView::resolveDroppedStemFolder (const juce::StringArray& files) const
{
    for (const auto& path : files)
    {
        auto file = juce::File (path);

        if (file.isDirectory())
            return file;

        if (file.existsAsFile() && file.hasFileExtension ("wav;WAV"))
            return file.getParentDirectory();
    }

    return {};
}

int ControllerView::countWavFilesInFolder (const juce::File& folder)
{
    if (! folder.isDirectory())
        return 0;

    int count = 0;
    for (const auto& entry : juce::RangedDirectoryIterator (folder, false, "*.wav", juce::File::findFiles))
        juce::ignoreUnused (entry), ++count;
    for (const auto& entry : juce::RangedDirectoryIterator (folder, false, "*.WAV", juce::File::findFiles))
        juce::ignoreUnused (entry), ++count;

    return count;
}

void ControllerView::sendCurrentPrompt()
{
    if (audioProcessor.isRequestInFlight())
        return;

    audioProcessor.setCurrentPrompt (promptEditor.getText());
    audioProcessor.planActions();
}
