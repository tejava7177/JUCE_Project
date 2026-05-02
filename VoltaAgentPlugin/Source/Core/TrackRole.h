#pragma once

#include <JuceHeader.h>

namespace volta
{
inline juce::StringArray getTrackRoleNames()
{
    return { "Vocal", "Bass", "Drum", "Guitar", "Keys", "Bus", "Master", "Other" };
}

inline juce::String getTrackRoleName (int index)
{
    auto names = getTrackRoleNames();
    return juce::isPositiveAndBelow (index, names.size()) ? names[index] : "Other";
}
}
