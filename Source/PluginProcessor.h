#pragma once

#include <JuceHeader.h>
#include <vector>
#include "Parameters.h"

#include "Diffusion/Allpass.h"
#include "Reverb/Reverb.h"

enum ChainPosition
{
    DiffusionPosition,
    FeedbackPosition
};

class ReferbishAudioProcessor : public juce::AudioProcessor
{
public:
    ReferbishAudioProcessor();
    ~ReferbishAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout CreateParameterLayout();

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS m_apvts { *this, nullptr, "Referbish_params", CreateParameterLayout() };

private:

#if DEBUG
    int inputDiffuserCount = 2;
#else
    int inputDiffuserCount = 4;
#endif

    std::vector<float> channelStep;
    //juce::AudioBuffer<float> leftProcessBuffer;
    //juce::AudioBuffer<float> rightProcessBuffer;

    std::vector<SchroederAllpass<float>> inputDiffusers; 
    SchroederReverb<float> reverb;

    //====================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReferbishAudioProcessor)
};