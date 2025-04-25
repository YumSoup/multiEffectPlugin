/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.
    Frontend - aesthetics, how values change
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
// Constructor: Caled when window is created
SimpleGainSliderAudioProcessorEditor::SimpleGainSliderAudioProcessorEditor (SimpleGainSliderAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Create slider attachments
	jassert(audioProcessor.treeState.getParameter(GAIN_ID) != nullptr); 
    gainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, GAIN_ID , gainSlider); 
	jassert(audioProcessor.treeState.getParameter(DELAY_FEEDBACK_ID) != nullptr); 
	delayFeedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, DELAY_FEEDBACK_ID, delayFeedbackSlider);
	jassert(audioProcessor.treeState.getParameter(DELAY_TIME_ID) != nullptr);
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, DELAY_TIME_ID, delayTimeSlider); 

	addAndMakeVisible(header);
	addAndMakeVisible(sidebar);
	addAndMakeVisible(footer);
	addAndMakeVisible(contentDelay);

	delaySectionLabel.setText("Delay Settings", juce::dontSendNotification);
	delaySectionLabel.setJustificationType(juce::Justification::centred);
	delaySectionLabel.setFont(juce::Font(16.0f, juce::Font::bold));
	contentDelay.addAndMakeVisible(delaySectionLabel);

	// === gainSlider Properties ===

    gainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 25);
	gainSlider.setTextValueSuffix(" dB");
    gainSlider.setRange(-48.0f, 6.0f);

	gainLabel.setText("Out Gain", juce::dontSendNotification);
	gainLabel.setJustificationType(juce::Justification::centred);
	gainLabel.setFont(juce::Font(14.0f, juce::Font::bold));
	gainLabel.setBorderSize(juce::BorderSize<int>(0, 0, -5, 0)); 
	
	sidebar.addAndMakeVisible(gainLabel);
	sidebar.addAndMakeVisible(gainSlider);

	// === delayFeedbackSlider Properties ===

	delayFeedbackSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
	delayFeedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 25);
	delayFeedbackSlider.setTextValueSuffix(" %");
	delayFeedbackSlider.setRange(0.0f, 100.0f);
	

	delayFeedbackLabel.setText("Feedback", juce::dontSendNotification);		// label settings
	delayFeedbackLabel.attachToComponent(&delayFeedbackSlider, false);
	delayFeedbackLabel.setJustificationType(juce::Justification::centred);
	delayFeedbackLabel.setBorderSize(juce::BorderSize<int>(0, 0, -5, 0));

	contentDelay.addAndMakeVisible(delayFeedbackSlider);		

	// === delayTimeSlider Properties ===

	delayTimeSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalDrag);
	delayTimeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 25);
	delayTimeSlider.setTextValueSuffix(" sec");
	delayTimeSlider.setRange(0.0f, 2.0f);

	delayTimeLabel.setText("Time", juce::dontSendNotification);	// label settings
	//delayTimeLabel.attachToComponent(&delayTimeSlider, false);
	//delayTimeLabel.setJustificationType(juce::Justification::centred);
	//delayTimeLabel.setBorderSize(juce::BorderSize<int>(0, 0, -5, 0));

	contentDelay.addAndMakeVisible(delayTimeSlider);

    setSize(200, 400);
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

void SimpleGainSliderAudioProcessorEditor::resized() 
{
	auto area = getLocalBounds();
	auto headerHeight = 40;
	auto footerHeight = 30;
	auto sidebarWidth = 70;
	header.setBounds(area.removeFromTop(headerHeight));
	footer.setBounds(area.removeFromBottom(footerHeight));
	sidebar.setBounds(area.removeFromLeft(sidebarWidth));
	contentDelay.setBounds(area.removeFromTop(area.getHeight() / 2));
    
	// ==== GAIN UI ====

	// Make flexbox layout
	juce::FlexBox sidebarFlex;
	sidebarFlex.flexDirection = juce::FlexBox::Direction::column;  
	sidebarFlex.justifyContent = juce::FlexBox::JustifyContent::flexStart; 
	sidebarFlex.alignItems = juce::FlexBox::AlignItems::center;    
	const int gainLabelHeight = gainLabel.getFont().getHeight() + 10; // Fixed label height

	sidebarFlex.items.addArray({
		juce::FlexItem(gainSlider).withWidth(sidebar.getWidth()).withHeight(sidebar.getHeight() - gainLabelHeight - 10).withMargin({0, 0, 0, 0}), // Slider takes space + 5px bottom margin
		juce::FlexItem(gainLabel).withHeight(gainLabelHeight).withWidth(sidebar.getWidth()) // Fixed label height
		});

	// Apply layout to sidebar
	sidebarFlex.performLayout(sidebar.getLocalBounds().reduced(5));

	// ==== DELAY UI ====

    // Make grid layout
    juce::Grid grid;
	grid.alignItems = juce::Grid::AlignItems::center;
	grid.justifyItems = juce::Grid::JustifyItems::center;

    grid.templateRows = {
        juce::Grid::TrackInfo(juce::Grid::Fr(1)),  // Title row
        juce::Grid::TrackInfo(juce::Grid::Fr(2))   // Sliders row
    };
    grid.templateColumns = {
        juce::Grid::Fr(1),
        juce::Grid::Fr(1)
    };

    grid.items = {
        juce::GridItem(delaySectionLabel).withArea(1, 1, 1, 3),
        juce::GridItem(delayFeedbackSlider).withMargin({0,0,20,0}),	// Top, Right, Bottom, Left	
		juce::GridItem(delayTimeSlider).withMargin({0,0,20,0})	// Top, Right, Bottom, Left
    };
	
    grid.performLayout(contentDelay.getLocalBounds().reduced(5));
	
}	


//class KnobLabel : public juce::Label
//{
//public:
//	KnobLabel(juce::Component& componentToAttachTo)
//	{
//		// Style the label
//		setColour(juce::Label::textColourId, juce::Colours::white);
//		setFont(juce::Font(14.0f, juce::Font::plain));
//		setJustificationType(juce::Justification::centred);
//		setBorderSize(juce::BorderSize<int>(0, 0, -5, 0));
//
//		// Attach to component and position below it
//		attachToComponent(&componentToAttachTo, false);
//	}
//
//	// Optional: Override to customize positioning
//	void resized() override
//	{
//		Label::resized();
//		setBounds(getBounds().withY(getAttachedComponent()->getBottom() + 5)); // 5px gap
//	}
//};