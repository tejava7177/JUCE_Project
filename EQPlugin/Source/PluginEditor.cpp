#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <thread>

namespace
{
constexpr auto aiEndpoint = "http://127.0.0.1:5000/parse-juce-intent";
constexpr auto aiRequestTimeoutMs = 20000;

void logAiDebug (const juce::String& message)
{
    juce::Logger::writeToLog ("[EQPlugin AI] " + message);
}

juce::String formatDecibels (float value)
{
    return juce::String (value, 1) + " dB";
}

juce::String formatPercent (float value)
{
    return juce::String (value * 100.0f, 1) + " %";
}

class SessionSerializer
{
public:
    static juce::String createRequestBody (EQPluginAudioProcessor& processor, const juce::String& prompt)
    {
        auto requestObject = std::make_unique<juce::DynamicObject>();
        requestObject->setProperty ("prompt", prompt);
        requestObject->setProperty ("session", createSession (processor));
        return juce::JSON::toString (juce::var (requestObject.release()));
    }

private:
    static juce::var createSession (EQPluginAudioProcessor& processor)
    {
        auto sessionObject = std::make_unique<juce::DynamicObject>();
        sessionObject->setProperty ("host", juce::String ("Ableton Live"));
        sessionObject->setProperty ("plugin", juce::String ("EQPlugin"));
        sessionObject->setProperty ("parameters", createParameters (processor));
        sessionObject->setProperty ("meters", createMeters (processor));
        return juce::var (sessionObject.release());
    }

    static juce::Array<juce::var> createParameters (EQPluginAudioProcessor& processor)
    {
        struct ParameterSpec
        {
            const char* id;
            const char* name;
            const char* unit;
            float minValue;
            float maxValue;
        };

        const ParameterSpec specs[] =
        {
            { "lowcut_freq", "Low Cut Freq", "Hz", 20.0f, 2000.0f },
            { "peak_freq", "Peak Freq", "Hz", 20.0f, 20000.0f },
            { "peak_gain", "Peak Gain", "dB", -24.0f, 24.0f },
            { "peak_q", "Peak Q", "", 0.1f, 10.0f },
            { "highcut_freq", "High Cut Freq", "Hz", 200.0f, 20000.0f }
        };

        juce::Array<juce::var> parameters;

        for (const auto& spec : specs)
        {
            if (auto* rawValue = processor.apvts.getRawParameterValue (spec.id))
            {
                auto parameterObject = std::make_unique<juce::DynamicObject>();
                parameterObject->setProperty ("id", juce::String (spec.id));
                parameterObject->setProperty ("name", juce::String (spec.name));
                parameterObject->setProperty ("value", rawValue->load());
                parameterObject->setProperty ("min", spec.minValue);
                parameterObject->setProperty ("max", spec.maxValue);
                parameterObject->setProperty ("unit", juce::String (spec.unit));
                parameters.add (juce::var (parameterObject.release()));
            }
        }

        auto slopeChoices = EQPluginAudioProcessor::getSlopeChoices();

        for (auto slopeId : { "lowcut_slope", "highcut_slope" })
        {
            if (auto* rawValue = processor.apvts.getRawParameterValue (slopeId))
            {
                auto parameterObject = std::make_unique<juce::DynamicObject>();
                auto slopeIndex = juce::jlimit (0, slopeChoices.size() - 1, static_cast<int> (rawValue->load()));
                auto isLowCutSlope = juce::String (slopeId) == "lowcut_slope";
                parameterObject->setProperty ("id", juce::String (slopeId));
                parameterObject->setProperty ("name", juce::String (isLowCutSlope ? "Low Cut Slope" : "High Cut Slope"));
                parameterObject->setProperty ("value", rawValue->load());
                parameterObject->setProperty ("choice", slopeChoices[slopeIndex]);
                parameterObject->setProperty ("min", 0);
                parameterObject->setProperty ("max", slopeChoices.size() - 1);
                parameters.add (juce::var (parameterObject.release()));
            }
        }

        return parameters;
    }

