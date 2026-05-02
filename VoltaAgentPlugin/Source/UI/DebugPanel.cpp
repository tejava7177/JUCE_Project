#include "DebugPanel.h"
#include "../PluginProcessor.h"

DebugPanel::DebugPanel (VoltaAgentPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    addAndMakeVisible (expandButton);
    expandButton.addListener (this);

    modeLabel.setText ("Mode", juce::dontSendNotification);
    agentIdLabel.setText ("Agent ID", juce::dontSendNotification);
    endpointLabel.setText ("Endpoint", juce::dontSendNotification);

    modeBox.addItem ("Agent", 1);
    modeBox.addItem ("Controller", 2);
    modeAttachment = std::make_unique<ComboAttachment> (audioProcessor.apvts, "plugin_mode", modeBox);

    agentIdEditor.addListener (this);
    endpointEditor.addListener (this);
    pollingToggle.addListener (this);

    addChildComponent (modeLabel);
    addChildComponent (agentIdLabel);
    addChildComponent (endpointLabel);
    addChildComponent (modeBox);
    addChildComponent (agentIdEditor);
    addChildComponent (endpointEditor);
    addChildComponent (pollingToggle);

    updateVisibility();
    refreshState();
}

DebugPanel::~DebugPanel()
{
    expandButton.removeListener (this);
    agentIdEditor.removeListener (this);
    endpointEditor.removeListener (this);
    pollingToggle.removeListener (this);
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
    constexpr int rowHeight = 26;
    constexpr int labelWidth = 80;

    auto row = area.removeFromTop (rowHeight);
    modeLabel.setBounds (row.removeFromLeft (labelWidth));
    modeBox.setBounds (row.removeFromLeft (160));

    area.removeFromTop (8);
    row = area.removeFromTop (rowHeight);
    agentIdLabel.setBounds (row.removeFromLeft (labelWidth));
    agentIdEditor.setBounds (row);

    area.removeFromTop (8);
    row = area.removeFromTop (rowHeight);
    endpointLabel.setBounds (row.removeFromLeft (labelWidth));
    endpointEditor.setBounds (row);

    area.removeFromTop (8);
    pollingToggle.setBounds (area.removeFromTop (rowHeight));
}

void DebugPanel::refreshState()
{
    auto state = audioProcessor.getAgentState();
    agentIdEditor.setText (state.agentId, juce::dontSendNotification);
    endpointEditor.setText (state.serverEndpoint, juce::dontSendNotification);
    pollingToggle.setToggleState (state.pollingEnabled, juce::dontSendNotification);
}

int DebugPanel::getPreferredHeight() const
{
    return expanded ? 170 : 52;
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
    }
    else if (button == &pollingToggle)
    {
        audioProcessor.setPollingEnabled (pollingToggle.getToggleState());
    }
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
    audioProcessor.setAgentId (agentIdEditor.getText());
    audioProcessor.setServerEndpoint (endpointEditor.getText());
    refreshState();
}

void DebugPanel::updateVisibility()
{
    modeLabel.setVisible (expanded);
    agentIdLabel.setVisible (expanded);
    endpointLabel.setVisible (expanded);
    modeBox.setVisible (expanded);
    agentIdEditor.setVisible (expanded);
    endpointEditor.setVisible (expanded);
    pollingToggle.setVisible (expanded);
}
