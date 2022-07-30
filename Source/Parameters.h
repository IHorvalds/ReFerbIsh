#pragma once

#include <JuceHeader.h>

struct Parameters
{
    void add(juce::AudioProcessor& processor) const
    {
        processor.addParameter(bypass);
        processor.addParameter(gain);
        processor.addParameter(diffusionModulationEnabled);
        processor.addParameter(diffusionModulationAmount);
        processor.addParameter(modernOrVintage);
        processor.addParameter(gain);
        processor.addParameter(unityFeedback);
        processor.addParameter(explosionFeedback);
        processor.addParameter(mix);
    }

    
    juce::AudioParameterBool* bypass = new juce::AudioParameterBool({"bp", 1}, "Bypass", false);

    // Delay time
    juce::AudioParameterFloat* delayTime = new juce::AudioParameterFloat({"dtime", 2}, "Delay Time",
                                                                         5.f, 1000.f, 324.f);
    
    // Modulation in diffusion steps
    juce::AudioParameterBool* diffusionModulationEnabled = new juce::AudioParameterBool({"dmod_enable", 3}, "Diffusion Modulation", false);
    juce::AudioParameterFloat* diffusionModulationAmount = new juce::AudioParameterFloat({"dmod_depth", 4}, "Diffusion Modulation Depth", 
                                                                                         0.f, 10.f, 3.f); // Value in Hz
                                                                                         // TODO see juce Oscillator class

    // Feedback gain
    // modern = even gain for all freq
    // vintage = freq dependent gain (i.e. filter)
    juce::AudioParameterChoice* modernOrVintage = new juce::AudioParameterChoice({"mdrnORvntg", 5}, "Feedback Type",
                                                                                 {"Modern", "Vintage"}, 0);
    // Exponential decay. This is not in decibels.
    juce::AudioParameterFloat* gain = new juce::AudioParameterFloat({"fdbk_gain", 6}, "Feedback Gain", 0.01f, 0.95f, 0.5f);
    // Set the gain to be exactly 1.
    juce::AudioParameterBool* unityFeedback = new juce::AudioParameterBool({"fdbk_hodl", 7}, "HODL", false);
    // Set the gain to be 1.1. Exponential growth. THIS MUST BE DOCUMENTED TO BE AUTOMATED. DO NOT SET AND FORGET.
    juce::AudioParameterBool* explosionFeedback = new juce::AudioParameterBool({"fdbk_expl", 8}, "Explosion", false);

    // Mix parameter
    // 0 -> Dry, 1 -> Wet
    juce::AudioParameterFloat* mix = new juce::AudioParameterFloat({"mix", 9}, "Mix", 0.f, 1.f, 0.4f);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Parameters)
};