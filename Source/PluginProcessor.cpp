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
    auto totalNumInputChannels = getTotalNumInputChannels();
    
	for (int i = 0; i < getTotalNumInputChannels(); i++)
    {
        env.add(0.0f);
        yh.add(0.0f);
        yb.add(0.0f);
        yl.add(0.0f);
        yh1.add(0.0f);
        yb1.add(0.0f);
        yl1.add(0.0f);
        yh2.add(0.0f);
        yb2.add(0.0f);
        yl2.add(0.0f);
        sample.add(0);
    }
    
    minFreq = 350.0f;
    depth = 1700.0f;
    maxFreq = minFreq + depth;
    freq = (minFreq + maxFreq) / 2;
    Q = 0.1;
    rate = 1.0;
    G = 1.0;
    wet = 0.5;
    dry = 0.5;
    firstVowel = 0;
    secondVowel = 0;
    humanizerOn = false;
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
    
    float alpha;
    float alpha_atk = exp(-1.0f / (atk * getSampleRate()));
    float alpha_rel = exp(-1.0f / (rel * getSampleRate()));
    
    float wet_now = wet;
    float dry_now = dry;
    
    maxFreq = minFreq + depth;
    
    for (int channel = 0; channel < totalNumInputChannels; channel++)
    {
        const float* channelInData = buffer.getReadPointer(channel);
        float* channelOutData = buffer.getWritePointer (channel);
        
        float newEnv;
        float normalizedFreq, F;
        
        for (int i = 0; i < numSamples; i++)
        {
            const float x = channelInData[i];
            const float abs_x = fabs(x);
            
            // level detector
            if (abs_x > env[channel])
                alpha = alpha_atk;
            else
                alpha = alpha_rel;
            
            newEnv = alpha * env[channel] + (1.0f - alpha) * abs_x;
			env.set(channel, newEnv);
            
			if(humanizerOn)
            {
                float firstVowelFirstFormant = formants[firstVowel][0];
                float firstVowelSecondFormant = formants[firstVowel][1];
                float secondVowelFirstFormant = formants[secondVowel][0];
                float secondVowelSecondFormant = formants[secondVowel][1];
                
                float freq1, freq2;
                
                // tempo
			    if (mode == 0)
                {
                    freq1 = ((firstVowelFirstFormant + secondVowelFirstFormant) / 2) + ((secondVowelFirstFormant - firstVowelFirstFormant) / 2) * sin (2 * M_PI * rate * sample[channel] / SAMPLE_RATE);
                    freq2 = ((firstVowelSecondFormant + secondVowelSecondFormant) / 2) + ((secondVowelSecondFormant - firstVowelSecondFormant) / 2) * sin (2 * M_PI * rate * sample[channel] / SAMPLE_RATE);
                }
			    // dynamic
			    else if (mode == 1)
                {
                    freq1 = firstVowelFirstFormant + (secondVowelFirstFormant - firstVowelFirstFormant) * newEnv;
                    freq2 = firstVowelSecondFormant + (secondVowelSecondFormant - firstVowelSecondFormant) * newEnv;
                }
                
                // update F
                float F1 = 2 * sin(M_PI * freq1 / SAMPLE_RATE);
                float F2 = 2 * sin(M_PI * freq2 / SAMPLE_RATE);
			    
			    // state variable filter
                yh1.set(channel, x - yl1[channel] - Q * yb1[channel]);
                yb1.set(channel, F1 * yh1[channel] + yb1[channel]);
                yl1.set(channel, F1 * yb1[channel] + yl1[channel]);
                
                yh2.set(channel, x - yl2[channel] - Q * yb2[channel]);
                yb2.set(channel, F2 * yh2[channel] + yb2[channel]);
                yl2.set(channel, F2 * yb2[channel] + yl2[channel]);
                
                float ylHumanizer = yl1[channel]/2.0 + yl2[channel]/2.0;
                float ybHumanizer = yb1[channel]/2.0 + yb2[channel]/2.0;
                float yhHumanizer = yh1[channel]/2.0 + yh2[channel]/2.0;
                
                // output
                float yHumanizer[] = { ylHumanizer, ybHumanizer, yhHumanizer };
                channelOutData[i] = G * (dry_now * x + wet_now * yHumanizer[filter]);
            }
            else
            {
                // tempo
			    if (mode == 0)
			        freq = ((minFreq + maxFreq) / 2) + ((maxFreq - minFreq) / 2) * sin (2 * M_PI * rate * sample[channel] / SAMPLE_RATE);
			    // dynamic
			    else if (mode == 1)
			        freq = minFreq + (maxFreq - minFreq) * newEnv;
            
			    // update F
			    F = 2 * sin(M_PI * freq / SAMPLE_RATE);
			    
			    // state variable filter
                yh.set(channel, x - yl[channel] - Q * yb[channel]);
                yb.set(channel, F * yh[channel] + yb[channel]);
                yl.set(channel, F * yb[channel] + yl[channel]);
                
                // output
                float y[] = { yl[channel], yb[channel], yh[channel] };
                channelOutData[i] = G * (dry_now * x + wet_now * y[filter]);
            }
            
            sample.set(channel, sample[channel]+1);
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

void WahAudioProcessor::set_depth(float val)
{
    depth = val;
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

void WahAudioProcessor::set_firstVowel(int val)
{
    firstVowel = val;
}

void WahAudioProcessor::set_secondVowel(int val)
{
    secondVowel = val;
}

void WahAudioProcessor::toggle_humanizer()
{
    humanizerOn = !humanizerOn;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WahAudioProcessor();
}