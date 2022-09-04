#pragma once

#include <JuceHeader.h>
#include <vector>
#include <gsl/algorithm>

#include "../Utils.h"

template <std::floating_point SampleType>
class SchroederAllpass
{
public:
    SchroederAllpass() = default;
    ~SchroederAllpass() = default;

    SampleType maxDiffusionDelayMs = 0.0;
    SampleType diffusionDelaySamples = 0.0;
    SampleType m_gain = 0.5;
    SampleType m_modulationAmplitude = 1.0;
    SampleType m_modulationFrequency = 1.5;
    int index = 0;

    //===================================================
    void prepare(const juce::dsp::ProcessSpec& spec) noexcept
    {
        m_delayLine.setMaximumDelayInSamples(gsl::narrow_cast<int>(Utilities::MillisecondsToSamples(maxDiffusionDelayMs, spec.sampleRate)));
        m_delayLine.prepare(spec);

        if (index % 2)
        {
            m_oscillator.initialise([](SampleType x)
            {
                return juce::dsp::FastMathApproximations::sin(x);
            });
        }
        else
        {
            m_oscillator.initialise([](SampleType x)
            {
                return juce::dsp::FastMathApproximations::cos(x);
            });
        }
        m_oscillator.prepare(spec);
        m_oscillator.setFrequency(m_modulationFrequency);
    }

    //===================================================
    void processStep(std::vector<SampleType>& channelStep) noexcept
    {
        m_oscillator.setFrequency(m_modulationFrequency);
        for (int i = 0; i < channelStep.size(); ++i)
        {
            SampleType delayed = m_delayLine.popSample(i, diffusionDelaySamples + m_oscillator.processSample(0.0) * m_modulationAmplitude);
            SampleType input = delayed * (-m_gain) + channelStep[i];

            // add to delay line
            m_delayLine.pushSample(i, input);

            // output
            channelStep[i] = m_gain * input + delayed;
        }
    }


private:
    dsp::DelayLine<SampleType> m_delayLine;
    dsp::Oscillator<SampleType> m_oscillator;
};