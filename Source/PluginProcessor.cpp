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
    spec.numChannels = std::max(getNumInputChannels(), getNumOutputChannels());
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;

    channelStep = std::vector<float>(spec.numChannels, { 0.f });

    inputDiffusers = std::vector<SchroederAllpass<float>>(inputDiffuserCount);

    //float maxDelayTime = 6e3f;
    reverb.prepare(spec);

    for (int i = 0; i < inputDiffuserCount; ++i)
    {
        inputDiffusers[i].index = i;
        inputDiffusers[i].maxDiffusionDelayMs = 50.f;
        inputDiffusers[i].diffusionDelaySamples = Utilities::MillisecondsToSamples((float)(i+1) / (float)inputDiffuserCount * 40.f, sampleRate);
        inputDiffusers[i].m_gain = 0.5f;
        inputDiffusers[i].m_modulationAmplitude = 0;
        inputDiffusers[i].prepare(spec);
    }
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
    int numChannels = buffer.getNumChannels();

    // parameters
    //float driveGain     = m_apvts.getRawParameterValue(DRIVEGAIN_PARAM)->load();
    float rt60              = m_apvts.getRawParameterValue(RT60_PARAM)->load();
    float lowCutoff         = m_apvts.getRawParameterValue(LOWPASS_PARAM)->load();
    float highCutoff        = m_apvts.getRawParameterValue(HIGHPASS_PARAM)->load();
    //bool shouldHold     = m_apvts.getRawParameterValue(HOLD_PARAM)->load();
    //bool shimmer        = m_apvts.getRawParameterValue(SHIMMER_PARAM)->load();
    //float shimmerAmount = m_apvts.getRawParameterValue(SHIMMERAMOUNT_PARAM)->load();
    float dry           = m_apvts.getRawParameterValue(DRY_PARAM)->load();
    float wet           = m_apvts.getRawParameterValue(WET_PARAM)->load();
    float delayInMs     = m_apvts.getRawParameterValue(DELAYTIME_PARAM)->load();
    //bool channelMix     = m_apvts.getRawParameterValue(CHANNEL_MIX_PARAM)->load();
    float modFreq       = m_apvts.getRawParameterValue(MODFREQ_PARAM)->load();
    float modAmp        = m_apvts.getRawParameterValue(MODAMP_PARAM)->load();
    bool bypass         = m_apvts.getRawParameterValue(BYPASS_PARAM)->load();

    //bool stereo = numChannels == 2;

    if (!bypass)
    {
        reverb.krt = rt60;
        reverb.highPassFreq = highCutoff;
        reverb.lowPassFreq = lowCutoff;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float* readPointer = buffer.getReadPointer(ch);
                channelStep[ch] = readPointer[sample];
            }

            for (auto& diffuser : inputDiffusers)
            {
                diffuser.processStep(channelStep);
            }
            reverb.processStep(channelStep, delayInMs, 0.5f, modFreq, modAmp);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* writePointer = buffer.getWritePointer(ch);
                writePointer[sample] = writePointer[sample] * dry + channelStep[ch] * wet;
            }
        }
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
    juce::MemoryOutputStream mos(destData, true);
    m_apvts.state.writeToStream(mos);
}

void ReferbishAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);

    if (tree.isValid()) {
        m_apvts.replaceState(tree);
        return;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout ReferbishAudioProcessor::CreateParameterLayout()
{
    auto layout = juce::AudioProcessorValueTreeState::ParameterLayout();

    layout.add(std::make_unique<juce::AudioParameterBool>(BYPASS_PARAM, BYPASS_PARAM, false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(DELAYTIME_PARAM, DELAYTIME_PARAM, 150.f, 6e3f, 200.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(RT60_PARAM, RT60_PARAM, 0.01f, 0.99f, 0.05f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(MODFREQ_PARAM, MODFREQ_PARAM, 0.5f, 10.f, 3.f));
    
    //layout.add(std::make_unique<juce::AudioParameterBool>(MODULATE_PARAM, MODULATE_PARAM, false));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(MODAMP_PARAM, MODAMP_PARAM, 1.f, 10.f, 1.5f));

    //layout.add(std::make_unique<juce::AudioParameterBool>(CHANNEL_MIX_PARAM, CHANNEL_MIX_PARAM, false));

    //layout.add(std::make_unique<juce::AudioParameterBool>(HOLD_PARAM, HOLD_PARAM, false));

    //layout.add(std::make_unique<juce::AudioParameterBool>(SHIMMER_PARAM, SHIMMER_PARAM, false));

    //layout.add(std::make_unique<juce::AudioParameterFloat>(SHIMMERAMOUNT_PARAM, SHIMMERAMOUNT_PARAM, 0.1f, .65f, 0.2f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(LOWPASS_PARAM, LOWPASS_PARAM, 200.f, 5e3f, 3e3f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(HIGHPASS_PARAM, HIGHPASS_PARAM, 50.f, 5e3f, 200.f));

    //layout.add(std::make_unique<juce::AudioParameterFloat>(DRIVEGAIN_PARAM, DRIVEGAIN_PARAM, 1.f, 30.f, 2.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(DRY_PARAM, DRY_PARAM, 0.f, 1.f, 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(WET_PARAM, WET_PARAM, 0.f, 1.f, 0.5f));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReferbishAudioProcessor();
}