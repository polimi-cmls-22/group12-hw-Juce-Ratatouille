/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WahAudioProcessor::WahAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

WahAudioProcessor::~WahAudioProcessor()
{
}

//==============================================================================
const juce::String WahAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WahAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WahAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WahAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WahAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int WahAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int WahAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WahAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String WahAudioProcessor::getProgramName (int index)
{
    return {};
}

void WahAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void WahAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    minFreq = 350.0;
    maxFreq = 2000.0f;
    depth = 350.0;
    freq = (minFreq + maxFreq) / 2;
    alpha = 0.0f;
    mode = 0;
    rate = 1.0;
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    
	for (int i = 0; i < getTotalNumInputChannels(); i++)
    {
        env.add(0.0f);
        yh.add(0.0f);
        yb.add(0.0f);
        yl.add(0.0f);
        sample.add(0);
    }
}

void WahAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool WahAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void WahAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    rmsLevelLeft.skip(buffer.getNumSamples());
    rmsLevelRight.skip(buffer.getNumSamples());

    {
        const auto value = juce::Decibels::gainToDecibels(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));
        if (value < rmsLevelLeft.getCurrentValue())
            rmsLevelLeft.setTargetValue(value);
        else
            rmsLevelLeft.setCurrentAndTargetValue(value);
    }

    {
        const auto value = juce::Decibels::gainToDecibels(buffer.getRMSLevel(1, 0, buffer.getNumSamples()));
        if (value < rmsLevelRight.getCurrentValue())
            rmsLevelRight.setTargetValue(value);
        else
            rmsLevelRight.setCurrentAndTargetValue(value);
    }
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    int numSamples = buffer.getNumSamples();
    
    float alpha_atk = exp(-1.0f / (atk * getSampleRate()));
    float alpha_rel = exp(-1.0f / (rel * getSampleRate()));
    
    float wet_now = wet;
    float dry_now = dry;
    
    for (int channel = 0; channel < totalNumInputChannels; channel++)
    {
        const float* channelInData = buffer.getReadPointer(channel);
        float* channelOutData = buffer.getWritePointer (channel);
        
        float newEnv, normalizedEnv;
        float normalizedFreq, F;
        
        for (int i = 0; i < numSamples; i++)
        {
            const float x = G * channelInData[i];
            const float abs_x = fabs(x);
            
            // level detector
            if (abs_x > env[channel])
                alpha = alpha_atk;
            else
                alpha = alpha_rel;
            newEnv = alpha * env[channel] + (1.0f - alpha) * abs_x;
			env.set(channel, newEnv);
			
			// normalizedEnv = newEnv / range;
			// if(normalizedEnv > 1) normalizedEnv = 1.0f;
			
			// update freq and F
			float minFreq_now = minFreq;
			float maxFreq_now = maxFreq;
			if(minFreq_now > maxFreq_now)
            {
                float tmp = minFreq_now;
                minFreq_now = maxFreq_now;
                maxFreq_now = tmp;
            }
			if (mode == 0)
			    freq = ((minFreq_now + maxFreq_now) / 2) + ((maxFreq_now - minFreq_now) / 2) * sin (2 * M_PI * rate * sample[channel] / SAMPLE_RATE);
			else if (mode == 1)
			    freq = minFreq_now + (maxFreq_now - minFreq_now) * newEnv;
            sample.set(channel, sample[channel] + 1);

			normalizedFreq = freq / SAMPLE_RATE;
			F = 2 * sin(M_PI * normalizedFreq);
			
			// DBG ("input: " << x << ", normalizedEnv: " << normalizedEnv << ", freq: " << freq << ", attack: " << atk << ", alpha: " << alpha);
			
			// State variable filter
            yh.set(channel, x - yl[channel] - Q * yb[channel]);
            yb.set(channel, F * yh[channel] + yb[channel]);
            yl.set(channel, F * yb[channel] + yl[channel]);
            
            // output
            float y[] = { yl[channel], yb[channel], yh[channel] };
            channelOutData[i] = dry_now * x + wet_now * y[filter]; 
        }
    }
}

//==============================================================================
bool WahAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* WahAudioProcessor::createEditor()
{
    return new WahAudioProcessorEditor (*this);
}

//==============================================================================
void WahAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void WahAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

float WahAudioProcessor::getRmsValue(const int channel) const
{
    jassert(channel == 0 || channel == 1);
    if(channel == 0)
        return rmsLevelLeft.getCurrentValue();
    if(channel == 1)
        return rmsLevelRight.getCurrentValue();
    return 0.f;
}

void WahAudioProcessor::set_wet(float val)
{
    wet = val;
}

void WahAudioProcessor::set_dry(float val)
{
    dry = val;
}

void WahAudioProcessor::set_minFreq(float val)
{
    minFreq = val;
}

void WahAudioProcessor::set_depth(float val)
{
    depth = val;
}

void WahAudioProcessor::reset_freq()
{
    minFreq = depth;
    maxFreq = 2000.0;
}

void WahAudioProcessor::set_maxFreq(float val)
{
    maxFreq = val;
}

void WahAudioProcessor::set_quality(float val)
{
    Q = val;
}

void WahAudioProcessor::set_decay(float val)
{
    rel = val;
}

void WahAudioProcessor::set_attack(float val)
{
    atk = val;
}

void WahAudioProcessor::set_gain(float val)
{
    G = val;
}

void WahAudioProcessor::set_mode(int val)
{
    mode = val;
}

void WahAudioProcessor::set_filter(int val)
{
    filter = val;
}

void WahAudioProcessor::set_rate(float val)
{
    rate = val;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WahAudioProcessor();
}