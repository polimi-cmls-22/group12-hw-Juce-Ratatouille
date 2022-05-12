/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Components/HorizontalMeter.h"
#include "Components/VerticalGradientMeter.h"

//==============================================================================
/**
*/


class RotarySliderLookandFeel : public juce::LookAndFeel_V4
{
    public:
        RotarySliderLookandFeel()
        {
            
            setColour(juce::Slider::ColourIds::rotarySliderFillColourId, juce::Colours::red); 
            setColour(juce::Slider::ColourIds::thumbColourId, juce::Colours::powderblue);
            setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, juce::Colours::powderblue);
            setColour(juce::Slider::ColourIds::textBoxOutlineColourId, juce::Colours::black);
            setColour(juce::Slider::ColourIds::textBoxTextColourId, juce::Colours::red);
        }
};

class KnobLookandFeel : public juce::LookAndFeel_V4
{
public:
        void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider &slider) override
        {
            float radius = juce::jmin(width / 2, height / 2) - 2.0f;
            float diameter = 2 * radius;
            float centerX = x + width * 0.5f;
            float centerY = y + height * 0.5f;
            float rx = centerX - radius;
            float ry = centerY - radius;
            float angle = rotaryStartAngle + (sliderPos * (rotaryEndAngle - rotaryStartAngle));

            juce::Rectangle<float> dialArea(rx, ry, diameter, diameter);

            g.setColour(juce::Colours::silver.brighter(0.2f));
            //g.drawRect(dialArea);
            g.fillEllipse(dialArea);

            g.setColour(juce::Colours::red);
            //g.fillEllipse(centerX, centerY, 5, 5);

            juce::Path dialTick;
            dialTick.addRectangle(0, -radius, 10.0f, radius * 0.33);

            g.fillPath(dialTick, juce::AffineTransform::rotation(angle).translated(centerX, centerY));

            g.setColour(juce::Colours::dimgrey);
            g.drawEllipse(rx, ry, diameter, diameter, 5.0f);
        }
};


class WahAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer, public juce::Slider::Listener, public juce::ComboBox::Listener
{
public:
    WahAudioProcessorEditor (WahAudioProcessor&);
    ~WahAudioProcessorEditor() override;

    //==============================================================================
    void timerCallback() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void humanizerButtonToggle();
    void firstVowelSelection();
    void secondVowelSelection();
    void modeSelection();
    void tapClicked();
    //void colouredVowels();

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    WahAudioProcessor& audioProcessor;

    RotarySliderLookandFeel rotarySliderLookandFeel;
    KnobLookandFeel knobLookandFeel;

    juce::Slider rangeSlider;
    juce::Slider qualitySlider;
    juce::Slider decaySlider;
    juce::Slider attackSlider;
    juce::Slider mixSlider;
    juce::Slider gainSlider;

    juce::Label rangeLabel;
    juce::Label qualityLabel;
    juce::Label decayLabel;
    juce::Label attackLabel;
    juce::Label mixLabel; 
    juce::Label gainLabel;

    juce::ImageButton humanizerButton;
    juce::Label humanizerLabel;

    juce::ComboBox filterType;

    juce::TextButton firstVowel;
    juce::TextButton secondVowel;
    
    juce::TextButton mode;

    juce::TextButton tapButton;
    juce::Label interval_ms;

    Gui::VerticalGradientMeter verticalGradientMeterL, verticalGradientMeterR;
    Gui::HorizontalMeter horizontalMeterL, horizontalMeterR;

    char vowel[5] = { 'A', 'E', 'I', 'O', 'U' };
    char firstCurrentVowel = 'A';
    char secondCurrentVowel = 'A';
    int currentMode = 0;

    double newTime;
    double oldTime;
    double diff;
    double diffBPM;
    int num_click = 0;
    
    void sliderValueChanged(juce::Slider* slider) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WahAudioProcessorEditor)
};
