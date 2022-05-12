/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

/*
#ifndef M_PI
#define M_PI (3.14159265)
#endif */

//==============================================================================
WahAudioProcessorEditor::WahAudioProcessorEditor (WahAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    verticalGradientMeterL([&]() { return audioProcessor.getRmsValue(0); }),
    verticalGradientMeterR([&]() { return audioProcessor.getRmsValue(1); })
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    //juce::LookAndFeel::setDefaultLookAndFeel (&myCustomLNF)
    
    setSize (800, 700);

    startTimerHz(24);

    tapButton.setButtonText("TAP");
    tapButton.setSize(90, 90);
    tapButton.onClick = [this]() { tapClicked(); };

    interval_ms.setColour(juce::Label::backgroundColourId, juce::Colours::black);
    interval_ms.setColour(juce::Label::textColourId, juce::Colours::white);
    interval_ms.setJustificationType(juce::Justification::centred);
    interval_ms.setSize(250, 40);

    rangeSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    rangeSlider.setRange(270.0, 420.0, 10.0);
    rangeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 64, 30);
    rangeSlider.setLookAndFeel(&rotarySliderLookandFeel);
    rangeSlider.setValue(350.0);
    rangeSlider.addListener(this);

    qualitySlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    qualitySlider.setRange(0.01, 1.0, 0.01);
    qualitySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 64, 30);
    qualitySlider.setLookAndFeel(&rotarySliderLookandFeel);
    qualitySlider.setValue(0.1);
    qualitySlider.addListener(this);

    decaySlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    decaySlider.setRange(0.001, 1.0, 0.001);
    decaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 64, 30);
    decaySlider.setLookAndFeel(&rotarySliderLookandFeel);
    decaySlider.setValue(0.01);
    decaySlider.addListener(this);

    attackSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    attackSlider.setRange(0.0001, 0.01, 0.0001);
    attackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 64, 30);
    attackSlider.setLookAndFeel(&rotarySliderLookandFeel);
    attackSlider.setValue(0.001);
    attackSlider.addListener(this);

    mixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixSlider.setRange(0.0, 1.0, 0.1);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 64, 30);
    mixSlider.setLookAndFeel(&rotarySliderLookandFeel);
    mixSlider.setValue(0.5);
    mixSlider.addListener(this);

   /* dryWetSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    dryWetSlider.setRange(0.1, 100, 0.1);
    dryWetSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 64, 30);
    dryWetSlider.setLookAndFeel(&rotarySliderLookandFeel);
    dryWetSlider.setValue(50); */

    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setRange(0.0, 3.0, 0.05);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 64, 30);
    gainSlider.setLookAndFeel(&knobLookandFeel);
    gainSlider.setValue(1.0);
    gainSlider.addListener(this);

    rangeLabel.setText("Depth", juce::dontSendNotification);
    rangeLabel.attachToComponent(&rangeSlider, false);
    rangeLabel.setJustificationType(juce::Justification::centredTop);
   
    qualityLabel.setText("Quality", juce::dontSendNotification);
    qualityLabel.attachToComponent(&qualitySlider, false);
    qualityLabel.setJustificationType(juce::Justification::centredTop);

    decayLabel.setText("Decay", juce::dontSendNotification);
    decayLabel.attachToComponent(&decaySlider, false);
    decayLabel.setJustificationType(juce::Justification::centredTop);

    attackLabel.setText("Attack", juce::dontSendNotification);
    attackLabel.attachToComponent(&attackSlider, false);
    attackLabel.setJustificationType(juce::Justification::centredTop);

    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.attachToComponent(&mixSlider, false);
    mixLabel.setJustificationType(juce::Justification::centredTop);

    /*dryWetLabel.setText("Dry/Wet", juce::dontSendNotification);
    dryWetLabel.attachToComponent(&dryWetSlider, false);
    dryWetLabel.setJustificationType(juce::Justification::centredTop);*/

    gainLabel.setText("GAIN", juce::dontSendNotification);
    gainLabel.attachToComponent(&gainSlider, false);
    gainLabel.setJustificationType(juce::Justification::horizontallyCentred);
    gainLabel.setFont(juce::Font("Calibri", 20.0f, juce::Font::italic));

    humanizerLabel.setText("Humanizer", juce::dontSendNotification);
   // humanizerLabel.attachToComponent(&humanizerButton, false);
    humanizerLabel.setJustificationType(juce::Justification::centredTop);

    juce::Image openMouthImage = juce::ImageCache::getFromMemory(BinaryData::openMouth_png, BinaryData::openMouth_pngSize);
    juce::Image closeMouthImage = juce::ImageCache::getFromMemory(BinaryData::closeMouth_png, BinaryData::closeMouth_pngSize);
    
    humanizerButton.setClickingTogglesState(true);
    humanizerButton.setToggleState(false, juce::dontSendNotification);
    humanizerButton.setImages(true, true, true, closeMouthImage, 1.0f, {},closeMouthImage, 1.0f, {}, openMouthImage, 1.0f, {});
    humanizerButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
    humanizerButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    humanizerButton.setColour(juce::TextButton::textColourOnId, juce::Colours::green.darker(0.6));
    humanizerButton.setColour(juce::TextButton::textColourOffId, juce::Colours::red.darker(0.5));
    humanizerButton.onClick = [this]() { humanizerButtonToggle(); };

    filterType.setTextWhenNothingSelected("Select Filter Type");
    filterType.setJustificationType(juce::Justification::centred);
    filterType.setColour(juce::ComboBox::backgroundColourId, juce::Colours::steelblue.brighter(0.8f));
    filterType.setColour(juce::ComboBox::textColourId, juce::Colours::steelblue.darker(0.6));
    filterType.addItem("LowPass", 1);
    filterType.addItem("BandPass", 2);
    filterType.addItem("HighPass", 3);
    filterType.addListener(this);

    firstVowel.setToggleState(true, juce::dontSendNotification);
    firstVowel.setClickingTogglesState(false);
    firstVowel.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker(0.2f));
    firstVowel.setColour(juce::TextButton::textColourOnId, juce::Colours::green.darker(0.6));
    firstVowel.setColour(juce::TextButton::textColourOffId, juce::Colours::red.darker(0.5));
    firstVowel.setButtonText("A");
    firstVowel.onClick = [this]() { firstVowelSelection(); };

    secondVowel.setToggleState(true, juce::dontSendNotification);
    secondVowel.setClickingTogglesState(false);
    secondVowel.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker(0.2f));
    secondVowel.setColour(juce::TextButton::textColourOnId, juce::Colours::green.darker(0.6));
    secondVowel.setColour(juce::TextButton::textColourOffId, juce::Colours::red.darker(0.5));
    secondVowel.setButtonText("A");
    secondVowel.onClick = [this]() { secondVowelSelection(); };

    mode.setToggleState(true, juce::dontSendNotification);
    mode.setClickingTogglesState(false);
    mode.setColour(juce::TextButton::buttonOnColourId, juce::Colours::steelblue);
    mode.setColour(juce::TextButton::buttonColourId, juce::Colours::steelblue.brighter(0.8f));
    mode.setColour(juce::TextButton::textColourOnId, juce::Colours::steelblue.brighter(0.8f));
    mode.setColour(juce::TextButton::textColourOffId, juce::Colours::steelblue);
    mode.setButtonText("SELECT MODE");
    mode.onClick = [this]() { modeSelection(); };

    addAndMakeVisible(rangeSlider);
    addAndMakeVisible(qualitySlider);
    addAndMakeVisible(decaySlider);
    addAndMakeVisible(attackSlider);
    addAndMakeVisible(mixSlider);
    //addAndMakeVisible(dryWetSlider);
    addAndMakeVisible(gainSlider);

    addAndMakeVisible(rangeLabel);
    addAndMakeVisible(qualityLabel);
    addAndMakeVisible(decayLabel);
    addAndMakeVisible(attackLabel);
    addAndMakeVisible(mixLabel);
    //addAndMakeVisible(dryWetLabel);
    addAndMakeVisible(gainLabel);

    addAndMakeVisible(humanizerButton);
    addAndMakeVisible(humanizerLabel);

    addAndMakeVisible(filterType);

    addAndMakeVisible(firstVowel);
    addAndMakeVisible(secondVowel);

    addAndMakeVisible(mode);

    addAndMakeVisible(tapButton);
    addAndMakeVisible(interval_ms);

    //addAndMakeVisible(horizontalMeterL);
    //addAndMakeVisible(horizontalMeterR);

    addAndMakeVisible(verticalGradientMeterL);
    addAndMakeVisible(verticalGradientMeterR);

    setResizable(true, true);
    setResizeLimits(500, 350, 2000, 1400);
    getConstrainer()->setFixedAspectRatio(1.45);

}

