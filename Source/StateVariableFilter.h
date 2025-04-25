/*
  ==============================================================================

    StateVariableFilter.h
    Created: 30 Mar 2025 10:01:47pm
    Author:  zk

  ==============================================================================
*/

#pragma once

#include <corecrt_math.h>
#include <corecrt_math_defines.h>

class StateVariableFilter {
private:
    float cutoff, resonance = 0.7f;
    float low, band, high;

public:
    void setCutoff(float freqHz, float sampleRate) {
        cutoff = 2.0f * sin(M_PI * freqHz / sampleRate);
    }

    float process(float input) {
        high = input - (low + resonance * band);
        band += cutoff * high;
        low += cutoff * band;
        return low; // Return LPF output
    }
};

