#pragma once

#include <JuceHeader.h>
#include "PluginMode.h"

namespace volta
{
struct AgentState
{
    juce::String agentId { "agent_01" };
    juce::String serverEndpoint { "http://127.0.0.1:5000/command" };
    PluginMode mode { PluginMode::agent };
    int trackRoleIndex = 0;
    float gainDb = 0.0f;
    bool lowCutEnabled = false;
    float lowCutFrequencyHz = 80.0f;
    float presenceDb = 0.0f;
    float compressionAmount = 0.0f;
    float warmthAmount = 0.0f;
    bool pollingEnabled = true;
};
}
