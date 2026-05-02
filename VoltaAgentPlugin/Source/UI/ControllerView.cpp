#include "ControllerView.h"
#include "../PluginProcessor.h"

ControllerView::ControllerView (VoltaAgentPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    commandLabel.setText ("Command", juce::dontSendNotification);
    agentsLabel.setText ("Connected Agents", juce::dontSendNotification);
    commandStatusTitle.setText ("Last Request", juce::dontSendNotification);
    analyzeStatusTitle.setText ("Analyze Status", juce::dontSendNotification);
    analyzeSummaryTitle.setText ("Summary", juce::dontSendNotification);
    analyzeSuggestionsTitle.setText ("Suggestions", juce::dontSendNotification);

    commandEditor.setMultiLine (true);
    commandEditor.setReturnKeyStartsNewLine (true);
    commandEditor.setText ("Make vocal clearer and bass less muddy", juce::dontSendNotification);

    agentsList.setMultiLine (true);
    agentsList.setReadOnly (true);
    agentsList.setScrollbarsShown (true);

    analyzeSummaryValue.setMultiLine (true);
    analyzeSummaryValue.setReadOnly (true);
    analyzeSummaryValue.setScrollbarsShown (true);

    analyzeSuggestionsValue.setMultiLine (true);
    analyzeSuggestionsValue.setReadOnly (true);
    analyzeSuggestionsValue.setScrollbarsShown (true);

    sendButton.addListener (this);
    analyzeButton.addListener (this);

    addAndMakeVisible (commandLabel);
    addAndMakeVisible (commandEditor);
    addAndMakeVisible (sendButton);
    addAndMakeVisible (analyzeButton);
    addAndMakeVisible (agentsLabel);
    addAndMakeVisible (agentsList);
    addAndMakeVisible (commandStatusTitle);
    addAndMakeVisible (commandStatusValue);
    addAndMakeVisible (analyzeStatusTitle);
    addAndMakeVisible (analyzeStatusValue);
    addAndMakeVisible (analyzeSummaryTitle);
    addAndMakeVisible (analyzeSummaryValue);
    addAndMakeVisible (analyzeSuggestionsTitle);
    addAndMakeVisible (analyzeSuggestionsValue);

    refreshState();
}

ControllerView::~ControllerView()
{
    sendButton.removeListener (this);
    analyzeButton.removeListener (this);
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
    commandLabel.setBounds (area.removeFromTop (20));
    commandEditor.setBounds (area.removeFromTop (64));
    area.removeFromTop (10);
    auto buttonRow = area.removeFromTop (30);
    sendButton.setBounds (buttonRow.removeFromLeft (140));
    buttonRow.removeFromLeft (10);
    analyzeButton.setBounds (buttonRow.removeFromLeft (140));
    area.removeFromTop (14);
    commandStatusTitle.setBounds (area.removeFromTop (20));
    commandStatusValue.setBounds (area.removeFromTop (24));
    area.removeFromTop (10);
    analyzeStatusTitle.setBounds (area.removeFromTop (20));
    analyzeStatusValue.setBounds (area.removeFromTop (24));
    area.removeFromTop (10);
    analyzeSummaryTitle.setBounds (area.removeFromTop (20));
    analyzeSummaryValue.setBounds (area.removeFromTop (48));
    area.removeFromTop (10);
    analyzeSuggestionsTitle.setBounds (area.removeFromTop (20));
    analyzeSuggestionsValue.setBounds (area.removeFromTop (96));
    area.removeFromTop (10);
    agentsLabel.setBounds (area.removeFromTop (20));
    agentsList.setBounds (area);
}

void ControllerView::refreshState()
{
    commandStatusValue.setText (audioProcessor.getLastCommandText(), juce::dontSendNotification);
    analyzeStatusValue.setText (audioProcessor.getAnalyzeStatusText(), juce::dontSendNotification);
    analyzeSummaryValue.setText (audioProcessor.getAnalyzeSummaryText(), juce::dontSendNotification);
    analyzeSuggestionsValue.setText (audioProcessor.getAnalyzeSuggestionsText(), juce::dontSendNotification);
    analyzeButton.setEnabled (! audioProcessor.isAnalyzeRequestInFlight());
    agentsList.setText (audioProcessor.getConnectedAgentsSummary(), juce::dontSendNotification);
}

void ControllerView::buttonClicked (juce::Button* button)
{
    if (button == &sendButton)
    {
        audioProcessor.submitControllerCommand (commandEditor.getText());
        refreshState();
    }
    else if (button == &analyzeButton)
    {
        audioProcessor.requestMixAnalysis();
        refreshState();
    }
}
