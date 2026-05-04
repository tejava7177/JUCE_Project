#pragma once

#include <JuceHeader.h>

namespace volta
{
enum class SessionRequestType
{
    health,
    sessionSummary,
    planActions,
    applyActions
};

struct SessionTrackInfo
{
    juce::String trackName;
    int deviceCount = 0;
    int supportedParameterCount = 0;
};

struct SessionOperation
{
    juce::String track;
    juce::String device;
    juce::String parameter;
    juce::String path;
    double oldValue = 0.0;
    double newValue = 0.0;
    double value = 0.0;
};

struct SessionControlResponse
{
    SessionRequestType type { SessionRequestType::health };
    bool succeeded = false;
    bool parseSucceeded = false;
    int statusCode = 0;
    bool sessionLoaded = false;
    int pendingApplyCount = 0;
    int trackCount = 0;
    int staged = 0;
    int applied = 0;
    int revision = 0;
    juce::String status;
    juce::String explanation;
    juce::String errorMessage;
    juce::String rawResponse;
    juce::Array<SessionTrackInfo> tracks;
    juce::Array<SessionOperation> operations;
};

class SessionControlClient : private juce::Thread
{
public:
    using ResultHandler = std::function<void (const SessionControlResponse&)>;

    SessionControlClient()
        : juce::Thread ("VoltaSessionControlClient")
    {
    }

    ~SessionControlClient() override
    {
        stop();
    }

    void setResultHandler (ResultHandler newHandler)
    {
        const juce::ScopedLock scopedLock (lock);
        resultHandler = std::move (newHandler);
    }

    void requestHealth (juce::String endpointUrl)
    {
        enqueueRequest (SessionRequestType::health, std::move (endpointUrl), {});
    }

    void requestSessionSummary (juce::String endpointUrl)
    {
        enqueueRequest (SessionRequestType::sessionSummary, std::move (endpointUrl), {});
    }

    void requestPlanActions (juce::String endpointUrl, juce::String prompt)
    {
        auto requestObject = std::make_unique<juce::DynamicObject>();
        requestObject->setProperty ("prompt", prompt);
        enqueueRequest (SessionRequestType::planActions,
                        std::move (endpointUrl),
                        juce::JSON::toString (juce::var (requestObject.release())));
    }

    void requestApplyActions (juce::String endpointUrl, const juce::Array<SessionOperation>& operations)
    {
        juce::Array<juce::var> serializedOperations;

        for (const auto& operation : operations)
        {
            auto operationObject = std::make_unique<juce::DynamicObject>();
            operationObject->setProperty ("track", operation.track);
            operationObject->setProperty ("device", operation.device);
            operationObject->setProperty ("parameter", operation.parameter);
            operationObject->setProperty ("path", operation.path);
            operationObject->setProperty ("value", operation.value);
            serializedOperations.add (juce::var (operationObject.release()));
        }

        auto requestObject = std::make_unique<juce::DynamicObject>();
        requestObject->setProperty ("operations", juce::var (serializedOperations));

        enqueueRequest (SessionRequestType::applyActions,
                        std::move (endpointUrl),
                        juce::JSON::toString (juce::var (requestObject.release())));
    }

    void stop()
    {
        signalThreadShouldExit();
        notify();
        stopThread (2000);
    }

private:
    struct PendingRequest
    {
        SessionRequestType type { SessionRequestType::health };
        juce::String endpoint;
        juce::String body;
    };

    static int getTimeoutMsForRequest (SessionRequestType type)
    {
        switch (type)
        {
            case SessionRequestType::health:
            case SessionRequestType::sessionSummary:
                return 4000;

            case SessionRequestType::planActions:
                return 45000;

            case SessionRequestType::applyActions:
                return 15000;
        }

        return 4000;
    }

    static juce::String getTransportErrorMessage (SessionRequestType type, int statusCode)
    {
        juce::ignoreUnused (statusCode);

        switch (type)
        {
            case SessionRequestType::health:
            case SessionRequestType::sessionSummary:
                return "Server offline";

            case SessionRequestType::planActions:
                return "Planning timed out";

            case SessionRequestType::applyActions:
                return "Apply staging timed out";
        }

        return "Server offline";
    }

