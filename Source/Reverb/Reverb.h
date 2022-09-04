#pragma once

#include <JuceHeader.h>
#include <vector>
#include <gsl/algorithm>

#include "../Diffusion/Allpass.h"
#include "../Diffusion/Diffuser.h"
#include "../Utils.h"

template <std::floating_point SampleType, int Steps = 4>
class SchroederReverb
{
public:
    SchroederReverb() = default;
    ~SchroederReverb() = default;
    
    SampleType krt = 0;
    SampleType lowPassFreq = 3000.0;
    SampleType highPassFreq = 200.0;
    SampleType maxDelayInMs = 6000.0;

    void prepare(dsp::ProcessSpec& spec) noexcept
    {
        this->m_spec = spec;
        m_oldOutput             = std::vector<SampleType>(m_spec.numChannels, { 0 });
        m_delayTimes            = std::vector<SampleType>(3 * Steps, { 0 });
        m_allpasses             = std::vector<SchroederAllpass<SampleType>>(2 * Steps);
        m_delayLines            = std::vector<dsp::DelayLine<SampleType>>(Steps);
        m_diffusers             = std::vector<Diffuser<SampleType>>(Steps);
        m_lowPass               = std::vector<dsp::IIR::Filter<SampleType>>(m_spec.numChannels);
        m_highPass              = std::vector<dsp::IIR::Filter<SampleType>>(m_spec.numChannels);
        for (int i = 0; i < m_allpasses.size(); ++i)
        {
            m_allpasses[i].maxDiffusionDelayMs = maxDelayInMs;
            m_allpasses[i].index = i;
            m_allpasses[i].prepare(m_spec);
        }

        for (Diffuser<SampleType>& diffuser : m_diffusers)
        {
            diffuser.m_maxDelayTimeMs = maxDelayInMs; // setMaximumDelayInSamples(gsl::narrow_cast<int>(std::floor(maxDelayInMs * 0.001 * m_spec.sampleRate)));
            diffuser.prepare(m_spec);
        }

        for (dsp::DelayLine<SampleType>& delayLine : m_delayLines)
        {
            delayLine.setMaximumDelayInSamples(gsl::narrow_cast<int>(Utilities::MillisecondsToSamples(maxDelayInMs, m_spec.sampleRate)));
            delayLine.prepare(m_spec);
        }
    }

    void calculateDelayTimes(SampleType delayTimeMs) noexcept
    {
        SampleType dMax = Utilities::MillisecondsToSamples(delayTimeMs, m_spec.sampleRate);
        SampleType dMin = Utilities::MillisecondsToSamples((SampleType)100.0, m_spec.sampleRate);
        for (size_t i = 0; i < m_delayTimes.size(); ++i)
        {
            SampleType dl = dMin * std::pow((dMax / dMin), (SampleType)i / m_delayTimes.size());
            SampleType mi = std::floor(gsl::narrow_cast<SampleType>(0.5) + std::log(dl) / std::log(gsl::narrow_cast<SampleType>(Utilities::primes[i])) );
            m_delayTimes[i] = std::pow(gsl::narrow_cast<SampleType>(Utilities::primes[i]), mi) / gsl::narrow_cast<SampleType>(m_spec.sampleRate) * gsl::narrow_cast<SampleType>(1000.0);
        }
    }

    void processStep(std::vector<SampleType>& channelStep, SampleType delayTimeMs, SampleType diffusionGain = 0.5, SampleType modFreq = 1.5, SampleType modAmp = 2) noexcept
    {
        for (auto& allpass : m_allpasses)
        {
            allpass.m_modulationFrequency = modFreq;
            allpass.m_modulationAmplitude = modAmp;
        }

        for (size_t i = 0; i < channelStep.size(); ++i)
        {
            channelStep[i] += m_oldOutput[i];
            *m_lowPass[i].coefficients = dsp::IIR::ArrayCoefficients<SampleType>::makeLowPass(m_spec.sampleRate, lowPassFreq);
            *m_highPass[i].coefficients = dsp::IIR::ArrayCoefficients<SampleType>::makeHighPass(m_spec.sampleRate, highPassFreq);
        }

        calculateDelayTimes(delayTimeMs);

        for (size_t i = 0; i < Steps; ++i)
        {
            m_allpasses[2 * i].diffusionDelaySamples = m_delayTimes[3 * i];
            m_allpasses[2 * i + 1].diffusionDelaySamples = m_delayTimes[3 * i + 1];

            m_allpasses[2 * i].m_gain = diffusionGain;
            m_allpasses[2 * i + 1].m_gain = diffusionGain;

            m_allpasses[2 * i].m_modulationFrequency = modFreq;
            m_allpasses[2 * i + 1].m_modulationAmplitude = modAmp;
            
            m_allpasses[2 * i].processStep(channelStep);
            m_allpasses[2 * i + 1].processStep(channelStep);

            m_diffusers[i].m_delayTimeMs = m_delayTimes[3 * i + 2];
            m_diffusers[i].processStep(channelStep);

            for (int ch = 0; ch < channelStep.size(); ++ch)
            {
                channelStep[ch] *= this->krt;
            }

            //for (int ch = 0; ch < channelStep.size(); ++ch) // TODO: replace this with a new ass diffuser
            //{
            //    m_delayLines[i].pushSample(ch, channelStep[ch]);
            //    channelStep[ch] = m_delayLines[i].popSample(ch, m_delayTimes[3 * i + 2]) * this->krt;
            //}
        }

        for (size_t i = 0; i < channelStep.size(); ++i)
        {
            m_oldOutput[i] = m_highPass[i].processSample(m_lowPass[i].processSample(channelStep[i]));
        }
    }
private:
    dsp::ProcessSpec m_spec;
    std::vector<SampleType> m_oldOutput;
    std::vector<SampleType> m_delayTimes;
    std::vector<SchroederAllpass<SampleType>> m_allpasses;
    std::vector<dsp::DelayLine<SampleType>> m_delayLines;
    std::vector<Diffuser<SampleType>> m_diffusers;
    std::vector<dsp::IIR::Filter<SampleType>> m_lowPass, m_highPass;
};