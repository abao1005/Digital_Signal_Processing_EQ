/*
  ==============================================================================

    FrequencySpectrum.cpp
    Created: 19 Oct 2021 4:29:27am
    Author:  robert

  ==============================================================================
*/

#include <JuceHeader.h>
#include "FrequencySpectrum.h"

FrequencySpectrum::FrequencySpectrum(MyAudioProcessor &p)
    : audioProcessor(p),
      spectrum(p.getSingleChannelSampleFifo())
{
    fftDataGenerator.changeOrder(12);
    pathBuffer.setSize(1, fftDataGenerator.getFFTSize());
    startTimerHz(30);
}

FrequencySpectrum::~FrequencySpectrum()
{

}

void FrequencySpectrum::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(50, 50, 50));
    g.setColour(juce::Colours::grey);
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 3, 3);
    
    drawBackgroundGrid(g);
    g.setColour(juce::Colours::white);
    g.strokePath(spectrumPath, juce::PathStrokeType(1.f));
    drawTextLabels(g);
}

void FrequencySpectrum::resized()
{

}

void FrequencySpectrum::timerCallback()
{
    generateSpectrum();
    repaint();
}

void FrequencySpectrum::generateSpectrum()
{
    juce::AudioBuffer<float> temp;
    while (spectrum.getNumCompleteBuffersAvailable() > 0) {
        if (spectrum.getAudioBuffer(temp)) {
            auto size = temp.getNumSamples();
            juce::FloatVectorOperations::copy(
                pathBuffer.getWritePointer(0, 0),
                pathBuffer.getReadPointer(0, size),
                pathBuffer.getNumSamples() - size
            );
            juce::FloatVectorOperations::copy(
                pathBuffer.getWritePointer(0, pathBuffer.getNumSamples() - size),
                temp.getReadPointer(0, 0),
                size
            );
            fftDataGenerator.produceFFTDataForRendering(pathBuffer, getGains().front());
        }
    }

    const auto fftsize = fftDataGenerator.getFFTSize();
    const auto binWidth = audioProcessor.getSampleRate() / double(fftsize);
    while (fftDataGenerator.getNumAvailableFFTDataBlocks() > 0) {
        std::vector<float> fftData;
        if(fftDataGenerator.getFFTData(fftData)) {
            pathProducer.generatePath(fftData, getAnalysisArea().toFloat(), fftsize, binWidth, getGains().front());
        }
    }
    while (pathProducer.getNumPathsAvailable() > 0) {
        pathProducer.getPath(spectrumPath);
    }
}

void FrequencySpectrum::drawBackgroundGrid(juce::Graphics &g)
{
    auto freqs = getFrequencies();
    auto gains = getGains();
    auto area = getRenderArea();
    auto left = area.getX();
    auto top = area.getY();
    auto right = area.getRight();
    auto bottom = area.getBottom();
    auto xs = getXs(freqs, getAnalysisArea().getX(), getAnalysisArea().getWidth());

    g.setColour(juce::Colours::dimgrey);
    for (auto x : xs) {
        g.drawVerticalLine(x, top, bottom);
    }

    g.setColour(juce::Colours::darkgrey);
    for (auto gain : gains) {
        auto y = juce::jmap(
            gain, gains.front(), gains.back(),
            float(getAnalysisArea().getBottom()),
            float(getAnalysisArea().getY())
        );
        g.drawHorizontalLine(y, left, right);
    }
}

void FrequencySpectrum::drawTextLabels(juce::Graphics &g)
{
    const int fontHeight = 10;
    g.setFont(fontHeight);
    g.setColour(juce::Colours::lightgrey);

    auto freqs = getFrequencies();
    auto gains = getGains();
    auto area = getAnalysisArea();
    auto left = area.getX();
    auto top = area.getY();
    auto bottom = area.getBottom();
    auto width = area.getWidth();
    auto xs = getXs(freqs, left, width);

    for (int i = 0; i < freqs.size(); ++i) {
        auto f = freqs[i];
        auto x = xs[i];

        bool isThousand = false;
        if (f > 999.f) {
            isThousand = true;
            f /= 1000.f;
        }

        juce::String str;
        str << f;
        if (isThousand)
            str << "k";
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);
        juce::Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(getRenderArea().getBottom() - fontHeight);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    for (auto gain : gains) {
        auto y = juce::jmap(gain, gains.front(), gains.back(), float(bottom), float(top));

        juce::String str;
        if (gain > 0)
            str << "+";
        str << gain << "dB";

        auto textWidth = g.getCurrentFont().getStringWidth(str);
        juce::Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(0, y - fontHeight / 2.f);
        r.setX(5);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
}

