#pragma once

#include <JuceHeader.h>

namespace volta
{
enum class SessionRequestType
{
    health,
    sessionSummary,
    planActions,
    applyActions,
    createAnalysisSession,
    uploadAnalysisTrack,
    fetchAnalysisSession,
    createProject,
    uploadProjectTrack,
    fetchProjectAnalysis,
    projectChat
};

struct SessionTrackInfo
{
    juce::String trackName;
    int deviceCount = 0;
    int supportedParameterCount = 0;
    double trackLengthSeconds = 0.0;
    juce::String trackLengthDisplay;
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
    int uploadedTracks = 0;
    juce::String status;
    juce::String explanation;
    juce::String errorMessage;
    juce::String rawResponse;
    juce::String projectSessionId;
    juce::String chatSessionId;
    juce::String analysisSessionId;
    juce::Array<SessionTrackInfo> tracks;
    juce::Array<SessionOperation> operations;
    juce::String analysisSummary;
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

    void requestCreateAnalysisSession (juce::String endpointUrl, juce::String projectName, juce::String dawName)
    {
        auto requestObject = std::make_unique<juce::DynamicObject>();
        requestObject->setProperty ("project_name", projectName);
        requestObject->setProperty ("daw", dawName);
        enqueueRequest (SessionRequestType::createAnalysisSession,
                        std::move (endpointUrl),
                        juce::JSON::toString (juce::var (requestObject.release())));
    }

    void requestUploadAnalysisTrack (juce::String endpointUrl,
                                     juce::String trackName,
                                     int trackIndex,
                                     juce::File fileToUpload)
    {
        PendingRequest request;
        request.type = SessionRequestType::uploadAnalysisTrack;
        request.endpoint = std::move (endpointUrl);
        request.formFields.set ("track_name", trackName);
        request.formFields.set ("track_index", juce::String (trackIndex));
        request.file = std::move (fileToUpload);
        request.fileFieldName = "file";
        enqueueRequest (std::move (request));
    }

    void requestAnalysisSession (juce::String endpointUrl)
    {
        enqueueRequest (SessionRequestType::fetchAnalysisSession, std::move (endpointUrl), {});
    }

    void requestCreateProject (juce::String endpointUrl)
    {
        enqueueRequest (SessionRequestType::createProject, std::move (endpointUrl), "{}");
    }

    void requestUploadProjectTrack (juce::String endpointUrl, juce::File fileToUpload)
    {
        PendingRequest request;
        request.type = SessionRequestType::uploadProjectTrack;
        request.endpoint = std::move (endpointUrl);
        request.file = std::move (fileToUpload);
        request.fileFieldName = "file";
        enqueueRequest (std::move (request));
    }

    void requestProjectAnalysis (juce::String endpointUrl)
    {
        enqueueRequest (SessionRequestType::fetchProjectAnalysis, std::move (endpointUrl), {});
    }

