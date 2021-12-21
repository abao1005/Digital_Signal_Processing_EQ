/*
  ==============================================================================

    SynthVoice.cpp
    Created: 4 Oct 2021 1:57:03am
    Author:  robert, abao

  ==============================================================================
*/

#include "SynthVoice.h"
#include <vector>
#include <cmath>
#include <string>
#define MY_PI 3.14159265358979323846

/***********************************************************
 * Debug API *
#include "atlbase.h"
#include "atlstr.h"
void OutputDebugPrintf(const char *strOutputString, ...)
{
    char strBuffer[4096] = {0};
    va_list vlArgs;
    va_start(vlArgs, strOutputString);
    _vsnprintf_s(strBuffer, sizeof(strBuffer) - 1, strOutputString, vlArgs);
    va_end(vlArgs);
    OutputDebugString(strBuffer);
}
**********************************************************/

SynthVoice::SynthVoice()
	:forwardFFT(fftOrder),
	window(std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris))
{
	overlap = new float[15]{ 0 };
	fftH = new float[fftSize * 2]{ 0 };
	fftH_freqOnly = new float[fftSize * 2]{ 0 };

	fftData = new float[fftSize * 2]{ 0 };
    genFilter();
}

bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*>(sound) != nullptr;
}

void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    // frequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    // tailOff = 0.0;
}

void SynthVoice::stopNote(float velocity, bool allowTailOff)
{
    // if (allowTailOff) {
    //     if (tailOff == 0.0)
    //         tailOff = 1.0;
    // } else {
    //     clearCurrentNote();
    // }
}

void SynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
    // handle pitch wheel moved midi event
}

void SynthVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
    // handle midi control change
}

void SynthVoice::renderNextBlock(juce::AudioBuffer <float> &outputBuffer, int startSample, int numSamples)
{
    float value;
	for (int i = 0; i < fftSize*2; i++)
	{
		fftData[i] = 0.0;
	}

	for (int i = startSample, j=0; i < (startSample + numSamples), j < numSamples; i++,j++) {
        value = random.nextFloat() * 0.25f - 0.125f;
		value *= level;
		
		fftData[j] = value;
    }

	//window->multiplyWithWindowingTable(fftData, fftSize);

	forwardFFT.performRealOnlyForwardTransform(fftData);
	/* Dot Product */
	for (int i = 0; i < fftSize *2; i+=2)
	{
		float sr = fftData[i];
		float si = fftData[i + 1];
		float fr = fftH[i];
		float fi = fftH[i + 1];
		fftData[i] = sr * fr - si * fi;
		fftData[i+1] = sr * fi + si * fr;
		/*DBG(i);
		DBG(fftData[i]);
		DBG(fftData[i+1]);*/

	}
	forwardFFT.performRealOnlyInverseTransform(fftData);
	
	/* Overlap-Add method */
	for (int i = 0; i < order - 1; i++)
	{
		fftData[i] += overlap[i];
	}
	for (int i = startSample, j = 0; i < (startSample + numSamples); i++, j++) {
		outputBuffer.addSample(0, i, fftData[j]);
		outputBuffer.addSample(1, i, fftData[j]);
	}
	for (int i = 0; i < order - 1; i++)
	{
		overlap[i] = fftData[numSamples + i];
	}
}

void SynthVoice::genFilter()
{
    /* clear vectors */
	for (int i = 0; i < 300; i++)
		h[i] = 0.0;
    x.clear();
    
    /* generate corresponding filter */
    switch (mode) {
        case 0:
            genAllPass();
            break;
        case 1:
            genLowPass();
            break;
        case 2:
            genHighPass();
            break;
        case 3:
            genBandPass();
            break;
    }

}

void SynthVoice::genAllPass()
{
	/* initialize the first few input signal to 0 */
	for (int i = 0; i < order; i++)
		x.push_back(0);

    /* Construct impulse response of All Pass Filter (FIR) */
    h[0] = 1.0f;
    for (int i = 1; i < order; i++)
        h[i] = 0.0f;

	/* FFT */
	for (int i = 0; i < fftSize*2; i++)
	{
		fftH[i] = (i < order) ? h[i] : 0;
		fftH_freqOnly[i] = (i < order) ? h[i] : 0;
	}
	forwardFFT.performRealOnlyForwardTransform(fftH);
	forwardFFT.performFrequencyOnlyForwardTransform(fftH_freqOnly);
}

void SynthVoice::genLowPass()
{
	/* initialize the first few input signal to 0 */
	for (int i = 0; i < order; i++)
		x.push_back(0);

    /* Construct impulse response of Low Pass Filter (FIR) */
	auto fc = f1 / getSampleRate();
	float impulse_response_sum = 0.0f;

	for (int i = -(order - 1) / 2,j=0; i <= order / 2; i++,j++) {
		if (i != 0)
			h[j]=(sin(2 * fc * i * MY_PI) / (2 * fc * i * MY_PI));
		else
			h[j]=(1.0);
	}

	/* Windowed-Sinc Filter */
	for (int i = 0; i < order; i++) {
		h[i] = h[i] * w.at(i);
		impulse_response_sum += h[i];
	}

	/* Normalized windowed-sinc filter */
	for (int i = 0; i < order; i++)
		h[i] /= impulse_response_sum;

	/* FFT */
	for (int i = 0; i < fftSize*2; i++)
	{
		fftH[i] = (i < order) ? h[i] : 0;
		fftH_freqOnly[i] = (i < order) ? h[i] : 0;
	}
	forwardFFT.performRealOnlyForwardTransform(fftH);
	forwardFFT.performFrequencyOnlyForwardTransform(fftH_freqOnly);
}

