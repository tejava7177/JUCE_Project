#pragma once

#include <JuceHeader.h>
#include "Communication/SessionControlClient.h"

class VoltaAgentPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    enum class ServerState
    {
        offline,
        connectedNoSession,
        connectedSessionLoaded
    };

    enum class PlanState
    {
        idle,
        refreshingSession,
        planning,
        planReady,
        stagingApply,
        applyStaged,
        error
    };

    enum class AnalysisState
    {
        idle,
        readyToUpload,
        creatingSession,
        uploading,
        fetchingResults,
        completed,
        error
    };

    enum class ProjectAction
    {
        none,
        createAndUploadStems,
        createAndSendChat
    };

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

    void setServerEndpoint (const juce::String& newServerEndpoint);
    juce::String getServerEndpointText() const;
    juce::String getServerStatusText() const;
    juce::String getSessionStatusText() const;
    juce::String getPromptText() const;
    juce::String getExplanationText() const;
    juce::String getTrackListText() const;
    juce::String getPlannedChangesText() const;
    juce::String getActivityLogText() const;
    juce::String getStemFolderText() const;
    juce::String getAnalysisStatusText() const;
    juce::String getProjectSessionText() const;
    bool canApplyPlan() const;
    bool isRequestInFlight() const;
    bool canStartAnalysisUpload() const;
    void setCurrentPrompt (const juce::String& promptText);
    void setStemFolder (const juce::File& folder);
    void requestServerHealth();
    void refreshSession();
    void planActions();
    void applyPlannedActions();
    void startAnalysisUpload();

private:
    struct SessionSummaryState
    {
        bool sessionLoaded = false;
        int trackCount = 0;
        juce::Array<volta::SessionTrackInfo> tracks;
    };

    struct AnalysisUploadState
    {
        AnalysisState state = AnalysisState::idle;
        juce::File stemFolder;
        juce::Array<juce::File> pendingFiles;
        juce::String analysisSessionId;
        juce::String projectSessionId;
        juce::String chatSessionId;
        juce::String analysisSummaryText { "No analysis results yet." };
        int uploadedCount = 0;
        int totalFiles = 0;
    };

    struct ProjectSessionState
    {
        juce::String projectSessionId;
        juce::String chatSessionId;
        juce::String analysisSessionId;
        juce::String currentStep;
        juce::String genre;
        juce::String pendingChatMessage;
        ProjectAction pendingAction = ProjectAction::none;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static juce::String buildServerBaseUrl (const juce::String& serverEndpoint);
    static juce::String buildSessionEndpoint (const juce::String& serverEndpoint, const juce::String& pathSuffix);
    static juce::String formatTrackListText (const SessionSummaryState& summaryState);
    static juce::String formatOperationsText (const juce::Array<volta::SessionOperation>& operations);
    static juce::String formatAnalysisTrackListText (const juce::Array<volta::SessionTrackInfo>& analysisTracks);
    static juce::String sanitizeResponseForLog (const juce::String& responseText);
    static juce::String formatBool (bool value);
    static juce::String getServerStateLabel (ServerState state, int pendingApplyCount);
    static juce::String getAnalysisStateLabel (const AnalysisUploadState& analysisState);
    void appendActivityLog (const juce::String& line);
    void uploadNextAnalysisFile();
    void handleSessionControlResponse (const volta::SessionControlResponse& response);

    volta::SessionControlClient sessionControlClient;
    juce::CriticalSection statusLock;
    std::atomic<bool> requestInFlight { false };
    ServerState serverState { ServerState::offline };
    PlanState planState { PlanState::idle };
    int pendingApplyCount = 0;
    int lastApplyRevision = 0;
    juce::String currentPrompt;
    juce::String explanationText { "Connecting to server..." };
    juce::String trackListText { "No session loaded." };
    juce::String plannedChangesText { "No planned changes yet." };
    juce::String activityLogText { "Connecting to server..." };
    SessionSummaryState sessionSummary;
    AnalysisUploadState analysisUploadState;
    ProjectSessionState projectSessionState;
    juce::Array<volta::SessionOperation> lastPlanOperations;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoltaAgentPluginAudioProcessor)
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
