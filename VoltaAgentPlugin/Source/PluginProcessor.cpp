#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto agentIdProperty = "agentId";
constexpr auto serverEndpointProperty = "serverEndpoint";
constexpr auto pollingEnabledProperty = "pollingEnabled";

juce::String toTrackTypeSlug (const juce::String& trackRoleText)
{
    return trackRoleText.toLowerCase().replaceCharacter (' ', '_');
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
    , trackAgent (apvts)
{
    if (! apvts.state.hasProperty (agentIdProperty))
        apvts.state.setProperty (agentIdProperty, "agent_01", nullptr);

    if (! apvts.state.hasProperty (serverEndpointProperty))
        apvts.state.setProperty (serverEndpointProperty, "http://127.0.0.1:5000/command", nullptr);

    if (! apvts.state.hasProperty (pollingEnabledProperty))
        apvts.state.setProperty (pollingEnabledProperty, true, nullptr);

    apvts.addParameterListener ("plugin_mode", this);

    localJsonClient.setCommandHandler ([this] (const volta::MixCommand& command)
    {
        enqueueIncomingCommand (command);
    });

    localJsonClient.setPollStatusHandler ([this] (bool succeeded, const juce::String& response)
    {
        if (succeeded)
        {
            lastSuccessfulPollTimeMs.store (juce::Time::currentTimeMillis());

            if (response.isNotEmpty())
            {
                const juce::ScopedLock scopedLock (statusLock);
                lastCommandText = response;
            }
        }
    });

    mixAnalyzeClient.setResultHandler ([this] (const volta::MixAnalyzeResult& result)
    {
        handleMixAnalyzeResult (result);
    });

    refreshPollingConfiguration();
    localJsonClient.startPolling();
}

VoltaAgentPluginAudioProcessor::~VoltaAgentPluginAudioProcessor()
{
    localJsonClient.stopPolling();
    mixAnalyzeClient.stop();
    apvts.removeParameterListener ("plugin_mode", this);
}

juce::AudioProcessorValueTreeState::ParameterLayout VoltaAgentPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        "plugin_mode",
        "Mode",
        juce::StringArray { "Agent", "Controller" },
        0));

    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        "track_role",
        "Track Type",
        volta::getTrackRoleNames(),
        0));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        "gain_db",
        "Volume",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f),
        0.0f));

    parameters.push_back (std::make_unique<juce::AudioParameterBool> ("low_cut_enabled", "Low Cut Enabled", false));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        "low_cut_freq_hz",
        "Low Cut Frequency",
        juce::NormalisableRange<float> (20.0f, 300.0f, 1.0f, 0.35f),
        80.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        "presence_db",
        "Presence",
        juce::NormalisableRange<float> (-6.0f, 6.0f, 0.1f),
        0.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        "compression_amount",
        "Compression",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        "warmth_amount",
        "Warmth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f));

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
    currentSampleRate.store (sampleRate);
    juce::ignoreUnused (samplesPerBlock);
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

    auto gainDb = apvts.getRawParameterValue ("gain_db")->load();
    auto linearGain = juce::Decibels::decibelsToGain (gainDb);

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        buffer.applyGain (channel, 0, numSamples, linearGain);
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

    refreshPollingConfiguration();
}

volta::AgentState VoltaAgentPluginAudioProcessor::getAgentState() const
{
    return trackAgent.captureState();
}

void VoltaAgentPluginAudioProcessor::setAgentId (const juce::String& newAgentId)
{
    apvts.state.setProperty (agentIdProperty, newAgentId.trim(), nullptr);
    refreshPollingConfiguration();
}

void VoltaAgentPluginAudioProcessor::setServerEndpoint (const juce::String& newServerEndpoint)
{
    apvts.state.setProperty (serverEndpointProperty, newServerEndpoint.trim(), nullptr);
    refreshPollingConfiguration();
}

void VoltaAgentPluginAudioProcessor::setPollingEnabled (bool shouldPoll)
{
    apvts.state.setProperty (pollingEnabledProperty, shouldPoll, nullptr);
    refreshPollingConfiguration();
}

