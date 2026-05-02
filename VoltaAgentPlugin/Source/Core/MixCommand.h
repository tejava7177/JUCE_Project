#pragma once

#include <JuceHeader.h>

namespace volta
{
struct MixCommand
{
    juce::String targetAgent;
    juce::String parameter;
    float value = 0.0f;
};
}
