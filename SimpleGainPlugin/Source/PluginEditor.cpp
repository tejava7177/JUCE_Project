/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto aiEndpoint = "http://127.0.0.1:5000/parse-juce-intent";
constexpr auto gainParameterId = "gain_db";
constexpr auto aiRequestTimeoutMs = 20000;

void logAiDebug (const juce::String& message)
{
    juce::Logger::writeToLog ("[SimpleGainPlugin AI] " + message);
}

class SessionSerializer
{
public:
    static juce::String createRequestBody (SimpleGainPluginAudioProcessor& processor, const juce::String& prompt)
    {
        auto requestObject = std::make_unique<juce::DynamicObject>();
        requestObject->setProperty ("prompt", prompt);
        requestObject->setProperty ("session", createSession (processor));
        return juce::JSON::toString (juce::var (requestObject.release()));
    }

private:
    static juce::var createSession (SimpleGainPluginAudioProcessor& processor)
    {
        auto sessionObject = std::make_unique<juce::DynamicObject>();
        sessionObject->setProperty ("host", juce::String ("Ableton Live"));
        sessionObject->setProperty ("plugin", juce::String ("SimpleGainPlugin"));
        sessionObject->setProperty ("parameters", createParameters (processor));
        sessionObject->setProperty ("sensor", createSensor (processor));
        return juce::var (sessionObject.release());
    }

    static juce::Array<juce::var> createParameters (SimpleGainPluginAudioProcessor& processor)
    {
        juce::Array<juce::var> parameters;

        auto* gainValue = processor.apvts.getRawParameterValue (gainParameterId);
        if (gainValue == nullptr)
            return parameters;

        auto parameterObject = std::make_unique<juce::DynamicObject>();
        parameterObject->setProperty ("id", juce::String (gainParameterId));
        parameterObject->setProperty ("name", juce::String ("Gain"));
        parameterObject->setProperty ("value", gainValue->load());
        parameterObject->setProperty ("min", -24.0f);
        parameterObject->setProperty ("max", 24.0f);
        parameterObject->setProperty ("unit", juce::String ("dB"));
        parameters.add (juce::var (parameterObject.release()));

        return parameters;
    }

    static juce::var createSensor (SimpleGainPluginAudioProcessor& processor)
    {
        auto sensorObject = std::make_unique<juce::DynamicObject>();
        sensorObject->setProperty ("rms_db", processor.getRmsDb());
        sensorObject->setProperty ("peak_db", processor.getPeakDb());
        sensorObject->setProperty ("crest_factor_db", processor.getCrestFactorDb());
        sensorObject->setProperty ("clip_count", processor.getClipCount());
        sensorObject->setProperty ("silence_ratio", processor.getSilenceRatio());
        return juce::var (sensorObject.release());
    }
};

class OperationApplier
{
public:
    struct Result
    {
        bool applied { false };
        bool matchedGainOperation { false };
        juce::String statusMessage;
        juce::String explanation;
        juce::String failureReason;
    };