WahAudioProcessorEditor::~WahAudioProcessorEditor()
{
}


void WahAudioProcessorEditor::timerCallback()
{
    horizontalMeterL.setLevel(audioProcessor.getRmsValue(0));
    horizontalMeterR.setLevel(audioProcessor.getRmsValue(1));

    horizontalMeterL.repaint();
    horizontalMeterR.repaint();
}

//==============================================================================
void WahAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::Image background = juce::ImageCache::getFromMemory(BinaryData::metallo_png, BinaryData::metallo_pngSize);
    g.drawImageAt(background, 220, 0);

    g.setFont(juce::Font("Arial", 40.0f, juce::Font::bold));

    juce::Rectangle<float> leftColumn(0, 0, getWidth() - 550, 380);

    g.setColour(juce::Colours::slategrey);
    g.fillRect(leftColumn);

    juce::Rectangle<float> bottomRow(0, 380, getWidth(), 320);

    g.setColour(juce::Colours::steelblue.brighter(0.8f));
    g.fillRect(bottomRow);

    g.setColour(juce::Colours::red.darker(0.1f));
    g.setFont(75.0f);
    g.setFont(juce::Font("Bauhaus 93", 90.0f, juce::Font::italic));

    g.drawFittedText(" WOW WAH", 250, 25, 550, 50, juce::Justification::centredTop, 1);
}
   
