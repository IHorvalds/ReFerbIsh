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
                       ),
        feedback(reverbChannels),
        diffusion(reverbChannels, 5)
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
    return 1.0;
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

    float delayTime = m_apvts.getRawParameterValue(DELAYTIME_PARAM)->load();
    diffusion.prepare(spec, delayTime);
    feedback.prepare(spec, delayTime);
    processBuffer.setSize(reverbChannels, samplesPerBlock);

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
    juce::ignoreUnused (midiMessages);
    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    int ch = 0; // only left channel for now. TODO: add right channel too

    const float* readPointer = buffer.getReadPointer(ch);
    float* writePointer = buffer.getWritePointer(ch);

    // split channel into multiple channels
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for(int i = 0; i < reverbChannels; ++i)
        {
            processBuffer.setSample(i, sample, readPointer[sample]);
        }
    }

    // Apply Diffusion steps
    diffusion.process(processBuffer);

    // add the shortcuts from the diffusion steps
    /*for (int i = 0; i < diffusion.getNumSteps(); ++i)
    {
        juce::AudioBuffer<float>& shortcut = diffusion.getShortcutAudio(i);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int diffChannel = 0; diffChannel < 16; ++diffChannel)
            {
                processBuffer.addSample(diffChannel, sample, shortcut.getReadPointer(diffChannel)[sample]);
            }
        }
    }*/

    // Apply feedback loop
    float driveGain = m_apvts.getRawParameterValue(DRIVEGAIN_PARAM)->load();
    float rt60 = m_apvts.getRawParameterValue(RT60_PARAM)->load();
    float cutoff = m_apvts.getRawParameterValue(LOWPASS_PARAM)->load();
    feedback.process(processBuffer, false, driveGain, rt60, cutoff);

    float dryWet = m_apvts.getRawParameterValue(DRYWET_PARAM)->load();

    // mix back down to one channel
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float delayed = 0.f;
        for (int i = 0; i < reverbChannels; ++i)
        {
            delayed += processBuffer.getReadPointer(i)[sample];
        }
        writePointer[sample] = readPointer[sample] * dryWet + delayed * ( 1.f - dryWet );
    }
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

    layout.add(std::make_unique<juce::AudioParameterFloat>(RT60_PARAM, RT60_PARAM, 1.f, 10.f, 5.f));

    layout.add(std::make_unique<juce::AudioParameterBool>(HOLD_PARAM, HOLD_PARAM, false));

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