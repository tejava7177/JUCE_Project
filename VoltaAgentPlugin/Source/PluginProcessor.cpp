#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto serverEndpointProperty = "serverEndpoint";
constexpr auto stemFolderProperty = "stemFolder";

juce::String initialProjectBriefPrompt()
{
    return juce::String::fromUTF8 (u8"프로젝트에 대해 간단히 설명해주세요. 장르, 보컬 유무, 주요 악기, 원하는 작업이 있으면 같이 알려주세요.");
}
}

VoltaAgentPluginAudioProcessor::VoltaAgentPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
                      #if ! JucePlugin_IsMidiEffect
                       #if ! JucePlugin_IsSynth
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                       #endif
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                      #endif
                        )
#endif
    , apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    if (! apvts.state.hasProperty (serverEndpointProperty))
        apvts.state.setProperty (serverEndpointProperty, "http://127.0.0.1:5000", nullptr);

    if (apvts.state.hasProperty (stemFolderProperty))
        analysisUploadState.stemFolder = juce::File (apvts.state.getProperty (stemFolderProperty).toString());

    sessionControlClient.setResultHandler ([this] (const volta::SessionControlResponse& response)
    {
        handleSessionControlResponse (response);
    });

    sessionScanClient.setResultHandler ([this] (const volta::SessionControlResponse& response)
    {
        handleSessionScanResponse (response);
    });

    requestServerHealth();
}

VoltaAgentPluginAudioProcessor::~VoltaAgentPluginAudioProcessor()
{
    sessionControlClient.stop();
    sessionScanClient.stop();
}

juce::AudioProcessorValueTreeState::ParameterLayout VoltaAgentPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    return { parameters.begin(), parameters.end() };
}

const juce::String VoltaAgentPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VoltaAgentPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VoltaAgentPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VoltaAgentPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VoltaAgentPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VoltaAgentPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int VoltaAgentPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VoltaAgentPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String VoltaAgentPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void VoltaAgentPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void VoltaAgentPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void VoltaAgentPluginAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VoltaAgentPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
   #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
   #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
   #endif
}
#endif

void VoltaAgentPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);
}

bool VoltaAgentPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* VoltaAgentPluginAudioProcessor::createEditor()
{
    return new VoltaAgentPluginAudioProcessorEditor (*this);
}

void VoltaAgentPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VoltaAgentPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xmlState);

        if (state.isValid())
            apvts.replaceState (state);
    }
}

void VoltaAgentPluginAudioProcessor::setServerEndpoint (const juce::String& newServerEndpoint)
{
    apvts.state.setProperty (serverEndpointProperty, buildServerBaseUrl (newServerEndpoint), nullptr);
}

juce::String VoltaAgentPluginAudioProcessor::getServerEndpointText() const
{
    return apvts.state.getProperty (serverEndpointProperty, "http://127.0.0.1:5000").toString();
}

juce::String VoltaAgentPluginAudioProcessor::getServerStatusText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return getServerStateLabel (serverState, pendingApplyCount);
}

juce::String VoltaAgentPluginAudioProcessor::getSessionStatusText() const
{
    const juce::ScopedLock scopedLock (statusLock);

    if (planState == PlanState::refreshingSession)
        return "Refreshing session...";

    if (! sessionSummary.sessionLoaded)
        return "No session loaded";

    return "Session: " + juce::String (sessionSummary.trackCount) + " tracks loaded";
}

juce::String VoltaAgentPluginAudioProcessor::getPromptText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return currentPrompt;
}

juce::String VoltaAgentPluginAudioProcessor::getExplanationText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return explanationText;
}

juce::String VoltaAgentPluginAudioProcessor::getTrackListText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return trackListText;
}

juce::String VoltaAgentPluginAudioProcessor::getPlannedChangesText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return plannedChangesText;
}

juce::String VoltaAgentPluginAudioProcessor::getProjectOverviewText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return projectOverviewText;
}

juce::String VoltaAgentPluginAudioProcessor::getActivityLogText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return activityLogText;
}

juce::String VoltaAgentPluginAudioProcessor::getStemFolderText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return analysisUploadState.stemFolder.getFullPathName();
}

juce::String VoltaAgentPluginAudioProcessor::getAnalysisStatusText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return getAnalysisStateLabel (analysisUploadState);
}

juce::String VoltaAgentPluginAudioProcessor::getProjectSessionText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    if (projectSessionState.projectSessionId.isEmpty())
        return "Not created";

    juce::StringArray fields;
    fields.add ("project=" + projectSessionState.projectSessionId);

    if (projectSessionState.chatSessionId.isNotEmpty())
        fields.add ("chat=" + projectSessionState.chatSessionId);

    if (projectSessionState.analysisSessionId.isNotEmpty())
        fields.add ("analysis=" + projectSessionState.analysisSessionId);

    return fields.joinIntoString (" | ");
}

