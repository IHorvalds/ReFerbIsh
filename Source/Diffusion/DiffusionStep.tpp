#pragma once 

inline DiffusionStep::DiffusionStep(double maxDelayTimeMs, int index) :
    _maxDelayTimeMs(maxDelayTimeMs),
    _index(index) { }

inline DiffusionStep::DiffusionStep(const DiffusionStep& other)
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

inline DiffusionStep& DiffusionStep::operator=(const DiffusionStep& other)
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

inline void DiffusionStep::prepare(const juce::dsp::ProcessSpec& spec)
{
    this->_spec = spec;
    this->_delayLine.prepare(_spec);
    this->_delayLine.setMaximumDelayInSamples(gsl::narrow_cast<int>(std::floor(_maxDelayTimeMs * 0.001 * _spec.sampleRate)));


    this->_scratchChannelStep = std::vector<float>(_spec.numChannels, { 0.f });
    this->_channelMapping = std::vector<int>(_spec.numChannels, { 0 });
    this->_shouldFlip = std::vector<float>(_spec.numChannels, { 0.f });
    this->_delayPerChannel = std::vector<float>(_spec.numChannels, { 0.f });

    Utilities::Random<int> intRandom;
    
    for (size_t i = 0; i < _spec.numChannels; ++i)
    {
        this->_channelMapping[i] = gsl::narrow_cast<int>(i);
        this->_shouldFlip[i] = this->_rng.nextBool() ? 1.f : -1.f;

        //int rangeLow = gsl::narrow_cast<int>(std::floor((double)this->_delayLine.getMaximumDelayInSamples() * (double)i / _spec.numChannels));
        //int rangeHigh = gsl::narrow_cast<int>(std::floor((double)this->_delayLine.getMaximumDelayInSamples() * (double)(i + 1) / _spec.numChannels));
        //this->_delayPerChannel[i] = intRandom.random_between(rangeLow, rangeHigh);
    }

    intRandom.rand_permutation(this->_channelMapping);
}

inline int DiffusionStep::getRandomPerChannelDelay(int delayInSamples, int channel) const
{
    double rangeLow = std::floor((double)delayInSamples * (double)channel / (double)_spec.numChannels);
    double rangeHigh = std::floor((double)delayInSamples * (double)(channel + 1) / (double)_spec.numChannels);
        
    return gsl::narrow_cast<int>(rangeLow + rangeHigh) / 2;
}

inline void DiffusionStep::processBuffer(juce::AudioBuffer<float>& buffer, double delayTimeMs)
{

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    int delayInSamples = gsl::narrow_cast<int>(delayTimeMs * 0.001 * _spec.sampleRate);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        _delayPerChannel[ch] = static_cast<float>(getRandomPerChannelDelay(delayInSamples, ch));
    }

    float** writePointers = buffer.getArrayOfWritePointers();
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            // read from delay line
            float delayed = 0.f;
            delayed = this->_delayLine.popSample(ch, _delayPerChannel[ch]);

            // push new, clean signal into the delay line
            this->_delayLine.pushSample(ch, writePointers[ch][sample]);

            // fill channel wise buffer, with channel mapping and randomly flipped sample
            _scratchChannelStep[this->_channelMapping[ch]] = delayed * this->_shouldFlip[ch];
        }

        // Give it some whisky-business!
        Utilities::InPlaceHadamardMix<float>(this->_scratchChannelStep.data(), 0, numChannels);

        float scale = 3.f / (float)numChannels;

        // write back to current audio buffer
        for (int ch = 0; ch < numChannels; ++ch)
        {
            writePointers[ch][sample] = this->_scratchChannelStep[ch] * scale;
        }
    }

}

template <typename ProcessContext>
void DiffusionStep::process(const ProcessContext& context, double delayTimeMs)
{
    auto&& inputBlock = context.getInputBlock();
    auto&& outputBlock = context.getOutputBlock();

    int inputChannels = inputBlock.getNumChannels();
    int outputChannels = outputBlock.getNumChannels();

    int inputNumSamples = inputBlock.getNumSamples();
    int outputNumSamples = outputBlock.getNumSamples();

    jassert(inputChannels == outputChannels);
    jassert(inputNumSamples == outputNumSamples);

    int delayInSamples = gsl::narrow_cast<int>(delayTimeMs * 0.001 * _spec.sampleRate);

    for (int ch = 0; ch < inputChannels; ++ch)
    {
        _delayPerChannel[ch] = static_cast<float>(getRandomPerChannelDelay(delayInSamples, ch));
    }

    for (int sample = 0; sample < inputNumSamples; ++sample)
    {
        for (int ch = 0; ch < inputChannels; ++ch)
        {
            const float* readPointer = inputBlock.getChannelPointer(ch);
            
            // read from delay line
            float delayed = 0.f;
            delayed = this->_delayLine.popSample(ch, _delayPerChannel[ch]);

            // push new, clean signal into the delay line
            this->_delayLine.pushSample(ch, readPointer[sample]);

            // fill channel wise buffer, with channel mapping and randomly flipped sample
            _scratchChannelStep[this->_channelMapping[ch]] = delayed * this->_shouldFlip[ch];
        }

        // Give it some whisky-business!
        Utilities::InPlaceHadamardMix<float>(this->_scratchChannelStep.data(), 0, inputChannels);

        float scale = 3.f / (float)inputChannels;

        // write back to current audio buffer
        for (int ch = 0; ch < inputChannels; ++ch)
        {
            float* writePointer = outputBlock.getChannelPointer(ch);
            writePointer[sample] = this->_scratchChannelStep[ch] * scale;
        }
    }

}