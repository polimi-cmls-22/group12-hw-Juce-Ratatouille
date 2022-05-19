/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
#define SAMPLE_RATE (44100)
#ifndef M_PI
#define M_PI (3.14159265)
#endif


/**
*/
class WahAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    WahAudioProcessor();
    ~WahAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    float getRmsValue(const int channel) const;
    
    void set_wet(float val);
    void set_dry(float val);
    void set_depth(float val);
    void set_quality(float val);
    void set_decay(float val);
    void set_attack(float val);
    void set_gain(float val);
    void set_mode(int val);
    void set_filter(int val);
    void set_rate(float val);
    void set_firstVowel(int val);
    void set_secondVowel(int val);
    void toggle_humanizer();

private:
    
    float minFreq, maxFreq, freq, Q, atk, rel, G, dry, wet, depth, rate;
    int filter, mode, firstVowel, secondVowel;
    bool humanizerOn;
    
    float formants [5][2] = {{1000.0,1400.0},{500.0,2300.0},{320.0,2500.0},{500.0,1000.0},{320.0,800.0}}; // A E I O U
    
    // Time in samples
    juce::Array<int> sample;
    
    // Envelope
    juce::Array<float> env;
    
    // State variable filter
    juce::Array<float> yh, yb, yl;
    juce::Array<float> yh1, yb1, yl1, yh2, yb2, yl2;
    
    juce::LinearSmoothedValue<float> rmsLevelLeft, rmsLevelRight;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WahAudioProcessor)
};
