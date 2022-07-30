#include "DiffusionStep.h"
#include "../Utils.cpp"

#include <chowdsp_math/Math/chowdsp_MatrixOps.h>

DiffusionStep::DiffusionStep(double delayTimeMs) :
    _delayTimeMs(delayTimeMs),
    _spec(juce::dsp::ProcessSpec()),
    _channelMapping(std::vector<int>()),
    _shouldFlip(std::vector<float>()),
    _delayPerChannel(std::vector<int>()),
    _scratchChannelStep(std::vector<float>()) { }

DiffusionStep::~DiffusionStep() {}

void DiffusionStep::prepare(juce::dsp::ProcessSpec& spec)
{
    this->_spec = spec;
    this->_delayLine.prepare(_spec);
    this->_delayLine.setMaximumDelayInSamples(static_cast<int>(std::floor(_delayTimeMs * 0.001 * _spec.sampleRate)));

    srand(NULL);
    this->_channelMapping = std::vector<int>(_spec.numChannels, { 0 });
    this->_shouldFlip = std::vector<float>(_spec.numChannels, { 0.f });
    this->_delayPerChannel = std::vector<int>(_spec.numChannels, { 0 });
    for (int i = 0; i < _spec.numChannels; ++i)
    {
        this->_channelMapping[i] = i;
        this->_shouldFlip[i] = rand() % 2 ? 1 : -1;

        int rangeLow = static_cast<int>(std::floor((double)this->_delayLine.getMaximumDelayInSamples() * (double)i / _spec.numChannels));
        int rangeHigh = static_cast<int>(std::floor((double)this->_delayLine.getMaximumDelayInSamples() * (double)(i + 1) / _spec.numChannels));
        this->_delayPerChannel[i] = random_between(rangeLow, rangeHigh);
    }

    rand_permutation(this->_channelMapping);

    this->_pitchShifter.prepare(this->_spec);
}

void DiffusionStep::process(juce::AudioBuffer<float>& buffer, float pitchShiftSemitones = -1.f)
{

    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();
    this->_pitchShifter.setShiftSemitones(pitchShiftSemitones);

    shortcutOut = buffer;

    auto** writePointers = buffer.getArrayOfWritePointers();
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float delayed = this->_delayLine.popSample(ch, this->_delayPerChannel[ch]);

            float detuned = this->_pitchShifter.processSample((size_t)ch, writePointers[ch][sample]);
            this->_delayLine.pushSample(ch, detuned);

            // fill channel wise buffer, with channel mapping and randomly flipped sample
            _scratchChannelStep[this->_channelMapping[ch]] = delayed * this->_shouldFlip[ch];
        }

        // Give it some whisky-business!
        InPlaceHadamardMix<float>(this->_scratchChannelStep.data(), 0, numChannels);

        // write back to current audio buffer
        for (int ch = 0; ch < numChannels; ++ch)
        {
            writePointers[ch][sample] = this->_scratchChannelStep[ch];
        }
    }

}