juce::String VoltaAgentPluginAudioProcessor::getLastSubmittedPromptText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return lastSubmittedPrompt;
}

bool VoltaAgentPluginAudioProcessor::isNamingApprovalPending() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return projectSessionState.namingApprovalPending;
}

int VoltaAgentPluginAudioProcessor::getPendingNamingApprovalCount() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return projectSessionState.pendingNamingApprovalCount;
}

bool VoltaAgentPluginAudioProcessor::isEqCleanupApprovalPending() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return projectSessionState.eqCleanupApprovalPending;
}

int VoltaAgentPluginAudioProcessor::getPendingEqCleanupApprovalCount() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return projectSessionState.pendingEqCleanupApprovalCount;
}

bool VoltaAgentPluginAudioProcessor::canApplyPlan() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return planState == PlanState::planReady && ! lastPlanOperations.isEmpty();
}

bool VoltaAgentPluginAudioProcessor::canStartAnalysisUpload() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return analysisUploadState.stemFolder.isDirectory() && ! requestInFlight.load();
}

bool VoltaAgentPluginAudioProcessor::isRequestInFlight() const
{
    return requestInFlight.load();
}

void VoltaAgentPluginAudioProcessor::setCurrentPrompt (const juce::String& promptText)
{
    const juce::ScopedLock scopedLock (statusLock);
    currentPrompt = promptText;
}

void VoltaAgentPluginAudioProcessor::setStemFolder (const juce::File& folder)
{
    const juce::ScopedLock scopedLock (statusLock);
    analysisUploadState.stemFolder = folder;
    analysisUploadState.state = folder.isDirectory() ? AnalysisState::readyToUpload : AnalysisState::idle;
    apvts.state.setProperty (stemFolderProperty, folder.getFullPathName(), nullptr);
}

void VoltaAgentPluginAudioProcessor::requestServerHealth()
{
    auto endpoint = buildSessionEndpoint (getServerEndpointText(), "/health");

    {
        const juce::ScopedLock scopedLock (statusLock);
        explanationText = "Connecting to server...";
        appendActivityLog ("health request start | GET " + endpoint);
    }

    requestInFlight.store (true);
    sessionControlClient.requestHealth (endpoint);
}

void VoltaAgentPluginAudioProcessor::refreshSession()
{
    auto endpoint = buildSessionEndpoint (getServerEndpointText(), "/session-summary");

    {
        const juce::ScopedLock scopedLock (statusLock);
        planState = PlanState::refreshingSession;
        explanationText = "Refreshing session...";
        appendActivityLog ("session-summary request start | GET " + endpoint);
    }

    requestInFlight.store (true);
    sessionControlClient.requestSessionSummary (endpoint);
}

void VoltaAgentPluginAudioProcessor::planActions()
{
    juce::String promptText;
    juce::String endpoint;
    bool shouldCreateProject = false;

    {
        const juce::ScopedLock scopedLock (statusLock);
        promptText = currentPrompt.trim();

        if (promptText.isEmpty())
        {
            planState = PlanState::error;
            explanationText = "Planning failed";
            appendActivityLog ("plan-actions request blocked | error=empty prompt");
            return;
        }

        planState = PlanState::planning;
        lastSubmittedPrompt = promptText;
        currentPrompt.clear();
        explanationText = "Planning changes...";
        plannedChangesText = "Waiting for plan response...";

        if (projectSessionState.projectSessionId.isEmpty())
        {
            projectSessionState.pendingAction = ProjectAction::createAndSendChat;
            projectSessionState.pendingChatMessage = promptText;
            endpoint = buildSessionEndpoint (getServerEndpointText(), "/projects");
            appendActivityLog ("project session request start | POST " + endpoint + " | reason=chat");
            shouldCreateProject = true;
        }
        else
        {
            endpoint = buildSessionEndpoint (getServerEndpointText(), "/projects/" + projectSessionState.projectSessionId + "/chat");
            appendActivityLog ("project chat request start | POST " + endpoint + " | prompt=" + promptText);
        }
    }

    requestInFlight.store (true);
    if (shouldCreateProject)
        sessionControlClient.requestCreateProject (endpoint);
    else
        sessionControlClient.requestProjectChat (endpoint, promptText);
}

void VoltaAgentPluginAudioProcessor::applyPlannedActions()
{
    juce::Array<volta::SessionOperation> operationsToApply;
    juce::String endpoint;

    {
        const juce::ScopedLock scopedLock (statusLock);

        if (lastPlanOperations.isEmpty())
        {
            planState = PlanState::error;
            explanationText = "Apply staging failed";
            appendActivityLog ("apply-actions request blocked | error=no planned operations");
            return;
        }

        planState = PlanState::stagingApply;
        explanationText = "Staging apply...";
        operationsToApply = lastPlanOperations;
        endpoint = buildSessionEndpoint (getServerEndpointText(), "/apply-actions");
        appendActivityLog ("apply-actions request start | POST " + endpoint
                           + " | operations=" + juce::String (operationsToApply.size()));
    }

    requestInFlight.store (true);
    sessionControlClient.requestApplyActions (endpoint, operationsToApply);
}

