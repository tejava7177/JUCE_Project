#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "UI/DebugPanel.cpp"
#include "UI/ControllerView.cpp"

VoltaAgentPluginAudioProcessorEditor::VoltaAgentPluginAudioProcessorEditor (VoltaAgentPluginAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      controllerView (audioProcessor),
      debugPanel (audioProcessor)
{
    setSize (1180, 820);

    titleLabel.setText ("Volta AI Mixing Assistant", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));

    statusTitleLabel.setText ("Server", juce::dontSendNotification);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (statusTitleLabel);
    addAndMakeVisible (statusValueLabel);
    addAndMakeVisible (controllerView);
    addAndMakeVisible (debugPanel);

    debugPanel.onLayoutChange = [this]
    {
        resized();
    };

    startTimerHz (8);
    refreshLayoutAndState();
}

VoltaAgentPluginAudioProcessorEditor::~VoltaAgentPluginAudioProcessorEditor()
{
    stopTimer();
}

void VoltaAgentPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (16, 18, 22));

    auto panelBounds = getLocalBounds().reduced (12).toFloat();
    g.setColour (juce::Colour::fromRGB (32, 36, 43));
    g.fillRoundedRectangle (panelBounds, 12.0f);

    g.setColour (juce::Colour::fromRGB (87, 225, 193));
    g.drawRoundedRectangle (panelBounds, 12.0f, 1.0f);
}

void VoltaAgentPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (24);
    titleLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (12);

    auto headerRow = area.removeFromTop (30);
    statusTitleLabel.setBounds (headerRow.removeFromLeft (52));
    statusValueLabel.setBounds (headerRow);

    area.removeFromTop (14);

    auto debugHeight = debugPanel.getPreferredHeight();
    auto contentHeight = juce::jmax (150, area.getHeight() - debugHeight - 14);
    auto contentArea = area.removeFromTop (contentHeight);
    controllerView.setBounds (contentArea);

    area.removeFromTop (14);
    debugPanel.setBounds (area.removeFromTop (debugHeight));
}

void VoltaAgentPluginAudioProcessorEditor::timerCallback()
{
    refreshLayoutAndState();
}

void VoltaAgentPluginAudioProcessorEditor::refreshLayoutAndState()
{
    statusValueLabel.setText (audioProcessor.getServerStatusText(), juce::dontSendNotification);
    controllerView.refreshState();
    debugPanel.refreshState();
    resized();
}
