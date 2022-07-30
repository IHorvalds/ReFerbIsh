#pragma once

#include <JuceHeader.h>
#include <chowdsp_dsp_utils/Delay/chowdsp_PitchShift.h>
#include <vector>

class Feedback
{
public:
    Feedback();
    ~Feedback();

    //============================
    void prepare(juce::dsp::ProcessSpec& spec, double delayTimeMs, double rt60);
    void process(juce::AudioBuffer<float>& buffer);
private:
    juce::dsp::ProcessSpec spec;
    juce::dsp::DelayLine<float> _delayLine;

    // Can be low pass or band pass, but pls not high pass.
    juce::dsp::StateVariableFilter::Filter<float> _freqDependentDecay;
    float decayGain;

    chowdsp::PitchShifter _pitchShifter;

    juce::dsp::WaveShaper<float> _waveShaper;

    std::vector<float> _scratchChannelStep;
};