    static juce::var createMeters (EQPluginAudioProcessor& processor)
    {
        auto meterObject = std::make_unique<juce::DynamicObject>();
        meterObject->setProperty ("output_rms_db", processor.getOutputRmsDb());
        meterObject->setProperty ("output_peak_db", processor.getOutputPeakDb());
        meterObject->setProperty ("low_band_balance", processor.getLowBandBalance());
        meterObject->setProperty ("mid_band_balance", processor.getMidBandBalance());
        meterObject->setProperty ("high_band_balance", processor.getHighBandBalance());
        return juce::var (meterObject.release());
    }
};

class OperationApplier
{
public:
    struct Result
    {
        bool appliedAny { false };
        juce::String statusMessage;
        juce::String explanation;
        juce::String failureReason;
    };

    static Result applyOperations (EQPluginAudioProcessor& processor, const juce::var& response)
    {
        Result result;

        if (auto* responseObject = response.getDynamicObject())
            result.explanation = responseObject->getProperty ("explanation").toString();

        auto operationsVar = response.getProperty ("operations", juce::var());
        if (! operationsVar.isArray())
        {
            result.statusMessage = "No valid operations returned";
            result.failureReason = "Response did not contain an operations array";
            return result;
        }

        int appliedCount = 0;

        for (const auto& operation : *operationsVar.getArray())
        {
            auto parameterId = operation.getProperty ("parameter_id", juce::var()).toString();
            if (parameterId.isEmpty())
                continue;

            auto* parameter = processor.apvts.getParameter (parameterId);
            if (parameter == nullptr)
                continue;

            auto targetValueVar = operation.getProperty ("value", juce::var());
            if (targetValueVar.isVoid())
                targetValueVar = operation.getProperty ("new_value", juce::var());

            if (! (targetValueVar.isDouble() || targetValueVar.isInt() || targetValueVar.isInt64() || targetValueVar.isBool()))
                continue;

            auto minValue = static_cast<float> (operation.getProperty ("min", 0.0f));
            auto maxValue = static_cast<float> (operation.getProperty ("max", 1.0f));
            auto targetValue = juce::jlimit (minValue, maxValue, static_cast<float> (targetValueVar));

            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (targetValue));
            parameter->endChangeGesture();
            ++appliedCount;

            logAiDebug ("Applied operation " + parameterId + " -> " + juce::String (targetValue, 3));
        }

        result.appliedAny = appliedCount > 0;
        result.statusMessage = appliedCount > 0
            ? "Applied " + juce::String (appliedCount) + " parameter change(s)"
            : "No matching EQ parameters returned";

        if (! result.appliedAny)
            result.failureReason = "Operations did not reference known EQ parameter ids";

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
        int statusCode { 0 };
        juce::String errorMessage;
        juce::String responseBody;
        juce::var json;
    };

    using StageCallback = std::function<void(Stage)>;

    static Response sendPrompt (const juce::String& requestBody, StageCallback stageCallback)
    {
        Response response;
        auto responseStatusCode = 0;

        if (stageCallback != nullptr)
            stageCallback (Stage::SendingRequest);

        logAiDebug ("POST " + juce::String (aiEndpoint));
        logAiDebug ("Request body: " + requestBody);

        auto url = juce::URL (aiEndpoint).withPOSTData (requestBody);
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                           .withExtraHeaders ("Content-Type: application/json\r\n")
                           .withConnectionTimeoutMs (aiRequestTimeoutMs)
                           .withStatusCode (&responseStatusCode);

        if (stageCallback != nullptr)
            stageCallback (Stage::WaitingForResponse);

        auto stream = url.createInputStream (options);
        if (stream == nullptr)
        {
            response.errorMessage = "Failed to open HTTP stream";
            return response;
        }

        if (stageCallback != nullptr)
            stageCallback (Stage::ReceivedHttpResponse);

        response.statusCode = responseStatusCode;
        response.responseBody = stream->readEntireStreamAsString();

        if (responseStatusCode < 200 || responseStatusCode >= 300)
        {
            response.errorMessage = "HTTP error: " + juce::String (responseStatusCode);
            return response;
        }

        if (stageCallback != nullptr)
            stageCallback (Stage::ParsingResponse);

        auto parsed = juce::JSON::parse (response.responseBody);
        if (parsed.isVoid())
        {
            response.errorMessage = "Failed to parse JSON response";
            return response;
        }

        response.ok = true;
        response.json = parsed;

        if (stageCallback != nullptr)
            stageCallback (Stage::Completed);

        return response;
    }
};

