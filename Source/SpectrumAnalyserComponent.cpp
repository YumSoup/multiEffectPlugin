#include "SpectrumAnalyserComponent.h"

// Use your actual processor class name here
SpectrumAnalyserComponent::SpectrumAnalyserComponent(SimpleGainSliderAudioProcessor& p) :
    audioProcessor(p)
{
    const auto numFreqBins = audioProcessor.getFftSize() / 2;

    fftDisplayData.resize(numFreqBins);

	fftSmoothedData.resize(numFreqBins);

	std::fill(fftDisplayData.begin(), fftDisplayData.end(), minDb);  // Fill display data with min db

    std::fill(fftSmoothedData.begin(), fftSmoothedData.end(), minDb);

    startTimerHz(30);
}

SpectrumAnalyserComponent::~SpectrumAnalyserComponent()
{
    stopTimer();
}

void SpectrumAnalyserComponent::timerCallback()
{
    if (audioProcessor.isNextFFTBlockReady())
    {
		fftDisplayData = audioProcessor.getLatestMagnitudesDb(); // Load FFT Db data

		audioProcessor.resetNextFFTBlockReady();    // Allow next FFT block to process

        // Apply smoothing to new data

        if (fftDisplayData.size() == fftSmoothedData.size())
        {
            // For EACH BIN:
            for (size_t i = 0; i < fftSmoothedData.size(); ++i)
            {
				// Interpolate between smoothed and new data
                fftSmoothedData[i] = (smoothingFactor * fftDisplayData[i]) + ((1.0f - smoothingFactor) * fftSmoothedData[i]);
            }
        }
        repaint(); // Redraw UI
    }
}

