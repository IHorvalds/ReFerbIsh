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
    void prepare(juce::dsp::ProcessSpec& spec, float maxDelayMs);
    void process(juce::AudioBuffer<float>& buffer, float diffusionDelayMs, bool modulate, float modulationFrequency, float modulationAmplitude);

    //===================================================
    //juce::AudioBuffer<float>& getShortcutAudio(int step);
    inline int getNumSteps() {
        return _steps;
    }

    void setPitchShift(float semitones) {
        this->_pitchShiftSemitones = semitones;
    }
private:
    int _channels;
    int _steps;
    
    std::vector<DiffusionStep> _diffusionSteps;
    juce::AudioBuffer<float> _shortcut;
    float _pitchShiftSemitones = -0.1f;
    
    chowdsp::PitchShifter<float> _pitchShifter;

};