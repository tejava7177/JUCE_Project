#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "UI/DebugPanel.cpp"
#include "UI/AgentView.cpp"
#include "UI/ControllerView.cpp"

VoltaAgentPluginAudioProcessorEditor::VoltaAgentPluginAudioProcessorEditor (VoltaAgentPluginAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      agentView (audioProcessor),
      controllerView (audioProcessor),
      debugPanel (audioProcessor)
{
    setSize (620, 560);

    titleLabel.setText ("Volta Agent", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));

    trackTypeLabel.setText ("Track Type", juce::dontSendNotification);
    statusTitleLabel.setText ("Status", juce::dontSendNotification);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (trackTypeLabel);
    addAndMakeVisible (statusTitleLabel);
    addAndMakeVisible (statusValueLabel);
    addAndMakeVisible (trackTypeBox);
    addAndMakeVisible (agentView);
    addAndMakeVisible (controllerView);
    addAndMakeVisible (debugPanel);

    auto roles = volta::getTrackRoleNames();
    for (auto index = 0; index < roles.size(); ++index)
        trackTypeBox.addItem (roles[index], index + 1);
    trackTypeAttachment = std::make_unique<ComboAttachment> (audioProcessor.apvts, "track_role", trackTypeBox);

    debugPanel.onLayoutChange = [this]
    {
        resized();
    };

    startTimerHz (10);
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

    auto state = audioProcessor.getAgentState();
    auto headerRow = area.removeFromTop (30);

    if (state.mode == volta::PluginMode::agent)
    {
        trackTypeLabel.setVisible (true);
        trackTypeBox.setVisible (true);
        trackTypeLabel.setBounds (headerRow.removeFromLeft (78));
        trackTypeBox.setBounds (headerRow.removeFromLeft (170));
        headerRow.removeFromLeft (20);
    }
    else
    {
        trackTypeLabel.setVisible (false);
        trackTypeBox.setVisible (false);
    }

    statusTitleLabel.setBounds (headerRow.removeFromLeft (50));
    statusValueLabel.setBounds (headerRow.removeFromLeft (230));

    area.removeFromTop (14);

    auto debugHeight = debugPanel.getPreferredHeight();
    auto contentHeight = juce::jmax (150, area.getHeight() - debugHeight - 14);
    auto contentArea = area.removeFromTop (contentHeight);

    agentView.setVisible (state.mode == volta::PluginMode::agent);
    controllerView.setVisible (state.mode == volta::PluginMode::controller);
    agentView.setBounds (contentArea);
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
    auto state = audioProcessor.getAgentState();
    titleLabel.setText (state.mode == volta::PluginMode::agent ? "Volta Agent" : "Volta Controller",
                        juce::dontSendNotification);
    statusValueLabel.setText (audioProcessor.getConnectionStatusText(), juce::dontSendNotification);
    agentView.refreshState();
    controllerView.refreshState();
    debugPanel.refreshState();
    resized();
}
