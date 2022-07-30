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
    void prepare(double totalDiffusionDelayMs);
    void process(juce::AudioBuffer<float>& buffer);

    //===================================================
    juce::AudioBuffer<float>& getShortcutAudio(int step);
private:
    int _channels;
    int _steps;
    std::vector<DiffusionStep> _diffusionSteps;
    std::vector<juce::AudioBuffer<float>> _shortcuts;
    std::vector<float> _pitchShiftSemitones;
};