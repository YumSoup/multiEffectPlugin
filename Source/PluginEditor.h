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
//#include <juce_core/juce_core.h>


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

    juce::Slider gainSlider;
	juce::Slider delayFeedbackSlider;
	juce::Slider delayTimeSlider;

	juce::GroupComponent header;
    juce::GroupComponent footer;
    juce::GroupComponent sidebar;
    juce::GroupComponent contentDelay;

    //LABELS
    juce::Label gainLabel;

    juce::Label delaySectionLabel;
    juce::Label delayFeedbackLabel;
    KnobLabel delayTimeLabel{ delayTimeSlider };
	

public:
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> gainSliderAttachment; //create smartpointer for a sliderAttatchments
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleGainSliderAudioProcessorEditor)
};

