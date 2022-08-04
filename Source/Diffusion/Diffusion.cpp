#include "Diffusion.h"
#include "../Utils.h"

Diffusion::Diffusion(int channels, int steps) : _channels(channels), _steps(steps)
{
    this->_diffusionSteps = std::vector<DiffusionStep>(steps, { 0.0, 0 });

    this->_pitchShiftSemitones = random_between_float(-0.01f, this->_pitchShiftSemitones);
}

Diffusion::~Diffusion()
{
}

void Diffusion::prepare(juce::dsp::ProcessSpec& spec, float maxDiffusionDelayMs)
{
    for (int i = 0; i < _steps; ++i)
    {
        //float rangeLow = maxDiffusionDelayMs * ((float)i / _channels);
        float rangeHigh = maxDiffusionDelayMs * ((float)(i + 1) / _channels);
        _diffusionSteps[i] = DiffusionStep((double)rangeHigh, i);
        juce::dsp::ProcessSpec _spec = spec;
        _spec.numChannels = _channels;

        _diffusionSteps[i].prepare(_spec);
    }

    this->_pitchShifter.prepare(spec);
}

void Diffusion::process(juce::AudioBuffer<float>& buffer, float diffusionDelayMs, bool modulate = false, float modulationFrequency = 2.f, float modulationAmplitude = 0.1f)
{
    _pitchShifter.setShiftSemitones(this->_pitchShiftSemitones);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* writePointer = buffer.getWritePointer(ch);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            writePointer[sample] = _pitchShifter.processSample(ch, writePointer[sample]);
        }
    }
    for (int i = 0; i < _steps; ++i)
    {
        float rangeHigh = diffusionDelayMs * ((float)(i + 1) / _channels);
        DBG("Diffusion step " + juce::String(i) + " delay ms " + juce::String(rangeHigh));
        _diffusionSteps[i].process(buffer, rangeHigh, modulate, modulationFrequency, modulationAmplitude);
        //_shortcuts[i] = _diffusionSteps[i].shortcutOut;
    }
}

//juce::AudioBuffer<float>& Diffusion::getShortcutAudio(int step)
//{
//    return this->_shortcuts[step];
//}
