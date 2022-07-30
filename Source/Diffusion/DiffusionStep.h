#pragma once

#include <JuceHeader.h>
#include <chowdsp_dsp_utils/Delay/chowdsp_PitchShift.h>
#include <vector>

class DiffusionStep
{
public:
    DiffusionStep(double delayTimeMs);
    ~DiffusionStep();

    //================================================
    void prepare(juce::dsp::ProcessSpec& spec);
    void process(juce::AudioBuffer<float>& buffer, float pitchShiftSemitones = -1.f);
    
    juce::AudioBuffer<float> shortcutOut; // essentially a bypass for the diffusion
private:
    double _delayTimeMs;

    juce::dsp::ProcessSpec _spec;
    juce::dsp::DelayLine<float> _delayLine;
    chowdsp::PitchShifter<float> _pitchShifter;

    std::vector<int> _channelMapping;
    std::vector<float> _shouldFlip;
    std::vector<int> _delayPerChannel;
    // an array of size channels, containing one sample for each channel
    std::vector<float> _scratchChannelStep;
};