void VoltaAgentPluginAudioProcessor::startAnalysisUpload()
{
    juce::File stemFolder;
    juce::String endpoint;

    {
        const juce::ScopedLock scopedLock (statusLock);
        stemFolder = analysisUploadState.stemFolder;
    }

    if (! stemFolder.isDirectory())
    {
        const juce::ScopedLock scopedLock (statusLock);
        analysisUploadState.state = AnalysisState::error;
        explanationText = "Select a stem folder first";
        appendActivityLog ("analysis upload blocked | error=no stem folder");
        return;
    }

    juce::Array<juce::File> discoveredFiles;
    auto fileIterator = juce::RangedDirectoryIterator (stemFolder, false, "*.wav", juce::File::findFiles);
    for (const auto& entry : fileIterator)
        discoveredFiles.add (entry.getFile());

    auto upperIterator = juce::RangedDirectoryIterator (stemFolder, false, "*.WAV", juce::File::findFiles);
    for (const auto& entry : upperIterator)
        discoveredFiles.addIfNotAlreadyThere (entry.getFile());

    if (discoveredFiles.isEmpty())
    {
        const juce::ScopedLock scopedLock (statusLock);
        analysisUploadState.state = AnalysisState::error;
        explanationText = "No WAV stems found";
        plannedChangesText = "Export WAV stems into the selected folder, then try again.";
        appendActivityLog ("analysis upload blocked | error=no wav stems | folder=" + stemFolder.getFullPathName());
        return;
    }

    endpoint = buildSessionEndpoint (getServerEndpointText(), "/projects");

    requestSessionScan();

    {
        const juce::ScopedLock scopedLock (statusLock);
        analysisUploadState.pendingFiles = discoveredFiles;
        analysisUploadState.totalFiles = discoveredFiles.size();
        analysisUploadState.uploadedCount = 0;
        analysisUploadState.analysisSessionId.clear();
        analysisUploadState.projectSessionId.clear();
        analysisUploadState.chatSessionId.clear();
        analysisUploadState.analysisSummaryText = "Preparing analysis session...";
        analysisUploadState.state = AnalysisState::creatingSession;
        projectSessionState.projectSessionId.clear();
        projectSessionState.chatSessionId.clear();
        projectSessionState.analysisSessionId.clear();
        projectSessionState.namingApprovalPending = false;
        projectSessionState.pendingNamingApprovalCount = 0;
        projectSessionState.eqCleanupApprovalPending = false;
        projectSessionState.pendingEqCleanupApprovalCount = 0;
        projectSessionState.pendingAction = ProjectAction::createAndUploadStems;
        projectSessionState.pendingChatMessage.clear();
        lastSubmittedPrompt.clear();
        projectOverviewText.clear();
        explanationText = "Creating analysis session...";
        plannedChangesText = "Preparing to upload " + juce::String (discoveredFiles.size()) + " WAV stem(s).";
        appendActivityLog ("project session request start | POST " + endpoint
                           + " | project=" + stemFolder.getFileName()
                           + " | wav_files=" + juce::String (discoveredFiles.size()));
    }

    requestInFlight.store (true);
    sessionControlClient.requestCreateProject (endpoint);
}

void VoltaAgentPluginAudioProcessor::requestSessionScan()
{
    auto endpoint = buildSessionEndpoint (getServerEndpointText(), "/request-session-scan");

    {
        const juce::ScopedLock scopedLock (statusLock);
        appendActivityLog ("session-scan request start | POST " + endpoint + " | source=juce_analyze_wav_stems");
    }

    sessionScanClient.requestSessionScan (endpoint, "juce_analyze_wav_stems", "analyze_wav_stems");
}

void VoltaAgentPluginAudioProcessor::requestProjectOverview()
{
    juce::String endpoint;
    juce::String message;

    {
        const juce::ScopedLock scopedLock (statusLock);
        if (projectSessionState.projectSessionId.isEmpty())
            return;

        endpoint = buildSessionEndpoint (getServerEndpointText(), "/projects/" + projectSessionState.projectSessionId + "/overview");
        message = lastSubmittedPrompt.isNotEmpty()
                    ? lastSubmittedPrompt
                    : juce::String::fromUTF8 (u8"이 프로젝트를 분석해서 장르, 분위기, 구성, 추천 믹싱 순서를 정리해줘");
        appendActivityLog ("project overview request start | POST " + endpoint);
        explanationText = "Generating project overview...";
    }

    requestInFlight.store (true);
    sessionControlClient.requestProjectOverview (endpoint, message);
}

