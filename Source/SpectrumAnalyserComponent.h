
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>

#include <vector>
#include "PluginProcessor.h" // Your processor header


//==============================================================================
class SpectrumAnalyserComponent : 
    public juce::Component,
    private juce::Timer, // Inherit from Timer to periodically update
    public juce::SettableTooltipClient
{
public:
    
    SpectrumAnalyserComponent(SimpleGainSliderAudioProcessor& processor);

    ~SpectrumAnalyserComponent() override;

    // Essential juce::Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    const float minDb = -36.0f;
    const float maxDb = 12.0f;
    const float minFreq = 20.0f;
    
    

private:
    SimpleGainSliderAudioProcessor& audioProcessor;
    
    void timerCallback() override;

 
    std::vector<float> fftDisplayData;
    std::vector<float> fftSmoothedData;
    const float smoothingFactor = 0.2f;
    juce::Image spectrogramImage;

 

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyserComponent)
};