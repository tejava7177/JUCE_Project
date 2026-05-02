#pragma once

#include <JuceHeader.h>
#include "../Core/JsonCommandParser.h"

namespace volta
{
class LocalJsonClient : private juce::Thread
{
public:
    using CommandHandler = std::function<void (const MixCommand&)>;
    using PollStatusHandler = std::function<void (bool, const juce::String&)>;

    LocalJsonClient()
        : juce::Thread ("VoltaLocalJsonClient")
    {
    }

    ~LocalJsonClient() override
    {
        stopPolling();
    }

    void configure (juce::String endpointUrl, juce::String agentIdentifier, bool shouldPoll)
    {
        const juce::ScopedLock scopedLock (lock);
        endpoint = std::move (endpointUrl);
        agentId = std::move (agentIdentifier);
        enabled = shouldPoll;
    }

    void setCommandHandler (CommandHandler newHandler)
    {
        const juce::ScopedLock scopedLock (lock);
        commandHandler = std::move (newHandler);
    }

    void setPollStatusHandler (PollStatusHandler newHandler)
    {
        const juce::ScopedLock scopedLock (lock);
        pollStatusHandler = std::move (newHandler);
    }

    void startPolling()
    {
        if (! isThreadRunning())
            startThread();
    }

    void stopPolling()
    {
        signalThreadShouldExit();
        stopThread (2000);
    }

private:
    void run() override
    {
        while (! threadShouldExit())
        {
            juce::String requestUrl;
            CommandHandler currentCommandHandler;
            PollStatusHandler currentPollStatusHandler;

            {
                const juce::ScopedLock scopedLock (lock);

                if (! enabled || endpoint.isEmpty() || agentId.isEmpty())
                {
                    wait (500);
                    continue;
                }

                auto separator = endpoint.containsChar ('?') ? "&" : "?";
                requestUrl = endpoint + separator + "agent_id=" + juce::URL::addEscapeChars (agentId, true);
                currentCommandHandler = commandHandler;
                currentPollStatusHandler = pollStatusHandler;
            }

            bool pollSucceeded = false;
            juce::String response;
            auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                               .withConnectionTimeoutMs (1000);

            if (auto stream = juce::URL (requestUrl).createInputStream (options))
            {
                pollSucceeded = true;
                response = stream->readEntireStreamAsString().trim();

                if (currentCommandHandler != nullptr && response.isNotEmpty())
                    if (auto command = JsonCommandParser::parseSingleCommand (response))
                        currentCommandHandler (*command);
            }

            if (currentPollStatusHandler != nullptr)
                currentPollStatusHandler (pollSucceeded, response);

            wait (500);
        }
    }

    juce::CriticalSection lock;
    juce::String endpoint;
    juce::String agentId;
    bool enabled = true;
    CommandHandler commandHandler;
    PollStatusHandler pollStatusHandler;
};
}
