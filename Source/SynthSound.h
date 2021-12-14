/*
  ==============================================================================

    SynthSound.h
    Created: 4 Oct 2021 1:56:53am
    Author:  robert

  ==============================================================================
*/

#pragma once

class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int midiNoteNumber) override
    {
        return true;
    }
    
    bool appliesToChannel(int midiNoteNumber) override
    {
        return true;
    }
};