void SynthVoice::genHighPass()
{
	/* initialize the first few input signal to 0 */
	for (int i = 0; i < order; i++)
		x.push_back(0);

    /* Construct impulse response of Low Pass Filter (FIR) first */
	auto fc = f1 / getSampleRate();
	float impulse_response_sum = 0.0f;

	for (int i = -(order - 1) / 2,j=0; i <= order / 2; i++,j++) {
		if (i != 0)
			h.at(j) = (sin(2 * fc * i * MY_PI) / (2 * fc * i * MY_PI));
		else
			h.at(j) = (1.0);
	}

	/* Windowed-Sinc Filter */
	for (int i = 0; i < order; i++) {
		h.at(i) = h.at(i) * w.at(i);
		impulse_response_sum += h.at(i);
	}

	/* Normalized windowed-sinc filter */
	for (int i = 0; i < order; i++)
		h.at(i) /= impulse_response_sum;

	/* Then using spectral inversion to convert it into a high-pass one. */
	for (int i = 0; i < order; i++)
		h.at(i) *= -1;
	h.at(order / 2) += 1;

	/* FFT */
	for (int i = 0; i < fftSize*2; i++)
	{
		fftH[i] = (i < order) ? h[i] : 0;
		fftH_freqOnly[i] = (i < order) ? h[i] : 0;
	}
	forwardFFT.performRealOnlyForwardTransform(fftH);
	forwardFFT.performFrequencyOnlyForwardTransform(fftH_freqOnly);
}

void SynthVoice::genBandPass()
{
	/* initialize the first few input signal to 0 */
	for (int i = 0; i < 2 * order - 1; i++)
		x.push_back(0);

	/* Construct impulse response of two Low Pass Filter (FIR) first */
	std::vector<float> low, high;
	auto fh = f1 / getSampleRate();
	auto fl = f2 / getSampleRate();
	float impulse_response_sum_LPF = 0.0f;
	float impulse_response_sum_HPF = 0.0f;

	for (int i = -(order - 1) / 2; i <= order / 2; i++) {
		if (i != 0) {
			low.push_back (sin(2 * fl * i * MY_PI) / (2 * fl * i * MY_PI));
			high.push_back(sin(2 * fh * i * MY_PI) / (2 * fh * i * MY_PI));
		} else {
			low.push_back(1.0);
			high.push_back(1.0);
		}
	}

	/* Windowed-Sinc Filter */
	for (int i = 0; i < order; i++) {
		low.at(i)  *= w.at(i);
		high.at(i) *= w.at(i);
		impulse_response_sum_LPF += low.at(i);
		impulse_response_sum_HPF += high.at(i);
	}

	/* Normalized windowed-sinc filter */
	for (int i = 0; i < order; i++) {
		low.at(i)  /= impulse_response_sum_LPF;
		high.at(i) /= impulse_response_sum_HPF;
	}

	/* Using spectral inversion to convert LPF into a high-pass one. */
	for (int i = 0; i < order; i++)
		high.at(i) *= -1;
	high.at(order / 2) += 1;

	/* Create a BPF by convolving the two filters.(LPF and HPF) */
	for (int i = 0; i < 2 * order - 1; i++)
		h.at(i) = (0.0);

	for (int i = 0; i < 2 * order - 1; i++) {
		const int jmn = (i >= order - 1) ? i - (order - 1) : 0;
		const int jmx = (i >= order - 1) ? order - 1       : i;
		for (int j = jmn; j <= jmx; j++) {
			h.at(i) += (low.at(j) * high.at(i - j));
		}
	}

	for (int i = 0; i < 2*fftSize; i++)
	{
		fftH[i] = (i < 2*order-1) ? h[i] : 0;
	}

	forwardFFT.performRealOnlyForwardTransform(fftH);
}

void SynthVoice::setLevel(float newLevel)
{
    level = newLevel;
}

void SynthVoice::setMode(int newMode)
{
    mode = newMode;
    genFilter();
}

void SynthVoice::setOrder(int newOrder)
{
	order = newOrder;

	/* Reset blackman window */
    w.clear();
	for (int i = 0; i < order; i++)
		w.push_back(0.42 - 0.5 * cos(2 * MY_PI * i / (order - 1)) + 0.08 * cos(4 * MY_PI * i / (order - 1)));

	/* Reset overlap size */
	delete[] overlap;
	overlap = new float[order] { 0 };

	/* Reset FFT size */
	//int newFFTsize = juce::nextPowerOfTwo(480 + order-1);
	//if (newFFTsize == fftSize) {}
	//else {
	//	fftSize = newFFTsize;
	//	fftOrder = std::log2(newFFTsize);
	//	/* Resize fftH */
	//	delete[] fftH;
	//	fftH = new float[fftSize*2] {0};
	//	/* Resize fftData */
	//	delete[] fftData;
	//	fftData = new float[fftSize*2] {0};
	//}

    genFilter();
}

void SynthVoice::setF1(float newF1)
{
	f1 = newF1;
    if (mode != 0)
        genFilter();
}

void SynthVoice::setF2(float newF2)
{
    f2 = newF2;
    if (mode == 3)
        genFilter();
}
