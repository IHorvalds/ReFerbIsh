//
// Created by ihorv on 16.07.2023.
//

#ifndef REFERBISH_SINGLE_CHANNEL_SCHROEDER_ALLPASS_H
#define REFERBISH_SINGLE_CHANNEL_SCHROEDER_ALLPASS_H

#include "../Utils.h"
#include <JuceHeader.h>

namespace ihpedals
{

template <std::floating_point SampleType>
class single_channel_schroeder_allpass
{
public:
    struct parameters
    {
        SampleType max_diffusion_delay_ms = 0.0;
        SampleType diffusion_delay_samples = 0.0;
        SampleType gain = 0.5;
        SampleType modulationAmplitude = 1.0;
        SampleType modulationFrequency = 1.5;
    };

    single_channel_schroeder_allpass(dsp::ProcessSpec const& spec)
    {
        dsp::ProcessSpec new_spec = spec;
        new_spec.numChannels = 1;
        m_delayLine.setMaximumDelayInSamples(gsl::narrow_cast<int>(
            ihpedals::utilities::milliseconds_to_samples(m_params.max_diffusion_delay_ms,
                                                         new_spec.sampleRate)
        ));

        m_delayLine.prepare(new_spec);

        m_oscillator.initialise([](SampleType x) { return juce::dsp::FastMathApproximations::cos(x); });
        m_oscillator.prepare(new_spec);
    }

    void set_parameters(parameters const& params)
    {
        m_params = params;
    }

    void operator()(gsl::span<SampleType> buffer, uint8_t thread_number)
    {
        m_oscillator.setFrequency(m_params.modulation_frequency);
        for (auto& sample : buffer)
        {
            SampleType delayed = m_delayLine.popSample(
                0,
                m_params.diffusion_delay_samples +
                    m_oscillator.processSample(thread_number * juce::MathConstants<SampleType>::halfPi) *
                        m_params.modulationAmplitude
            );
            SampleType input = delayed * (-m_params.gain) + sample;

            // add to delay line
            m_delayLine.pushSample(0, input);

            // output
            sample = m_params.gain * input + delayed;
        }
    }

private:
    dsp::DelayLine<SampleType> m_delayLine;
    dsp::Oscillator<SampleType> m_oscillator;

    parameters m_params;
};

} // namespace ihpedals

#endif // REFERBISH_SINGLE_CHANNEL_SCHROEDER_ALLPASS_H