    static Result applyGainOperation (SimpleGainPluginAudioProcessor& processor, const juce::var& response)
    {
        Result result;

        if (auto* responseObject = response.getDynamicObject())
            result.explanation = responseObject->getProperty ("explanation").toString();

        auto operationsVar = response.getProperty ("operations", juce::var());
        if (! operationsVar.isArray())
        {
            result.statusMessage = "No valid operations returned";
            result.failureReason = "Response did not contain an operations array";
            logAiDebug ("Operation apply failed: operations field missing or not an array");
            return result;
        }

        logAiDebug ("Parsed operations count: " + juce::String (operationsVar.getArray()->size()));

        auto* gainParameter = processor.apvts.getParameter (gainParameterId);
        if (gainParameter == nullptr)
        {
            result.statusMessage = "Failed to apply gain_db";
            result.failureReason = "gain_db parameter not found";
            logAiDebug ("Operation apply failed: gain_db parameter not found in APVTS");
            return result;
        }

        for (const auto& operation : *operationsVar.getArray())
        {
            auto parameterId = operation.getProperty ("parameter_id", juce::var()).toString();
            logAiDebug ("Inspecting operation parameter_id=" + parameterId);
            if (parameterId != gainParameterId)
                continue;

            result.matchedGainOperation = true;

            auto minValue = static_cast<float> (operation.getProperty ("min", -24.0f));
            auto maxValue = static_cast<float> (operation.getProperty ("max", 24.0f));

            juce::var targetValueVar = operation.getProperty ("value", juce::var());
            auto usedFieldName = juce::String ("value");
            if (targetValueVar.isVoid())
            {
                targetValueVar = operation.getProperty ("new_value", juce::var());
                usedFieldName = "new_value";
            }

            logAiDebug ("Matched gain_db operation. min=" + juce::String (minValue, 2)
                        + " max=" + juce::String (maxValue, 2)
                        + " field=" + usedFieldName
                        + " rawValue=" + targetValueVar.toString());

            if (! (targetValueVar.isDouble() || targetValueVar.isInt() || targetValueVar.isInt64() || targetValueVar.isBool()))
            {
                result.statusMessage = "Failed to apply gain_db";
                result.failureReason = "Matched gain_db operation did not include value or new_value";
                logAiDebug ("Operation apply failed: gain_db operation missing numeric target value");
                return result;
            }

            auto rawTargetValue = static_cast<float> (targetValueVar);
            auto targetValue = juce::jlimit (minValue, maxValue, rawTargetValue);
            logAiDebug ("Applying gain_db. rawValue=" + juce::String (rawTargetValue, 3)
                        + " clampedValue=" + juce::String (targetValue, 3));

            gainParameter->beginChangeGesture();
            gainParameter->setValueNotifyingHost (gainParameter->convertTo0to1 (targetValue));
            gainParameter->endChangeGesture();

            result.applied = true;
            result.statusMessage = "Applied gain_db";
            logAiDebug ("Applied gain_db successfully: " + juce::String (targetValue, 3) + " dB");
            return result;
        }

        result.statusMessage = "No valid operations returned";
        result.failureReason = "No operation with parameter_id == gain_db";
        logAiDebug ("Operation apply failed: no gain_db operation matched");
        return result;
    }
};

class AiClient
{
public:
    enum class Stage
    {
        SendingRequest,
        WaitingForResponse,
        ReceivedHttpResponse,
        ParsingResponse,
        Completed
    };

    struct Response
    {
        bool ok { false };
        bool timedOut { false };
        int statusCode { 0 };
        Stage stage { Stage::SendingRequest };
        juce::String errorMessage;
        juce::String responseBody;
        juce::var json;
    };

    using StageCallback = std::function<void(Stage)>;

    static Response sendPrompt (const juce::String& requestBody, StageCallback stageCallback)
    {
        Response response;
        auto responseStatusCode = 0;
        auto startTime = juce::Time::getCurrentTime();

        response.stage = Stage::SendingRequest;
        if (stageCallback != nullptr)
            stageCallback (Stage::SendingRequest);

        logAiDebug ("Request started at " + startTime.toString (true, true));
        logAiDebug ("POST " + juce::String (aiEndpoint));
        logAiDebug ("Request body: " + requestBody);

        auto url = juce::URL (aiEndpoint).withPOSTData (requestBody);
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                           .withExtraHeaders ("Content-Type: application/json\r\n")
                           .withConnectionTimeoutMs (aiRequestTimeoutMs)
                           .withStatusCode (&responseStatusCode);

        response.stage = Stage::WaitingForResponse;
        if (stageCallback != nullptr)
            stageCallback (Stage::WaitingForResponse);
        logAiDebug ("Waiting for AI response with timeout " + juce::String (aiRequestTimeoutMs) + " ms");

        auto stream = url.createInputStream (options);
        if (stream == nullptr)
        {
            auto endTime = juce::Time::getCurrentTime();
            auto elapsedMs = endTime.toMilliseconds() - startTime.toMilliseconds();
            response.timedOut = elapsedMs >= aiRequestTimeoutMs;
            response.errorMessage = response.timedOut ? "Request timed out" : "Failed to open HTTP stream";
            logAiDebug ("Request failed before receiving response. elapsedMs=" + juce::String (elapsedMs)
                        + " reason=" + response.errorMessage);
            return response;
        }

        response.stage = Stage::ReceivedHttpResponse;
        response.statusCode = responseStatusCode;
        if (stageCallback != nullptr)
            stageCallback (Stage::ReceivedHttpResponse);

        auto responseText = stream->readEntireStreamAsString();
        response.responseBody = responseText;

        auto endTime = juce::Time::getCurrentTime();
        auto elapsedMs = endTime.toMilliseconds() - startTime.toMilliseconds();

        logAiDebug ("Received HTTP response. statusCode=" + juce::String (responseStatusCode)
                    + " elapsedMs=" + juce::String (elapsedMs));
        logAiDebug ("Raw response body: " + responseText);

        if (responseStatusCode < 200 || responseStatusCode >= 300)
        {
            response.errorMessage = "HTTP error: " + juce::String (responseStatusCode);
            if (responseText.isNotEmpty())
                logAiDebug ("HTTP error body: " + responseText);
            return response;
        }

        response.stage = Stage::ParsingResponse;
        if (stageCallback != nullptr)
            stageCallback (Stage::ParsingResponse);
        logAiDebug ("Parsing AI response JSON");

        auto parsed = juce::JSON::parse (responseText);
        if (parsed.isVoid())
        {
            response.errorMessage = "Failed to parse JSON response";
            logAiDebug ("JSON parse failed");
            return response;
        }

        response.ok = true;
        response.stage = Stage::Completed;
        response.json = parsed;
        logAiDebug ("JSON parse succeeded");
        return response;
    }
};

