#include "Feedback.h"
#include "Feedback.h"
#include "../Utils.h"

Feedback::Feedback(int channels) : _channels(channels)
{
    _scratchChannelStep = std::vector<float>(channels, { 0.f });
    _delayPerChannel = std::vector<int>(channels, { 0 });
}

Feedback::Feedback(const Feedback& other)
{
    this->_channels = other._channels;
    this->_maxDelayTimeMs = other._maxDelayTimeMs;
    this->_decayGain = other._decayGain;
    this->_delayLine = other._delayLine;
    this->_delayPerChannel = other._delayPerChannel;
    this->_scratchChannelStep = other._scratchChannelStep;
    this->_dryChannelStep = other._dryChannelStep;
    this->_spec = other._spec;
    this->_waveShaper = other._waveShaper;
}

Feedback::~Feedback()
{
}

void Feedback::setupLowPassFilter(float cutoff, int ch)
{
    _freqDependentDecay[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(_spec.sampleRate, cutoff);
    //_freqDependentDecay.reset();
}

float Feedback::getDecayGain(float rt60, float delayTimeMs)
{
    float loopMs = delayTimeMs * 1.5f;
    float loopsPerRt60 = rt60 / (loopMs * 0.001f);

    float dbPerCycle = -60 / loopsPerRt60;

    return std::powf(10.f, dbPerCycle * 0.05f);
}

void Feedback::setRandomDelayPerChannel(int delayInSamples)
{
    for (size_t i = 0; i < _spec.numChannels; ++i)
    {

        int rangeLow = static_cast<int>(std::floor((float)delayInSamples * (float)i / _spec.numChannels));
        int rangeHigh = static_cast<int>(std::floor((float)delayInSamples * (float)(i + 1) / _spec.numChannels));
        this->_delayPerChannel[i] = (rangeLow + rangeHigh) / 2;
    }
}

void Feedback::prepare(juce::dsp::ProcessSpec& spec, float maxDelayTimeMs)
{

    this->_spec = spec;
    this->_maxDelayTimeMs = maxDelayTimeMs;

    this->_delayLine.prepare(_spec);
    this->_delayLine.setMaximumDelayInSamples(static_cast<int>(std::floor(_maxDelayTimeMs * 0.001 * _spec.sampleRate)));

    srand(time(nullptr));
    this->_scratchChannelStep = std::vector<float>(_spec.numChannels, { 0.f });
    this->_dryChannelStep = std::vector<float>(_spec.numChannels, { 0.f });
    this->_delayPerChannel = std::vector<int>(_spec.numChannels, { 0 });
    this->_freqDependentDecay = std::vector<Filter>(_spec.numChannels);
    
    juce::dsp::ProcessSpec localSpec = _spec;
    localSpec.numChannels = 1;
    
    for (size_t i = 0; i < _spec.numChannels; ++i)
    {

        int rangeLow = static_cast<int>(std::floor((float)this->_delayLine.getMaximumDelayInSamples() * (float)i / _spec.numChannels));
        int rangeHigh = static_cast<int>(std::floor((float)this->_delayLine.getMaximumDelayInSamples() * (float)(i + 1) / _spec.numChannels));
        this->_delayPerChannel[i] = random_between_int(rangeLow, rangeHigh);
        _freqDependentDecay[i].prepare(localSpec);
        setupLowPassFilter(3e3f, i);
        _freqDependentDecay[i].reset();
    }

    //setupDecayGain(5.f, maxDelayTimeMs);

    this->_pitchShifter.prepare(this->_spec);
    this->_pitchShifter.reset();
    this->_pitchShifter.setShiftSemitones(12.f);
    _waveShaper.functionToUse = softclipper;
    _waveShaper.prepare(this->_spec);
}




// In the feedback loop, the order is
// Pitch shift -> waveshaper -> filter -> decay -> mix
void Feedback::process(juce::AudioBuffer<float>& buffer, float delayTimeMs, bool shimmerOn, float shimmerGain, float driveGain, bool hold, float rt60, float lowPassCutoff)
{
    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();
    auto** readPointers = buffer.getArrayOfReadPointers();
    auto** writePointers = buffer.getArrayOfWritePointers();

    DBG(_decayGain);
    int delayInSamples = static_cast<int>(delayTimeMs * 0.001f * (float)_spec.sampleRate);
    //setRandomDelayPerChannel(delayInSamples);


    /*this->_pitchShifter.setShiftSemitones(12.f);
    this->_pitchShifter.prepare(this->_spec);*/

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            setupLowPassFilter(lowPassCutoff, ch);
            float _delay = delayInSamples * (float)(ch + 1) / (float)numChannels;
            // Fill dry buffer, because readonly pointers are affected by the writeable pointers...
            _dryChannelStep[ch] = readPointers[ch][sample];

            // Read from Delay line
            _scratchChannelStep[ch] = _delayLine.popSample(ch, _delay);
            DBG("feedback delay" + juce::String(_delay));

            // Write to output
            // WARNING: Does writing to the write pointers affect read pointers? YES IT DOES, FFS.
            writePointers[ch][sample] = _scratchChannelStep[ch];


            // decay gain, if not hold
            float decayGain = getDecayGain(rt60, (float)(_delay * 1e3f * _spec.sampleRate));
            if (!hold)
            {
                _scratchChannelStep[ch] *= decayGain;

            }

                
            // pitch shifter
            if (shimmerOn)
            {
                _scratchChannelStep[ch] += shimmerGain * _pitchShifter.processSample(ch, writePointers[ch][sample]);
            }

            // non-linearity
            _scratchChannelStep[ch] = _waveShaper.processSample(driveGain * _scratchChannelStep[ch]);
            //_scratchChannelStep[ch] /= 2.f;



            // Apply filter
            _scratchChannelStep[ch] = _freqDependentDecay[ch].processSample(_scratchChannelStep[ch]);

        }

        // apply mix matrix
        InPlaceHouseholderMix<float>(_scratchChannelStep.data(), numChannels);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            _scratchChannelStep[ch] = _scratchChannelStep[ch] + _dryChannelStep[ch];
            //_scratchChannelStep[ch] /= 2.f;
            jlimit(-2.f, 2.f, _scratchChannelStep[ch]);

            // feed back into the delay line
            _delayLine.pushSample(ch, _scratchChannelStep[ch]);

            _freqDependentDecay[ch].snapToZero();
        }
    }
    
}
