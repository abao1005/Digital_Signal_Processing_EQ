/*
  ==============================================================================

    SliderContainer.cpp
    Created: 15 Oct 2021 12:44:52am
    Author:  robert

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SliderContainer.h"

MySlider::MySlider(MyAudioProcessor &p, juce::String labelName)
    : audioProcessor(p)
{
    setLookAndFeel(&lnf);
    slider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, slider.getTextBoxWidth(), slider.getTextBoxHeight());
    label.setText(labelName, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    attachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(
        audioProcessor.tree, juce::String{labelName}.toLowerCase(), slider
    ));
    addAndMakeVisible(slider);
    addAndMakeVisible(label);
}

MySlider::~MySlider()
{
    setLookAndFeel(nullptr);
}

void MySlider::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MySlider::resized()
{
    auto area = getLocalBounds();
    label.setBounds(area.removeFromTop(15));
    slider.setBounds(area.reduced(4));
}

SliderContainer::SliderContainer(MyAudioProcessor &p)
    : audioProcessor(p),
      levelSlider(p, "level"),
      f1Slider(p, "f1"),
      f2Slider(p, "f2"),
      orderSlider(p, "order")

{    
    for (auto &slider : sliders) {
        addAndMakeVisible(slider);
    }
}

SliderContainer::~SliderContainer()
{

}

void SliderContainer::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SliderContainer::resized()
{
    juce::FlexBox flexBox;
    for (auto &slider : sliders) {
        juce::FlexItem item {*slider};
        flexBox.items.add(item.withFlex(1.0));
    }
    flexBox.performLayout(getLocalBounds());
    for (auto &slider : sliders) {
        slider->setBounds(slider->getBounds().reduced(5));
    }
}
