/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.
    Frontend - aesthetics, how values change
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_gui_extra/juce_gui_extra.h>


//==============================================================================
// Constructor: Caled when window is created
SimpleGainSliderAudioProcessorEditor::SimpleGainSliderAudioProcessorEditor (SimpleGainSliderAudioProcessor& p)
	: AudioProcessorEditor (&p), audioProcessor (p),
	spectrumAnalyser(p) // Create spectrum analyser component
{
    // Create slider attachments
	jassert(audioProcessor.treeState.getParameter(GAIN_ID) != nullptr); 
    gainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, GAIN_ID , gainSlider); 
	jassert(audioProcessor.treeState.getParameter(DELAY_FEEDBACK_ID) != nullptr); 
	delayFeedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, DELAY_FEEDBACK_ID, delayFeedbackSlider);
	jassert(audioProcessor.treeState.getParameter(DELAY_TIME_ID) != nullptr);
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, DELAY_TIME_ID, delayTimeSlider); 

	addAndMakeVisible(header);
	addAndMakeVisible(contentGain);
	//addAndMakeVisible(footer);
	addAndMakeVisible(contentDelay);
	addAndMakeVisible(contentSpectrum);

	delaySectionLabel.setText("Delay Settings", juce::dontSendNotification);
	delaySectionLabel.setJustificationType(juce::Justification::centred);
	delaySectionLabel.setFont(juce::Font(16.0f, juce::Font::bold));
	contentDelay.addAndMakeVisible(delaySectionLabel);

	// === gainSlider Properties ===
	contentGain.setColour(juce::GroupComponent::outlineColourId, juce::Colours::lightblue.withAlpha(0.5f));

		// Slider settings
    gainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 25);
	gainSlider.setTextValueSuffix(" dB");
    gainSlider.setRange(-48.0f, 6.0f);
	gainSlider.setTooltip("Adjusts the overall output volume of the plugin after delay effect is applied.");
	gainSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

		// Label settings
	gainLabel.setText("Out Gain", juce::dontSendNotification);
	gainLabel.setJustificationType(juce::Justification::centred);
	gainLabel.setFont(juce::Font(14.0f, juce::Font::bold));
	
	contentGain.addAndMakeVisible(gainLabel);
	contentGain.addAndMakeVisible(gainSlider);

	// === DELAY FEEDBACK Properties ===
	contentDelay.setColour(juce::GroupComponent::outlineColourId, juce::Colours::lightblue.withAlpha(0.5f));

		// Slider settings
	delayFeedbackSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
	delayFeedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 25);
	delayFeedbackSlider.setTextValueSuffix(" %");
	delayFeedbackSlider.setRange(0.0f, 100.0f);
	delayFeedbackSlider.setTooltip("Controls the volume of audio fed back into the delay effect. Inrease to make the echoes repeat more and fade out slower.");
	gainSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

		// Label settings
	delayFeedbackLabel.setText("Feedback", juce::dontSendNotification);	
	delayFeedbackLabel.setJustificationType(juce::Justification::centred);
	gainLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);


	contentDelay.addAndMakeVisible(delayFeedbackLabel);	
	contentDelay.addAndMakeVisible(delayFeedbackSlider);		

	// === DELAY TIME Properties ===

		//Slider settings
	delayTimeSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
	delayTimeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 25);
	delayTimeSlider.setTextValueSuffix(" sec");
	delayTimeSlider.setRange(0.0f, 2.0f);
	delayTimeSlider.setTooltip("Controls the time between the original sound and its repetitions. Increase for faster echoes. Very small values and modulation can result in interesting effects.");
	gainSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

		// Label settings
	delayTimeLabel.setText("Time", juce::dontSendNotification);	// label settings
	delayTimeLabel.setJustificationType(juce::Justification::centred);
	gainLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);

	contentDelay.addAndMakeVisible(delayTimeLabel);
	contentDelay.addAndMakeVisible(delayTimeSlider);

	// === Spectrum Analyser properties ===
	contentSpectrum.setColour(juce::GroupComponent::outlineColourId, juce::Colours::transparentBlack);
	contentSpectrum.addAndMakeVisible(spectrumAnalyser);
	spectrumAnalyser.setTooltip("Visualizes the frequency spectrum of the audio signal after all effects are applied.");

    setSize(650, 400);
}

