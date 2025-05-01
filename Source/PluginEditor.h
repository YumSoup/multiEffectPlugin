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
class SimpleGainSliderAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Label::Listener
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

	juce::Slider attackSlider;
	juce::Slider releaseSlider;
    juce::Slider ratioSlider;
	juce::Slider thresholdSlider;

	juce::Component header;
    juce::GroupComponent footer;
    juce::GroupComponent contentInGain;
    juce::GroupComponent contentOutGain;
    juce::GroupComponent contentDelay;
	juce::GroupComponent contentSpectrum;
    juce::GroupComponent contentCompressor;

    SpectrumAnalyserComponent spectrumAnalyser;


    //LABELS
    juce::Label inGainLabel;
	juce::Label outGainLabel;

    juce::Label delaySectionLabel;
    juce::Label delayFeedbackLabel;
    juce::Label delayTimeLabel;

	juce::Label attackLabel;
	juce::Label releaseLabel;
	juce::Label ratioLabel;
	juce::Label thresholdLabel;
	juce::Label compressorSectionLabel;

    juce::Label ratioValueLabel;
    juce::Label thresholdValueLabel;
    juce::Label attackValueLabel;
    juce::Label releaseValueLabel;
	
    void labelTextChanged(juce::Label* labelChanged) override;

    void configureGainSlider(juce::Slider& slider, const juce::String& tooltip);

	void configureKnobLabel(juce::Label& label, const juce::String& text)
	{
		label.setText(text, juce::dontSendNotification);
		label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(13.0f, juce::Font::bold));
	}

public:
	// Slider attachments
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> inGainSliderAttachment;
	std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> outGainSliderAttachment;

    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment;

	std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> compressorAttackAttachment;
	std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> compressorReleaseAttachment;
	std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> compressorRatioAttachment;
	std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> compressorThresholdAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleGainSliderAudioProcessorEditor)
};

