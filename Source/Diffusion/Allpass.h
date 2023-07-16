#pragma once

#include <JuceHeader.h>
#include <gsl/algorithm>
#include <vector>

#include "../Utils.h"

template <std::floating_point SampleType>
class schroeder_allpass
{
public:
    schroeder_allpass() = default;
    ~schroeder_allpass() = default;

    SampleType m_max_diffusion_delay_ms = 0.0;
    SampleType m_diffusion_delay_samples = 0.0;
    SampleType m_gain = 0.5;
    SampleType m_modulationAmplitude = 1.0;
    SampleType m_modulationFrequency = 1.5;
    int index = 0;

    //===================================================
    void prepare(const juce::dsp::ProcessSpec& spec) noexcept
    {
        m_delayLine.setMaximumDelayInSamples(gsl::narrow_cast<int>(
            ihpedals::utilities::milliseconds_to_samples(m_max_diffusion_delay_ms, spec.sampleRate)
        ));
        m_delayLine.prepare(spec);

        if (index % 2)
        {
            m_oscillator.initialise([](SampleType x) { return juce::dsp::FastMathApproximations::sin(x); });
        }
        else
        {
            m_oscillator.initialise([](SampleType x) { return juce::dsp::FastMathApproximations::cos(x); });
        }
        m_oscillator.prepare(spec);
        m_oscillator.setFrequency(m_modulationFrequency);
    }

    //===================================================
    void process_step(std::vector<SampleType>& channelStep) noexcept
    {
        m_oscillator.setFrequency(m_modulationFrequency);
        for (int i = 0; i < channelStep.size(); ++i)
        {
            SampleType delayed = m_delayLine.popSample(
                i, m_diffusion_delay_samples + m_oscillator.processSample(0.0) * m_modulationAmplitude
            );
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