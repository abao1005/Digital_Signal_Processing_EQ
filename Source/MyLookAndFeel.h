/*
  ==============================================================================

    MyLookAndFeel.h
    Created: 15 Oct 2021 2:58:45am
    Author:  robert

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class MyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
                            const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bound = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);
        auto radius = juce::jmin(bound.getWidth(), bound.getHeight()) / 2.0f;
        auto centreX = bound.getCentreX();
        auto centreY = bound.getCentreY();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // fill
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        g.setColour(juce::Colours::black);
        g.fillEllipse(rx + 1.0f, ry + 1.0f, rw + 2.5f, rw + 2.5f);
        g.setColour(juce::Colour::fromString("FF3C3C3C"));
        g.fillEllipse(rx, ry, rw, rw);

        // outline
        g.setColour(juce::Colours::grey);
        g.drawEllipse(rx, ry, rw, rw, 3.0f);

        // pointer
        juce::Path p;
        auto pointerLength = radius * 0.33f;
        auto pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colours::white);
        g.fillPath(p);
    }
};