void WahAudioProcessorEditor::resized()
{
    /*auto r = getLocalBounds();
    auto bottomSection = r.removeFromBottom(300);

    frequencySlider.setBounds(bottomSection.removeFromLeft(200). reduced (20));
    qualitySlider.setBounds(bottomSection.removeFromLeft(200).reduced(20));
    decaySlider.setBounds(bottomSection.removeFromLeft(200).reduced(20));
    sensSlider.setBounds(bottomSection.removeFromLeft(200).reduced(20));
    dryWetSlider.setBounds(bottomSection.removeFromLeft(200).reduced(20)); */
    //leftmargin, topmargin

    auto rotarySize = getWidth() * 0.13;
    auto bottomRowHeight = getHeight() * 0.75;

    /*rangeSlider.setBounds(getWidth() * 0.13, bottomRowHeight, rotarySize, rotarySize); //when using rotary you must have width = height
    qualitySlider.setBounds(getWidth() * 2 * 0.13 , bottomRowHeight, rotarySize, rotarySize);
    decaySlider.setBounds(getWidth() * 3 * 0.13, bottomRowHeight, rotarySize, rotarySize);
    attackSlider.setBounds(getWidth() * 4 * 0.13, bottomRowHeight, rotarySize, rotarySize);
    mixSlider.setBounds(getWidth() * 5 * 0.13, bottomRowHeight, rotarySize, rotarySize);
    dryWetSlider.setBounds(getWidth() * 6 * 0.13, bottomRowHeight, rotarySize, rotarySize); */

    //levelSlider.setBounds(getWidth() * 0.05, bottomRowHeight, 50 , rotarySize);

    humanizerLabel.setBounds(0, (getHeight() / 2) - 95, 250, 50);
    humanizerButton.setBounds(75, (getHeight() / 2) - 90, 100, 100);
    firstVowel.setBounds(25, getHeight() / 2, 100 , 70);
    secondVowel.setBounds(125, getHeight() / 2, 100, 70);

    auto area = getLocalBounds();
    auto bottomSection = area.removeFromBottom(150);
    //auto leftSection = area.removeFromLeft(250);
    //auto gainSection = area.removeFromRight(200);
    auto sliderSection = bottomSection.removeFromRight(600);
    //auto levelSection = bottomSection.removeFromLeft(250);
    //auto meterSection = bottomSection.removeFromLeft(150);

    // gainSlider.setBounds(getWidth() - 300, getHeight() * 0.2, 1.15 * rotarySize, 1.15 * rotarySize);
     
     mode.setBounds(50, 20, 150, 50);
     filterType.setBounds(25, getHeight() * 0.15, 200, 70);
     
     tapButton.setBounds(600, getHeight() / 2 - 130, 100, 100);
     interval_ms.setBounds(600-15, getHeight() / 2, 130, 50);


     gainSlider.setBounds(getWidth() / 2 - 100, getHeight() / 2 - 130, 200, 200);
    // gainLabel.setBounds(800, 100, 100, 50);
     //levelSlider.setBounds(levelSection.reduced(10));
     rangeSlider.setBounds(sliderSection.removeFromRight(120).reduced(10));
     qualitySlider.setBounds(sliderSection.removeFromRight(120).reduced(10));
     decaySlider.setBounds(sliderSection.removeFromRight(120).reduced(10));
     attackSlider.setBounds(sliderSection.removeFromRight(120).reduced(10));
     mixSlider.setBounds(sliderSection.removeFromRight(120).reduced(10));
     //dryWetSlider.setBounds(sliderSection.removeFromRight(120).reduced(10));

     verticalGradientMeterL.setBounds(bottomSection.removeFromLeft(80).reduced(15));
     verticalGradientMeterR.setBounds(bottomSection.removeFromLeft(80).reduced(15));


}

