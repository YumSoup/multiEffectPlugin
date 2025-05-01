/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.
    Frontend - aesthetics, how values change
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <regex>

//==============================================================================
// Constructor: Caled when window is created
SimpleGainSliderAudioProcessorEditor::SimpleGainSliderAudioProcessorEditor (SimpleGainSliderAudioProcessor& p)
	: AudioProcessorEditor (&p), audioProcessor (p),
	spectrumAnalyser(p) // Create spectrum analyser component
{

    // Create slider attachments
	
    inGainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, INGAIN_ID , inGainSlider); 
	outGainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, OUTGAIN_ID, outGainSlider);

	delayFeedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, DELAY_FEEDBACK_ID, delayFeedbackSlider);
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, DELAY_TIME_ID, delayTimeSlider); 

	compressorAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, ATTACK_ID, attackSlider);
	compressorReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, RELEASE_ID, releaseSlider);
	
	compressorThresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, THRESHOLD_ID, thresholdSlider);
	compressorRatioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, RATIO_ID, ratioSlider);


	addAndMakeVisible(header);
	addAndMakeVisible(contentInGain);
	addAndMakeVisible(contentOutGain);
	addAndMakeVisible(contentDelay);
	addAndMakeVisible(contentSpectrum);
	addAndMakeVisible(contentCompressor);
	
	// Set outline colours
	contentInGain.setColour(juce::GroupComponent::outlineColourId, juce::Colours::lightblue.withAlpha(0.5f));
	contentOutGain.setColour(juce::GroupComponent::outlineColourId, juce::Colours::lightblue.withAlpha(0.5f));
	contentDelay.setColour(juce::GroupComponent::outlineColourId, juce::Colours::lightblue.withAlpha(0.5f));
	contentCompressor.setColour(juce::GroupComponent::outlineColourId, juce::Colours::lightblue.withAlpha(0.5f));

	inGainSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::lightblue.withAlpha(0.5f));
	outGainSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::lightblue.withAlpha(0.5f));
	delayFeedbackSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::lightblue.withAlpha(0.5f));
	delayTimeSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::lightblue.withAlpha(0.5f));
	attackSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::lightblue.withAlpha(0.5f));
	releaseSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::lightblue.withAlpha(0.5f));
	thresholdSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::lightblue.withAlpha(0.5f));
	ratioSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::lightblue.withAlpha(0.5f));

	// === gainSlider Properties ===
	
		// Slider settings
	configureGainSlider(inGainSlider, "Adjusts the input gain before processing.");
	configureGainSlider(outGainSlider, "Adjusts the overall output volume of the plugin after delay effect is applied.");

		// Label settings
	configureKnobLabel(inGainLabel, "In Gain");
	inGainLabel.setFont(juce::Font(14.0f, juce::Font::bold));

	configureKnobLabel(outGainLabel, "Out Gain");
	outGainLabel.setFont(juce::Font(14.0f, juce::Font::bold));

	contentInGain.addAndMakeVisible(inGainLabel);
	contentInGain.addAndMakeVisible(inGainSlider);
	contentOutGain.addAndMakeVisible(outGainLabel);
	contentOutGain.addAndMakeVisible(outGainSlider);

	// === DELAY FEEDBACK Properties ===

		// Delay section settings
	configureKnobLabel(delaySectionLabel, "Delay Settings");
	delaySectionLabel.setFont(juce::Font(16.0f, juce::Font::bold));
	contentDelay.addAndMakeVisible(delaySectionLabel);

		// Slider settings
	delayFeedbackSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
	delayFeedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 25);
	delayFeedbackSlider.setTextValueSuffix(" %");
	delayFeedbackSlider.setRange(0.0f, 100.0f);
	delayFeedbackSlider.setTooltip("Controls the volume of audio fed back into the delay effect. Inrease to make the echoes repeat more and fade out slower.");
	
		// Label settings
	configureKnobLabel(delayFeedbackLabel, "Feedback"); 

	contentDelay.addAndMakeVisible(delayFeedbackLabel);	
	contentDelay.addAndMakeVisible(delayFeedbackSlider);		

	// === DELAY TIME Properties ===

		//Slider settings
	delayTimeSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
	delayTimeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 25);
	delayTimeSlider.setTextValueSuffix(" sec");
	delayTimeSlider.setRange(0.0f, 2.0f);
	delayTimeSlider.setTooltip("Controls the time between the original sound and its repetitions. Increase for faster echoes. Very small values and modulation can result in interesting effects.");
	
		// Label settings
	configureKnobLabel(delayTimeLabel, "Delay Time");

	contentDelay.addAndMakeVisible(delayTimeLabel);
	contentDelay.addAndMakeVisible(delayTimeSlider);

	// === Compressor Properties ===
	
		// Compressor section settings
	configureKnobLabel(compressorSectionLabel, "Compressor Settings");
	compressorSectionLabel.setFont(juce::Font(16.0f, juce::Font::bold));
	contentCompressor.addAndMakeVisible(compressorSectionLabel);

	//  --- Ratio ---
	const auto& ratioChoices = SimpleGainSliderAudioProcessor::getRatioChoices();

	ratioSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
	ratioSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	ratioSlider.setRange(0.0, ratioChoices.size() - 1.0, 1.0);

	// Labels
	configureKnobLabel(ratioLabel, "Ratio");

	ratioValueLabel.setJustificationType(juce::Justification::centred);
	ratioSlider.onValueChange = [this, ratioChoices]() // Capture needed variables (this for label, ratioChoices)
		{
			int index = static_cast<int>(std::round(ratioSlider.getValue()));	// In case of fp error

			// Ensure in range
			index = juce::jlimit(0, ratioChoices.size() - 1, index);

			juce::String choiceText = ratioChoices[index];

			// Add to value label
			ratioValueLabel.setText(choiceText + ":1", juce::dontSendNotification);
		};


	// -- Threshold --

	thresholdSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
	thresholdSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	thresholdSlider.setRange(-60.0f, 12.0f);

	configureKnobLabel(thresholdLabel, "Threshold");
	thresholdValueLabel.setEditable(false, true, false);
	thresholdValueLabel.addListener(this);
	thresholdSlider.onValueChange = [this]()
		{
			juce::String thresholdText = juce::String(thresholdSlider.getValue(), 1);
			thresholdValueLabel.setText(thresholdText + " dB", juce::dontSendNotification);
		};

	// -- Attack --

	attackSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
	attackSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	attackSlider.setRange(5.0f, 500.0f);

	configureKnobLabel(attackLabel, "Attack");
	attackValueLabel.setEditable(false, true, false);
	attackValueLabel.addListener(this);
	attackSlider.onValueChange = [this]()
		{
			juce::String attackText = juce::String(attackSlider.getValue(), 1);
			attackValueLabel.setText(attackText + " ms", juce::dontSendNotification);
		};

	// -- Release --

	releaseSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
	releaseSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	releaseSlider.setRange(5.0f, 500.0f);

	configureKnobLabel(releaseLabel, "Release");
	releaseValueLabel.setEditable(false, true, false);
	releaseValueLabel.addListener(this);
	releaseSlider.onValueChange = [this]()
		{
			juce::String releaseText = juce::String(releaseSlider.getValue(), 1);
			releaseValueLabel.setText(releaseText + " ms", juce::dontSendNotification);
		};

	contentCompressor.addAndMakeVisible(compressorSectionLabel);

	contentCompressor.addAndMakeVisible(ratioSlider);
	contentCompressor.addAndMakeVisible(ratioLabel);
	contentCompressor.addAndMakeVisible(ratioValueLabel);

	contentCompressor.addAndMakeVisible(thresholdSlider);
	contentCompressor.addAndMakeVisible(thresholdLabel);
	contentCompressor.addAndMakeVisible(thresholdValueLabel);

	contentCompressor.addAndMakeVisible(attackSlider);
	contentCompressor.addAndMakeVisible(attackLabel);
	contentCompressor.addAndMakeVisible(attackValueLabel);

	contentCompressor.addAndMakeVisible(releaseSlider);
	contentCompressor.addAndMakeVisible(releaseLabel);
	contentCompressor.addAndMakeVisible(releaseValueLabel);



	// === Spectrum Analyser properties ===
	contentSpectrum.setColour(juce::GroupComponent::outlineColourId, juce::Colours::transparentBlack);
	contentSpectrum.addAndMakeVisible(spectrumAnalyser);
	spectrumAnalyser.setTooltip("Visualizes the frequency spectrum of the audio signal after all effects are applied.");

    setSize(800, 400);
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

	auto gainContentWidth = 70;

	contentInGain.setBounds(area.removeFromLeft(gainContentWidth));
	contentOutGain.setBounds(area.removeFromRight(gainContentWidth));

	auto contentSpaceWidth = area.getWidth();
	auto contentSpaceHeight = area.getHeight();

	contentSpectrum.setBounds(area.removeFromBottom(contentSpaceHeight/2));
	contentDelay.setBounds(area.removeFromLeft(contentSpaceWidth / 2));
	contentCompressor.setBounds(area);
	
	
	// ==== GAIN UI ====

	// Input gain
	juce::FlexBox leftSidebarFlex;
	leftSidebarFlex.flexDirection = juce::FlexBox::Direction::column;  
	leftSidebarFlex.justifyContent = juce::FlexBox::JustifyContent::flexStart; 
	leftSidebarFlex.alignItems = juce::FlexBox::AlignItems::center;    
	const int gainLabelHeight = inGainLabel.getFont().getHeight() + 10; // Fixed label height

	leftSidebarFlex.items.addArray({
		juce::FlexItem(inGainSlider).withWidth(contentInGain.getWidth()).withHeight(contentInGain.getHeight() - gainLabelHeight - 10).withMargin({0, 0, 0, 0}), // Slider takes space + 5px bottom margin
		juce::FlexItem(inGainLabel).withHeight(gainLabelHeight).withWidth(contentInGain.getWidth()) // Fixed label height
		});

	// Apply layout to sidebar
	leftSidebarFlex.performLayout(contentInGain.getLocalBounds().reduced(5));

	// Output gain
	juce::FlexBox rightSidebarFlex;
	rightSidebarFlex.flexDirection = juce::FlexBox::Direction::column;
	rightSidebarFlex.justifyContent = juce::FlexBox::JustifyContent::flexStart;
	rightSidebarFlex.alignItems = juce::FlexBox::AlignItems::center;

	rightSidebarFlex.items.addArray({
		juce::FlexItem(outGainSlider).withWidth(contentOutGain.getWidth()).withHeight(contentOutGain.getHeight() - gainLabelHeight - 10).withMargin({0, 0, 0, 0}), // Slider takes space + 5px bottom margin
		juce::FlexItem(outGainLabel).withHeight(gainLabelHeight).withWidth(contentOutGain.getWidth()) // Fixed label height
		});

	// Apply layout to sidebar
	rightSidebarFlex.performLayout(contentOutGain.getLocalBounds().reduced(5));


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

	// ==== COMPRESSOR UI ====

	// Set initial value display
	ratioSlider.onValueChange();
	thresholdSlider.onValueChange();
	attackSlider.onValueChange();
	releaseSlider.onValueChange();

		// Make grid layout
	juce::Grid compGrid;
	compGrid.alignItems = juce::Grid::AlignItems::center;
	compGrid.justifyItems = juce::Grid::JustifyItems::center;

	compGrid.templateRows = {
		juce::Grid::TrackInfo(juce::Grid::Fr(1)),  // Title row
		juce::Grid::TrackInfo(juce::Grid::Fr(2)),  // Ratio | Attack row
		juce::Grid::TrackInfo(juce::Grid::Fr(2)),  // Ratio Val | Attack Val row
		juce::Grid::TrackInfo(juce::Grid::Fr(2)),   // Threshhold | Release row
		juce::Grid::TrackInfo(juce::Grid::Fr(2))   // Threshhold Val | Release Val row
	};

	compGrid.templateColumns = {
		juce::Grid::Fr(2),	// Ratio and threshold labels
		juce::Grid::Fr(3),	// Ratio and threshold knobs

		juce::Grid::Fr(3),	// Attack and release knobs
		juce::Grid::Fr(2),	// Attack and release knobs
	};

	compGrid.items = {
		juce::GridItem(compressorSectionLabel).withArea(1, 1, 1, 5),

		// --- Left Side ---
		juce::GridItem(ratioLabel).withArea(2, 1), // Row 2, Col 1
		juce::GridItem(ratioValueLabel).withArea(3, 1).withMargin({ - 10, 0, 0, 0 }), // Row 3, Col 1
		juce::GridItem(ratioSlider).withArea(2,2,4,2), // Rows 2-3, Col 2

		juce::GridItem(thresholdLabel).withArea(4, 1), // Row 4, Col 1
		juce::GridItem(thresholdValueLabel).withArea(5, 1).withMargin({ -10, 0, 0, 0 }), // Row 5, Col 1
		juce::GridItem(thresholdSlider).withArea(4, 2, 6, 2), // Rows 4-5, Col 2

		// --- Right Side ---
		juce::GridItem(attackLabel).withArea(2, 4), // Row 2, Col 4
		juce::GridItem(attackValueLabel).withArea(3, 4).withMargin({ -10, 0, 0, 0 }), // Row 3, Col 4
		juce::GridItem(attackSlider).withArea(2, 3, 4, 3), // Rows 2-3, Col 3

		juce::GridItem(releaseLabel).withArea(4, 4), // Row 4, Col 4
		juce::GridItem(releaseValueLabel).withArea(5, 4).withMargin({ -10, 0, 0, 0 }), // Row 5, Col 4
		juce::GridItem(releaseSlider).withArea(4, 3, 6, 3)  // Rows 4-5, Col 3
	};


	compGrid.performLayout(contentCompressor.getLocalBounds().reduced(5)); // Set bounds for compressor grid
}	