    static juce::String getErrorMessageFromObject (juce::DynamicObject* object)
    {
        if (object == nullptr)
            return {};

        if (auto errorValue = object->getProperty ("error"); errorValue.isObject())
        {
            if (auto* errorObject = errorValue.getDynamicObject())
                return errorObject->getProperty ("message").toString();
        }

        return {};
    }

    static SessionControlResponse parseHealthResponse (int statusCode, const juce::String& responseText)
    {
        SessionControlResponse response;
        response.type = SessionRequestType::health;
        response.statusCode = statusCode;
        response.rawResponse = responseText;

        auto parsed = juce::JSON::parse (responseText);
        auto* object = parsed.getDynamicObject();

        if (object == nullptr)
        {
            response.errorMessage = "Invalid response from server";
            return response;
        }

        response.parseSucceeded = true;
        response.errorMessage = getErrorMessageFromObject (object);

        response.sessionLoaded = static_cast<bool> (object->getProperty ("session_loaded"));
        response.pendingApplyCount = static_cast<int> (object->getProperty ("pending_apply_count"));
        auto statusText = object->getProperty ("status").toString();

        if (statusCode >= 200 && statusCode < 300 && statusText == "ok")
        {
            response.succeeded = true;
            return response;
        }

        if (response.errorMessage.isEmpty())
            response.errorMessage = "Server offline";

        return response;
    }

    static SessionControlResponse parseSessionSummaryResponse (int statusCode, const juce::String& responseText)
    {
        SessionControlResponse response;
        response.type = SessionRequestType::sessionSummary;
        response.statusCode = statusCode;
        response.rawResponse = responseText;

        auto parsed = juce::JSON::parse (responseText);
        auto* object = parsed.getDynamicObject();

        if (object == nullptr)
        {
            response.errorMessage = "Invalid response from server";
            return response;
        }

        response.parseSucceeded = true;
        response.errorMessage = getErrorMessageFromObject (object);
        response.sessionLoaded = static_cast<bool> (object->getProperty ("session_loaded"));
        response.trackCount = static_cast<int> (object->getProperty ("track_count"));

        if (auto tracksValue = object->getProperty ("tracks"); tracksValue.isArray())
        {
            if (auto* tracksArray = tracksValue.getArray())
            {
                for (const auto& entry : *tracksArray)
                {
                    if (auto* trackObject = entry.getDynamicObject())
                    {
                        SessionTrackInfo trackInfo;
                        trackInfo.trackName = trackObject->getProperty ("track_name").toString();
                        trackInfo.deviceCount = static_cast<int> (trackObject->getProperty ("device_count"));
                        trackInfo.supportedParameterCount = static_cast<int> (trackObject->getProperty ("supported_parameter_count"));
                        response.tracks.add (trackInfo);
                    }
                }
            }
        }

        if (statusCode >= 200 && statusCode < 300)
        {
            response.succeeded = true;
            return response;
        }

        if (response.errorMessage.isEmpty())
            response.errorMessage = response.sessionLoaded ? "Failed to refresh session" : "Session not loaded";

        return response;
    }

    static SessionControlResponse parsePlanResponse (int statusCode, const juce::String& responseText)
    {
        SessionControlResponse response;
        response.type = SessionRequestType::planActions;
        response.statusCode = statusCode;
        response.rawResponse = responseText;

        auto parsed = juce::JSON::parse (responseText);
        auto* object = parsed.getDynamicObject();

        if (object == nullptr)
        {
            response.errorMessage = "Invalid response from server";
            return response;
        }

        response.parseSucceeded = true;
        response.errorMessage = getErrorMessageFromObject (object);
        response.explanation = object->getProperty ("explanation").toString();

        if (auto operationsValue = object->getProperty ("operations"); operationsValue.isArray())
        {
            if (auto* operationsArray = operationsValue.getArray())
            {
                for (const auto& entry : *operationsArray)
                {
                    if (auto* operationObject = entry.getDynamicObject())
                    {
                        SessionOperation operation;
                        operation.track = operationObject->getProperty ("track").toString();
                        operation.device = operationObject->getProperty ("device").toString();
                        operation.parameter = operationObject->getProperty ("parameter").toString();
                        operation.path = operationObject->getProperty ("path").toString();
                        operation.oldValue = static_cast<double> (operationObject->getProperty ("old_value"));
                        operation.newValue = static_cast<double> (operationObject->getProperty ("new_value"));
                        operation.value = static_cast<double> (operationObject->getProperty ("value"));
                        response.operations.add (operation);
                    }
                }
            }
        }

        if (statusCode >= 200 && statusCode < 300)
        {
            response.succeeded = true;
            return response;
        }

        if (response.errorMessage.isEmpty())
            response.errorMessage = "Planning failed";

        return response;
    }

