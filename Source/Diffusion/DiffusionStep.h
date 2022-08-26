#pragma once

#include <JuceHeader.h>
#include <vector>
#include <gsl/algorithm>

#include "../Utils.h"

class DiffusionStep
{
public:
    DiffusionStep(double maxDelayTimeMs, int index);
    DiffusionStep(const DiffusionStep&);
    DiffusionStep& operator=(const DiffusionStep&);
    ~DiffusionStep() = default;

    //================================================
    void prepare(const juce::dsp::ProcessSpec& spec);
    void processBuffer(juce::AudioBuffer<float>& buffer, double delayTimeMs);

    template <typename ProcessContext>
    void process(const ProcessContext& context, double delayTimeMs);
private:
    double _maxDelayTimeMs;
    int _index;

    juce::dsp::ProcessSpec _spec;
    juce::dsp::DelayLine<float> _delayLine;
    juce::Random _rng = juce::Random(time(nullptr));

    std::vector<int> _channelMapping;
    std::vector<float> _shouldFlip;
    std::vector<float> _delayPerChannel;
    // an array of size channels, containing one sample for each channel
    std::vector<float> _scratchChannelStep;

    int getRandomPerChannelDelay(int delayInSamples, int channel) const;

    inline double calculateDelay(double delayTimeMs)
    {
        return std::pow(Utilities::tenPrimes[_index], std::floor(0.5 + std::log(delayTimeMs) / std::log(Utilities::tenPrimes[_index])));
    }
};

#include "DiffusionStep.tpp"