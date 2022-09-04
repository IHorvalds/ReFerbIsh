#pragma once

#include <JuceHeader.h>
#include <vector>

#include "../Utils.h"

template <std::floating_point SampleType, int Lines = 8>
class Diffuser
{
public:
    Diffuser() = default;
    ~Diffuser() = default;

    SampleType m_maxDelayTimeMs = 0.0;
    SampleType m_delayTimeMs = 0.0;

    void prepare(dsp::ProcessSpec& spec) noexcept
    {
        this->m_spec = spec;
        dsp::ProcessSpec delaySpec = spec;
        delaySpec.numChannels = Lines * spec.numChannels;
        
        m_delayStep = std::vector<SampleType>(Lines);
        m_delayTimes = std::vector<SampleType>(Lines * spec.numChannels);
        m_channelMapping = std::vector<SampleType>(Lines);
        m_shouldFlip = std::vector<SampleType>(Lines);

        juce::Random _rng;
        Utilities::Random<SampleType> __rng;

        for (int i = 0; i < Lines; ++i)
        {
            m_channelMapping[i] = (SampleType)i;
            m_shouldFlip[i] = _rng.nextBool() ? 1.0 : -1.0;
        }

        __rng.rand_permutation(m_channelMapping);

        m_delayLine.prepare(delaySpec);
        m_delayLine.setMaximumDelayInSamples(gsl::narrow_cast<int>(Utilities::MillisecondsToSamples(m_maxDelayTimeMs, spec.sampleRate)));
    }

    void calculateDelayTimes()
    {
        for (uint32 ch = 0; ch < this->m_spec.numChannels; ++ch)
        {
            for (int line = 0; line < Lines; ++line)
            {
                m_delayTimes[ch * Lines + line] = Utilities::MillisecondsToSamples((SampleType)(line+1) / (SampleType)Lines * m_delayTimeMs, this->m_spec.sampleRate);
            }
        }
    }

    void processStep(std::vector<SampleType>& channelStep) noexcept
    {
        constexpr SampleType multiplier = 1.0 / (SampleType)Lines;
        calculateDelayTimes();
        for (int ch = 0; ch < channelStep.size(); ++ch)
        {
            /*m_delayLine.pushSample(ch * Lines, channelStep[ch]);
            channelStep[ch] = m_delayLine.popSample(ch * Lines, m_delayTimes[ch * Lines]);*/
            for (int line = 0; line < Lines; ++line)
            {
                m_delayLine.pushSample(ch * Lines + line, channelStep[ch]);
                m_delayStep[m_channelMapping[line]] = m_delayLine.popSample(ch * Lines + line, m_delayTimes[ch * Lines + line]) * m_shouldFlip[line];
            }

            Utilities::InPlaceHadamardMix<SampleType>(m_delayStep.data(), 0, m_delayStep.size());
            channelStep[ch] = (SampleType)0.0;

            for (int line = 0; line < Lines; ++line)
            {
                channelStep[ch] += (m_delayStep[line] * multiplier);
            }

        }
    }
private:
    dsp::ProcessSpec m_spec;
    dsp::DelayLine<SampleType> m_delayLine;
    std::vector<SampleType> m_delayStep;
    std::vector<SampleType> m_delayTimes;
    std::vector<SampleType> m_channelMapping;
    std::vector<SampleType> m_shouldFlip;
};