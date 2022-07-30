#include "Diffusion.h"
#include "../Utils.cpp"

Diffusion::Diffusion(int channels, int steps) : _channels(channels), _steps(steps)
{
    this->_diffusionSteps.reserve(steps);
    this->_pitchShiftSemitones.reserve(steps);

    for (int i = 0; i < steps; ++i)
    {
        this->_pitchShiftSemitones[i] = random_between(-0.1f, -1.f);
    }
}

Diffusion::~Diffusion()
{
}

void Diffusion::prepare(juce::dsp::ProcessSpec& spec, float totalDiffusionDelayMs)
{
    for (int i = 0; i < _steps; ++i)
    {
        float rangeLow = totalDiffusionDelayMs * (float)i / _channels);
        float rangeHigh = totalDiffusionDelayMs * (float)(i + 1) / _channels);
        _diffusionSteps[i] = DiffusionStep((double)random_between(rangeLow, rangeHigh));
        juce::dsp::ProcessSpec _spec = spec;
        _spec.numChannels = _channels;

        _diffusionSteps[i].prepare(_spec);
    }
}

void Diffusion::process(juce::AudioBuffer<float>& buffer)
{
    for (int i = 0; i < _steps; ++i)
    {
        _diffusionSteps[i].process(buffer, this->_pitchShiftSemitones[i]);
        _shortcuts[i] = _diffusionSteps[i].shortcutOut;
    }
}

juce::AudioBuffer<float>& Diffusion::getShortcutAudio(int step)
{
    return this->_shortcuts[step];
}
