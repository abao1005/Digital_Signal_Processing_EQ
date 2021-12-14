/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

MyAudioProcessor::MyAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                     #if ! JucePlugin_IsSynth
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     #endif
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                    ),
      tree(*this, nullptr, "PARAM", {
            std::make_unique<juce::AudioParameterFloat>(
                "level",
                "Level",
                juce::NormalisableRange<float>(0.0f, 10.0f, 0.5f), 0.0f,
                juce::String(),
                juce::AudioProcessorParameter::genericParameter,
                [](float value, int){ return juce::String(value); },
                [](juce::String text){ return text.getFloatValue(); }),
            std::make_unique<juce::AudioParameterFloat>(
                "f1",
                "F1",
                juce::NormalisableRange<float>(0.0f, 20000.0f, 1000.0f), 5000.0f,
                juce::String(),
                juce::AudioProcessorParameter::genericParameter,
                [](float value, int){ return juce::String(value); },
                [](juce::String text){ return text.getFloatValue(); }),
            std::make_unique<juce::AudioParameterFloat>(
                "f2",
                "F2",
                juce::NormalisableRange<float>(0.0f, 20000.0f, 1000.0f), 15000.0f,
                juce::String(),
                juce::AudioProcessorParameter::genericParameter,
                [](float value, int){ return juce::String(value); },
                [](juce::String text){ return text.getFloatValue(); }),
            std::make_unique<juce::AudioParameterFloat>(
                "order",
                "Order",
                juce::NormalisableRange<float>(15.0f, 99.0f, 2.0f), 15.0f,
                juce::String(),
                juce::AudioProcessorParameter::genericParameter,
                [](float value, int){ return juce::String(value); },
                [](juce::String text){ return text.getFloatValue(); }),
            std::make_unique<juce::AudioParameterChoice>(
                "mode",
                "Mode",
                juce::StringArray({ "none", "low-pass", "high-pass", "band-pass" }), 0)
            })
#endif
{
    mySynth.clearSounds();
    mySynth.addSound(new SynthSound());
    
    mySynth.clearVoices();
    for (int i = 0; i < 5; i++)
        mySynth.addVoice(new SynthVoice());
}

MyAudioProcessor::~MyAudioProcessor()
{
}

const juce::String MyAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MyAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MyAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MyAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MyAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MyAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MyAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MyAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MyAudioProcessor::getProgramName(int index)
{
    return {};
}

void MyAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

void MyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    lastSampleRate = sampleRate;
    mySynth.setCurrentPlaybackSampleRate(sampleRate);
    singleChannelSampleFifo.prepare(samplesPerBlock);
}

void MyAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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
#endif

void MyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    float newF1 = tree.getRawParameterValue("f1")->load();
    float newF2 = tree.getRawParameterValue("f2")->load();
    int newOrder = tree.getRawParameterValue("order")->load();
    int newMode = tree.getRawParameterValue("mode")->load();
    for (int i = 0; i < mySynth.getNumVoices(); i++) {
        auto* myVoice = dynamic_cast<SynthVoice*>(mySynth.getVoice(i));
        myVoice->setLevel(tree.getRawParameterValue("level")->load());
        if (myVoice->getF1() != newF1)
            myVoice->setF1(newF1);
        if (myVoice->getF2() != newF2)
            myVoice->setF2(newF2);
        if (myVoice->getOrder() != newOrder)
            myVoice->setOrder(newOrder);
        if (myVoice->getMode() != newMode)
            myVoice->setMode(newMode);
    }
    mySynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    singleChannelSampleFifo.update(buffer);
}

bool MyAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MyAudioProcessor::createEditor()
{
    return new MyAudioProcessorEditor(*this);
}

void MyAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void MyAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MyAudioProcessor();
}
