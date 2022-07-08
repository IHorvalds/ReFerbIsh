#pragma once

#include "PluginProcessor.h"

class ReferbishAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ReferbishAudioProcessorEditor(ReferbishAudioProcessor& p);
private:
    void paint(juce::Graphics&) override;
    void resized() override;

    ReferbishAudioProcessor& _processor;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReferbishAudioProcessorEditor)
};