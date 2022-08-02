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
    this->_decayGain = other._decayGain;
    this->_delayLine = other._delayLine;
    this->_delayPerChannel = other._delayPerChannel;
    this->_delayTimeMs = other._delayTimeMs;
    this->_scratchChannelStep = other._scratchChannelStep;
    this->_spec = other._spec;
    this->_waveShaper = other._waveShaper;
}

Feedback::~Feedback()
{
}

void Feedback::SetupLowPassFilter(float cutoff)
{
    _freqDependentDecay.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(_spec.sampleRate, cutoff);
    //_freqDependentDecay.reset();
}

void Feedback::SetupDecayGain(float rt60)
{
    float loopMs = _delayTimeMs * 1.5f;
    float loopsPerRt60 = rt60 / (loopMs * 0.001f);

    float dbPerCycle = -60 / loopsPerRt60;

    _decayGain = std::powf(10.f, dbPerCycle * 0.05f);
}

void Feedback::prepare(juce::dsp::ProcessSpec& spec, float delayTimeMs)
{

    this->_spec = spec;
    this->_delayTimeMs = delayTimeMs;

    this->_delayLine.prepare(_spec);
    this->_delayLine.setMaximumDelayInSamples(static_cast<int>(std::floor(_delayTimeMs * 0.001 * _spec.sampleRate)));

    srand(time(nullptr));
    this->_scratchChannelStep = std::vector<float>(_spec.numChannels, { 0.f });
    this->_dryChannelStep = std::vector<float>(_spec.numChannels, { 0.f });
    this->_delayPerChannel = std::vector<int>(_spec.numChannels, { 0 });
    for (int i = 0; i < _spec.numChannels; ++i)
    {

        int rangeLow = static_cast<int>(std::floor((float)this->_delayLine.getMaximumDelayInSamples() * (float)i / _spec.numChannels));
        int rangeHigh = static_cast<int>(std::floor((float)this->_delayLine.getMaximumDelayInSamples() * (float)(i + 1) / _spec.numChannels));
        this->_delayPerChannel[i] = random_between_int(rangeLow, rangeHigh);
    }

    _freqDependentDecay.prepare(this->_spec);
    SetupLowPassFilter(3e3f);
    _freqDependentDecay.reset();
    SetupDecayGain(delayTimeMs);

    this->_pitchShifter.setShiftSemitones(12.f);
    this->_pitchShifter.prepare(this->_spec);
    _waveShaper.functionToUse = softclipper;
}


// In the feedback loop, the order is
// Pitch shift -> waveshaper -> filter -> decay -> mix
void Feedback::process(juce::AudioBuffer<float>& buffer, bool shimmerOn, float driveGain, float rt60, float lowPassCutoff)
{
    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();
    auto** readPointers = buffer.getArrayOfReadPointers();
    auto** writePointers = buffer.getArrayOfWritePointers();

    SetupLowPassFilter(lowPassCutoff);
    SetupDecayGain(rt60);

    /*this->_pitchShifter.setShiftSemitones(12.f);
    this->_pitchShifter.prepare(this->_spec);*/

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            // Fill dry buffer, because readonly pointers are affected by the writeable pointers...
            _dryChannelStep[ch] = readPointers[ch][sample];

            // Read from Delay line
            _scratchChannelStep[ch] = _delayLine.popSample(ch, this->_delayPerChannel[ch]);
            
            // Apply filter
            _scratchChannelStep[ch] = _freqDependentDecay.processSample(_scratchChannelStep[ch]);

            // Write to output
            // WARNING: Does writing to the write pointers affect read pointers? YES IT DOES, FFS.
            writePointers[ch][sample] = _scratchChannelStep[ch];

            // pitch shifter
            _scratchChannelStep[ch] += (int)shimmerOn * _pitchShifter.processSample(ch, _scratchChannelStep[ch]);

            // non-linearity
            _scratchChannelStep[ch] += _waveShaper.processSample(driveGain * _scratchChannelStep[ch]);
            _scratchChannelStep[ch] /= 2.f;


            // decay gain
            _scratchChannelStep[ch] *= _decayGain;

        }

        // apply mix matrix
        InPlaceHouseholderMix<float>(_scratchChannelStep.data(), numChannels);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            _scratchChannelStep[ch] = _scratchChannelStep[ch] + _dryChannelStep[ch];
            _scratchChannelStep[ch] /= 2.f;

            // feed back into the delay line
            _delayLine.pushSample(ch, _scratchChannelStep[ch]);
        }
    }
    
}