juce::String stageToStatusText (AiClient::Stage stage)
{
    switch (stage)
    {
        case AiClient::Stage::SendingRequest:      return "Sending request to Flask server...";
        case AiClient::Stage::WaitingForResponse:  return "Waiting for AI response...";
        case AiClient::Stage::ReceivedHttpResponse:return "Received HTTP response";
        case AiClient::Stage::ParsingResponse:     return "Parsing AI response...";
        case AiClient::Stage::Completed:           return {};
    }

    return {};
}
} // namespace

void SensorBar::setDecibelValue (float newValueDb)
{
    value = juce::jlimit (-60.0f, 0.0f, newValueDb);
    mode = Mode::Decibel;
    repaint();
}

void SensorBar::setRatioValue (float newRatio)
{
    value = juce::jlimit (0.0f, 1.0f, newRatio);
    mode = Mode::Ratio;
    repaint();
}

void SensorBar::setMode (Mode newMode)
{
    mode = newMode;
    repaint();
}

void SensorBar::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colours::darkgrey);
    g.fillRoundedRectangle (bounds, 3.0f);

    auto normalisedValue = mode == Mode::Decibel
        ? juce::jmap (value, -60.0f, 0.0f, 0.0f, 1.0f)
        : value;

    auto fillBounds = bounds.reduced (2.0f);
    fillBounds.setWidth (fillBounds.getWidth() * normalisedValue);

    if (mode == Mode::Ratio)
    {
        g.setColour (juce::Colours::cornflowerblue);
    }
    else if (value >= -1.0f)
    {
        g.setColour (juce::Colours::red);
    }
    else if (value >= -6.0f)
    {
        g.setColour (juce::Colours::orange);
    }
    else
    {
        g.setColour (juce::Colours::green);
    }

    g.fillRoundedRectangle (fillBounds, 2.0f);
}

//==============================================================================
SimpleGainPluginAudioProcessorEditor::SimpleGainPluginAudioProcessorEditor (SimpleGainPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    titleLabel.setText ("Track Sensor", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

    statusLabel.setText ("Healthy", juce::dontSendNotification);
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);

    for (auto* label : { &rmsLabel, &peakLabel, &crestFactorLabel, &clipCountLabel, &silenceRatioLabel })
    {
        label->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*label);
    }

    rmsBar.setMode (SensorBar::Mode::Decibel);
    peakBar.setMode (SensorBar::Mode::Decibel);
    silenceRatioBar.setMode (SensorBar::Mode::Ratio);
    addAndMakeVisible (rmsBar);
    addAndMakeVisible (peakBar);
    addAndMakeVisible (silenceRatioBar);

    gainLabel.setText ("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (gainLabel);

    gainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    gainSlider.setTextValueSuffix (" dB");
    addAndMakeVisible (gainSlider);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts,
        "gain_db",
        gainSlider);

    commandEditor.setTextToShowWhenEmpty ("UP or DOWN", juce::Colours::grey);
    commandEditor.setJustification (juce::Justification::centredLeft);
    addAndMakeVisible (commandEditor);

    applyButton.setButtonText ("Apply");
    applyButton.onClick = [this] { handleApplyCommand(); };
    addAndMakeVisible (applyButton);

    commandStatusLabel.setText ("Command: ready", juce::dontSendNotification);
    commandStatusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (commandStatusLabel);

    promptLabel.setText ("AI Prompt", juce::dontSendNotification);
    promptLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (promptLabel);

    promptEditor.setTextToShowWhenEmpty ("make it louder", juce::Colours::grey);
    promptEditor.setJustification (juce::Justification::centredLeft);
    addAndMakeVisible (promptEditor);

    sendButton.setButtonText ("Send");
    sendButton.onClick = [this] { handleSendPrompt(); };
    addAndMakeVisible (sendButton);

    promptStatusLabel.setText ("AI Status: idle", juce::dontSendNotification);
    promptStatusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (promptStatusLabel);

    promptExplanationLabel.setText ("Explanation: -", juce::dontSendNotification);
    promptExplanationLabel.setJustificationType (juce::Justification::topLeft);
    promptExplanationLabel.setMinimumHorizontalScale (0.8f);
    addAndMakeVisible (promptExplanationLabel);

    setSize (420, 620);
    startTimerHz (20);
}

