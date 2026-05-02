#pragma once

#include <JuceHeader.h>

namespace volta
{
struct MixSuggestion
{
    juce::String parameter;
    juce::String action;
    double value = 0.0;
    juce::String unit;
    juce::String reason;
};

struct MixAnalyzeResult
{
    bool succeeded = false;
    int statusCode = 0;
    juce::String summary;
    juce::String errorMessage;
    juce::String rawResponse;
    juce::Array<MixSuggestion> suggestions;
};

class MixAnalyzeClient : private juce::Thread
{
public:
    using ResultHandler = std::function<void (const MixAnalyzeResult&)>;

    MixAnalyzeClient()
        : juce::Thread ("VoltaMixAnalyzeClient")
    {
    }

    ~MixAnalyzeClient() override
    {
        stop();
    }

    void setResultHandler (ResultHandler newHandler)
    {
        const juce::ScopedLock scopedLock (lock);
        resultHandler = std::move (newHandler);
    }

    void requestAnalysis (juce::String endpointUrl, juce::String requestBody)
    {
        {
            const juce::ScopedLock scopedLock (lock);
            endpoint = std::move (endpointUrl);
            body = std::move (requestBody);
            hasPendingRequest = endpoint.isNotEmpty() && body.isNotEmpty();
        }

        startIfNeeded();
        notify();
    }

    void stop()
    {
        signalThreadShouldExit();
        notify();
        stopThread (2000);
    }

private:
    static bool isKnownParameter (const juce::String& parameter)
    {
        return parameter == "gain_db"
            || parameter == "low_cut_freq_hz"
            || parameter == "presence_db"
            || parameter == "compression_amount"
            || parameter == "warmth_amount";
    }

    static MixAnalyzeResult parseResponse (int statusCode, const juce::String& responseText)
    {
        MixAnalyzeResult result;
        result.statusCode = statusCode;
        result.rawResponse = responseText;

        auto parsed = juce::JSON::parse (responseText);

        if (! parsed.isObject())
        {
            result.errorMessage = responseText.isNotEmpty() ? responseText : "Server returned a non-JSON response.";
            return result;
        }

        if (auto* object = parsed.getDynamicObject())
        {
            if (auto errorValue = object->getProperty ("error"); errorValue.isObject())
            {
                if (auto* errorObject = errorValue.getDynamicObject())
                {
                    auto message = errorObject->getProperty ("message").toString();
                    result.errorMessage = message.isNotEmpty() ? message : "Server returned an error object.";
                }
                else
                {
                    result.errorMessage = "Server returned an error object.";
                }

                return result;
            }

            result.summary = object->getProperty ("summary").toString();

            if (auto suggestionsValue = object->getProperty ("suggestions"); suggestionsValue.isArray())
            {
                if (auto* suggestionArray = suggestionsValue.getArray())
                {
                    for (const auto& entry : *suggestionArray)
                    {
                        if (auto* suggestionObject = entry.getDynamicObject())
                        {
                            auto parameter = suggestionObject->getProperty ("parameter").toString();

                            if (! isKnownParameter (parameter))
                                continue;

                            MixSuggestion suggestion;
                            suggestion.parameter = parameter;
                            suggestion.action = suggestionObject->getProperty ("action").toString();
                            suggestion.value = static_cast<double> (suggestionObject->getProperty ("value"));
                            suggestion.unit = suggestionObject->getProperty ("unit").toString();
                            suggestion.reason = suggestionObject->getProperty ("reason").toString();
                            result.suggestions.add (suggestion);
                        }
                    }
                }
            }

            if (statusCode >= 200 && statusCode < 300)
            {
                result.succeeded = true;
                return result;
            }

            if (result.errorMessage.isEmpty())
                result.errorMessage = result.summary.isNotEmpty() ? result.summary : "Server returned an unsuccessful HTTP status.";
        }

        return result;
    }

    void startIfNeeded()
    {
        if (! isThreadRunning())
            startThread();
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            juce::String endpointToUse;
            juce::String bodyToUse;
            ResultHandler handlerToUse;

            {
                const juce::ScopedLock scopedLock (lock);

                if (! hasPendingRequest)
                {
                    wait (-1);
                    continue;
                }

                endpointToUse = endpoint;
                bodyToUse = body;
                handlerToUse = resultHandler;
                hasPendingRequest = false;
            }

            MixAnalyzeResult result;
            int statusCode = 0;
            auto headers = juce::String ("Content-Type: application/json\r\nAccept: application/json\r\n");

            auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                               .withExtraHeaders (headers)
                               .withConnectionTimeoutMs (4000)
                               .withStatusCode (&statusCode);

            if (auto stream = juce::URL (endpointToUse).withPOSTData (bodyToUse).createInputStream (options))
            {
                result = parseResponse (statusCode, stream->readEntireStreamAsString().trim());

                if (! result.succeeded && result.errorMessage.isEmpty())
                    result.errorMessage = "Analysis request failed.";
            }
            else
            {
                result.statusCode = statusCode;
                result.errorMessage = "Could not connect to the mix analysis server.";
            }

            if (handlerToUse != nullptr)
                handlerToUse (result);
        }
    }

    juce::CriticalSection lock;
    juce::String endpoint;
    juce::String body;
    bool hasPendingRequest = false;
    ResultHandler resultHandler;
};
}