    void requestProjectChat (juce::String endpointUrl, juce::String message)
    {
        auto requestObject = std::make_unique<juce::DynamicObject>();
        requestObject->setProperty ("message", message);
        enqueueRequest (SessionRequestType::projectChat,
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
        juce::StringPairArray formFields;
        juce::File file;
        juce::String fileFieldName;
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

            case SessionRequestType::createAnalysisSession:
                return 10000;

            case SessionRequestType::uploadAnalysisTrack:
            case SessionRequestType::fetchAnalysisSession:
                return 30000;

            case SessionRequestType::createProject:
                return 10000;

            case SessionRequestType::uploadProjectTrack:
            case SessionRequestType::fetchProjectAnalysis:
                return 30000;

            case SessionRequestType::projectChat:
                return 120000;
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

            case SessionRequestType::createAnalysisSession:
            case SessionRequestType::uploadAnalysisTrack:
            case SessionRequestType::fetchAnalysisSession:
                return "Analysis request timed out";

            case SessionRequestType::createProject:
            case SessionRequestType::uploadProjectTrack:
            case SessionRequestType::fetchProjectAnalysis:
                return "Project request timed out";

            case SessionRequestType::projectChat:
                return "Chat request timed out";
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
                        trackInfo.trackLengthSeconds = static_cast<double> (trackObject->getProperty ("track_length_seconds"));
                        trackInfo.trackLengthDisplay = trackObject->getProperty ("track_length_display").toString();
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

    static SessionControlResponse parseCreateAnalysisSessionResponse (int statusCode, const juce::String& responseText)
    {
        SessionControlResponse response;
        response.type = SessionRequestType::createAnalysisSession;
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
        response.projectSessionId = object->getProperty ("project_session_id").toString();
        response.chatSessionId = object->getProperty ("chat_session_id").toString();
        response.analysisSessionId = object->getProperty ("analysis_session_id").toString();
        response.status = object->getProperty ("status").toString();

        if (statusCode >= 200 && statusCode < 300 && response.status == "created")
        {
            response.succeeded = true;
            return response;
        }

        if (response.errorMessage.isEmpty())
            response.errorMessage = "Failed to create analysis session";

        return response;
    }

    static SessionControlResponse parseUploadAnalysisTrackResponse (int statusCode, const juce::String& responseText)
    {
        SessionControlResponse response;
        response.type = SessionRequestType::uploadAnalysisTrack;
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
        response.analysisSessionId = object->getProperty ("analysis_session_id").toString();
        response.uploadedTracks = static_cast<int> (object->getProperty ("uploaded_tracks"));
        response.status = object->getProperty ("status").toString();

        if (statusCode >= 200 && statusCode < 300 && response.status == "uploaded")
        {
            response.succeeded = true;
            return response;
        }

        if (response.errorMessage.isEmpty())
            response.errorMessage = "Track upload failed";

        return response;
    }

    static SessionControlResponse parseFetchAnalysisSessionResponse (int statusCode, const juce::String& responseText)
    {
        SessionControlResponse response;
        response.type = SessionRequestType::fetchAnalysisSession;
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
        response.analysisSessionId = object->getProperty ("analysis_session_id").toString();
        response.projectSessionId = object->getProperty ("project_session_id").toString();
        response.status = object->getProperty ("status").toString();
        response.trackCount = static_cast<int> (object->getProperty ("track_count"));

        if (auto summaryValue = object->getProperty ("analysis_summary"); summaryValue.isObject())
        {
            if (auto* summaryObject = summaryValue.getDynamicObject())
            {
                juce::StringArray lines;
                lines.add ("Tracks: " + juce::String (static_cast<int> (summaryObject->getProperty ("track_count"))));
                lines.add ("Avg RMS: " + juce::String (static_cast<double> (summaryObject->getProperty ("average_rms_db")), 2) + " dB");
                lines.add ("Avg LUFS: " + juce::String (static_cast<double> (summaryObject->getProperty ("average_integrated_lufs")), 2));
                lines.add ("Avg Centroid: " + juce::String (static_cast<double> (summaryObject->getProperty ("average_spectral_centroid_mean")), 2));
                response.analysisSummary = lines.joinIntoString ("\n");
            }
        }

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
                        trackInfo.trackLengthSeconds = static_cast<double> (trackObject->getProperty ("duration_seconds"));
                        trackInfo.trackLengthDisplay = juce::String (static_cast<double> (trackObject->getProperty ("integrated_lufs")), 2);
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
            response.errorMessage = "Failed to fetch analysis session";

        return response;
    }

    static SessionControlResponse parseCreateProjectResponse (int statusCode, const juce::String& responseText)
    {
        SessionControlResponse response;
        response.type = SessionRequestType::createProject;
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
        response.projectSessionId = object->getProperty ("project_session_id").toString();
        response.chatSessionId = object->getProperty ("chat_session_id").toString();
        response.analysisSessionId = object->getProperty ("analysis_session_id").toString();
        response.explanation = object->getProperty ("initial_assistant_message").toString();

        if (statusCode >= 200 && statusCode < 300 && response.projectSessionId.isNotEmpty())
        {
            response.succeeded = true;
            return response;
        }

        response.errorMessage = "Failed to create project session";
        return response;
    }

    static SessionControlResponse parseUploadProjectTrackResponse (int statusCode, const juce::String& responseText)
    {
        SessionControlResponse response;
        response.type = SessionRequestType::uploadProjectTrack;
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
        response.status = object->getProperty ("analysis_status").toString();
        response.uploadedTracks = 1;

        if (statusCode >= 200 && statusCode < 300)
        {
            response.succeeded = true;
            return response;
        }

        response.errorMessage = "Project stem upload failed";
        return response;
    }

    static SessionControlResponse parseFetchProjectAnalysisResponse (int statusCode, const juce::String& responseText)
    {
        SessionControlResponse response;
        response.type = SessionRequestType::fetchProjectAnalysis;
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
        response.analysisSessionId = object->getProperty ("analysis_session_id").toString();
        response.projectSessionId = object->getProperty ("project_session_id").toString();
        response.status = object->getProperty ("status").toString();

        if (auto tracksValue = object->getProperty ("tracks"); tracksValue.isArray())
        {
            if (auto* tracksArray = tracksValue.getArray())
            {
                for (const auto& entry : *tracksArray)
                {
                    if (auto* trackObject = entry.getDynamicObject())
                    {
                        SessionTrackInfo trackInfo;
                        trackInfo.trackName = trackObject->getProperty ("filename").toString();

                        if (auto autoValue = trackObject->getProperty ("auto"); autoValue.isObject())
                        {
                            if (auto* autoObject = autoValue.getDynamicObject())
                            {
                                if (auto basicValue = autoObject->getProperty ("basic_wav"); basicValue.isObject())
                                {
                                    if (auto* basicObject = basicValue.getDynamicObject())
                                    {
                                        trackInfo.trackLengthSeconds = static_cast<double> (basicObject->getProperty ("duration_sec"));
                                        trackInfo.trackLengthDisplay = juce::String (static_cast<double> (basicObject->getProperty ("lufs")), 2);
                                        trackInfo.supportedParameterCount = static_cast<int> (basicObject->getProperty ("onset_count"));
                                    }
                                }
                            }
                        }

                        response.tracks.add (trackInfo);
                    }
                }
            }
        }

        response.trackCount = response.tracks.size();

        if (statusCode >= 200 && statusCode < 300)
        {
            response.succeeded = true;
            return response;
        }

        response.errorMessage = "Failed to fetch project analysis";
        return response;
    }

    static SessionControlResponse parseProjectChatResponse (int statusCode, const juce::String& responseText)
    {
        SessionControlResponse response;
        response.type = SessionRequestType::projectChat;
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
        response.explanation = object->getProperty ("assistant").toString();
        response.status = object->getProperty ("current_step").toString();
        if (auto toolsValue = object->getProperty ("tool_calls"); toolsValue.isArray())
        {
            if (auto* toolsArray = toolsValue.getArray())
            {
                juce::StringArray toolLines;
                for (const auto& entry : *toolsArray)
                {
                    if (auto* toolObject = entry.getDynamicObject())
                    {
                        auto name = toolObject->getProperty ("name").toString();
                        toolLines.add (name.isNotEmpty() ? name : "tool_call");
                    }
                }
                response.analysisSummary = toolLines.joinIntoString ("\n");
            }
        }

        if (statusCode >= 200 && statusCode < 300)
        {
            response.succeeded = true;
            return response;
        }

        response.errorMessage = "Project chat failed";
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
            case SessionRequestType::createAnalysisSession: return parseCreateAnalysisSessionResponse (statusCode, responseText);
            case SessionRequestType::uploadAnalysisTrack:   return parseUploadAnalysisTrackResponse (statusCode, responseText);
            case SessionRequestType::fetchAnalysisSession:  return parseFetchAnalysisSessionResponse (statusCode, responseText);
            case SessionRequestType::createProject:         return parseCreateProjectResponse (statusCode, responseText);
            case SessionRequestType::uploadProjectTrack:    return parseUploadProjectTrackResponse (statusCode, responseText);
            case SessionRequestType::fetchProjectAnalysis:  return parseFetchProjectAnalysisResponse (statusCode, responseText);
            case SessionRequestType::projectChat:           return parseProjectChatResponse (statusCode, responseText);
        }

        return {};
    }

    void enqueueRequest (SessionRequestType type, juce::String endpointUrl, juce::String requestBody)
    {
        enqueueRequest (PendingRequest { type, std::move (endpointUrl), std::move (requestBody), {}, {}, {} });
    }

    void enqueueRequest (PendingRequest request)
    {
        {
            const juce::ScopedLock scopedLock (lock);
            pendingRequest = std::move (request);
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
            juce::String headers ("Accept: application/json\r\n");

            juce::URL url (request.endpoint);
            juce::URL requestUrl = url;

            if (request.type == SessionRequestType::uploadAnalysisTrack
                || request.type == SessionRequestType::uploadProjectTrack)
            {
                for (int i = 0; i < request.formFields.size(); ++i)
                    requestUrl = requestUrl.withParameter (request.formFields.getAllKeys()[i], request.formFields.getAllValues()[i]);

                requestUrl = requestUrl.withFileToUpload (request.fileFieldName, request.file, "audio/wav");
            }
            else if (request.body.isNotEmpty())
            {
                headers = "Content-Type: application/json\r\nAccept: application/json\r\n";
                requestUrl = url.withPOSTData (request.body);
            }

            auto usePostData = request.body.isNotEmpty()
                            || request.type == SessionRequestType::uploadAnalysisTrack
                            || request.type == SessionRequestType::uploadProjectTrack;
            auto options = juce::URL::InputStreamOptions (usePostData
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
