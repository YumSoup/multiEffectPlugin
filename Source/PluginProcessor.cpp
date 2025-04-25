/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
SimpleGainSliderAudioProcessor::SimpleGainSliderAudioProcessor():
#ifndef JucePlugin_PreferredChannelConfigurations
      AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    

#endif
    //initialisation list
    treeState(*this, nullptr, "PARAMETERS", createParameterLayout())  //construct treeState with  parameter list

{  //processor construtor code:

}

// Destructor
SimpleGainSliderAudioProcessor::~SimpleGainSliderAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleGainSliderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleGainSliderAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleGainSliderAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleGainSliderAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleGainSliderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleGainSliderAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleGainSliderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleGainSliderAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleGainSliderAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleGainSliderAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}
//==============================================================================

void SimpleGainSliderAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)  //in case  of change in sample rate or buffer size
{
    auto delayBufferSize = sampleRate * 2.5; //delay buffer is 2 seconds of sample time
    delayBuffer.setSize(getNumOutputChannels(), (int)delayBufferSize);
	delayBuffer.clear();

	gainValueSmoothed.reset(sampleRate, 0.05f);

    delayTimeSmoothedChannels[0].reset(sampleRate, 0.2f);
	delayTimeSmoothedChannels[1].reset(sampleRate, 0.2f);
	delayFeedbackSmoothed.reset(sampleRate, 0.05f);

}

void SimpleGainSliderAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleGainSliderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

// __________________________________PROCESS BLOCK__________________________________________________________________
void SimpleGainSliderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();
    auto delayFeedbackParameter = treeState.getRawParameterValue(DELAY_FEEDBACK_ID)->load();
    auto delayTimeParameter = treeState.getRawParameterValue(DELAY_TIME_ID)->load();
	auto gainSliderParameter = treeState.getRawParameterValue(GAIN_ID)->load();

    // Clear junk data in output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {     
        buffer.clear(i, 0, bufferSize);
        delayBuffer.clear(i, 0, delayBufferSize);
    }
	
    gainValueSmoothed.setTargetValue(gainSliderParameter);

	delayFeedbackSmoothed.setTargetValue(delayFeedbackParameter / 100); // set target value for delay gain
	delayTimeSmoothedChannels[0].setTargetValue(delayTimeParameter); // set target value for delay time
    delayTimeSmoothedChannels[1].setTargetValue(delayTimeParameter);

    // For EACH CHANNEL:
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
		// Delay processing
        auto* channelData = buffer.getWritePointer(channel);
        auto* delayData = delayBuffer.getWritePointer(channel);
        for (int sample = 0; sample < bufferSize; ++sample) { 	// iterate through every sample in buffer
			int sampleWritePosition = (writePosition + sample) % delayBufferSize;
            

			// Read from delay buffer
            float delayTime = delayTimeSmoothedChannels[channel].getNextValue();
            double readPos = (writePosition + sample) - (delayTime * getSampleRate());
            if (readPos < 0) readPos += delayBufferSize;

            // Get read positions
            int readPosInt = static_cast<int>(readPos);
			int nextReadPos = (readPosInt + 1) % delayBufferSize;
			int nextnextReadPos = (readPosInt + 2) % delayBufferSize;
			int prevReadPos = (readPosInt - 1 + delayBufferSize) % delayBufferSize;

			float frac = readPos - readPosInt; // fraction from readPos to nextReadPos

            // Cubic interpolate
			float delayedSample = cubicHermiteInterpolate(delayData[prevReadPos], delayData[readPosInt], delayData[nextReadPos], delayData[nextnextReadPos], frac);

			// Write dry + delayedSample to delay buffer
            delayData[sampleWritePosition] = channelData[sample] + (delayedSample * delayFeedbackSmoothed.getNextValue());

            // Mix delayedSample into output
            channelData[sample] += delayedSample;
		}

        //Gain function 
		for (int sample = 0; sample < bufferSize; ++sample) {      // iterate through every sample in buffer
            channelData[sample] = buffer.getSample(channel, sample) * (pow(10, gainValueSmoothed.getNextValue() / 20));     // multiply input stream by gain volume
        }
    }

	updateDelayBufferWritePosition(bufferSize); // Increment writePosition by amount of buffer copied to delayBuffer
} 
// ____________________________________________END PROCESS BLOCK_________________________________________________________________


// Increment delay buffer write position by buffersize
void SimpleGainSliderAudioProcessor::updateDelayBufferWritePosition(int bufferSize) {
	int delayBufferSize = delayBuffer.getNumSamples();
    writePosition += bufferSize; //increment writePosition
	writePosition %= delayBufferSize; //wrap around
}

// Cubic Hermite Interpolation function
float SimpleGainSliderAudioProcessor::cubicHermiteInterpolate(float y0, float y1, float y2, float y3, float fractionalPosition) // Interpolate between y1 and y2
{   
    // Audio optimised formula based off of: https://github.com/kmatheussen/radium/blob/master/audio/SampleInterpolator.cpp
    // written by Kjetil Matheussen
    

    // Calculates a windowed cubic spline interpolation, using custom coefficients instead of Hermite basis functions.
    // This priorities speed over mathmatical accuracy (unlike like windowed sinc)

	// Half-weight the outer points to reduce pull of distant samples
    float half_y0 = 0.5f * y0;
    float half_y3 = 0.5f * y3;

    // Get tangeant at y1
    float slope_y1_to_y2 = (0.5f * y2) - half_y0;

    // Calculate the curvature components
	float curvature_term1 = (y0 + (2.0f * y2)) - (half_y3 + (2.5f * y1));       // dampens y1
	float curvature_term2 = (half_y3 + (1.5f * y1)) - ((1.5f * y2) + half_y0); // Flatten curve at y2 to prevent overshoot 

    // Combine all components using Horner's method for efficiency
    float interpolatedValue = y1 + fractionalPosition * (
        slope_y1_to_y2 + fractionalPosition * (
            curvature_term1 + fractionalPosition * (
                curvature_term2
                )
            )
        );

    return interpolatedValue;
}