SimpleGainPluginAudioProcessorEditor::~SimpleGainPluginAudioProcessorEditor()
{
}

//==============================================================================
void SimpleGainPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
}

void SimpleGainPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (20);

    titleLabel.setBounds (bounds.removeFromTop (32));
    statusLabel.setBounds (bounds.removeFromTop (30));
    bounds.removeFromTop (8);

    rmsLabel.setBounds (bounds.removeFromTop (24));
    rmsBar.setBounds (bounds.removeFromTop (16));
    bounds.removeFromTop (8);

    peakLabel.setBounds (bounds.removeFromTop (24));
    peakBar.setBounds (bounds.removeFromTop (16));
    bounds.removeFromTop (8);

    crestFactorLabel.setBounds (bounds.removeFromTop (28));
    clipCountLabel.setBounds (bounds.removeFromTop (28));
    silenceRatioLabel.setBounds (bounds.removeFromTop (28));
    silenceRatioBar.setBounds (bounds.removeFromTop (16));
    bounds.removeFromTop (12);

    gainLabel.setBounds (bounds.removeFromTop (24));
    gainSlider.setBounds (bounds.removeFromTop (100).withSizeKeepingCentre (110, 100));
    bounds.removeFromTop (12);

    auto commandRow = bounds.removeFromTop (28);
    commandEditor.setBounds (commandRow.removeFromLeft (260));
    commandRow.removeFromLeft (8);
    applyButton.setBounds (commandRow);
    commandStatusLabel.setBounds (bounds.removeFromTop (28));

    bounds.removeFromTop (12);
    promptLabel.setBounds (bounds.removeFromTop (24));

    auto promptRow = bounds.removeFromTop (28);
    promptEditor.setBounds (promptRow.removeFromLeft (260));
    promptRow.removeFromLeft (8);
    sendButton.setBounds (promptRow);

    bounds.removeFromTop (8);
    promptStatusLabel.setBounds (bounds.removeFromTop (24));
    promptExplanationLabel.setBounds (bounds.removeFromTop (60));
}

void SimpleGainPluginAudioProcessorEditor::timerCallback()
{
    auto rms = audioProcessor.getRmsDb();
    auto peak = audioProcessor.getPeakDb();
    auto crestFactor = audioProcessor.getCrestFactorDb();
    auto clips = audioProcessor.getClipCount();
    auto silence = audioProcessor.getSilenceRatio();

    rmsLabel.setText ("RMS Level: "
                          + juce::String (rms, 1)
                          + " dB",
                      juce::dontSendNotification);

    peakLabel.setText ("Peak Level: "
                           + juce::String (peak, 1)
                           + " dB",
                       juce::dontSendNotification);

    crestFactorLabel.setText ("Crest Factor: "
                                  + juce::String (crestFactor, 1)
                                  + " dB",
                              juce::dontSendNotification);

    clipCountLabel.setText ("Clip Count: "
                                + juce::String (clips),
                            juce::dontSendNotification);
    clipCountLabel.setColour (juce::Label::textColourId, clips > 0 ? juce::Colours::red : juce::Colours::white);

    silenceRatioLabel.setText ("Silence Ratio: "
                                   + juce::String (silence * 100.0f, 1)
                                   + " %",
                               juce::dontSendNotification);

    rmsBar.setDecibelValue (rms);
    peakBar.setDecibelValue (peak);
    silenceRatioBar.setRatioValue (silence);

    juce::String status = "Healthy";

    if (clips > 0)
        status = "Clipping";
    else if (peak > -1.0f)
        status = "Hot";
    else if (silence > 0.7f)
        status = "Mostly Silent";
    else if (crestFactor > 10.0f)
        status = "Dynamic";

    statusLabel.setText ("Status: " + status, juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, status == "Clipping" ? juce::Colours::red : juce::Colours::white);
}

