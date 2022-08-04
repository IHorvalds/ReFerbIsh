#pragma once

#include <JuceHeader.h>
#include <chowdsp_dsp_utils/Delay/chowdsp_PitchShift.h>
#include <vector>
#include "../Utils.h"

class DiffusionStep
{
public:
    DiffusionStep(double maxDelayTimeMs, int index);
    DiffusionStep(const DiffusionStep&);
    DiffusionStep& operator=(const DiffusionStep&);
    ~DiffusionStep();

    //================================================
    void prepare(juce::dsp::ProcessSpec& spec);
    void process(juce::AudioBuffer<float>& buffer, double delayTimeMs, bool shouldModulate, float lfoFreq, float lfoAmp);
    
    juce::AudioBuffer<float> shortcutOut; // essentially a bypass for the diffusion
private:
    double _maxDelayTimeMs;
    int _index;

    juce::dsp::ProcessSpec _spec;
    juce::dsp::DelayLine<float> _delayLine;

    std::vector<int> _channelMapping;
    std::vector<float> _shouldFlip;
    std::vector<int> _delayPerChannel;
    // an array of size channels, containing one sample for each channel
    std::vector<float> _scratchChannelStep;
    //juce::dsp::Oscillator<float> _LFO;

    void setRandomPerChannelDelay(int delayInSamples, bool shouldModulate, int lfoAmpInSamples);

    inline double calculateDelay(double delayTimeMs)
    {
        return std::pow(tenPrimes[_index], std::floor(0.5 + std::log(delayTimeMs) / std::log(tenPrimes[_index])));
    }
};