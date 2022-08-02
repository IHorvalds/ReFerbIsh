#pragma once

#include <JuceHeader.h>
#include <chowdsp_dsp_utils/Delay/chowdsp_PitchShift.h>
#include <vector>

class Feedback
{
public:
    Feedback(int channels);
    Feedback(const Feedback&);
    ~Feedback();

    //============================
    void prepare(juce::dsp::ProcessSpec& spec, float delayTimeMs);
    void process(juce::AudioBuffer<float>& buffer, bool shimmerOn, float driveGain, float rt60, float lowPassCutoff);
private:
    int _channels;
    double _delayTimeMs;
    juce::dsp::ProcessSpec _spec;
    juce::dsp::DelayLine<float> _delayLine;

    // Can be low pass or band pass, but pls not high pass.
    using Filter = juce::dsp::IIR::Filter<float>;

    Filter _freqDependentDecay;
    float _decayGain = 0.95f;

    chowdsp::PitchShifter<float> _pitchShifter;

    juce::dsp::WaveShaper<float> _waveShaper;

    std::vector<float> _scratchChannelStep, _dryChannelStep;

    std::vector<int> _delayPerChannel;

    //==============================================================
    void SetupLowPassFilter(float cutoff);
    void SetupDecayGain(float rt60);
};