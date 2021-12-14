/*
  ==============================================================================

    SliderContainer.h
    Created: 15 Oct 2021 12:44:52am
    Author:  robert

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MyLookAndFeel.h"

class MySlider : public juce::Component
{
public:
    MySlider(MyAudioProcessor &, juce::String);
    ~MySlider() override;

    void paint(juce::Graphics &) override;
    void resized() override;

private:
    MyAudioProcessor &audioProcessor;
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    MyLookAndFeel lnf;
};

class SliderContainer : public juce::Component
{
public:
    SliderContainer(MyAudioProcessor &);
    ~SliderContainer() override;

    void paint(juce::Graphics &) override;
    void resized() override;

private:
    MyAudioProcessor &audioProcessor;
    MySlider levelSlider, f1Slider, f2Slider, orderSlider;

    std::vector<MySlider *> sliders { &levelSlider, &f1Slider, &f2Slider, &orderSlider };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderContainer);
};
