#include "DebugPanel.h"
#include "../PluginProcessor.h"

DebugPanel::DebugPanel (VoltaAgentPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    addAndMakeVisible (expandButton);
    expandButton.addListener (this);

    endpointLabel.setText ("Server URL", juce::dontSendNotification);
    endpointEditor.addListener (this);

    addChildComponent (endpointLabel);
    addChildComponent (endpointEditor);

    updateVisibility();
    refreshState();
}

DebugPanel::~DebugPanel()
{
    expandButton.removeListener (this);
    endpointEditor.removeListener (this);
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
    endpointLabel.setBounds (row.removeFromLeft (labelWidth));
    endpointEditor.setBounds (row);
}

void DebugPanel::refreshState()
{
    endpointEditor.setText (audioProcessor.getServerEndpointText(), juce::dontSendNotification);
}

int DebugPanel::getPreferredHeight() const
{
    return expanded ? 92 : 52;
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
}