    static SessionControlResponse parseApplyResponse (int statusCode, const juce::String& responseText)
    {
        SessionControlResponse response;
        response.type = SessionRequestType::applyActions;
        response.statusCode = statusCode;
        response.rawResponse = responseText;

        auto parsed = juce::JSON::parse (responseText);
        auto* object = parsed.getDynamicObject();

        if (object == nullptr)
        {
            response.errorMessage = "Invalid response from server";
            return response;
        }

        response.parseSucceeded = true;
        response.errorMessage = getErrorMessageFromObject (object);
        response.status = object->getProperty ("status").toString();
        response.staged = static_cast<int> (object->getProperty ("staged"));
        response.applied = static_cast<int> (object->getProperty ("applied"));
        response.revision = static_cast<int> (object->getProperty ("revision"));

        if (statusCode >= 200 && statusCode < 300 && response.status == "staged")
        {
            response.succeeded = true;
            return response;
        }

        if (response.errorMessage.isEmpty())
            response.errorMessage = "Apply staging failed";

        return response;
    }

    static SessionControlResponse parseResponse (SessionRequestType type, int statusCode, const juce::String& responseText)
    {
        switch (type)
        {
            case SessionRequestType::health:         return parseHealthResponse (statusCode, responseText);
            case SessionRequestType::sessionSummary: return parseSessionSummaryResponse (statusCode, responseText);
            case SessionRequestType::planActions:    return parsePlanResponse (statusCode, responseText);
            case SessionRequestType::applyActions:   return parseApplyResponse (statusCode, responseText);
        }

        return {};
    }

    void enqueueRequest (SessionRequestType type, juce::String endpointUrl, juce::String requestBody)
    {
        {
            const juce::ScopedLock scopedLock (lock);
            pendingRequest = PendingRequest { type, std::move (endpointUrl), std::move (requestBody) };
            hasPendingRequest = true;
        }

        if (! isThreadRunning())
            startThread();

        notify();
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            PendingRequest request;
            ResultHandler handlerToUse;

            {
                const juce::ScopedLock scopedLock (lock);

                if (! hasPendingRequest)
                {
                    wait (2000);
                    continue;
                }

                request = pendingRequest;
                handlerToUse = resultHandler;
                hasPendingRequest = false;
            }

            int statusCode = 0;
            juce::String responseBody;
            auto headers = juce::String ("Content-Type: application/json\r\nAccept: application/json\r\n");

            juce::URL url (request.endpoint);
            juce::URL requestUrl = request.body.isNotEmpty() ? url.withPOSTData (request.body) : url;

            auto options = juce::URL::InputStreamOptions (request.body.isNotEmpty()
                                                              ? juce::URL::ParameterHandling::inPostData
                                                              : juce::URL::ParameterHandling::inAddress)
                               .withExtraHeaders (headers)
                               .withConnectionTimeoutMs (getTimeoutMsForRequest (request.type))
                               .withStatusCode (&statusCode);

            SessionControlResponse response;
            response.type = request.type;

            if (auto stream = requestUrl.createInputStream (options))
            {
                responseBody = stream->readEntireStreamAsString().trim();
                response = parseResponse (request.type, statusCode, responseBody);
            }
            else
            {
                response.statusCode = statusCode;
                response.errorMessage = getTransportErrorMessage (request.type, statusCode);
            }

            if (handlerToUse != nullptr)
                handlerToUse (response);
        }
    }

    juce::CriticalSection lock;
    PendingRequest pendingRequest;
    bool hasPendingRequest = false;
    ResultHandler resultHandler;
};
}
