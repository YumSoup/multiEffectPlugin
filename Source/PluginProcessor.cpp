
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

    // Initialisation list:

    treeState(*this, nullptr, "PARAMETERS", createParameterLayout()),  // Construct treeState with parameter list

	forwardFFT(fftOrder),   // Init forwardFFT with size of window
  
	window(fftSize, juce::dsp::WindowingFunction<float>::hann) // Init window as Hann 

{  // Processor construtor code:
    
    fftMagnitudesDb.resize(fftSize / 2);  // Set size of fftMagnitudesDb
    std::fill(fftMagnitudesDb.begin(), fftMagnitudesDb.end(), minDb); // Set entire fftMagnitudesDb to dB floor
    fifo.fill(0.0f);
	fftData.fill(0.0f);

	// Cast parameter pointers to their types
    attackParamPtr = dynamic_cast<juce::AudioParameterFloat*>(treeState.getParameter(ATTACK_ID)); 
    releaseParamPtr = dynamic_cast<juce::AudioParameterFloat*>(treeState.getParameter(RELEASE_ID));
    thresholdParamPtr = dynamic_cast<juce::AudioParameterFloat*>(treeState.getParameter(THRESHOLD_ID));
	ratioParamPtr = dynamic_cast<juce::AudioParameterChoice*>(treeState.getParameter(RATIO_ID));

	inGainParamPtr = dynamic_cast<juce::AudioParameterFloat*>(treeState.getParameter(INGAIN_ID));
    outGainParamPtr = dynamic_cast<juce::AudioParameterFloat*>(treeState.getParameter(OUTGAIN_ID));

	delayFeedbackParamPtr = dynamic_cast<juce::AudioParameterFloat*>(treeState.getParameter(DELAY_FEEDBACK_ID)); 
	delayTimeParamPtr = dynamic_cast<juce::AudioParameterFloat*>(treeState.getParameter(DELAY_TIME_ID)); 

    treeState.addParameterListener(ATTACK_ID, this);
    treeState.addParameterListener(RELEASE_ID, this);
    treeState.addParameterListener(THRESHOLD_ID, this);
    treeState.addParameterListener(RATIO_ID, this);

}

