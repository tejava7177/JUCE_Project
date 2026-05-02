#pragma once

#include <JuceHeader.h>
#include "MixCommand.h"

namespace volta
{
class JsonCommandParser
{
public:
    static std::optional<MixCommand> parseSingleCommand (const juce::String& jsonText)
    {
        auto parsed = juce::JSON::parse (jsonText);

        if (! parsed.isObject())
            return std::nullopt;

        if (auto* object = parsed.getDynamicObject())
        {
            if (object->hasProperty ("command"))
                return parseCommandVar (object->getProperty ("command"));

            return parseCommandVar (parsed);
        }

        return std::nullopt;
    }

private:
    static std::optional<MixCommand> parseCommandVar (const juce::var& value)
    {
        if (auto* object = value.getDynamicObject())
        {
            MixCommand command;
            command.targetAgent = object->getProperty ("target_agent").toString();
            command.parameter = object->getProperty ("parameter").toString();

            command.value = static_cast<float> (object->getProperty ("value"));

            if (command.targetAgent.isNotEmpty() && command.parameter.isNotEmpty())
                return command;
        }

        return std::nullopt;
    }
};
}