void SimpleGainPluginAudioProcessorEditor::handleApplyCommand()
{
    auto command = commandEditor.getText().trim().toUpperCase();

    auto* gainParameter = audioProcessor.apvts.getParameter ("gain_db");
    auto* gainValue = audioProcessor.apvts.getRawParameterValue ("gain_db");

    if (gainParameter == nullptr || gainValue == nullptr)
    {
        commandStatusLabel.setText ("Command: gain parameter not found", juce::dontSendNotification);
        return;
    }

    auto currentGainDb = gainValue->load();
    auto newGainDb = currentGainDb;

    if (command == "UP")
    {
        newGainDb = currentGainDb + 1.0f;
    }
    else if (command == "DOWN")
    {
        newGainDb = currentGainDb - 1.0f;
    }
    else
    {
        commandStatusLabel.setText ("Unknown command. Use UP or DOWN", juce::dontSendNotification);
        return;
    }

    newGainDb = juce::jlimit (-24.0f, 24.0f, newGainDb);

    gainParameter->beginChangeGesture();
    gainParameter->setValueNotifyingHost (gainParameter->convertTo0to1 (newGainDb));
    gainParameter->endChangeGesture();

    commandStatusLabel.setText ("Command: " + command + " -> Gain "
                                    + juce::String (newGainDb, 1)
                                    + " dB",
                                juce::dontSendNotification);
}

void SimpleGainPluginAudioProcessorEditor::handleSendPrompt()
{
    if (promptRequestInFlight.exchange (true))
    {
        setPromptStatus ("AI Status: request already in flight", juce::Colours::yellow);
        return;
    }

    auto prompt = promptEditor.getText().trim();
    if (prompt.isEmpty())
    {
        promptRequestInFlight.store (false);
        setPromptStatus ("AI Status: enter a prompt first", juce::Colours::yellow);
        return;
    }

    setPromptStatus ("AI Status: sending...", juce::Colours::white);
    setPromptExplanation ("Explanation: waiting for server response");
    sendButton.setEnabled (false);

    auto requestBody = SessionSerializer::createRequestBody (audioProcessor, prompt);
    auto editorSafePointer = juce::Component::SafePointer<SimpleGainPluginAudioProcessorEditor> (this);

    std::thread ([editorSafePointer, requestBody]
    {
        auto response = AiClient::sendPrompt (requestBody, [editorSafePointer] (AiClient::Stage stage)
        {
            juce::MessageManager::callAsync ([editorSafePointer, stage]
            {
                if (editorSafePointer == nullptr)
                    return;

                auto statusText = stageToStatusText (stage);
                if (statusText.isNotEmpty())
                    editorSafePointer->setPromptStatus ("AI Status: " + statusText, juce::Colours::white);
            });
        });

        juce::MessageManager::callAsync ([editorSafePointer, response]
        {
            if (editorSafePointer == nullptr)
                return;

            editorSafePointer->promptRequestInFlight.store (false);
            editorSafePointer->sendButton.setEnabled (true);

            if (! response.ok)
            {
                editorSafePointer->setPromptStatus ("AI Status: " + response.errorMessage, juce::Colours::red);
                if (response.responseBody.isNotEmpty())
                    editorSafePointer->setPromptExplanation ("Explanation: " + response.responseBody);
                return;
            }

            auto result = OperationApplier::applyGainOperation (editorSafePointer->audioProcessor, response.json);

            if (result.explanation.isNotEmpty())
                editorSafePointer->setPromptExplanation ("Explanation: " + result.explanation);

            if (! result.applied && result.failureReason.isNotEmpty() && result.explanation.isEmpty())
                editorSafePointer->setPromptExplanation ("Explanation: " + result.failureReason);

            editorSafePointer->setPromptStatus ("AI Status: " + result.statusMessage,
                                                result.applied ? juce::Colours::lightgreen : juce::Colours::yellow);
        });
    }).detach();
}

void SimpleGainPluginAudioProcessorEditor::setPromptStatus (const juce::String& newStatus, juce::Colour colour)
{
    promptStatusLabel.setText (newStatus, juce::dontSendNotification);
    promptStatusLabel.setColour (juce::Label::textColourId, colour);
}

void SimpleGainPluginAudioProcessorEditor::setPromptExplanation (const juce::String& newExplanation)
{
    promptExplanationLabel.setText (newExplanation, juce::dontSendNotification);
}