void VoltaAgentPluginAudioProcessor::refreshPollingConfiguration()
{
    auto state = getAgentState();
    auto shouldPoll = state.mode == volta::PluginMode::agent && state.pollingEnabled;
    localJsonClient.configure (state.serverEndpoint, state.agentId, shouldPoll);
}

juce::String VoltaAgentPluginAudioProcessor::getTrackRoleText() const
{
    return volta::getTrackRoleName (getAgentState().trackRoleIndex);
}

juce::String VoltaAgentPluginAudioProcessor::getConnectionStatusText() const
{
    auto state = getAgentState();

    if (state.mode == volta::PluginMode::controller)
        return "Controller Mode";

    if (! state.pollingEnabled)
        return "Polling Disabled";

    auto nowMs = juce::Time::currentTimeMillis();
    auto lastSeenMs = lastSuccessfulPollTimeMs.load();
    return (lastSeenMs > 0 && nowMs - lastSeenMs < 2500) ? "Connected" : "Offline";
}

juce::String VoltaAgentPluginAudioProcessor::getLastCommandText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return lastCommandText;
}

juce::String VoltaAgentPluginAudioProcessor::getLastAppliedText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return lastAppliedText;
}

juce::String VoltaAgentPluginAudioProcessor::getConnectedAgentsSummary() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return connectedAgentsSummary;
}

juce::String VoltaAgentPluginAudioProcessor::getAnalyzeStatusText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return analyzeStatusText;
}

juce::String VoltaAgentPluginAudioProcessor::getAnalyzeSummaryText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return analyzeSummaryText;
}

juce::String VoltaAgentPluginAudioProcessor::getAnalyzeSuggestionsText() const
{
    const juce::ScopedLock scopedLock (statusLock);
    return analyzeSuggestionsText;
}

bool VoltaAgentPluginAudioProcessor::isAnalyzeRequestInFlight() const
{
    return analyzeRequestInFlight.load();
}

void VoltaAgentPluginAudioProcessor::requestMixAnalysis()
{
    auto state = getAgentState();
    auto analyzeEndpoint = buildAnalyzeEndpointFromServerEndpoint (state.serverEndpoint);
    auto requestBody = buildMixAnalyzeRequestBody();

    {
        const juce::ScopedLock scopedLock (statusLock);
        analyzeRequestInFlight.store (true);
        analyzeStatusText = "Analyzing...";
        analyzeSummaryText = "Waiting for server response...";
        analyzeSuggestionsText = "Request sent to " + analyzeEndpoint;
    }

    mixAnalyzeClient.requestAnalysis (analyzeEndpoint, requestBody);
}

void VoltaAgentPluginAudioProcessor::submitControllerCommand (const juce::String& promptText)
{
    const juce::ScopedLock scopedLock (statusLock);
    lastCommandText = promptText.isNotEmpty() ? promptText : "Waiting for command...";
    lastAppliedText = "Controller UI only. Command dispatch is not wired yet.";
}

juce::String VoltaAgentPluginAudioProcessor::buildAnalyzeEndpointFromServerEndpoint (const juce::String& serverEndpoint)
{
    auto trimmed = serverEndpoint.trim();

    if (trimmed.isEmpty())
        return "http://127.0.0.1:5000/api/mix/analyze";

    if (trimmed.containsIgnoreCase ("/api/mix/analyze"))
        return trimmed;

    auto endpointWithoutQuery = trimmed.upToFirstOccurrenceOf ("?", false, false);

    for (auto suffix : { "/command", "/parse-juce-intent", "/parse-intent" })
    {
        if (endpointWithoutQuery.endsWithIgnoreCase (suffix))
        {
            endpointWithoutQuery = endpointWithoutQuery.dropLastCharacters (juce::String (suffix).length());
            break;
        }
    }

    if (endpointWithoutQuery.endsWithChar ('/'))
        endpointWithoutQuery = endpointWithoutQuery.dropLastCharacters (1);

    return endpointWithoutQuery + "/api/mix/analyze";
}