// Destructor
SimpleGainSliderAudioProcessor::~SimpleGainSliderAudioProcessor()
{
    treeState.removeParameterListener(ATTACK_ID, this);
    treeState.removeParameterListener(RELEASE_ID, this);
    treeState.removeParameterListener(THRESHOLD_ID, this);
    treeState.removeParameterListener(RATIO_ID, this);
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

void SimpleGainSliderAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)  // In case  of change in sample rate or buffer size
{
    auto delayBufferSize = sampleRate * 2.5; // Delay buffer is 2.5 seconds of sample time
    delayBuffer.setSize(getNumOutputChannels(), (int)delayBufferSize);
	delayBuffer.clear();

	inGainValueSmoothed.reset(sampleRate, 0.05f);
	outGainValueSmoothed.reset(sampleRate, 0.05f);

    delayTimeSmoothedChannels[0].reset(sampleRate, 0.2f);
	delayTimeSmoothedChannels[1].reset(sampleRate, 0.2f);
	delayFeedbackSmoothed.reset(sampleRate, 0.05f);

    // Compressor

	juce::dsp::ProcessSpec spec;
	spec.maximumBlockSize = samplesPerBlock; 
	spec.sampleRate = sampleRate;
	spec.numChannels = getTotalNumOutputChannels();
	compressor.prepare(spec); 
    compressor.reset();
    compressor.setAttack(attackParamPtr->get());
    compressor.setRelease(releaseParamPtr->get());
    compressor.setThreshold(thresholdParamPtr->get());
    compressor.setRatio(ratioParamPtr->getCurrentChoiceName().getFloatValue());

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

// __________________________________PROCESS BLOCK__________________________________________________________________________

void SimpleGainSliderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

	auto delayFeedbackParameter = delayFeedbackParamPtr->get(); 
	auto delayTimeParameter = delayTimeParamPtr->get(); 
    auto inGainSliderParameter = inGainParamPtr->get();
	auto outGainSliderParameter = outGainParamPtr->get();
	
    // Clear junk data in output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {     
        buffer.clear(i, 0, bufferSize);
        delayBuffer.clear(i, 0, delayBufferSize);
    }
	
	// Set targets for smoothed values
    inGainValueSmoothed.setTargetValue(inGainSliderParameter);
	outGainValueSmoothed.setTargetValue(outGainSliderParameter);

	delayFeedbackSmoothed.setTargetValue(delayFeedbackParameter / 100); 
	delayTimeSmoothedChannels[0].setTargetValue(delayTimeParameter); 
    delayTimeSmoothedChannels[1].setTargetValue(delayTimeParameter);


    // === INGAIN AND DELAY PROCESSING ===
  
    // For EACH CHANNEL:
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* delayData = delayBuffer.getWritePointer(channel);

        // For EACH SAMPLE:
        for (int sample = 0; sample < bufferSize; ++sample) { 	

            // IN Gain processing
            channelData[sample] = buffer.getSample(channel, sample) * (pow(10, inGainValueSmoothed.getNextValue() / 20));     // Multiply sample by gain volume
			
			int sampleWritePosition = (writePosition + sample) % delayBufferSize;   // Get writepos for sample for this loop's sample
			float delayTime = delayTimeSmoothedChannels[channel].getNextValue();    // Get delay time 

			// No delay operation, write to delay buffer
            if (delayTime <= 0) {
                delayData[sampleWritePosition] = channelData[sample];
                continue;
            }

			// Get main read position
            double readPos = (writePosition + sample) - (delayTime * getSampleRate());
            if (readPos < 0) readPos += delayBufferSize;

            // Get positions for interpolation
            int readPosInt = static_cast<int>(readPos);
            int nextReadPos = (readPosInt + 1) % delayBufferSize;
            int nextnextReadPos = (readPosInt + 2) % delayBufferSize;
            int prevReadPos = (readPosInt - 1 + delayBufferSize) % delayBufferSize;

            float frac = readPos - readPosInt; // Fraction from readPos to nextReadPos

            // Cubic interpolate
            float delayedSample = cubicHermiteInterpolate(delayData[prevReadPos], delayData[readPosInt], delayData[nextReadPos], delayData[nextnextReadPos], frac);

            // Write dry + delayedSample to delay buffer
            delayData[sampleWritePosition] = channelData[sample] + (delayedSample * delayFeedbackSmoothed.getNextValue());

            // Mix delayedSample into output
            channelData[sample] += delayedSample;
		}
    }
    updateDelayBufferWritePosition(bufferSize); // Increment delay line writePos by amount of buffer copied to delayBuffer

    // === COMPRESSOR PROCESSING === 

    auto block = juce::dsp::AudioBlock<float>(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(block); // Create process context
    compressor.process(context);



	// === OUTGAIN AND FIFO PROCESSING ===

    // For EACH CHANNEL:
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        // For EACH SAMPLE:
        for (int sample = 0; sample < bufferSize; ++sample) {

			// OUT Gain processing
            channelData[sample] = buffer.getSample(channel, sample) * (pow(10, outGainValueSmoothed.getNextValue() / 20));     // Multiply by gain volume
			
            // FFT start process
            if (channel == 0)
            {
                pushNextSampleIntoFifo(channelData[sample]);
            }
            
        }
    }
} 
// ____________________________________________END PROCESS BLOCK_________________________________________________________________

void SimpleGainSliderAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == ATTACK_ID)
    {
        if (attackParamPtr)
            compressor.setAttack(attackParamPtr->get() * 100);
		
        
    }
    else if (parameterID == RELEASE_ID)
    {
        if (releaseParamPtr)
            compressor.setRelease(releaseParamPtr->get());

    }
    else if (parameterID == THRESHOLD_ID)
    {
        if (thresholdParamPtr)
            compressor.setThreshold(thresholdParamPtr->get());

    }
    else if (parameterID == RATIO_ID)
    {
        if (ratioParamPtr)
            compressor.setRatio(ratioParamPtr->getCurrentChoiceName().getFloatValue());
    }
}

// Puts sample into FIFO. If full, calls performFFTProcessing()
void SimpleGainSliderAudioProcessor::pushNextSampleIntoFifo(float sample)
{
    if (fifoIndex == fftSize)  // Fifo is full
    {
         
		if (!isNextFFTBlockReady()) // If UI has consumed the last block
		{
            performFFTProcessing();
		}
        fifoIndex = 0;
    }

    fifo[fifoIndex] = sample;  // Add sample to FIFO at index

    // Increment index
    fifoIndex++;
}

