#pragma once

#include <JuceHeader.h>
#include <vector>

#include "DiffusionStep.h"
#include "../Utils.h"

template <int Steps>
class Diffusion
{
public:
    Diffusion() = default;
    ~Diffusion() = default;

    float maxDiffusionDelayMs = 0.f;
    float diffusionDelayMs = 0.f;

    //===================================================
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        this->_diffusionSteps = std::vector<DiffusionStep>(Steps, { 0.0, 0 });
        for (int i = 0; i < Steps; ++i)
        {
            //float rangeLow = maxDiffusionDelayMs * ((float)i / _channels);
            float rangeHigh = maxDiffusionDelayMs * ((float)(i + 1) / Steps);
            _diffusionSteps[i] = DiffusionStep((double)rangeHigh, i);

            _diffusionSteps[i].prepare(spec);
        }
    }

    //===================================================
    void processBuffer(juce::AudioBuffer<float>& buffer)
    {
        for (int i = 0; i < Steps; ++i)
        {
            float rangeHigh = diffusionDelayMs * ((float)(i + 1) / Steps);
#if DEBUG
            //DBG("Diffusion step " + juce::String(i) + " delay ms " + juce::String(rangeHigh));
#endif
            _diffusionSteps[i].processBuffer(buffer, rangeHigh);
        }
    }

    //===================================================
    template <typename ProcessContext>
    void process(const ProcessContext& context)
    {
        for (int i = 0; i < Steps; ++i)
        {
            float rangeHigh = diffusionDelayMs * ((float)(i + 1) / Steps);
#if DEBUG && DEBUG_PRINTS
            DBG("Diffusion step " + juce::String(i) + " delay ms " + juce::String(rangeHigh));
#endif
            _diffusionSteps[i].process(context, rangeHigh);
        }
    }

    //===================================================
    constexpr inline int getNumSteps()
    {
        return Steps;
    }

private:
    std::vector<DiffusionStep> _diffusionSteps;
};