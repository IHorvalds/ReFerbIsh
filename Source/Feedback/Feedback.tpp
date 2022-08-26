#pragma once

inline void Feedback::setupLowPassFilter(float cutoff, int channel)
{
    //_freqDependentDecay[channel].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(_spec.sampleRate, cutoff);
    _freqDependentDecay[channel].coefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(cutoff, _spec.sampleRate, 1 * 2)[0]; // First order butterworth low pass
}

inline float Feedback::getDecayGain(float rt60, float delayTimeMs) const
{
    float loopMs = delayTimeMs * 1.5f;
    float loopsPerRt60 = rt60 / (loopMs * 0.001f);

    float dbPerCycle = -60 / loopsPerRt60;

    return std::powf(10.f, dbPerCycle * 0.05f);
}

inline int Feedback::getRandomDelayPerChannel(int delayInSamples, int channel) const
{
    int rangeLow = gsl::narrow_cast<int>(std::floor((float)delayInSamples * (float)channel / (float)_spec.numChannels));
    int rangeHigh = gsl::narrow_cast<int>(std::floor((float)delayInSamples * (float)(channel + 1) / (float)_spec.numChannels));
    return (rangeLow + rangeHigh) / 2; // intRandom.random_between(rangeLow, rangeHigh);
}

inline void Feedback::prepare(const juce::dsp::ProcessSpec& spec)
{

    this->_spec = spec;

    this->_delayLine.prepare(_spec);
    this->_delayLine.setMaximumDelayInSamples(static_cast<int>(std::floor(maxDelayTimeMs * 0.001 * _spec.sampleRate)));

    this->_scratchChannelStep   = std::vector<float>(_spec.numChannels, { 0.f });
    this->_dryChannelStep       = std::vector<float>(_spec.numChannels, { 0.f });
    this->_delayPerChannel      = std::vector<float>(_spec.numChannels, { 0.f });
    this->_decayPerChannel      = std::vector<float>(_spec.numChannels, { 0.f });
    this->_freqDependentDecay   = std::vector<Filter>(_spec.numChannels);
    
    juce::dsp::ProcessSpec localSpec = _spec;
    localSpec.numChannels = 1;

    Utilities::Random<int> intRandom;
    

    for (size_t i = 0; i < _spec.numChannels; ++i)
    {
        setupLowPassFilter(3e3f, gsl::narrow_cast<int>(i));
        /*int rangeLow = gsl::narrow_cast<int>(std::floor((float)this->_delayLine.getMaximumDelayInSamples() * (float)i / (float)_spec.numChannels));
        int rangeHigh = gsl::narrow_cast<int>(std::floor((float)this->_delayLine.getMaximumDelayInSamples() * (float)(i + 1) / (float)_spec.numChannels));
        this->_delayPerChannel[i] = intRandom.random_between(rangeLow, rangeHigh);*/

        _freqDependentDecay[i].prepare(localSpec);
        _freqDependentDecay[i].reset();
    }

    this->_pitchShifter.prepare(this->_spec);
    this->_pitchShifter.reset();
    this->_pitchShifter.setShiftSemitones(12.f);
    _waveShaper.functionToUse = Utilities::softclipper;
    _waveShaper.prepare(this->_spec);
}




// In the feedback loop, the order is
// Pitch shift -> waveshaper -> filter -> decay -> mix
inline void Feedback::processBuffer(juce::AudioBuffer<float>& buffer /*float delayTimeMs, bool shimmerOn, float shimmerGain, float driveGain, bool hold, float rt60, float lowPassCutoff*/)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    float** writePointers = buffer.getArrayOfWritePointers();

    int delayInSamples = gsl::narrow_cast<int>(_delayTimeMs * 0.001f * gsl::narrow_cast<float>(_spec.sampleRate));
    //setRandomDelayPerChannel(delayInSamples);


    /*this->_pitchShifter.setShiftSemitones(12.f);
    this->_pitchShifter.prepare(this->_spec);*/

    for (int ch = 0; ch < numChannels; ++ch)
    {
        setupLowPassFilter(_lowPassCutoff, ch);
        _delayPerChannel[ch] = static_cast<float>(getRandomDelayPerChannel(delayInSamples, ch));
        _decayPerChannel[ch] = getDecayGain(_rt60, _delayPerChannel[ch] * 1e3f * gsl::narrow_cast<float>(_spec.sampleRate));
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            
            float _delay = _delayPerChannel[ch];

            // Fill dry buffer, because readonly pointers are affected by the writeable pointers...
            _dryChannelStep[ch] = writePointers[ch][sample];

            // Read from Delay line
            _scratchChannelStep[ch] = _delayLine.popSample(ch, (float)_delay);

#if DEBUG && DEBUG_PRINTS
            DBG("feedback delay" + juce::String(_delay));
#endif

            // Write to output
            // WARNING: Does writing to the write pointers affect read pointers? YES IT DOES, FFS.
            writePointers[ch][sample] = _scratchChannelStep[ch];


            // decay gain, if not hold
            if (!_hold)
            {
                _scratchChannelStep[ch] *= _decayPerChannel[ch];
            }

                
            // pitch shifter
            _scratchChannelStep[ch] += _shimmerOn * _shimmerGain * _pitchShifter.processSample(ch, writePointers[ch][sample]);

            // non-linearity
            _scratchChannelStep[ch] = _waveShaper.processSample(_driveGain * _scratchChannelStep[ch]);
            //_scratchChannelStep[ch] /= 2.f;



            // Apply filter
            _scratchChannelStep[ch] = _freqDependentDecay[ch].processSample(_scratchChannelStep[ch]);

        }

        // apply mix matrix
        Utilities::InPlaceHouseholderMix<float>(_scratchChannelStep);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            _scratchChannelStep[ch] = _scratchChannelStep[ch] + _dryChannelStep[ch];
            //_scratchChannelStep[ch] /= 2.f;
            jlimit(-1.5f, 1.5f, _scratchChannelStep[ch]);

            // feed back into the delay line
            _delayLine.pushSample(ch, _scratchChannelStep[ch]);

        }
    }

    for (int ch = 0; ch < numChannels; ++ch)
    {
        _freqDependentDecay[ch].snapToZero();
    }
    
}