juce::String VoltaAgentPluginAudioProcessor::buildMixAnalyzeRequestBody() const
{
    auto state = getAgentState();
    auto requestObject = std::make_unique<juce::DynamicObject>();
    requestObject->setProperty ("trackType", toTrackTypeSlug (getTrackRoleText()));
    requestObject->setProperty ("sampleRate", currentSampleRate.load());
    requestObject->setProperty ("bpm", 0);

    auto featuresObject = std::make_unique<juce::DynamicObject>();
    featuresObject->setProperty ("rms", -18.2);
    featuresObject->setProperty ("peak", -6.1);
    featuresObject->setProperty ("spectralCentroid", 1200);
    requestObject->setProperty ("features", juce::var (featuresObject.release()));

    auto currentParamsObject = std::make_unique<juce::DynamicObject>();
    currentParamsObject->setProperty ("gain_db", state.gainDb);
    currentParamsObject->setProperty ("low_cut_freq_hz", state.lowCutFrequencyHz);
    currentParamsObject->setProperty ("presence_db", state.presenceDb);
    currentParamsObject->setProperty ("compression_amount", state.compressionAmount);
    currentParamsObject->setProperty ("warmth_amount", state.warmthAmount);
    requestObject->setProperty ("currentParams", juce::var (currentParamsObject.release()));

    return juce::JSON::toString (juce::var (requestObject.release()));
}

void VoltaAgentPluginAudioProcessor::handleMixAnalyzeResult (const volta::MixAnalyzeResult& result)
{
    juce::String suggestionsText;

    if (result.suggestions.isEmpty())
    {
        suggestionsText = result.succeeded ? "No recognized suggestions returned." : "No suggestions available.";
    }
    else
    {
        juce::StringArray lines;

        for (const auto& suggestion : result.suggestions)
        {
            auto line = suggestion.parameter
                        + " | "
                        + suggestion.action
                        + " | "
                        + juce::String (suggestion.value, 2);

            if (suggestion.unit.isNotEmpty())
                line << " " << suggestion.unit;

            if (suggestion.reason.isNotEmpty())
                line << " | " << suggestion.reason;

            lines.add (line);
        }

        suggestionsText = lines.joinIntoString ("\n");
    }

    {
        const juce::ScopedLock scopedLock (statusLock);
        analyzeRequestInFlight.store (false);

        if (result.succeeded)
        {
            analyzeStatusText = "Analysis complete";
            analyzeSummaryText = result.summary.isNotEmpty() ? result.summary : "Analysis succeeded without summary text.";
            analyzeSuggestionsText = suggestionsText;
        }
        else
        {
            analyzeStatusText = result.statusCode > 0
                                    ? "Analysis failed (" + juce::String (result.statusCode) + ")"
                                    : "Analysis failed";
            analyzeSummaryText = result.errorMessage.isNotEmpty() ? result.errorMessage : "Unknown analysis error.";
            analyzeSuggestionsText = result.rawResponse.isNotEmpty() ? result.rawResponse : suggestionsText;
        }
    }
}

void VoltaAgentPluginAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (newValue);

    if (parameterID == "plugin_mode")
        triggerAsyncUpdate();
}

void VoltaAgentPluginAudioProcessor::handleAsyncUpdate()
{
    std::optional<volta::MixCommand> commandToApply;

    {
        const juce::ScopedLock scopedLock (incomingCommandLock);
        commandToApply = pendingCommand;
        pendingCommand.reset();
    }

    if (commandToApply.has_value())
        applyIncomingCommand (*commandToApply);

    refreshPollingConfiguration();
}

void VoltaAgentPluginAudioProcessor::enqueueIncomingCommand (const volta::MixCommand& command)
{
    {
        const juce::ScopedLock scopedLock (incomingCommandLock);
        pendingCommand = command;
    }

    triggerAsyncUpdate();
}

void VoltaAgentPluginAudioProcessor::applyIncomingCommand (const volta::MixCommand& command)
{
    auto state = getAgentState();

    if (state.mode != volta::PluginMode::agent)
        return;

    if (command.targetAgent != state.agentId)
        return;

    trackAgent.applyCommand (command);

    const juce::ScopedLock scopedLock (statusLock);
    lastAppliedText = command.parameter + " -> " + juce::String (command.value, 2);
    connectedAgentsSummary = state.agentId + " / " + getTrackRoleText() + " / " + getConnectionStatusText();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoltaAgentPluginAudioProcessor();
}