void SimpleGainSliderAudioProcessorEditor::labelTextChanged(juce::Label* labelChanged)
{
	// Find label edited
	juce::String paramID = ""; 
	juce::Slider* correspondingSlider = nullptr; 

	if (labelChanged == &thresholdValueLabel)
	{
		paramID = THRESHOLD_ID;
		correspondingSlider = &thresholdSlider;
	}
	else if (labelChanged == &attackValueLabel)
	{
		paramID = ATTACK_ID;
		correspondingSlider = &attackSlider;
	}
	else if (labelChanged == &releaseValueLabel)
	{
		paramID = RELEASE_ID;
		correspondingSlider = &releaseSlider;
	}
	else
	{
		return;
	}

	// --- Get the parameter and the entered text ---
	auto* parameterBase = audioProcessor.treeState.getParameter(paramID);
	juce::String newText = labelChanged->getText(); // Raw user input

	auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(parameterBase);
		
	// Remove units
	juce::String parsedText = newText.removeCharacters(" dBms");

	static const std::regex pattern("^-?\\d+(\\.\\d*)?$");

	if (std::regex_match(parsedText.toStdString(), pattern))
	{
		// Check if in value range
		auto range = floatParam->getNormalisableRange(); // Get the parameter's valid range
		auto parsedValue = parsedText.getFloatValue();

		if (range.getRange().contains(parsedValue))
		{
			*floatParam = parsedValue; // Update the parameter
		}
		else
		{
			// Value entered is out of range
			if (correspondingSlider != nullptr)
			{
				correspondingSlider->onValueChange(); // Reset val label
			}
		}
	}

	else
	{
		// Value entered is out of range
		if (correspondingSlider != nullptr)
		{
			correspondingSlider->onValueChange(); // Reset val label
		}
	}

}

void SimpleGainSliderAudioProcessorEditor::configureGainSlider(juce::Slider& slider, const juce::String& tooltip)
{
	slider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
	slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 25);
	slider.setTextValueSuffix(" dB");
	slider.setRange(-48.0f, 24.0f);
	slider.setTooltip(tooltip);
	slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}