juce::String stageToStatusText (AiClient::Stage stage)
{
    switch (stage)
    {
        case AiClient::Stage::SendingRequest:       return "Sending request to local LLM bridge...";
        case AiClient::Stage::WaitingForResponse:   return "Waiting for AI response...";
        case AiClient::Stage::ReceivedHttpResponse: return "Received HTTP response";
        case AiClient::Stage::ParsingResponse:      return "Parsing AI response...";
        case AiClient::Stage::Completed:            return {};
    }

    return {};
}
}

void MeterBar::setValue (float newValue)
{
    value = juce::jlimit (0.0f, 1.0f, newValue);
    repaint();
}

void MeterBar::setAccentColour (juce::Colour newColour)
{
    accentColour = newColour;
    repaint();
}

void MeterBar::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff20262f));
    g.fillRoundedRectangle (bounds, 6.0f);

    auto fill = bounds.reduced (2.0f);
    fill.setWidth (fill.getWidth() * value);

    g.setColour (accentColour);
    g.fillRoundedRectangle (fill, 4.0f);
}

ResponseCurveComponent::ResponseCurveComponent (EQPluginAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    startTimerHz (30);
}

ResponseCurveComponent::~ResponseCurveComponent() = default;

void ResponseCurveComponent::timerCallback()
{
    repaint();
}

float ResponseCurveComponent::xToFrequency (float x) const
{
    auto proportion = x / static_cast<float> (getWidth());
    return 20.0f * std::pow (1000.0f, proportion);
}

float ResponseCurveComponent::frequencyToX (float frequency) const
{
    auto proportion = std::log10 (frequency / 20.0f) / std::log10 (1000.0f);
    return proportion * static_cast<float> (getWidth());
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff0f1318));
    g.fillRoundedRectangle (bounds, 14.0f);

    auto graphArea = bounds.reduced (18.0f, 16.0f);

    g.setColour (juce::Colour (0x22ffffff));
    for (auto db : { -24.0f, -12.0f, 0.0f, 12.0f, 24.0f })
    {
        auto y = juce::jmap (db, 24.0f, -24.0f, graphArea.getY(), graphArea.getBottom());
        g.drawHorizontalLine (juce::roundToInt (y), graphArea.getX(), graphArea.getRight());
    }

    for (auto frequency : { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f })
    {
        auto x = graphArea.getX() + frequencyToX (static_cast<float> (frequency)) * graphArea.getWidth() / static_cast<float> (getWidth());
        g.drawVerticalLine (juce::roundToInt (x), graphArea.getY(), graphArea.getBottom());
    }

    juce::Path responseCurve;
    auto width = juce::jmax (1, static_cast<int> (graphArea.getWidth()));

    for (int i = 0; i < width; ++i)
    {
        auto frequency = 20.0 * std::pow (1000.0, static_cast<double> (i) / static_cast<double> (width));
        auto magnitude = processor.getMagnitudeForFrequency (frequency);
        auto magnitudeDb = juce::Decibels::gainToDecibels (magnitude, -24.0f);
        auto y = juce::jmap (magnitudeDb, 24.0f, -24.0f, graphArea.getY(), graphArea.getBottom());
        auto x = graphArea.getX() + static_cast<float> (i);

        if (i == 0)
            responseCurve.startNewSubPath (x, y);
        else
            responseCurve.lineTo (x, y);
    }

    g.setColour (juce::Colour (0xfff8d56b));
    g.strokePath (responseCurve, juce::PathStrokeType (2.25f));

    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("EQ Response", graphArea.removeFromTop (20).toNearestInt(), juce::Justification::topLeft);
}

