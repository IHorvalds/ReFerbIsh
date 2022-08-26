#pragma once

#include <JuceHeader.h>
#include <chowdsp_dsp_utils/Delay/chowdsp_PitchShift.h>
#include <vector>

#include "../Utils.h"
#include "gsl/algorithm"

using Filter = juce::dsp::IIR::Filter<float>;

class Feedback
{
public:
    Feedback() = default;
    Feedback(const Feedback&) = default;
    ~Feedback() = default;
    

    //============================
    void prepare(const juce::dsp::ProcessSpec& spec);

    void processBuffer(juce::AudioBuffer<float>& buffer);

    template <typename ProcessContext>
    void process(const ProcessContext& buffer);

    inline void updateParameters(float delayTimeMs, bool shimmerOn, float shimmerGain, float driveGain, bool hold, float rt60, float lowPassCutoff)
    {
        _shimmerOn = shimmerOn;
        _hold = hold;
        _delayTimeMs = delayTimeMs;
        _shimmerGain = shimmerGain;
        _driveGain = driveGain;
        _rt60 = rt60;
        _lowPassCutoff = lowPassCutoff;
    }

    double maxDelayTimeMs = 0.0;

private:
    bool _shimmerOn      = false;
    bool _hold           = false;
    float _delayTimeMs   = 0.f;
    float _shimmerGain   = 0.f;
    float _driveGain     = 0.f;
    float _rt60          = 0.f;
    float _lowPassCutoff = 0.f;

    juce::dsp::ProcessSpec _spec;
    juce::dsp::DelayLine<float> _delayLine;
    juce::Random _rng = juce::Random(Time::currentTimeMillis());

    // Can be low pass or band pass, but pls not high pass.
    std::vector<Filter> _freqDependentDecay;

    chowdsp::PitchShifter<float> _pitchShifter;

    juce::dsp::WaveShaper<float> _waveShaper;

    std::vector<float> _scratchChannelStep;
    std::vector<float> _dryChannelStep;

    std::vector<float> _delayPerChannel;
    std::vector<float> _decayPerChannel;

    //==============================================================
    void setupLowPassFilter(float cutoff, int channel);
    float getDecayGain(float rt60, float delayTimeMs) const;
    int getRandomDelayPerChannel(int delayInSamples, int channel) const;
};

#include "Feedback.tpp"