// Perform FFT processing on FIFO. Prepares fftMagnitudesDb for UI
void SimpleGainSliderAudioProcessor::performFFTProcessing() {

    // Apply windowing
	window.multiplyWithWindowingTable(fifo.data(), fftSize); // Apply window to FIFO

	// Copy windowed FIFO data into fftData
    std::fill(fftData.begin(), fftData.end(), 0.0f);    
    std::copy(fifo.begin(), fifo.end(), fftData.begin()); 

	// Perform FFT on fftData
	forwardFFT.performRealOnlyForwardTransform(fftData.data());

	auto numFreqBins = fftSize / 2; 
	std::vector<float> unlockedFftMagnitudesDb(numFreqBins);  // Hold FftMagnitudesDb before locking

    // Calc normalisation factor

    const float normalizationFactor = (float)fftSize / 4.0f;    // Average energy accross fft window, compensating for Hann Window power reduction. 
    const float normalizationFactorSquared = normalizationFactor * normalizationFactor; // Square for dB conversion

    // Get magnitudes and convert to dB
	// For EACH FREQ BIN:
    for (int freqBin = 0; freqBin < numFreqBins; ++freqBin)
    {
        float real = fftData[freqBin * 2];
        float imag = fftData[freqBin * 2 + 1];
        float magSquared = real * real + imag * imag;  // Get magnitude^2 of current freq bin
		float normalisedMagSquared = magSquared / normalizationFactorSquared; // Normalise magnitude^2
        
		//  Convert to dB
        float dBValue;
        if (magSquared > 1e-10f) {
            dBValue = 10.0f * std::log10(normalisedMagSquared);
        }
        else {
            dBValue = minDb; // set to minDb if too small
        }

		unlockedFftMagnitudesDb[freqBin] = std::max(dBValue, minDb); // Store dB value
    }

	// Copy magnitudes to fftMagnitudesDb
	{
        const juce::ScopedLock lock(scopeLock); // Lock during copy
        std::copy(unlockedFftMagnitudesDb.begin(), unlockedFftMagnitudesDb.end(), fftMagnitudesDb.begin());
	}

    nextFFTBlockReady.store(true); // New data is ready for UI

}

// fftSize get function
int SimpleGainSliderAudioProcessor::getFftSize() const { return fftSize; }

// Increment delay buffer write position by buffersize
void SimpleGainSliderAudioProcessor::updateDelayBufferWritePosition(int bufferSize) {
	int delayBufferSize = delayBuffer.getNumSamples();
    writePosition += bufferSize; // Increment writePosition
	writePosition %= delayBufferSize; // Wrap around
}

// Cubic Hermite Interpolation function
float SimpleGainSliderAudioProcessor::cubicHermiteInterpolate(float y0, float y1, float y2, float y3, float fractionalPosition) // Interpolate between y1 and y2
{   
    // Audio optimised formula based off of: https://github.com/kmatheussen/radium/blob/master/audio/SampleInterpolator.cpp
    // Written by Kjetil Matheussen
    
    // Calculates a windowed cubic spline interpolation, using custom coefficients instead of Hermite basis functions.

	// Half-weight the outer points to reduce pull of distant samples
    float half_y0 = 0.5f * y0;
    float half_y3 = 0.5f * y3;

    // Get tangeant at y1
    float slope_y1_to_y2 = (0.5f * y2) - half_y0;

    // Calculate the curvature components
	float curvature_term1 = (y0 + (2.0f * y2)) - (half_y3 + (2.5f * y1));       // Dampens y1
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
	auto ratioChoices = getRatioChoices();  


    //PARAMETER LIST - ID, name, min, max, default

	auto inGainParam = std::make_unique<juce::AudioParameterFloat>(INGAIN_ID, INGAIN_NAME, -48.0f, 24.0f, -1.0f); 
    auto outGainParam = std::make_unique<juce::AudioParameterFloat>(OUTGAIN_ID, OUTGAIN_NAME, -48.0f, 24.0f, -1.0f);
	
    auto delayFeedbackParam = std::make_unique<juce::AudioParameterFloat>(DELAY_FEEDBACK_ID, DELAY_FEEDBACK_NAME, juce::NormalisableRange < float>(0.0f, 100.0f, 0.1f,1.7f),25.0f);
	auto delayTimeParam = std::make_unique<juce::AudioParameterFloat>(DELAY_TIME_ID, DELAY_TIME_NAME, juce::NormalisableRange < float>(0.0f, 2.0f, 0.01f, 0.4f), 0.6f);

    
	auto threshParam = std::make_unique<juce::AudioParameterFloat>(THRESHOLD_ID, THRESHOLD_NAME, juce::NormalisableRange<float>(-60, 12, 1, 1 ), 0); //threshold: -50db min, +12db max, 1db step, 1 skew factor
	auto attackParam = std::make_unique<juce::AudioParameterFloat>(ATTACK_ID, ATTACK_NAME, attackReleaseRange, 50);
	auto releaseParam = std::make_unique<juce::AudioParameterFloat>(RELEASE_ID, RELEASE_NAME, attackReleaseRange, 250);

	auto ratioParam = std::make_unique<juce::AudioParameterChoice>(RATIO_ID, RATIO_NAME, ratioChoices, 3);
	

	// Add parameters to container
    params.push_back(std::move(inGainParam)); 
	params.push_back(std::move(outGainParam));

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