EQPluginAudioProcessorEditor::EQPluginAudioProcessorEditor (EQPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), responseCurve (p)
{
    titleLabel.setText ("Ableton EQ Console", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (24.0f));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("3-band EQ with automation, visual response, and local LLM prompt control", juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.68f));
    addAndMakeVisible (subtitleLabel);

    addAndMakeVisible (responseCurve);

    controlsGroup.setText ("EQ Controls");
    addAndMakeVisible (controlsGroup);

    configureDial (lowCutFreqSlider, " Hz");
    configureDial (peakFreqSlider, " Hz");
    configureDial (peakGainSlider, " dB");
    configureDial (peakQSlider, "");
    configureDial (highCutFreqSlider, " Hz");
    configureSlopeBox (lowCutSlopeBox);
    configureSlopeBox (highCutSlopeBox);

    addValueLabel (lowCutFreqLabel, "Low Cut Freq");
    addValueLabel (lowCutSlopeLabel, "Low Cut Slope");
    addValueLabel (peakFreqLabel, "Peak Freq");
    addValueLabel (peakGainLabel, "Peak Gain");
    addValueLabel (peakQLabel, "Peak Q");
    addValueLabel (highCutFreqLabel, "High Cut Freq");
    addValueLabel (highCutSlopeLabel, "High Cut Slope");

    addAndMakeVisible (lowCutFreqSlider);
    addAndMakeVisible (lowCutSlopeBox);
    addAndMakeVisible (peakFreqSlider);
    addAndMakeVisible (peakGainSlider);
    addAndMakeVisible (peakQSlider);
    addAndMakeVisible (highCutFreqSlider);
    addAndMakeVisible (highCutSlopeBox);

    lowCutFreqAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "lowcut_freq", lowCutFreqSlider);
    lowCutSlopeAttachment = std::make_unique<ComboAttachment> (audioProcessor.apvts, "lowcut_slope", lowCutSlopeBox);
    peakFreqAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "peak_freq", peakFreqSlider);
    peakGainAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "peak_gain", peakGainSlider);
    peakQAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "peak_q", peakQSlider);
    highCutFreqAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "highcut_freq", highCutFreqSlider);
    highCutSlopeAttachment = std::make_unique<ComboAttachment> (audioProcessor.apvts, "highcut_slope", highCutSlopeBox);

    monitorGroup.setText ("Visual Monitor");
    addAndMakeVisible (monitorGroup);

    for (auto* label : { &rmsLabel, &peakLabel, &balanceTitleLabel, &lowBalanceLabel, &midBalanceLabel, &highBalanceLabel })
    {
        label->setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (*label);
    }

    balanceTitleLabel.setText ("Band Energy", juce::dontSendNotification);
    lowBalanceBar.setAccentColour (juce::Colour (0xff65d6a4));
    midBalanceBar.setAccentColour (juce::Colour (0xfff7b955));
    highBalanceBar.setAccentColour (juce::Colour (0xff70c8ff));
    addAndMakeVisible (lowBalanceBar);
    addAndMakeVisible (midBalanceBar);
    addAndMakeVisible (highBalanceBar);

    promptGroup.setText ("Prompt Bridge");
    addAndMakeVisible (promptGroup);

    promptLabel.setText ("Send a natural-language EQ request to the local Flask bridge", juce::dontSendNotification);
    promptLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
    addAndMakeVisible (promptLabel);

    promptEditor.setMultiLine (true);
    promptEditor.setReturnKeyStartsNewLine (false);
    promptEditor.setTextToShowWhenEmpty ("examples: cut sub rumble, add some air, tame harsh mids around 3k", juce::Colours::grey);
    addAndMakeVisible (promptEditor);

    sendButton.setButtonText ("Send Prompt");
    sendButton.onClick = [this] { handleSendPrompt(); };
    addAndMakeVisible (sendButton);

    promptStatusLabel.setText ("AI Status: idle", juce::dontSendNotification);
    addAndMakeVisible (promptStatusLabel);

    promptExplanationLabel.setText ("Explanation: -", juce::dontSendNotification);
    promptExplanationLabel.setMinimumHorizontalScale (0.7f);
    addAndMakeVisible (promptExplanationLabel);

    setSize (1100, 720);
    startTimerHz (24);
}

