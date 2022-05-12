/*
  ==============================================================================

    VerticalGradientMeter.h
    Created: 10 May 2022 12:18:21pm
    Author:  Asus

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace Gui
{
    class VerticalGradientMeter : public juce::Component, public juce::Timer
    {
    public:
        VerticalGradientMeter(std::function<float()>&& valueFunction) : valueSupplier(std::move(valueFunction))
        {
            startTimerHz(24);
        }

        void paint(juce::Graphics& g) override
        {
            const auto level = valueSupplier();

            auto bounds = getLocalBounds().toFloat();

            g.setColour(juce::Colours::black);
            g.fillRect(bounds);

            g.setGradientFill(gradient);
            const auto scaledY =
                juce::jmap(level, -60.f, 6.f, 0.f, static_cast<float>(getHeight()));
            g.fillRect(bounds.removeFromBottom(scaledY));
        }

        void resized() override
        {
            const auto bounds = getLocalBounds().toFloat();
            gradient = juce::ColourGradient{
                        juce::Colours::green,
                        bounds.getBottomLeft(),
                        juce::Colours::red,
                        bounds.getTopLeft(),
                        false
            };
            gradient.addColour(0.5, juce::Colours::yellow);
        }

        void timerCallback() override
        {
            repaint();
        }


    private:
        std::function<float()> valueSupplier;
        juce::ColourGradient gradient{};
    };
}

