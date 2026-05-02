#pragma once

#include <JuceHeader.h>
#include "Agent/TrackAgent.h"
#include "Communication/LocalJsonClient.h"
#include "Communication/MixAnalyzeClient.h"
#include "Core/TrackRole.h"

class VoltaAgentPluginAudioProcessor  : public juce::AudioProcessor,
                                        private juce::AudioProcessorValueTreeState::Listener,
                                        private juce::AsyncUpdater
{
public:
    VoltaAgentPluginAudioProcessor();
    ~VoltaAgentPluginAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    volta::AgentState getAgentState() const;
    void setAgentId (const juce::String& newAgentId);
    void setServerEndpoint (const juce::String& newServerEndpoint);
    void setPollingEnabled (bool shouldPoll);
    void refreshPollingConfiguration();

    juce::String getTrackRoleText() const;
    juce::String getConnectionStatusText() const;
    juce::String getLastCommandText() const;
    juce::String getLastAppliedText() const;
    juce::String getConnectedAgentsSummary() const;
    juce::String getAnalyzeStatusText() const;
    juce::String getAnalyzeSummaryText() const;
    juce::String getAnalyzeSuggestionsText() const;
    bool isAnalyzeRequestInFlight() const;
    void requestMixAnalysis();
    void submitControllerCommand (const juce::String& promptText);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static juce::String buildAnalyzeEndpointFromServerEndpoint (const juce::String& serverEndpoint);
    juce::String buildMixAnalyzeRequestBody() const;
    void handleMixAnalyzeResult (const volta::MixAnalyzeResult& result);
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void enqueueIncomingCommand (const volta::MixCommand& command);
    void applyIncomingCommand (const volta::MixCommand& command);

    volta::TrackAgent trackAgent;
    volta::LocalJsonClient localJsonClient;
    volta::MixAnalyzeClient mixAnalyzeClient;
    juce::CriticalSection incomingCommandLock;
    juce::CriticalSection statusLock;
    std::optional<volta::MixCommand> pendingCommand;
    std::atomic<int64_t> lastSuccessfulPollTimeMs { 0 };
    std::atomic<double> currentSampleRate { 44100.0 };
    std::atomic<bool> analyzeRequestInFlight { false };
    juce::String lastCommandText { "Waiting for command..." };
    juce::String lastAppliedText { "No changes yet" };
    juce::String analyzeStatusText { "Idle" };
    juce::String analyzeSummaryText { "No analysis yet." };
    juce::String analyzeSuggestionsText { "No suggestions yet." };
    juce::String connectedAgentsSummary { "No agents discovered yet.\nInsert Agent mode instances on track channels to populate this list." };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoltaAgentPluginAudioProcessor)
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
