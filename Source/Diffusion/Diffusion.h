#pragma once

#include <JuceHeader.h>
#include <vector>

#include "DiffusionStep.h"

class Diffusion
{
public:
    Diffusion(int channels, int steps);
    ~Diffusion();

    //===================================================
    void prepare(juce::dsp::ProcessSpec& spec, float totalDiffusionDelayMs);
    void process(juce::AudioBuffer<float>& buffer);

    //===================================================
    juce::AudioBuffer<float>& getShortcutAudio(int step);
    inline int getNumSteps() {
        return _steps;
    }
private:
    int _channels;
    int _steps;
    std::vector<DiffusionStep> _diffusionSteps;
    std::vector<juce::AudioBuffer<float>> _shortcuts;
    float _pitchShiftSemitones = -0.06f;
    chowdsp::PitchShifter<float> _pitchShifter;
};