void WahAudioProcessorEditor::humanizerButtonToggle()
{
    bool state = humanizerButton.getToggleState();
    
    if(state)
    {
        if (firstCurrentVowel == vowel[1]) {
            audioProcessor.set_minFreq(500.0);
        }
        else if (firstCurrentVowel == vowel[2]) {
            audioProcessor.set_minFreq(320.0);
        }
        else if (firstCurrentVowel == vowel[3]) {
            audioProcessor.set_minFreq(500.0);
        }
        else if (firstCurrentVowel == vowel[4]) {
            audioProcessor.set_minFreq(320.0);
        }
        else if (firstCurrentVowel == vowel[0]) {
            audioProcessor.set_minFreq(1000.0);
        }
    
        if (secondCurrentVowel == vowel[1]) {
            audioProcessor.set_maxFreq(500.0);
        }
        else if (secondCurrentVowel == vowel[2]) {
            audioProcessor.set_maxFreq(320.0);
        }
        else if (secondCurrentVowel == vowel[3]) {
            audioProcessor.set_maxFreq(500.0);
        }
        else if (secondCurrentVowel == vowel[4]) {
            audioProcessor.set_maxFreq(320.0);
        }
        else if (secondCurrentVowel == vowel[0]) {
            audioProcessor.set_maxFreq(1000.0);
        }
    }
    else
    {
        audioProcessor.reset_freq();
    }
}

void WahAudioProcessorEditor::firstVowelSelection()
{
    bool state = humanizerButton.getToggleState();
    
    if (firstCurrentVowel == vowel[0]) {
        firstVowel.setButtonText("E");
        firstVowel.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
        firstCurrentVowel = vowel[1];
        if(state)
            audioProcessor.set_minFreq(500.0);
    }
    else if (firstCurrentVowel == vowel[1]) {
        firstVowel.setButtonText("I");
        firstVowel.setColour(juce::TextButton::buttonOnColourId, juce::Colours::pink);
        firstCurrentVowel = vowel[2];
        if(state)
            audioProcessor.set_minFreq(320.0);
    }
    else if (firstCurrentVowel == vowel[2]) {
        firstVowel.setButtonText("O");
        firstVowel.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow);
        firstCurrentVowel = vowel[3];
        if(state)
            audioProcessor.set_minFreq(500.0);
    }
    else if (firstCurrentVowel == vowel[3]) {
        firstVowel.setButtonText("U");
        firstVowel.setColour(juce::TextButton::buttonOnColourId, juce::Colours::blue);
        firstCurrentVowel = vowel[4];
        if(state)
            audioProcessor.set_minFreq(320.0);
    }
    else if (firstCurrentVowel == vowel[4]) {
        firstVowel.setButtonText("A");
        firstVowel.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker(0.2f));
        firstCurrentVowel = vowel[0];
        if(state)
            audioProcessor.set_minFreq(1000.0);
    }
}