EQPluginAudioProcessorEditor::~EQPluginAudioProcessorEditor() = default;

juce::Label& EQPluginAudioProcessorEditor::addValueLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (label);
    return label;
}

void EQPluginAudioProcessorEditor::configureDial (juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 86, 22);
    slider.setTextValueSuffix (suffix);
}

void EQPluginAudioProcessorEditor::configureSlopeBox (juce::ComboBox& comboBox)
{
    comboBox.addItemList (EQPluginAudioProcessor::getSlopeChoices(), 1);
}

void EQPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient background (juce::Colour (0xff10151c), 0.0f, 0.0f,
                                     juce::Colour (0xff19222d), 0.0f, static_cast<float> (getHeight()), false);
    g.setGradientFill (background);
    g.fillAll();

    g.setColour (juce::Colour (0x18ffffff));
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (12.0f), 18.0f);
}

void EQPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (28);

    titleLabel.setBounds (bounds.removeFromTop (34));
    subtitleLabel.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (14);

    auto topSection = bounds.removeFromTop (280);
    responseCurve.setBounds (topSection);

    bounds.removeFromTop (18);

    auto lowerSection = bounds;
    auto leftColumn = lowerSection.removeFromLeft (680);
    lowerSection.removeFromLeft (16);
    auto rightColumn = lowerSection;

    controlsGroup.setBounds (leftColumn);
    auto controlsArea = leftColumn.reduced (20, 34);

    auto row1 = controlsArea.removeFromTop (170);
    auto row2 = controlsArea.removeFromTop (170);
    auto cellWidth = row1.getWidth() / 4;

    auto layoutDial = [] (juce::Rectangle<int> area, juce::Label& label, juce::Component& component)
    {
        label.setBounds (area.removeFromTop (22));
        component.setBounds (area.removeFromTop (118));
    };

    auto lowCutArea = row1.removeFromLeft (cellWidth);
    layoutDial (lowCutArea, lowCutFreqLabel, lowCutFreqSlider);

    auto lowSlopeArea = row1.removeFromLeft (cellWidth);
    lowCutSlopeLabel.setBounds (lowSlopeArea.removeFromTop (22));
    lowCutSlopeBox.setBounds (lowSlopeArea.removeFromTop (28));

    auto peakFreqArea = row1.removeFromLeft (cellWidth);
    layoutDial (peakFreqArea, peakFreqLabel, peakFreqSlider);

    auto peakGainArea = row1;
    layoutDial (peakGainArea, peakGainLabel, peakGainSlider);

    auto peakQArea = row2.removeFromLeft (cellWidth);
    layoutDial (peakQArea, peakQLabel, peakQSlider);

    auto highCutArea = row2.removeFromLeft (cellWidth);
    layoutDial (highCutArea, highCutFreqLabel, highCutFreqSlider);

    auto highSlopeArea = row2.removeFromLeft (cellWidth);
    highCutSlopeLabel.setBounds (highSlopeArea.removeFromTop (22));
    highCutSlopeBox.setBounds (highSlopeArea.removeFromTop (28));

    monitorGroup.setBounds (rightColumn.removeFromTop (220));
    auto monitorArea = monitorGroup.getBounds().reduced (18, 34);
    rmsLabel.setBounds (monitorArea.removeFromTop (28));
    peakLabel.setBounds (monitorArea.removeFromTop (28));
    monitorArea.removeFromTop (8);
    balanceTitleLabel.setBounds (monitorArea.removeFromTop (24));
    lowBalanceLabel.setBounds (monitorArea.removeFromTop (22));
    lowBalanceBar.setBounds (monitorArea.removeFromTop (16));
    monitorArea.removeFromTop (8);
    midBalanceLabel.setBounds (monitorArea.removeFromTop (22));
    midBalanceBar.setBounds (monitorArea.removeFromTop (16));
    monitorArea.removeFromTop (8);
    highBalanceLabel.setBounds (monitorArea.removeFromTop (22));
    highBalanceBar.setBounds (monitorArea.removeFromTop (16));

    rightColumn.removeFromTop (16);

    promptGroup.setBounds (rightColumn);
    auto promptArea = promptGroup.getBounds().reduced (18, 34);
    promptLabel.setBounds (promptArea.removeFromTop (38));
    promptEditor.setBounds (promptArea.removeFromTop (130));
    promptArea.removeFromTop (10);
    sendButton.setBounds (promptArea.removeFromTop (32).removeFromLeft (150));
    promptArea.removeFromTop (12);
    promptStatusLabel.setBounds (promptArea.removeFromTop (24));
    promptExplanationLabel.setBounds (promptArea.removeFromTop (90));
}