SimpleGainSliderAudioProcessorEditor::~SimpleGainSliderAudioProcessorEditor()
{
}

//==============================================================================
// What to happen within the window
void SimpleGainSliderAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(juce::Colour::fromString("FF560573"));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));

}


void SimpleGainSliderAudioProcessorEditor::resized()  //
{
	// Set bounds for header, footer, sidebar and contentDelay
	auto area = getLocalBounds();
	auto headerHeight = 40;
	auto footerHeight = 30;
	header.setBounds(area.removeFromTop(headerHeight));
	//footer.setBounds(area.removeFromBottom(footerHeight));

	auto contentSpaceHeight = area.getHeight();
	auto gainContentWidth = 70;

	contentGain.setBounds(area.removeFromLeft(gainContentWidth));
	contentDelay.setBounds(area.removeFromTop(contentSpaceHeight / 2));
	contentSpectrum.setBounds(area);// Spectrum analyser takes remaining space
     
	// ==== GAIN UI ====

	// Make flexbox layout
	juce::FlexBox sidebarFlex;
	sidebarFlex.flexDirection = juce::FlexBox::Direction::column;  
	sidebarFlex.justifyContent = juce::FlexBox::JustifyContent::flexStart; 
	sidebarFlex.alignItems = juce::FlexBox::AlignItems::center;    
	const int gainLabelHeight = gainLabel.getFont().getHeight() + 10; // Fixed label height

	sidebarFlex.items.addArray({
		juce::FlexItem(gainSlider).withWidth(contentGain.getWidth()).withHeight(contentGain.getHeight() - gainLabelHeight - 10).withMargin({0, 0, 0, 0}), // Slider takes space + 5px bottom margin
		juce::FlexItem(gainLabel).withHeight(gainLabelHeight).withWidth(contentGain.getWidth()) // Fixed label height
		});

	// Apply layout to sidebar
	sidebarFlex.performLayout(contentGain.getLocalBounds().reduced(5));

	// ==== DELAY UI ====

    // Make grid layout
    juce::Grid delayGrid;
	delayGrid.alignItems = juce::Grid::AlignItems::stretch;
	delayGrid.justifyItems = juce::Grid::JustifyItems::center;

    delayGrid.templateRows = {
        juce::Grid::TrackInfo(juce::Grid::Fr(1)),  // Title row
		juce::Grid::TrackInfo(juce::Grid::Fr(1)),  // Label row
        juce::Grid::TrackInfo(juce::Grid::Fr(4))   // Sliders row
    };
    delayGrid.templateColumns = {
        juce::Grid::Fr(1),	// Feedback slider
		juce::Grid::Fr(1)	// Time slider
    };

    delayGrid.items = {
        juce::GridItem(delaySectionLabel).withArea(1, 1, 1, 3),

		juce::GridItem(delayFeedbackLabel).withArea(2, 1).withMargin({0,0,10,0}),	// Top, Right, Bottom, Left
	
        juce::GridItem(delayFeedbackSlider)
			.withArea(3, 1)
			.withMargin({0,0,10,0}),

		juce::GridItem(delayTimeLabel).withArea(2, 2).withMargin({0,0,10,0}),	// Top, Right, Bottom, Left

		juce::GridItem(delayTimeSlider)
		.withArea(3, 2)	
		.withMargin({0,0,10,0})	// Top, Right, Bottom, Left
    };
	
    delayGrid.performLayout(contentDelay.getLocalBounds().reduced(5));

	// ==== SPECTRUM UI ====
	spectrumAnalyser.setBounds(contentSpectrum.getLocalBounds().reduced(5)); // Set bounds for spectrum analyser
	
}	
