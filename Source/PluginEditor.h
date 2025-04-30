/*
  ==============================================================================
    This file contains the basic framework code for a JUCE plugin editor.

    .h holds variables
    Frontend - aesthetics, how values change
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpectrumAnalyserComponent.h"
#include <juce_gui_extra/juce_gui_extra.h>


//==============================================================================
/**
*/

class KnobLabel : public juce::Label
{
public:
    KnobLabel(juce::Component& componentToAttachTo)
    {
        // Style the label
        setColour(juce::Label::textColourId, juce::Colours::white);
        setFont(juce::Font(14.0f, juce::Font::plain));
        setJustificationType(juce::Justification::centred);
        setBorderSize(juce::BorderSize<int>(0, 0, -5, 0));

        // Attach to component and position below it
        attachToComponent(&componentToAttachTo, false);
    }
};


 
// inherits component class
class SimpleGainSliderAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimpleGainSliderAudioProcessorEditor (SimpleGainSliderAudioProcessor&);
    ~SimpleGainSliderAudioProcessorEditor() override;
    //==============================================================================

    void paint (juce::Graphics&) override;
    void resized() override;

    

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleGainSliderAudioProcessor& audioProcessor;

    juce::TooltipWindow tooltipWindow{ this, 700 };

	// UI COMPONENTS
    juce::Slider inGainSlider;
	juce::Slider outGainSlider;
	juce::Slider delayFeedbackSlider;
	juce::Slider delayTimeSlider;

	juce::Component header;
    juce::GroupComponent footer;
    juce::GroupComponent contentInGain;
    juce::GroupComponent contentOutGain;
    juce::GroupComponent contentDelay;
	juce::GroupComponent contentSpectrum;

    SpectrumAnalyserComponent spectrumAnalyser;


    //LABELS
    juce::Label inGainLabel;
	juce::Label outGainLabel;
    juce::Label delaySectionLabel;
    juce::Label delayFeedbackLabel;
    juce::Label delayTimeLabel;
	

    void configureGainSlider(juce::Slider& slider, const juce::String& tooltip);

public:
	// Slider attachments
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> inGainSliderAttachment;
	std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> outGainSliderAttachment;

    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleGainSliderAudioProcessorEditor)
};

