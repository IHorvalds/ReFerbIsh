#pragma once

#include <JuceHeader.h>
#include <vector>
#include <gsl/algorithm>

#include "../Diffusion/Diffusion.h"
#include "../Utils.h"

template <std::floating_point SampleType>
class SchroederReverb
{
public:
    SchroederReverb() = default;
    ~SchroederReverb() = default;
    
    SampleType krt = 0;
    SampleType lowPassFreq = 3000.0;
    SampleType highPassFreq = 200.0;
    SampleType maxDelayInMs = 6000.0;

    void prepare(dsp::ProcessSpec& spec)
    {
        this->spec = spec;
        m_intermediateBuffer    = std::vector<SampleType>(spec.numChannels, { 0 });
        m_oldOutput             = std::vector<SampleType>(spec.numChannels, { 0 });
        m_delayTimes            = std::vector<SampleType>(12, { 0 });
        m_diffusers             = std::vector<Diffusion<SampleType>>(8);
        m_delayLines            = std::vector<dsp::DelayLine<SampleType>>(4);
        m_lowPass               = std::vector<dsp::IIR::Filter<SampleType>>(spec.numChannels);
        m_highPass              = std::vector<dsp::IIR::Filter<SampleType>>(spec.numChannels);
        for (int i = 0; i < m_diffusers.size(); ++i)
        {
            m_diffusers[i].maxDiffusionDelayMs = maxDelayInMs;
            m_diffusers[i].index = i;
            m_diffusers[i].prepare(spec);
        }

        for (dsp::DelayLine<SampleType>& delayLine : m_delayLines)
        {
            delayLine.setMaximumDelayInSamples(gsl::narrow_cast<int>(std::floor(maxDelayInMs * 0.001 * spec.sampleRate)));
            delayLine.prepare(spec);
        }
    }

    void calculateDelayTimesMs(SampleType delayTimeMs)
    {
        // We decided on the magic number 4 steps of diffusion (2 allpasses in series followed by another delay and output)
        // which means we have 4 steps * 3 delays per step = 12 delays in total.
        SampleType dMax = delayTimeMs * 0.001 * spec.sampleRate;
        SampleType dMin = 0.1 * spec.sampleRate;
        for (int i = 0; i < 12; ++i)
        {
            SampleType dl = dMin * std::pow((dMax / dMin), (SampleType)i / 12);
            SampleType mi = std::floor(0.5 + std::log(dl) / std::log(Utilities::primes[i]) );
            m_delayTimes[i] = std::pow(Utilities::primes[i], mi) / spec.sampleRate * 1000.0;
        }
    }

    void processStep(std::vector<SampleType>& channelStep, SampleType delayTimeMs, SampleType diffusionGain = 0.5, SampleType modFreq = 1.5, SampleType modAmp = 2)
    {
        for (auto& diffuser : m_diffusers)
        {
            diffuser.m_modulationFrequency = modFreq;
            diffuser.m_modulationAmplitude = modAmp;
        }

        for (int i = 0; i < channelStep.size(); ++i)
        {
            channelStep[i] += m_oldOutput[i];
            *m_lowPass[i].coefficients = dsp::IIR::ArrayCoefficients<SampleType>::makeLowPass(spec.sampleRate, lowPassFreq);
            *m_highPass[i].coefficients = dsp::IIR::ArrayCoefficients<SampleType>::makeHighPass(spec.sampleRate, highPassFreq);
        }

        calculateDelayTimesMs(delayTimeMs);

        for (int i = 0; i < 4; ++i)
        {
            m_diffusers[2 * i].diffusionDelayMs = m_delayTimes[3 * i];
            m_diffusers[2 * i + 1].diffusionDelayMs = m_delayTimes[3 * i + 1];

            m_diffusers[2 * i].m_gain = diffusionGain;
            m_diffusers[2 * i + 1].m_gain = diffusionGain;

            m_diffusers[2 * i].m_modulationFrequency = modFreq;
            m_diffusers[2 * i + 1].m_modulationAmplitude = modAmp;
            
            m_diffusers[2 * i].processStep(channelStep);
            m_diffusers[2 * i + 1].processStep(channelStep);

            for (int ch = 0; ch < channelStep.size(); ++ch)
            {
                m_delayLines[i].pushSample(ch, channelStep[ch]);
                m_intermediateBuffer[ch] += m_delayLines[i].popSample(ch, delayTimeMs * 0.001 * spec.sampleRate);
                channelStep[ch] *= this->krt;
            }
        }

        for (int i = 0; i < channelStep.size(); ++i)
        {
            m_oldOutput[i] = m_highPass[i].processSample(m_lowPass[i].processSample(channelStep[i]));
        }
    }
private:
    dsp::ProcessSpec spec;
    std::vector<SampleType> m_intermediateBuffer;
    std::vector<SampleType> m_oldOutput;
    std::vector<SampleType> m_delayTimes;
    std::vector<Diffusion<SampleType>> m_diffusers;
    std::vector<dsp::DelayLine<SampleType>> m_delayLines;
    std::vector<dsp::IIR::Filter<SampleType>> m_lowPass, m_highPass;
};