void VoltaAgentPluginAudioProcessor::approvePendingNamingProposal()
{
    juce::String endpoint;

    {
        const juce::ScopedLock scopedLock (statusLock);

        if (projectSessionState.projectSessionId.isEmpty() || ! projectSessionState.namingApprovalPending)
            return;

        endpoint = buildSessionEndpoint (getServerEndpointText(), "/projects/" + projectSessionState.projectSessionId + "/chat");
        explanationText = "Submitting naming approval...";
        appendActivityLog ("project chat request start | POST " + endpoint + " | prompt=네이밍과 그룹핑 승인");
    }

    requestInFlight.store (true);
    sessionControlClient.requestProjectChat (endpoint, juce::String::fromUTF8 (u8"네이밍과 그룹핑 승인"));
}

void VoltaAgentPluginAudioProcessor::rejectPendingNamingProposal()
{
    const juce::ScopedLock scopedLock (statusLock);
    projectSessionState.namingApprovalPending = false;
    projectSessionState.pendingNamingApprovalCount = 0;
    explanationText = juce::String::fromUTF8 (u8"네이밍과 그루핑 제안을 보류했습니다. 다른 이름 규칙이나 수정 요청을 바로 입력할 수 있습니다.");
    appendActivityLog ("naming approval dismissed locally");
}

void VoltaAgentPluginAudioProcessor::approvePendingEqCleanupProposal()
{
    juce::String endpoint;

    {
        const juce::ScopedLock scopedLock (statusLock);

        if (projectSessionState.projectSessionId.isEmpty() || ! projectSessionState.eqCleanupApprovalPending)
            return;

        endpoint = buildSessionEndpoint (getServerEndpointText(), "/projects/" + projectSessionState.projectSessionId + "/chat");
        explanationText = "Submitting EQ cleanup approval...";
        appendActivityLog ("project chat request start | POST " + endpoint + " | prompt=로우컷 하이컷 승인");
    }

    requestInFlight.store (true);
    sessionControlClient.requestProjectChat (endpoint, juce::String::fromUTF8 (u8"로우컷 하이컷 승인"));
}

void VoltaAgentPluginAudioProcessor::rejectPendingEqCleanupProposal()
{
    const juce::ScopedLock scopedLock (statusLock);
    projectSessionState.eqCleanupApprovalPending = false;
    projectSessionState.pendingEqCleanupApprovalCount = 0;
    explanationText = juce::String::fromUTF8 (u8"로우컷과 하이컷 제안을 보류했습니다. 다른 EQ 규칙이나 수정 요청을 바로 입력할 수 있습니다.");
    appendActivityLog ("eq cleanup approval dismissed locally");
}

juce::String VoltaAgentPluginAudioProcessor::buildServerBaseUrl (const juce::String& serverEndpoint)
{
    auto trimmed = serverEndpoint.trim();

    if (trimmed.isEmpty())
        return "http://127.0.0.1:5000";

    auto base = trimmed.upToFirstOccurrenceOf ("?", false, false);

    if (base.startsWithIgnoreCase ("http://") || base.startsWithIgnoreCase ("https://"))
    {
        auto schemeSeparator = base.indexOf ("://");
        auto pathStart = base.indexOfChar (schemeSeparator + 3, '/');

        if (pathStart > 0)
            base = base.substring (0, pathStart);
    }

    if (base.endsWithChar ('/'))
        base = base.dropLastCharacters (1);

    return base;
}

juce::String VoltaAgentPluginAudioProcessor::buildSessionEndpoint (const juce::String& serverEndpoint, const juce::String& pathSuffix)
{
    return buildServerBaseUrl (serverEndpoint) + pathSuffix;
}

juce::String VoltaAgentPluginAudioProcessor::formatTrackListText (const SessionSummaryState& summaryState)
{
    if (! summaryState.sessionLoaded)
        return "No session loaded.";

    if (summaryState.tracks.isEmpty())
        return "Session loaded, but no tracks were returned.";

    juce::StringArray lines;

    for (const auto& track : summaryState.tracks)
    {
        auto line = "- " + track.trackName;

        if (track.trackLengthDisplay.isNotEmpty())
            line += " | length " + track.trackLengthDisplay;

        line += " | devices " + juce::String (track.deviceCount)
             + " | supported params " + juce::String (track.supportedParameterCount);

        lines.add (line);
    }

    return lines.joinIntoString ("\n");
}

juce::String VoltaAgentPluginAudioProcessor::formatOperationsText (const juce::Array<volta::SessionOperation>& operations)
{
    if (operations.isEmpty())
        return "No planned changes yet.";

    juce::StringArray lines;

    for (const auto& operation : operations)
    {
        lines.add (operation.track + " | "
                  + operation.device + " | "
                  + operation.parameter + " | "
                  + juce::String (operation.oldValue, 2) + " -> "
                  + juce::String (operation.newValue, 2)
                  + " | path " + operation.path);
    }

    return lines.joinIntoString ("\n");
}

