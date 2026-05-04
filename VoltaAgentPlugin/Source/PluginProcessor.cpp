#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto serverEndpointProperty = "serverEndpoint";
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

    sessionControlClient.setResultHandler ([this] (const volta::SessionControlResponse& response)
    {
        handleSessionControlResponse (response);
    });

    requestServerHealth();
}

VoltaAgentPluginAudioProcessor::~VoltaAgentPluginAudioProcessor()
{
    sessionControlClient.stop();
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

juce::String VoltaAgentPluginAudioProcessor::getActivityLogText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return activityLogText;
}

bool VoltaAgentPluginAudioProcessor::canApplyPlan() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return planState == PlanState::planReady && ! lastPlanOperations.isEmpty();
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
        explanationText = "Planning changes...";
        plannedChangesText = "Waiting for plan response...";
        endpoint = buildSessionEndpoint (getServerEndpointText(), "/plan-actions");
        appendActivityLog ("plan-actions request start | POST " + endpoint + " | prompt=" + promptText);
    }

    requestInFlight.store (true);
    sessionControlClient.requestPlanActions (endpoint, promptText);
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

void VoltaAgentPluginAudioProcessor::appendActivityLog (const juce::String& line)
{
    auto timestamp = juce::Time::getCurrentTime().formatted ("%H:%M:%S");

    if (activityLogText.startsWith ("Connecting to server..."))
        activityLogText.clear();

    activityLogText = "[" + timestamp + "] " + line + (activityLogText.isEmpty() ? "" : "\n" + activityLogText);
}

void VoltaAgentPluginAudioProcessor::handleSessionControlResponse (const volta::SessionControlResponse& response)
{
    bool shouldRefreshSession = false;
    bool shouldRefreshHealth = false;

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
        }
    }

    if (shouldRefreshSession)
    {
        refreshSession();
        return;
    }

    if (shouldRefreshHealth)
        requestServerHealth();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoltaAgentPluginAudioProcessor();
}