void SpectrumAnalyserComponent::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::black); // Background colour
    
    // Component bounds
    auto bounds = getLocalBounds().toFloat().reduced(10.0f); 
    auto width = bounds.getWidth();
    auto height = bounds.getHeight();
    auto left = bounds.getX();
    auto top = bounds.getY();
    auto bottom = bounds.getBottom();
    auto right = bounds.getRight();

	const auto& magnitudes = fftSmoothedData;
    
    auto fftSize = audioProcessor.getFftSize();
	auto numFreqBins = magnitudes.size();
    float sampleRate = audioProcessor.getSampleRate();
    const float maxFreq = (float)sampleRate / 2.0f;

    // Error catching
	if (magnitudes.empty())	return; // No data to display
    if (sampleRate <= 0 || fftSize <= 0) return;
	
    // Set dB and freq ranges

	if (maxFreq <= minFreq) return; // Invalid freq range

    const float logMinFreq = std::log(minFreq);
    const float logFreqRange = std::log(maxFreq) - logMinFreq;

    juce::Path spectrumPath;
    bool pathStarted = false;

    float freqBinWidth = (float)sampleRate / (float)fftSize;

    // For EACH FFT BIN :
    for (size_t fftBin = 1; fftBin < numFreqBins; ++fftBin)
    {
		
        // Get X position of current bin
        float currentBinFreq = freqBinWidth * (float)fftBin;

        // Error catching
		if (currentBinFreq < minFreq) continue;  // Skip out of range bins
		if (currentBinFreq > maxFreq) break; 
		
        float logFreq = std::log(currentBinFreq);
        float normalisedX = juce::jmap(logFreq, logMinFreq, logMinFreq + logFreqRange, 0.0f, 1.0f); // Map frequency logarithmically to  0.0 - 1.0
		float x = left + (width * normalisedX); 

		// Get Y position of current bin

        float currentBinDb = magnitudes[fftBin];
		float normalisedY = juce::jmap(currentBinDb, minDb, maxDb, 0.0f, 1.0f); // Map dB to 0.0 - 1.0
  
        float y = bottom - height * juce::jlimit(0.0f, 1.0f, normalisedY); // Clamp normY

        // Process Path

		if (!pathStarted)
		{
			spectrumPath.startNewSubPath(x, y); // Start path at x,y
			pathStarted = true;
		}
		else
		{
			spectrumPath.lineTo(x, y); // Path to x,y
		}
    }

	// Draw path

	g.setColour(juce::Colours::green); // Path color
	g.strokePath(spectrumPath, juce::PathStrokeType(1.5f)); // Stroke path with 2.0f width

	// Draw axes

	g.setColour(juce::Colours::white);
	g.drawLine(left, top, left, bottom); // Y axis
	g.drawLine(left, bottom, right, bottom); // X axis
	g.drawText("dB", left - 30, top, 20, height, juce::Justification::centred); // dB label
	g.drawText("Freq", right - 50, bottom + 10, 50, 20, juce::Justification::centred); // Freq label

    // === Frequency Lines ===

    juce::Font labelFont(10.0f); // Small font for labels
    g.setFont(labelFont);
    g.setColour(juce::Colours::white.withAlpha(0.3f));

    // Frequencies to draw gridlines for

    std::vector<float> freqsToPlot = {
        30.0f, 50.0f, 100.0f, 200.0f, 500.0f,
        1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f
    };

    // Lambda func to format frequency labels
    auto formatFrequencyLabel = [](float freq) -> juce::String {
        if (freq >= 1000.0f)
			return juce::String(freq / 1000.0f, 1) + "k"; // Add "k" for kHz
        else
            return juce::String(static_cast<int>(freq)); 
        };

	// For EACH FREQ:
    for (float freq : freqsToPlot)
    {
        if (freq >= minFreq && freq <= maxFreq)
        {
            // Calc X position using the same log mapping
            float logFreq = std::log(freq);
            float normalisedX = (logFreq - logMinFreq) / logFreqRange;
            float x = left + width * normalisedX;

            // Draw vertical line
            g.drawVerticalLine(juce::roundToInt(x), top, bottom);

            // Draw Label
            juce::String label = formatFrequencyLabel(freq);
            int textWidth = labelFont.getStringWidth(label);
            int textHeight = labelFont.getHeight();

            // Put label below  bottom axis
            
            juce::Rectangle<int> labelBounds(juce::roundToInt(x - textWidth / 2.0f),
                juce::roundToInt(bottom + 2), // Position below bottom + offset
                textWidth,
                textHeight);

            // Set label colour
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.drawText(label, labelBounds, juce::Justification::centred, 1);
        }
    }

	g.setColour(juce::Colours::white.withAlpha(0.3f));  // Colour for dB lines

	// === Draw dB lines ===
    
    // -72dB line
    float y72 = bottom - height * juce::jmap(-72.0f, minDb, maxDb, 0.0f, 1.0f);
    g.drawHorizontalLine(juce::roundToInt(y72), left, right);
    g.drawText("-72", juce::roundToInt(left + 2), juce::roundToInt(y72 - labelFont.getHeight()), 20, labelFont.getHeight(), juce::Justification::topLeft, 1);

    // -48dB line
    float y48 = bottom - height * juce::jmap(-48.0f, minDb, maxDb, 0.0f, 1.0f);
    g.drawHorizontalLine(juce::roundToInt(y48), left, right);
    g.drawText("-48", juce::roundToInt(left + 2), juce::roundToInt(y48 - labelFont.getHeight()), 20, labelFont.getHeight(), juce::Justification::topLeft, 1);

    // -24dB line
    float y24 = bottom - height * juce::jmap(-24.0f, minDb, maxDb, 0.0f, 1.0f);
    g.drawHorizontalLine(juce::roundToInt(y24), left, right);
    g.drawText("-24", juce::roundToInt(left + 2), juce::roundToInt(y24 - labelFont.getHeight()), 20, labelFont.getHeight(), juce::Justification::topLeft, 1);

    // -12dB line
    float y12 = bottom - height * juce::jmap(-12.0f, minDb, maxDb, 0.0f, 1.0f);
    g.drawHorizontalLine(juce::roundToInt(y12), left, right);
    g.drawText("-12", juce::roundToInt(left + 2), juce::roundToInt(y12 - labelFont.getHeight()), 20, labelFont.getHeight(), juce::Justification::topLeft, 1);

    // 0dB line
    float y0 = bottom - height * juce::jmap(0.0f, minDb, maxDb, 0.0f, 1.0f);
    g.drawHorizontalLine(juce::roundToInt(y0), left, right);
    g.drawText("0", juce::roundToInt(left + 2), juce::roundToInt(y0 - labelFont.getHeight()), 20, labelFont.getHeight(), juce::Justification::topLeft, 1);

}


void SpectrumAnalyserComponent::resized()
{
}