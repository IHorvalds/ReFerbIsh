#include "DiffusionStep.h"
#include "../Utils.h"

#include <chowdsp_math/Math/chowdsp_MatrixOps.h>

DiffusionStep::DiffusionStep(double maxDelayTimeMs, int index) :
    _maxDelayTimeMs(maxDelayTimeMs),
    _index(index),
    _spec(juce::dsp::ProcessSpec()),
    _channelMapping(std::vector<int>()),
    _shouldFlip(std::vector<float>()),
    _delayPerChannel(std::vector<int>()),
    _scratchChannelStep(std::vector<float>()) { }

DiffusionStep::DiffusionStep(const DiffusionStep& other)
{
    this->_delayLine = other._delayLine;
    this->_index = other._index;
    this->_maxDelayTimeMs = other._maxDelayTimeMs;
    this->_channelMapping = other._channelMapping;
    this->_delayPerChannel = other._delayPerChannel;
    this->_shouldFlip = other._shouldFlip;
    this->_scratchChannelStep = other._scratchChannelStep;
    this->_spec = other._spec;
}

DiffusionStep& DiffusionStep::operator=(const DiffusionStep& other)
{
    this->_delayLine = other._delayLine;
    this->_index = other._index;
    this->_maxDelayTimeMs = other._maxDelayTimeMs;
    this->_channelMapping = other._channelMapping;
    this->_delayPerChannel = other._delayPerChannel;
    this->_shouldFlip = other._shouldFlip;
    this->_scratchChannelStep = other._scratchChannelStep;
    this->_spec = other._spec;

    return *this;
}

DiffusionStep::~DiffusionStep() {}

void DiffusionStep::prepare(juce::dsp::ProcessSpec& spec)
{
    this->_spec = spec;
    this->_delayLine.prepare(_spec);
    this->_delayLine.setMaximumDelayInSamples(static_cast<int>(std::floor(_maxDelayTimeMs * 0.001 * _spec.sampleRate)));

    srand(time(nullptr));
    this->_scratchChannelStep = std::vector<float>(_spec.numChannels, { 0.f });
    this->_channelMapping = std::vector<int>(_spec.numChannels, { 0 });
    this->_shouldFlip = std::vector<float>(_spec.numChannels, { 0.f });
    this->_delayPerChannel = std::vector<int>(_spec.numChannels, { 0 });
    for (int i = 0; i < _spec.numChannels; ++i)
    {
        this->_channelMapping[i] = i;
        this->_shouldFlip[i] = rand() % 2 ? 1 : -1;

        int rangeLow = static_cast<int>(std::floor((double)this->_delayLine.getMaximumDelayInSamples() * (double)i / _spec.numChannels));
        int rangeHigh = static_cast<int>(std::floor((double)this->_delayLine.getMaximumDelayInSamples() * (double)(i + 1) / _spec.numChannels));
        this->_delayPerChannel[i] = random_between_int(rangeLow, rangeHigh);
    }

    rand_permutation(this->_channelMapping);

    //_LFO.prepare(spec);
    //_LFO.initialise([] (float x) 
    //{
    //    return juce::dsp::FastMathApproximations::sin<float>(x);
    //}); // Does this make sense? How many points should we actually keeo?
    //_LFO.setFrequency(2.f);
}

void DiffusionStep::setRandomPerChannelDelay(int delayInSamples, bool shouldModulate, int lfoAmpInSamples)
{
    for (int ch = 0; ch < _spec.numChannels; ++ch)
    {
        int rangeLow = static_cast<int>(std::floor((double)delayInSamples * (double)ch / _spec.numChannels));
        int rangeHigh = static_cast<int>(std::floor((double)delayInSamples * (double)(ch + 1) / _spec.numChannels));
     
        if (shouldModulate)
        {
            rangeLow += lfoAmpInSamples;
            rangeHigh -= lfoAmpInSamples;
        }

        
        this->_delayPerChannel[ch] = (rangeLow + rangeHigh) / 2;
    }
}

void DiffusionStep::process(juce::AudioBuffer<float>& buffer, double delayTimeMs, bool shouldModulate, float lfoFreq, float lfoAmp)
{

    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    //shortcutOut.makeCopyOf(buffer, true);

    int delayInSamples = static_cast<int>(delayTimeMs * 0.001 * _spec.sampleRate);
    int lfoAmpInSamples = static_cast<int>((double)lfoAmp * 0.001 * _spec.sampleRate);
    //setRandomPerChannelDelay(delayInSamples, false, 0);

    /*bool willModulate = false;
    if (shouldModulate && delayInSamples / _spec.numChannels > 2 * lfoAmpInSamples)
    {
        willModulate = true;
        setRandomPerChannelDelay(delayInSamples, true, lfoAmpInSamples);
        _LFO.setFrequency(lfoFreq);
    }
    else {
        setRandomPerChannelDelay(delayInSamples, false, 0);
    }*/
    setRandomPerChannelDelay(delayInSamples, false, 0);


    auto** writePointers = buffer.getArrayOfWritePointers();
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            // read from delay line
            float delayed = 0.f;
            /*if (willModulate)
            {
                delayed = this->_delayLine.popSample(ch, lfoAmpInSamples * _LFO.processSample(_delayPerChannel[ch]));
            }
            else {
                delayed = this->_delayLine.popSample(ch, _delayPerChannel[ch]);
            }*/
            delayed = this->_delayLine.popSample(ch, _delayPerChannel[ch]);

            // push new, clean signal into the delay line
            this->_delayLine.pushSample(ch, writePointers[ch][sample]);

            // fill channel wise buffer, with channel mapping and randomly flipped sample
            _scratchChannelStep[this->_channelMapping[ch]] = delayed * this->_shouldFlip[ch];
        }

        // Give it some whisky-business!
        InPlaceHadamardMix<float>(this->_scratchChannelStep.data(), 0, numChannels);

        float scale = 3.f / (float)numChannels;

        // write back to current audio buffer
        for (int ch = 0; ch < numChannels; ++ch)
        {
            writePointers[ch][sample] = this->_scratchChannelStep[ch] * scale;
        }
    }

}