juce::String VoltaAgentPluginAudioProcessor::formatAnalysisTrackListText (const juce::Array<volta::SessionTrackInfo>& analysisTracks)
{
    if (analysisTracks.isEmpty())
        return "No analysis results yet.";

    juce::StringArray lines;

    for (const auto& track : analysisTracks)
    {
        lines.add (track.trackName
                   + " | duration " + juce::String (track.trackLengthSeconds, 2) + " s"
                   + " | LUFS " + track.trackLengthDisplay);
    }

    return lines.joinIntoString ("\n");
}

juce::String VoltaAgentPluginAudioProcessor::sanitizeResponseForLog (const juce::String& responseText)
{
    auto flattened = responseText.replaceCharacters ("\r\n", "  ").trim();

    if (flattened.length() > 240)
        flattened = flattened.substring (0, 240) + "...";

    return flattened.isEmpty() ? "<empty>" : flattened;
}

juce::String VoltaAgentPluginAudioProcessor::formatBool (bool value)
{
    return value ? "true" : "false";
}

juce::String VoltaAgentPluginAudioProcessor::getServerStateLabel (ServerState state, int pendingCount)
{
    switch (state)
    {
        case ServerState::offline:
            return "Server offline";
        case ServerState::connectedNoSession:
            return "Server connected | No session loaded | Pending actions: " + juce::String (pendingCount);
        case ServerState::connectedSessionLoaded:
            return "Server connected | Pending actions: " + juce::String (pendingCount);
    }

    return "Server offline";
}

juce::String VoltaAgentPluginAudioProcessor::getAnalysisStateLabel (const AnalysisUploadState& analysisState)
{
    switch (analysisState.state)
    {
        case AnalysisState::idle:
            return "No stem folder selected";
        case AnalysisState::readyToUpload:
            return "Ready to upload stems";
        case AnalysisState::creatingSession:
            return "Creating analysis session";
        case AnalysisState::uploading:
            return "Uploading stems (" + juce::String (analysisState.uploadedCount) + "/" + juce::String (analysisState.totalFiles) + ")";
        case AnalysisState::fetchingResults:
            return "Fetching analysis results";
        case AnalysisState::completed:
            return "Analysis completed";
        case AnalysisState::error:
            return "Analysis failed";
    }

    return "No stem folder selected";
}

void VoltaAgentPluginAudioProcessor::appendActivityLog (const juce::String& line)
{
    auto timestamp = juce::Time::getCurrentTime().formatted ("%H:%M:%S");

    if (activityLogText.startsWith ("Connecting to server..."))
        activityLogText.clear();

    activityLogText = "[" + timestamp + "] " + line + (activityLogText.isEmpty() ? "" : "\n" + activityLogText);
}

void VoltaAgentPluginAudioProcessor::uploadNextAnalysisFile()
{
    juce::File nextFile;
    juce::String projectSessionId;
    int trackIndex = 0;

    {
        const juce::ScopedLock scopedLock (statusLock);

        if (analysisUploadState.pendingFiles.isEmpty())
        {
            analysisUploadState.state = AnalysisState::fetchingResults;
            explanationText = "Fetching analysis results...";
            auto endpoint = buildSessionEndpoint (getServerEndpointText(), "/projects/" + projectSessionState.projectSessionId + "/analysis");
            appendActivityLog ("project analysis request start | GET " + endpoint);
            requestInFlight.store (true);
            sessionControlClient.requestProjectAnalysis (endpoint);
            return;
        }

        nextFile = analysisUploadState.pendingFiles.removeAndReturn (0);
        trackIndex = analysisUploadState.uploadedCount;
        projectSessionId = projectSessionState.projectSessionId;
        analysisUploadState.state = AnalysisState::uploading;
        explanationText = "Uploading stem " + nextFile.getFileName() + "...";
        appendActivityLog ("analysis track upload start | file=" + nextFile.getFileName()
                           + " | remaining=" + juce::String (analysisUploadState.pendingFiles.size()));
    }

    juce::ignoreUnused (trackIndex);
    auto endpoint = buildSessionEndpoint (getServerEndpointText(), "/projects/" + projectSessionId + "/upload");
    requestInFlight.store (true);
    sessionControlClient.requestUploadProjectTrack (endpoint, nextFile);
}