void WahAudioProcessorEditor::secondVowelSelection()
{
    bool state = humanizerButton.getToggleState();

    if (secondCurrentVowel == vowel[0]) {
        secondVowel.setButtonText("E");
        secondVowel.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
        secondCurrentVowel = vowel[1];
        if(state)
            audioProcessor.set_maxFreq(500.0);
    }
    else if (secondCurrentVowel == vowel[1]) {
        secondVowel.setButtonText("I");
        secondVowel.setColour(juce::TextButton::buttonOnColourId, juce::Colours::pink);
        secondCurrentVowel = vowel[2];
        if(state)
            audioProcessor.set_maxFreq(320.0);
    }
    else if (secondCurrentVowel == vowel[2]) {
        secondVowel.setButtonText("O");
        secondVowel.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow);
        secondCurrentVowel = vowel[3];
        if(state)
            audioProcessor.set_maxFreq(500.0);
    }
    else if (secondCurrentVowel == vowel[3]) {
        secondVowel.setButtonText("U");
        secondVowel.setColour(juce::TextButton::buttonOnColourId, juce::Colours::blue);
        secondCurrentVowel = vowel[4];
        if(state)
            audioProcessor.set_maxFreq(320.0);
    }
    else if (secondCurrentVowel == vowel[4]) {
        secondVowel.setButtonText("A");
        secondVowel.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker(0.2f));
        secondCurrentVowel = vowel[0];
        if(state)
            audioProcessor.set_maxFreq(1000.0);
    }
}

void WahAudioProcessorEditor::modeSelection()
{
    if (!currentMode) {
        mode.setToggleState(false, juce::NotificationType::dontSendNotification);
        mode.setButtonText("DYNAMIC");
        currentMode = 1;
        audioProcessor.set_mode(1);
    }
    else if (currentMode) {
        mode.setToggleState(true, juce::NotificationType::dontSendNotification);
        mode.setButtonText("TEMPO");
        currentMode = 0;
        audioProcessor.set_mode(0);
    }

}

void WahAudioProcessorEditor::tapClicked()
{
    if (!num_click) {
        num_click++;
        oldTime = juce::Time::getMillisecondCounterHiRes();
    }
    else if (num_click) {
        newTime = juce::Time::getMillisecondCounterHiRes();
        diff = newTime - oldTime;
        diffBPM = (60 / diff) * 1000;
        int diffPrint = (int)round(diffBPM);
        oldTime = newTime;
        std::string diff_str = std::to_string(diffPrint);
        interval_ms.setText(diff_str, juce::dontSendNotification);
        audioProcessor.set_rate(1.0/(diff/1000.0));
    }
}

void WahAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &rangeSlider)
    {
        bool state = humanizerButton.getToggleState();
        
        if(state)
            audioProcessor.set_minFreq(rangeSlider.getValue());
        audioProcessor.set_depth(rangeSlider.getValue());
    }
    else if (slider == &qualitySlider)
    {
        audioProcessor.set_quality(qualitySlider.getValue());
    }
    else if (slider == &decaySlider)
    {
        audioProcessor.set_decay(decaySlider.getValue());
    }
    else if (slider == &attackSlider)
    {
        audioProcessor.set_attack(attackSlider.getValue());
    }
    else if (slider == &mixSlider)
    {
        audioProcessor.set_wet(mixSlider.getValue());
        audioProcessor.set_dry(1.0 - mixSlider.getValue());
    }
    else if (slider == &gainSlider)
    {
        audioProcessor.set_gain(gainSlider.getValue());
    }
}

void WahAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox->getText() == "LowPass")
    {
        audioProcessor.set_filter(0);
    }
    else if (comboBox->getText() == "BandPass")
    {
        audioProcessor.set_filter(1);
    }
    else if (comboBox->getText() == "HighPass")
    {
        audioProcessor.set_filter(2);
    }
}
