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
    juce::Slider gainSlider;
	juce::Slider delayFeedbackSlider;
	juce::Slider delayTimeSlider;

	juce::Component header;
    juce::GroupComponent footer;
    juce::GroupComponent contentGain;
    juce::GroupComponent contentDelay;
	juce::GroupComponent contentSpectrum;

    SpectrumAnalyserComponent spectrumAnalyser;


    //LABELS
    juce::Label gainLabel;
    juce::Label delaySectionLabel;
    juce::Label delayFeedbackLabel;
    juce::Label delayTimeLabel;
	



public:
	// Slider attachments
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> gainSliderAttachment;
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleGainSliderAudioProcessorEditor)
};