template <typename ProcessContext>
void Feedback::process(const ProcessContext& context /*float delayTimeMs, bool shimmerOn, float shimmerGain, float driveGain, bool hold, float rt60, float lowPassCutoff*/)
{
    auto&& inputBlock = context.getInputBlock();
    auto&& outputBlock = context.getOutputBlock();

    int inputChannels = inputBlock.getNumChannels();
    int outputChannels = outputBlock.getNumChannels();

    int inputNumSamples = inputBlock.getNumSamples();
    int outputNumSamples = outputBlock.getNumSamples();

    jassert(inputChannels == outputChannels);
    jassert(inputNumSamples == outputNumSamples);

    int delayInSamples = gsl::narrow_cast<int>(_delayTimeMs * 0.001f * gsl::narrow_cast<float>(_spec.sampleRate));

    for (int ch = 0; ch < inputChannels; ++ch)
    {
        setupLowPassFilter(_lowPassCutoff, ch);
        _delayPerChannel[ch] = static_cast<float>(getRandomDelayPerChannel(delayInSamples, ch));
        _decayPerChannel[ch] = getDecayGain(_rt60, _delayPerChannel[ch] * 1e3f * gsl::narrow_cast<float>(_spec.sampleRate));
    }

    for (size_t sample = 0; sample < inputNumSamples; ++sample)
    {
        for (int ch = 0; ch < inputChannels; ++ch)
        {
            const float* readPointer = inputBlock.getChannelPointer(ch);
            float* writePointer = outputBlock.getChannelPointer(ch);

            float _delay = _delayPerChannel[ch];

            // Fill dry buffer, because readonly pointers are affected by the writeable pointers...
            _dryChannelStep[ch] = readPointer[sample];

            // Read from Delay line
            _scratchChannelStep[ch] = _delayLine.popSample(ch, _delay);

#if DEBUG && DEBUG_PRINTS
            DBG("feedback delay" + juce::String(_delay));
#endif

            // Write to output
            // WARNING: Does writing to the write pointers affect read pointers? YES IT DOES, FFS.
            writePointer[sample] = _scratchChannelStep[ch];


            // decay gain, if not hold
            if (!_hold)
            {
                _scratchChannelStep[ch] *= _decayPerChannel[ch];
            }


            // pitch shifter
            _scratchChannelStep[ch] += _shimmerOn * _shimmerGain * _pitchShifter.processSample(ch, writePointer[sample]);

            // non-linearity
            _scratchChannelStep[ch] = _waveShaper.processSample(_driveGain * _scratchChannelStep[ch]);
            //_scratchChannelStep[ch] /= 2.f;



            // Apply filter
            _scratchChannelStep[ch] = _freqDependentDecay[ch].processSample(_scratchChannelStep[ch]);

        }

        // apply mix matrix
        Utilities::InPlaceHouseholderMix<float>(_scratchChannelStep);

        for (int ch = 0; ch < inputChannels; ++ch)
        {
            _scratchChannelStep[ch] = _scratchChannelStep[ch] + _dryChannelStep[ch];
            //_scratchChannelStep[ch] /= 2.f;
            jlimit(-1.5f, 1.5f, _scratchChannelStep[ch]);

            // feed back into the delay line
            _delayLine.pushSample(ch, _scratchChannelStep[ch]);
        }
    }

    for (int ch = 0; ch < inputChannels; ++ch)
    {
        _freqDependentDecay[ch].snapToZero();
    }
}