// Parameter layout
juce::AudioProcessorValueTreeState::ParameterLayout SimpleGainSliderAudioProcessor::createParameterLayout()
{
    std::vector <std::unique_ptr<juce::RangedAudioParameter>> params; //container

    auto attackReleaseRange = juce::NormalisableRange<float>(5, 500, 1, 1);
	auto ratioChoices = juce::StringArray{ "1", "1.5", "2", "3", "4", "5", "10", "100"}; // ratio choices


    //PARAMETER LIST - ID, name, min, max, default

	auto gainParam = std::make_unique<juce::AudioParameterFloat>(GAIN_ID, GAIN_NAME, -48.0f, 0.0f, -1.0f); 
	auto delayFeedbackParam = std::make_unique<juce::AudioParameterFloat>(DELAY_FEEDBACK_ID, DELAY_FEEDBACK_NAME, 0.0f, 100.0f, 50.f);
	auto delayTimeParam = std::make_unique<juce::AudioParameterFloat>(DELAY_TIME_ID, DELAY_TIME_NAME, 0.0f, 2.0f, 0.6f);

    //threshold: -50db min, +12db max, 1db step, 1 skew factor
	auto threshParam = std::make_unique<juce::AudioParameterFloat>(THRESHOLD_ID, THRESHOLD_NAME, juce::NormalisableRange<float>(-60, 12, 1, 1 ), 0);
	auto attackParam = std::make_unique<juce::AudioParameterFloat>(ATTACK_ID, ATTACK_NAME, attackReleaseRange, 50);
	auto releaseParam = std::make_unique<juce::AudioParameterFloat>(RELEASE_ID, RELEASE_NAME, attackReleaseRange, 250);

	auto ratioParam = std::make_unique<juce::AudioParameterChoice>(RATIO_ID, RATIO_NAME, ratioChoices, 3); // ratio choices
	


	// Add parameters to container
    params.push_back(std::move(gainParam)); 
	params.push_back(std::move(delayFeedbackParam));
	params.push_back(std::move(delayTimeParam));
    params.push_back(std::move(threshParam));
	params.push_back(std::move(attackParam));
	params.push_back(std::move(releaseParam));
	params.push_back(std::move(ratioParam));
    
    return { params.begin(), params.end() };

}

// Deprecated - Read from delay buffer and add to buffer
void SimpleGainSliderAudioProcessor::readFromDelayBuffer(juce::AudioBuffer<float>& buffer, int channel)
{
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

    auto delayGain = (treeState.getRawParameterValue(DELAY_FEEDBACK_ID)->load()); //get delay gain parameter
    auto delayTime = (treeState.getRawParameterValue(DELAY_TIME_ID)->load());

    auto readPosition = writePosition - (delayTime * getSampleRate());   // set readPos relative to writePos in delayBuffer
    if (readPosition < 0) {
        readPosition += delayBufferSize; // if readPosition is negative, wrap around
    }

    // Adding bufferSize portion of delayBuffer to buffer
    if (readPosition + bufferSize < delayBufferSize)    // If bufferSized portion of delayBuffer has no loop
    {
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), bufferSize, delayGain, delayGain);
    }
    else {
        // bufferSized portion of delayBuffer has loop
        auto numSamplesToEnd = delayBufferSize - readPosition;
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), numSamplesToEnd, delayGain, delayGain);

        auto numSamplesAtStart = bufferSize - numSamplesToEnd;
        buffer.addFromWithRamp(channel, numSamplesToEnd, delayBuffer.getReadPointer(channel, 0), numSamplesAtStart, delayGain, delayGain);
    }
}

// Deprecated - Copy main buffer into delay buffer
void SimpleGainSliderAudioProcessor::fillBuffer(juce::AudioBuffer<float>& buffer, int channel) {

    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

    //check if main buffer copies to delay buffer without wrapping
    if (delayBufferSize >= writePosition + bufferSize) {

        //copy main buffer to delay buffer
        delayBuffer.copyFrom(channel, writePosition, buffer.getWritePointer(channel), bufferSize);
    }
    //if wrapping
    else
    {
        auto numSamplesToEnd = delayBufferSize - writePosition;      // space remaining until end of delay buffer
        delayBuffer.copyFrom(channel, writePosition, buffer.getWritePointer(channel), numSamplesToEnd);

        auto numSamplesAtStart = bufferSize - numSamplesToEnd;         // Calc the remaining content amount
        delayBuffer.copyFrom(channel, 0, buffer.getWritePointer(channel, numSamplesToEnd), numSamplesAtStart);
    }

    /* DBG("Delay buffer size: " << delayBufferSize);
     DBG("Buffer size: " << bufferSize);
     DBG("Write position: " << writePosition);*/
}

//==============================================================================
bool SimpleGainSliderAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleGainSliderAudioProcessor::createEditor()
{
    return new SimpleGainSliderAudioProcessorEditor (*this);
}

void SimpleGainSliderAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Store parameters from treeState

    juce::MemoryOutputStream memOutStream(destData, true);
    treeState.state.writeToStream(memOutStream);

}

void SimpleGainSliderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	// Restore parameters to treeState

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        treeState.replaceState(tree);
    }

}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() // This creates new instances of the plugin.
{
    return new SimpleGainSliderAudioProcessor();
}
