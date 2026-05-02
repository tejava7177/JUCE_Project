#pragma once

#include <JuceHeader.h>
#include "../Core/AgentState.h"
#include "../Core/MixCommand.h"
#include "../Core/TrackRole.h"

namespace volta
{
class TrackAgent
{
public:
    explicit TrackAgent (juce::AudioProcessorValueTreeState& apvtsToUse)
        : apvts (apvtsToUse)
    {
    }

    AgentState captureState() const
    {
        AgentState snapshot;
        snapshot.agentId = apvts.state.getProperty ("agentId", "agent_01").toString();
        snapshot.serverEndpoint = apvts.state.getProperty ("serverEndpoint", "http://127.0.0.1:5000/command").toString();
        snapshot.mode = static_cast<PluginMode> (static_cast<int> (apvts.getRawParameterValue ("plugin_mode")->load()));
        snapshot.trackRoleIndex = static_cast<int> (apvts.getRawParameterValue ("track_role")->load());
        snapshot.gainDb = apvts.getRawParameterValue ("gain_db")->load();
        snapshot.lowCutEnabled = apvts.getRawParameterValue ("low_cut_enabled")->load() >= 0.5f;
        snapshot.lowCutFrequencyHz = apvts.getRawParameterValue ("low_cut_freq_hz")->load();
        snapshot.presenceDb = apvts.getRawParameterValue ("presence_db")->load();
        snapshot.compressionAmount = apvts.getRawParameterValue ("compression_amount")->load();
        snapshot.warmthAmount = apvts.getRawParameterValue ("warmth_amount")->load();
        snapshot.pollingEnabled = static_cast<bool> (apvts.state.getProperty ("pollingEnabled", true));
        return snapshot;
    }

    void applyCommand (const MixCommand& command)
    {
        if (command.parameter == "gain_db")
            setFloatParameter ("gain_db", juce::jlimit (-24.0f, 12.0f, command.value));
        else if (command.parameter == "low_cut_enabled")
            setBoolParameter ("low_cut_enabled", command.value >= 0.5f);
        else if (command.parameter == "low_cut_freq_hz")
            setFloatParameter ("low_cut_freq_hz", juce::jlimit (20.0f, 300.0f, command.value));
        else if (command.parameter == "presence_db")
            setFloatParameter ("presence_db", juce::jlimit (-6.0f, 6.0f, command.value));
        else if (command.parameter == "compression_amount")
            setFloatParameter ("compression_amount", juce::jlimit (0.0f, 1.0f, command.value));
        else if (command.parameter == "warmth_amount")
            setFloatParameter ("warmth_amount", juce::jlimit (0.0f, 1.0f, command.value));
    }

private:
    void setFloatParameter (const juce::String& parameterId, float value)
    {
        if (auto* parameter = apvts.getParameter (parameterId))
        {
            auto normalizedValue = parameter->convertTo0to1 (value);
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (normalizedValue);
            parameter->endChangeGesture();
        }
    }

    void setBoolParameter (const juce::String& parameterId, bool enabled)
    {
        if (auto* parameter = apvts.getParameter (parameterId))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (enabled ? 1.0f : 0.0f);
            parameter->endChangeGesture();
        }
    }

    juce::AudioProcessorValueTreeState& apvts;
};
}
