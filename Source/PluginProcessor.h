/*
  ==============================================================================
    This file contains the basic framework code for a JUCE plugin processor.

    .h holds variables
    Algorithmns and math processing goes here
    
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

#define INGAIN_ID "inGain"
#define INGAIN_NAME "Input Gain"

#define OUTGAIN_ID "outGain"
#define OUTGAIN_NAME "Output Gain"

#define DELAY_FEEDBACK_ID "delayGain"
#define DELAY_FEEDBACK_NAME "Delay Gain"

#define DELAY_TIME_ID "delayTime"
#define DELAY_TIME_NAME "Delay Time"

#define THRESHOLD_ID "threshold"
#define THRESHOLD_NAME "Threshold"

#define ATTACK_ID "attack"
#define ATTACK_NAME "Attack"

#define RELEASE_ID "release"
#define RELEASE_NAME "Release"

#define RATIO_ID "ratio"
#define RATIO_NAME "Ratio"

//==============================================================================
/**
*/

// StateStorage class template for thread-safe storage of state variables
template <typename T>
class StateStorage
{
public:
    // Constructor
    StateStorage(T initialValue = T{}) : value(initialValue) {}

    // Set 
    void set(T newValue)
    {
        value.store(newValue); 
    }

    // Get 
    T get() const
    {
        return value.load(); 
    }

private:
    std::atomic<T> value; // std::atomic for thread safety
};




class SimpleGainSliderAudioProcessor  : public juce::AudioProcessor,
    public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    SimpleGainSliderAudioProcessor();
    ~SimpleGainSliderAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    int getFftSize() const;

    juce::AudioProcessorValueTreeState treeState; //declare a treeState the processor should have
    
    static const juce::StringArray getRatioChoices() {
        static const juce::StringArray choices{ "1", "1.5", "2", "3", "4", "5", "10", "100" };
        return choices;
    }

    // Get FFT results
    std::vector<float> SimpleGainSliderAudioProcessor::getLatestMagnitudesDb() const
    {
        const juce::ScopedLock lock(scopeLock); // Lock during read
        return fftMagnitudesDb; // Return a copy of the member variable
    }

    bool isNextFFTBlockReady() const noexcept
    {
        return nextFFTBlockReady.load();
    }

    void resetNextFFTBlockReady() noexcept
    {
        nextFFTBlockReady.store(false);
    }

    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
	// == Parameters ==
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioParameterFloat* attackParamPtr{ nullptr };   //compressor
    juce::AudioParameterFloat* releaseParamPtr{ nullptr };
    juce::AudioParameterFloat* thresholdParamPtr{ nullptr };
    juce::AudioParameterChoice* ratioParamPtr{ nullptr };

	juce::AudioParameterFloat* inGainParamPtr{ nullptr };   //gain
	juce::AudioParameterFloat* outGainParamPtr{ nullptr };

	juce::AudioParameterFloat* delayFeedbackParamPtr{ nullptr };    //delay
	juce::AudioParameterFloat* delayTimeParamPtr{ nullptr };

	// === Gain ===
    juce::LinearSmoothedValue<float> inGainValueSmoothed{ 0.0f };
    juce::LinearSmoothedValue<float> outGainValueSmoothed{ 0.0f };

	// === Delay ===
	juce::LinearSmoothedValue<float> delayFeedbackSmoothed{ 0.0f };
    std::array<juce::LinearSmoothedValue<float>, 2> delayTimeSmoothedChannels;
    juce::AudioBuffer<float> delayBuffer;
    int writePosition{ 0 };
    
	// == Compressor ===

	juce::dsp::Compressor<float> compressor;



    //=== Fast Fourier Transform === 
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;



	std::vector<float> fftMagnitudesDb; // Stores magnitudes of FFT data in dB
    juce::CriticalSection scopeLock;               // Protects fftMagnitudesDb access

	static constexpr int fftOrder = 11;     // Size of the FFT window
	static constexpr int fftSize = 1 << fftOrder; // Calculate the size of the FFT - 2^11
    const float minDb = -100.0f;
    std::atomic<bool> nextFFTBlockReady{ false };        // Whether next FFT block is ready for processing     

	std::array<float, fftSize> fifo;        // Contains incoming samples FIFO
    int fifoIndex = 0;                     // Count of samples in FIFO

	std::array<float, fftSize * 2> fftData; // Stores results of FFT

    float cubicHermiteInterpolate(float y0, float y1, float y2, float y3, float fractionalPosition);
    void pushNextSampleIntoFifo(float sample);
    void performFFTProcessing();
    void updateDelayBufferWritePosition(int bufferSize);

    // ==== Legacy buffer functions ====
    void fillBuffer(juce::AudioBuffer<float>& buffer, int channel);
    void readFromDelayBuffer(juce::AudioBuffer<float>& buffer, int channel);
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleGainSliderAudioProcessor)
};
