#include "Feedback.h"
#include "../Utils.cpp"

Feedback::Feedback()
{

}

Feedback::~Feedback()
{
}

void Feedback::prepare(juce::dsp::ProcessSpec& spec, double delayTimeMs, double rt60)
{
}


// In the feedback loop, the order is
// Pitch shift -> waveshaper -> filter -> decay -> mix
void Feedback::process(juce::AudioBuffer<float>& buffer)
{
    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();
    auto writePointers = buffer.getArrayOfWritePointers();
    
}
