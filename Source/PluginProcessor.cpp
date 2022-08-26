#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ReferbishAudioProcessor::ReferbishAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
}

ReferbishAudioProcessor::~ReferbishAudioProcessor()
{
}

//==============================================================================
const juce::String ReferbishAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ReferbishAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ReferbishAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ReferbishAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ReferbishAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ReferbishAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ReferbishAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ReferbishAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String ReferbishAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void ReferbishAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void ReferbishAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.numChannels = reverbChannels;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;

    float maxDelayTime = 1e3f;
    /*diffusion.prepare(spec, maxDelayTime);
    feedback.prepare(spec, maxDelayTime);*/

    leftChain.get<ChainPosition::DiffusionPosition>().maxDiffusionDelayMs = maxDelayTime;
    rightChain.get<ChainPosition::DiffusionPosition>().maxDiffusionDelayMs = maxDelayTime;
    leftChain.get<ChainPosition::FeedbackPosition>().maxDelayTimeMs = maxDelayTime;
    rightChain.get<ChainPosition::FeedbackPosition>().maxDelayTimeMs = maxDelayTime;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    int channels = std::max(this->getTotalNumOutputChannels(), this->getTotalNumInputChannels());
    channelStep = std::vector<float>(channels, { 0.f });
    leftProcessBuffer.setSize(reverbChannels, samplesPerBlock); // we will hold all channels in a single buffer. i.e. for each channel in channels we hold (reverb channels)
    rightProcessBuffer.setSize(reverbChannels, samplesPerBlock); // we will hold all channels in a single buffer. i.e. for each channel in channels we hold (reverb channels)
    

    /*diffusion = std::vector<Diffusion>();
    feedback = std::vector<Feedback>();
    for (int i = 0; i < getNumInputChannels(); ++i)
    {
        processBuffers.push_back(juce::AudioBuffer<float>(16, samplesPerBlock));
        diffusion.push_back(std::move(Diffusion(spec.numChannels, 3)));
        feedback.push_back(std::move(Feedback(spec.numChannels)));
    }

    for (int i = 0; i < getNumInputChannels(); ++i)
    {
        diffusion[i].prepare(spec, 80);
        feedback[i].prepare(spec, 80);
    }*/
}

void ReferbishAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool ReferbishAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void ReferbishAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{

    juce::ScopedNoDenormals noDenormals;

    juce::ignoreUnused (midiMessages);
    int numSamples = buffer.getNumSamples();

    // parameters
    float driveGain     = m_apvts.getRawParameterValue(DRIVEGAIN_PARAM)->load();
    float rt60          = m_apvts.getRawParameterValue(RT60_PARAM)->load();
    float cutoff        = m_apvts.getRawParameterValue(LOWPASS_PARAM)->load();
    bool shouldHold     = m_apvts.getRawParameterValue(HOLD_PARAM)->load();
    bool shimmer        = m_apvts.getRawParameterValue(SHIMMER_PARAM)->load();
    float shimmerAmount = m_apvts.getRawParameterValue(SHIMMERAMOUNT_PARAM)->load();
    float dryWet        = m_apvts.getRawParameterValue(DRYWET_PARAM)->load();
    float delayInMs     = m_apvts.getRawParameterValue(DELAYTIME_PARAM)->load();
    bool channelMix     = m_apvts.getRawParameterValue(CHANNEL_MIX_PARAM)->load();
    bool bypass         = m_apvts.getRawParameterValue(BYPASS_PARAM)->load();

    if (!bypass)
    {

        leftChain.get<ChainPosition::DiffusionPosition>().diffusionDelayMs  = delayInMs;
        rightChain.get<ChainPosition::DiffusionPosition>().diffusionDelayMs = delayInMs;

        leftChain.get<ChainPosition::FeedbackPosition>().updateParameters(delayInMs, shimmer, shimmerAmount, driveGain, shouldHold, rt60, cutoff);
        rightChain.get<ChainPosition::FeedbackPosition>().updateParameters(delayInMs, shimmer, shimmerAmount, driveGain, shouldHold, rt60, cutoff);



        // split channel into multiple channels
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float* readPointerLeft = buffer.getReadPointer(0);
            const float* readPointerRight = buffer.getReadPointer(1);
            for(int i = 0; i < reverbChannels; ++i)
            {
                leftProcessBuffer.setSample(i, sample, readPointerLeft[sample]);
                rightProcessBuffer.setSample(i, sample, readPointerRight[sample]);
            }
        }


        leftChain.get<ChainPosition::DiffusionPosition>().processBuffer(leftProcessBuffer);
        rightChain.get<ChainPosition::DiffusionPosition>().processBuffer(rightProcessBuffer);

        leftChain.get<ChainPosition::FeedbackPosition>().processBuffer(leftProcessBuffer);
        rightChain.get<ChainPosition::FeedbackPosition>().processBuffer(rightProcessBuffer);

        for (int sample = 0; sample < numSamples; ++sample)
        {

            float delayedLeft = 0.f;
            float delayedRight = 0.f;
            for (int i = 0; i < reverbChannels; ++i)
            {
                delayedLeft += leftProcessBuffer.getReadPointer(i)[sample];
                delayedRight += rightProcessBuffer.getReadPointer(i)[sample];
            }
            
            channelStep[0] = delayedLeft;
            channelStep[1] = delayedRight;

            if (channelMix)
            {
                Utilities::InPlaceHouseholderMix<float>(channelStep);
            }

            // TODO: remove the * dryWet so we don't lose volume
            float* leftWritePointer = buffer.getWritePointer(0);
            leftWritePointer[sample] = leftWritePointer[sample] * dryWet + channelStep[0] * (1.f - dryWet);
            
            float* rightWritePointer = buffer.getWritePointer(1);
            rightWritePointer[sample] = rightWritePointer[sample] * dryWet + channelStep[1] * (1.f - dryWet);
        }
    }

    // TODO: Have a choice whether to mix channels together or output separately
    // 
    // 
    //for (int ch = 0; ch < numChannels; ++ch)
    //{
    //    const float* readPointer = buffer.getReadPointer(ch);
    //    float* writePointer = buffer.getWritePointer(ch);

    //    // split channel into multiple channels
    //    for (int sample = 0; sample < numSamples; ++sample)
    //    {
    //        for(int i = 0; i < reverbChannels; ++i)
    //        {
    //            processBuffer.setSample(i, sample, readPointer[sample]);
    //        }
    //    }

    //    diffusion.process(processBuffer, delayInMs);
    //    DBG(delayInMs);

    //    // Apply feedback loop
    //    feedback.process(processBuffer, delayInMs, shimmer, shimmerAmount, driveGain, shouldHold, rt60, cutoff);


    //    // mix back down to one channel
    //    for (int sample = 0; sample < numSamples; ++sample)
    //    {
    //        float delayed = 0.f;
    //        for (int i = 0; i < reverbChannels; ++i)
    //        {
    //            delayed += processBuffer.getReadPointer(i)[sample];
    //        }
    //        //// TODO: remove the * dryWet so we don't lose volume
    //        //writePointer[sample] = readPointer[sample] * dryWet + delayed * ( 1.f - dryWet );
    //    }
    //}
}

//==============================================================================
bool ReferbishAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ReferbishAudioProcessor::createEditor()
{
    // TODO: Actual editor
    return new GenericAudioProcessorEditor (*this);
}

//==============================================================================
void ReferbishAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void ReferbishAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

juce::AudioProcessorValueTreeState::ParameterLayout ReferbishAudioProcessor::CreateParameterLayout()
{
    auto layout = juce::AudioProcessorValueTreeState::ParameterLayout();

    layout.add(std::make_unique<juce::AudioParameterBool>(BYPASS_PARAM, BYPASS_PARAM, false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(DELAYTIME_PARAM, DELAYTIME_PARAM, 5.f, 1e3f, 200.f));

    //layout.add(std::make_unique<juce::AudioParameterFloat>(MODFREQ_PARAM, MODFREQ_PARAM, 0.5f, 10.f, 3.f));
    //
    //layout.add(std::make_unique<juce::AudioParameterBool>(MODULATE_PARAM, MODULATE_PARAM, false));
    //
    //layout.add(std::make_unique<juce::AudioParameterFloat>(MODAMP_PARAM, MODAMP_PARAM, 0.001f, 0.1f, 0.05f));

    layout.add(std::make_unique<juce::AudioParameterBool>(CHANNEL_MIX_PARAM, CHANNEL_MIX_PARAM, false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(RT60_PARAM, RT60_PARAM, 1.f, 1000.f, 5.f));

    layout.add(std::make_unique<juce::AudioParameterBool>(HOLD_PARAM, HOLD_PARAM, false));

    layout.add(std::make_unique<juce::AudioParameterBool>(SHIMMER_PARAM, SHIMMER_PARAM, false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(SHIMMERAMOUNT_PARAM, SHIMMERAMOUNT_PARAM, 0.1f, .65f, 0.2f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(LOWPASS_PARAM, LOWPASS_PARAM, 200.f, 5e3f, 3e3f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(DRIVEGAIN_PARAM, DRIVEGAIN_PARAM, 1.f, 30.f, 2.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(DRYWET_PARAM, DRYWET_PARAM, 0.f, 1.f, 0.5f));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReferbishAudioProcessor();
}