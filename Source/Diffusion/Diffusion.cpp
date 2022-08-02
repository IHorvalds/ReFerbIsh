#include "Diffusion.h"
#include "../Utils.h"

Diffusion::Diffusion(int channels, int steps) : _channels(channels), _steps(steps)
{
    this->_diffusionSteps = std::vector<DiffusionStep>(steps, { 0.0 });
    this->_shortcuts = std::vector<juce::AudioBuffer<float>>(steps);

    this->_pitchShiftSemitones = random_between_float(-0.01f, this->_pitchShiftSemitones);
}

Diffusion::~Diffusion()
{
}

void Diffusion::prepare(juce::dsp::ProcessSpec& spec, float totalDiffusionDelayMs)
{
    for (int i = 0; i < _steps; ++i)
    {
        float rangeLow = totalDiffusionDelayMs * ((float)i / _channels);
        float rangeHigh = totalDiffusionDelayMs * ((float)(i + 1) / _channels);
        _diffusionSteps[i] = DiffusionStep((double)random_between_float(rangeLow, rangeHigh));
        juce::dsp::ProcessSpec _spec = spec;
        _spec.numChannels = _channels;

        _diffusionSteps[i].prepare(_spec);
    }

    this->_pitchShifter.prepare(spec);
}

void Diffusion::process(juce::AudioBuffer<float>& buffer)
{
    // TODO: detune here
    for (int i = 0; i < _steps; ++i)
    {
        _diffusionSteps[i].process(buffer);
        //_shortcuts[i] = _diffusionSteps[i].shortcutOut;
    }
}

juce::AudioBuffer<float>& Diffusion::getShortcutAudio(int step)
{
    return this->_shortcuts[step];
}