void VoltaAgentPluginAudioProcessor::handleSessionControlResponse (const volta::SessionControlResponse& response)
{
    bool shouldRefreshSession = false;
    bool shouldRefreshHealth = false;
    bool shouldContinueAnalysisUpload = false;

    {
        const juce::ScopedLock scopedLock (statusLock);
        requestInFlight.store (false);

        auto parseSuccess = formatBool (response.parseSucceeded);
        auto opsCount = juce::String (response.operations.size());
        auto rawResponse = sanitizeResponseForLog (response.rawResponse);

        switch (response.type)
        {
            case volta::SessionRequestType::health:
            {
                if (response.succeeded)
                {
                    pendingApplyCount = response.pendingApplyCount;
                    serverState = response.sessionLoaded ? ServerState::connectedSessionLoaded : ServerState::connectedNoSession;
                    explanationText = response.sessionLoaded ? "Server connected" : "No session loaded";
                    appendActivityLog ("health request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | pending=" + juce::String (pendingApplyCount)
                                       + " | raw=" + rawResponse);
                    shouldRefreshSession = response.sessionLoaded && sessionSummary.tracks.isEmpty();
                }
                else
                {
                    pendingApplyCount = 0;
                    serverState = ServerState::offline;
                    explanationText = "Server offline";
                    appendActivityLog (juce::String ("health request end | Server offline")
                                       + " | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | error=" + (response.errorMessage.isNotEmpty() ? response.errorMessage : "connection failed")
                                       + " | raw=" + rawResponse);
                }

                break;
            }

            case volta::SessionRequestType::sessionSummary:
            {
                if (response.succeeded)
                {
                    sessionSummary.sessionLoaded = response.sessionLoaded;
                    sessionSummary.trackCount = response.trackCount;
                    sessionSummary.tracks = response.tracks;
                    trackListText = formatTrackListText (sessionSummary);
                    serverState = sessionSummary.sessionLoaded ? ServerState::connectedSessionLoaded : ServerState::connectedNoSession;
                    planState = PlanState::idle;

                    if (sessionSummary.sessionLoaded)
                    {
                        explanationText = "Server connected";
                        appendActivityLog ("session-summary request end | http " + juce::String (response.statusCode)
                                           + " | parse=" + parseSuccess
                                           + " | tracks=" + juce::String (sessionSummary.trackCount)
                                           + " | raw=" + rawResponse);
                    }
                    else
                    {
                        explanationText = "No session loaded";
                        appendActivityLog ("session-summary request end | http " + juce::String (response.statusCode)
                                           + " | parse=" + parseSuccess
                                           + " | raw=" + rawResponse);
                    }
                }
                else
                {
                    planState = PlanState::error;
                    explanationText = response.errorMessage.isNotEmpty() ? response.errorMessage : "Invalid response from server";
                    appendActivityLog ("session-summary request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | error=" + (response.errorMessage.isNotEmpty() ? response.errorMessage : "Invalid response from server")
                                       + " | raw=" + rawResponse);
                }

                break;
            }

            case volta::SessionRequestType::planActions:
            {
                if (response.succeeded)
                {
                    lastPlanOperations = response.operations;
                    explanationText = response.explanation.isNotEmpty() ? response.explanation : "Plan ready";
                    plannedChangesText = formatOperationsText (lastPlanOperations);
                    planState = PlanState::planReady;
                    appendActivityLog ("plan-actions request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | operations=" + opsCount
                                       + " | raw=" + rawResponse);
                }
                else
                {
                    lastPlanOperations.clear();
                    plannedChangesText = "No planned changes available.";
                    explanationText = response.errorMessage.isNotEmpty() ? response.errorMessage : "Planning failed";
                    planState = PlanState::error;
                    appendActivityLog ("plan-actions request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | error=" + (response.errorMessage.isNotEmpty() ? response.errorMessage : "Planning failed")
                                       + " | operations=" + opsCount
                                       + " | raw=" + rawResponse);
                }

                break;
            }

            case volta::SessionRequestType::applyActions:
            {
                if (response.succeeded)
                {
                    lastApplyRevision = response.revision;
                    planState = PlanState::applyStaged;
                    explanationText = "Apply staged";
                    appendActivityLog ("apply-actions request end | Apply staged (revision " + juce::String (lastApplyRevision) + ")"
                                       + " | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | operations=" + opsCount
                                       + " | staged=" + juce::String (response.staged)
                                       + " | raw=" + rawResponse);
                    pendingApplyCount = juce::jmax (pendingApplyCount, response.staged);
                    shouldRefreshHealth = true;
                }
                else
                {
                    planState = PlanState::error;
                    explanationText = response.errorMessage.isNotEmpty() ? response.errorMessage : "Apply staging failed";
                    appendActivityLog ("apply-actions request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | error=" + (response.errorMessage.isNotEmpty() ? response.errorMessage : "Apply staging failed")
                                       + " | operations=" + juce::String (lastPlanOperations.size())
                                       + " | raw=" + rawResponse);
                }

                break;
            }

            case volta::SessionRequestType::createAnalysisSession:
            {
                if (response.succeeded)
                {
                    analysisUploadState.analysisSessionId = response.analysisSessionId;
                    analysisUploadState.projectSessionId = response.projectSessionId;
                    analysisUploadState.chatSessionId = response.chatSessionId;
                    analysisUploadState.state = AnalysisState::uploading;
                    explanationText = "Analysis session created";
                    appendActivityLog ("analysis session request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | analysis_session_id=" + response.analysisSessionId
                                       + " | raw=" + rawResponse);
                    shouldContinueAnalysisUpload = true;
                }
                else
                {
                    analysisUploadState.state = AnalysisState::error;
                    explanationText = response.errorMessage.isNotEmpty() ? response.errorMessage : "Failed to create analysis session";
                    appendActivityLog ("analysis session request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | error=" + explanationText
                                       + " | raw=" + rawResponse);
                }

                break;
            }

            case volta::SessionRequestType::uploadAnalysisTrack:
            {
                if (response.succeeded)
                {
                    ++analysisUploadState.uploadedCount;
                    explanationText = "Uploaded " + juce::String (analysisUploadState.uploadedCount)
                                      + " / " + juce::String (analysisUploadState.totalFiles) + " stem(s)";
                    appendActivityLog ("analysis track upload end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | uploaded=" + juce::String (analysisUploadState.uploadedCount)
                                       + " | raw=" + rawResponse);
                    shouldContinueAnalysisUpload = true;
                }
                else
                {
                    analysisUploadState.state = AnalysisState::error;
                    explanationText = response.errorMessage.isNotEmpty() ? response.errorMessage : "Track upload failed";
                    appendActivityLog ("analysis track upload end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | error=" + explanationText
                                       + " | raw=" + rawResponse);
                }

                break;
            }

            case volta::SessionRequestType::fetchAnalysisSession:
            {
                if (response.succeeded)
                {
                    analysisUploadState.state = AnalysisState::completed;
                    analysisUploadState.analysisSummaryText = response.analysisSummary;
                    explanationText = initialProjectBriefPrompt();
                    plannedChangesText = response.analysisSummary + "\n\n" + formatAnalysisTrackListText (response.tracks);
                    appendActivityLog ("analysis result request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | tracks=" + juce::String (response.trackCount)
                                       + " | raw=" + rawResponse);
                }
                else
                {
                    analysisUploadState.state = AnalysisState::error;
                    explanationText = response.errorMessage.isNotEmpty() ? response.errorMessage : "Failed to fetch analysis results";
                    appendActivityLog ("analysis result request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | error=" + explanationText
                                       + " | raw=" + rawResponse);
                }

                break;
            }

            case volta::SessionRequestType::createProject:
            {
                if (response.succeeded)
                {
                    projectSessionState.projectSessionId = response.projectSessionId;
                    projectSessionState.chatSessionId = response.chatSessionId;
                    projectSessionState.analysisSessionId = response.analysisSessionId;
                    explanationText = response.explanation.isNotEmpty() ? response.explanation : "Project session created";
                    appendActivityLog ("project session request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | project_session_id=" + response.projectSessionId
                                       + " | raw=" + rawResponse);

                    if (projectSessionState.pendingAction == ProjectAction::createAndUploadStems)
                        shouldContinueAnalysisUpload = true;
                    else if (projectSessionState.pendingAction == ProjectAction::createAndSendChat)
                    {
                        auto endpoint = buildSessionEndpoint (getServerEndpointText(), "/projects/" + projectSessionState.projectSessionId + "/chat");
                        auto pendingMessage = projectSessionState.pendingChatMessage;
                        projectSessionState.pendingAction = ProjectAction::none;
                        requestInFlight.store (true);
                        appendActivityLog ("project chat request start | POST " + endpoint + " | prompt=" + pendingMessage);
                        sessionControlClient.requestProjectChat (endpoint, pendingMessage);
                        return;
                    }
                }
                else
                {
                    analysisUploadState.state = AnalysisState::error;
                    planState = PlanState::error;
                    explanationText = response.errorMessage.isNotEmpty() ? response.errorMessage : "Failed to create project session";
                    appendActivityLog ("project session request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | error=" + explanationText
                                       + " | raw=" + rawResponse);
                }

                projectSessionState.pendingAction = ProjectAction::none;
                break;
            }

            case volta::SessionRequestType::uploadProjectTrack:
            {
                if (response.succeeded)
                {
                    ++analysisUploadState.uploadedCount;
                    explanationText = "Uploaded " + juce::String (analysisUploadState.uploadedCount)
                                      + " / " + juce::String (analysisUploadState.totalFiles) + " stem(s)";
                    appendActivityLog ("project stem upload end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | uploaded=" + juce::String (analysisUploadState.uploadedCount)
                                       + " | raw=" + rawResponse);
                    shouldContinueAnalysisUpload = true;
                }
                else
                {
                    analysisUploadState.state = AnalysisState::error;
                    explanationText = response.errorMessage.isNotEmpty() ? response.errorMessage : "Project stem upload failed";
                    appendActivityLog ("project stem upload end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | error=" + explanationText
                                       + " | raw=" + rawResponse);
                }

                break;
            }

            case volta::SessionRequestType::fetchProjectAnalysis:
            {
                if (response.succeeded)
                {
                    analysisUploadState.state = AnalysisState::completed;
                    analysisUploadState.analysisSummaryText = "Project analysis complete";
                    explanationText = "Generating project overview...";
                    plannedChangesText = formatAnalysisTrackListText (response.tracks);
                    projectOverviewText.clear();
                    appendActivityLog ("project analysis request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | tracks=" + juce::String (response.trackCount)
                                       + " | raw=" + rawResponse);
                    requestProjectOverview();
                    return;
                }
                else
                {
                    analysisUploadState.state = AnalysisState::error;
                    explanationText = response.errorMessage.isNotEmpty() ? response.errorMessage : "Failed to fetch project analysis";
                    appendActivityLog ("project analysis request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | error=" + explanationText
                                       + " | raw=" + rawResponse);
                }

                break;
            }

            case volta::SessionRequestType::projectChat:
            {
                if (response.succeeded)
                {
                    lastPlanOperations = response.operations;
                    explanationText = response.explanation.isNotEmpty() ? response.explanation : "Reply ready";

                    if (response.namingApplyStatus == "awaiting_approval")
                    {
                        projectSessionState.namingApprovalPending = true;
                        projectSessionState.pendingNamingApprovalCount = response.namingApplyCount;
                    }
                    else if (response.namingApplyStatus == "staged")
                    {
                        projectSessionState.namingApprovalPending = false;
                        projectSessionState.pendingNamingApprovalCount = 0;
                    }

                    if (response.eqCleanupApplyStatus == "awaiting_approval")
                    {
                        projectSessionState.eqCleanupApprovalPending = true;
                        projectSessionState.pendingEqCleanupApprovalCount = response.eqCleanupApplyCount;
                    }
                    else if (response.eqCleanupApplyStatus == "approved_pending_executor")
                    {
                        projectSessionState.eqCleanupApprovalPending = false;
                        projectSessionState.pendingEqCleanupApprovalCount = 0;
                    }

                    if (! lastPlanOperations.isEmpty())
                    {
                        auto operationText = formatOperationsText (lastPlanOperations);
                        if (response.analysisSummary.isNotEmpty())
                            plannedChangesText = response.analysisSummary + "\n\n" + operationText;
                        else
                            plannedChangesText = operationText;
                        planState = PlanState::planReady;
                    }
                    else
                    {
                        if (response.analysisSummary.isNotEmpty())
                            plannedChangesText = response.analysisSummary;
                        else if (response.explanation.isNotEmpty())
                            plannedChangesText = response.explanation;
                        planState = PlanState::idle;
                    }
                    appendActivityLog ("project chat request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | operations=" + juce::String (lastPlanOperations.size())
                                       + " | naming_apply=" + response.namingApplyStatus
                                       + " | eq_cleanup_apply=" + response.eqCleanupApplyStatus
                                       + " | raw=" + rawResponse);
                }
                else
                {
                    lastPlanOperations.clear();
                    planState = PlanState::error;
                    explanationText = response.errorMessage.isNotEmpty() ? response.errorMessage : "Project chat failed";
                    appendActivityLog ("project chat request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | error=" + explanationText
                                       + " | raw=" + rawResponse);
                }

                break;
            }

            case volta::SessionRequestType::projectOverview:
            {
                if (response.succeeded)
                {
                    projectOverviewText = response.analysisSummary;
                    explanationText = response.explanation.isNotEmpty() ? response.explanation : initialProjectBriefPrompt();
                    appendActivityLog ("project overview request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | raw=" + rawResponse);
                }
                else
                {
                    explanationText = response.errorMessage.isNotEmpty() ? response.errorMessage : "Project overview failed";
                    appendActivityLog ("project overview request end | http " + juce::String (response.statusCode)
                                       + " | parse=" + parseSuccess
                                       + " | error=" + explanationText
                                       + " | raw=" + rawResponse);
                }

                break;
            }

            case volta::SessionRequestType::requestSessionScan:
            {
                break;
            }
        }
    }

    if (shouldContinueAnalysisUpload)
    {
        uploadNextAnalysisFile();
        return;
    }

    if (shouldRefreshSession)
    {
        refreshSession();
        return;
    }

    if (shouldRefreshHealth)
        requestServerHealth();
}

void VoltaAgentPluginAudioProcessor::handleSessionScanResponse (const volta::SessionControlResponse& response)
{
    const juce::ScopedLock scopedLock (statusLock);

    auto parseSuccess = formatBool (response.parseSucceeded);
    auto rawResponse = sanitizeResponseForLog (response.rawResponse);

    if (response.succeeded)
    {
        appendActivityLog ("session-scan request end | http " + juce::String (response.statusCode)
                           + " | parse=" + parseSuccess
                           + " | status=" + (response.status.isNotEmpty() ? response.status : "requested")
                           + " | raw=" + rawResponse);
        return;
    }

    appendActivityLog ("session-scan request end | http " + juce::String (response.statusCode)
                       + " | parse=" + parseSuccess
                       + " | error=" + (response.errorMessage.isNotEmpty() ? response.errorMessage : "request failed")
                       + " | raw=" + rawResponse);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoltaAgentPluginAudioProcessor();
}