void EQPluginAudioProcessorEditor::timerCallback()
{
    auto rms = audioProcessor.getOutputRmsDb();
    auto peak = audioProcessor.getOutputPeakDb();
    auto lowBalance = audioProcessor.getLowBandBalance();
    auto midBalance = audioProcessor.getMidBandBalance();
    auto highBalance = audioProcessor.getHighBandBalance();

    rmsLabel.setText ("Output RMS: " + formatDecibels (rms), juce::dontSendNotification);
    peakLabel.setText ("Output Peak: " + formatDecibels (peak), juce::dontSendNotification);
    lowBalanceLabel.setText ("Low Energy: " + formatPercent (lowBalance), juce::dontSendNotification);
    midBalanceLabel.setText ("Mid Energy: " + formatPercent (midBalance), juce::dontSendNotification);
    highBalanceLabel.setText ("High Energy: " + formatPercent (highBalance), juce::dontSendNotification);

    lowBalanceBar.setValue (lowBalance);
    midBalanceBar.setValue (midBalance);
    highBalanceBar.setValue (highBalance);
}

void EQPluginAudioProcessorEditor::handleSendPrompt()
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
    auto editorSafePointer = juce::Component::SafePointer<EQPluginAudioProcessorEditor> (this);

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

            auto result = OperationApplier::applyOperations (editorSafePointer->audioProcessor, response.json);

            if (result.explanation.isNotEmpty())
                editorSafePointer->setPromptExplanation ("Explanation: " + result.explanation);
            else if (result.failureReason.isNotEmpty())
                editorSafePointer->setPromptExplanation ("Explanation: " + result.failureReason);

            editorSafePointer->setPromptStatus ("AI Status: " + result.statusMessage,
                                                result.appliedAny ? juce::Colours::lightgreen : juce::Colours::yellow);
        });
    }).detach();
}

void EQPluginAudioProcessorEditor::setPromptStatus (const juce::String& newStatus, juce::Colour colour)
{
    promptStatusLabel.setText (newStatus, juce::dontSendNotification);
    promptStatusLabel.setColour (juce::Label::textColourId, colour);
}

void EQPluginAudioProcessorEditor::setPromptExplanation (const juce::String& newExplanation)
{
    promptExplanationLabel.setText (newExplanation, juce::dontSendNotification);
}
