/*
  ==============================================================================

    SynthVoice.h
    Created: 4 Oct 2021 1:57:03am
    Author:  robert, abao

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "SynthSound.h"

class SynthVoice : public juce::SynthesiserVoice
{
public:
	SynthVoice();

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    
    void stopNote(float velocity, bool allowTailOff) override;

    void pitchWheelMoved(int newPitchWheelValue) override;
    
    void controllerMoved(int controllerNumber, int newControllerValue) override;
   
    void renderNextBlock(juce::AudioBuffer <float> &outputBuffer, int startSample, int numSamples) override;
    
	int getMode() { return mode; }
	int getOrder() { return order; }
	float getF1() { return f1; }
	float getF2() { return f2; }

    void setLevel(float newLevel);
    void setMode(int newMode);
	void setOrder(int newOrder);
	void setF1(float newF1);
	void setF2(float newF2);

	//static constexpr auto fftOrder = 9;               // [1]
	//static constexpr auto fftSize = 1 << fftOrder;     // [2]


private:
    void genFilter();
    void genAllPass();
    void genLowPass();
    void genHighPass();
    void genBandPass();
		
	int mode;
	int order;
    float level;
	float f1, f2;

	int fftOrder = 9;
	int fftSize = 1 << fftOrder;
	float* fftH;
	float* fftData;
	float* overlap;

	std::array<float, 200> h = { 0.0 };
	std::vector <float> x;  // input signal
	std::vector <float> w;  // window function

	juce::Random random;
	juce::dsp::FFT forwardFFT;  // Declare a dsp::FFT object to perform the forward FFT on.

	std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
};
