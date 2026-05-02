#include "AgentView.h"
#include "../PluginProcessor.h"

namespace
{
void configureSlider (juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 88, 24);
    slider.setTextValueSuffix (suffix);
}
}

AgentView::AgentView (VoltaAgentPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    sectionLabel.setText ("MIX CONTROL", juce::dontSendNotification);
    sectionLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));

    volumeLabel.setText ("Volume", juce::dontSendNotification);
    lowCutLabel.setText ("Low Cut", juce::dontSendNotification);
    presenceLabel.setText ("Presence", juce::dontSendNotification);
    compressionLabel.setText ("Compression", juce::dontSendNotification);
    warmthLabel.setText ("Warmth / Saturation", juce::dontSendNotification);
    lastCommandTitle.setText ("Last Command", juce::dontSendNotification);
    lastAppliedTitle.setText ("Last Applied", juce::dontSendNotification);

    configureSlider (volumeSlider, " dB");
    configureSlider (lowCutFrequencySlider, " Hz");
    configureSlider (presenceSlider, " dB");
    configureSlider (compressionSlider, "");
    configureSlider (warmthSlider, "");

    addAndMakeVisible (sectionLabel);
    addAndMakeVisible (volumeLabel);
    addAndMakeVisible (lowCutLabel);
    addAndMakeVisible (presenceLabel);
    addAndMakeVisible (compressionLabel);
    addAndMakeVisible (warmthLabel);
    addAndMakeVisible (lastCommandTitle);
    addAndMakeVisible (lastCommandValue);
    addAndMakeVisible (lastAppliedTitle);
    addAndMakeVisible (lastAppliedValue);
    addAndMakeVisible (volumeSlider);
    addAndMakeVisible (lowCutToggle);
    addAndMakeVisible (lowCutFrequencySlider);
    addAndMakeVisible (presenceSlider);
    addAndMakeVisible (compressionSlider);
    addAndMakeVisible (warmthSlider);

    volumeAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "gain_db", volumeSlider);
    lowCutToggleAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "low_cut_enabled", lowCutToggle);
    lowCutFrequencyAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "low_cut_freq_hz", lowCutFrequencySlider);
    presenceAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "presence_db", presenceSlider);
    compressionAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "compression_amount", compressionSlider);
    warmthAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "warmth_amount", warmthSlider);

    refreshState();
}

void AgentView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 18));
    g.fillRoundedRectangle (bounds, 10.0f);
    g.setColour (juce::Colour::fromRGBA (87, 225, 193, 90));
    g.drawRoundedRectangle (bounds, 10.0f, 1.0f);
}

void AgentView::resized()
{
    auto area = getLocalBounds().reduced (16);
    sectionLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (10);

    constexpr int rowHeight = 30;
    constexpr int labelWidth = 140;

    auto row = area.removeFromTop (rowHeight);
    volumeLabel.setBounds (row.removeFromLeft (labelWidth));
    volumeSlider.setBounds (row);

    area.removeFromTop (8);
    row = area.removeFromTop (rowHeight);
    lowCutLabel.setBounds (row.removeFromLeft (labelWidth));
    lowCutToggle.setBounds (row.removeFromLeft (90));
    row.removeFromLeft (8);
    lowCutFrequencySlider.setBounds (row);

    area.removeFromTop (8);
    row = area.removeFromTop (rowHeight);
    presenceLabel.setBounds (row.removeFromLeft (labelWidth));
    presenceSlider.setBounds (row);

    area.removeFromTop (8);
    row = area.removeFromTop (rowHeight);
    compressionLabel.setBounds (row.removeFromLeft (labelWidth));
    compressionSlider.setBounds (row);

    area.removeFromTop (8);
    row = area.removeFromTop (rowHeight);
    warmthLabel.setBounds (row.removeFromLeft (labelWidth));
    warmthSlider.setBounds (row);

    area.removeFromTop (14);
    lastCommandTitle.setBounds (area.removeFromTop (20));
    lastCommandValue.setBounds (area.removeFromTop (26));
    area.removeFromTop (6);
    lastAppliedTitle.setBounds (area.removeFromTop (20));
    lastAppliedValue.setBounds (area.removeFromTop (26));
}

void AgentView::refreshState()
{
    auto state = audioProcessor.getAgentState();
    lowCutFrequencySlider.setEnabled (state.lowCutEnabled);
    lastCommandValue.setText (audioProcessor.getLastCommandText(), juce::dontSendNotification);
    lastAppliedValue.setText (audioProcessor.getLastAppliedText(), juce::dontSendNotification);
}
