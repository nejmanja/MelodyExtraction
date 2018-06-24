#pragma once
#define dab bool

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class FFTComponent    : public Component, private Timer
{
public:
	FFTComponent() :forwardFFT(fftOrder), spectrogramImage(Image::RGB, 512, 512, true)
    {
		startTimerHz(30);
    }

    ~FFTComponent()
    {
    }

    void paint (Graphics& g) override
    {
		g.fillAll(Colours::black);

		g.setOpacity(1.0f);
		g.drawImage(spectrogramImage, getLocalBounds().toFloat());
    }

    void resized() override
    {

    }

	void pushNextSampleIntoFifo(float sample) noexcept
	{
		if (fifoIndex == fftSize)
		{
			if (!nextFFTBlockReady)
			{
				zeromem(fftData, sizeof(fftData));
				memcpy(fftData, fifo, sizeof(fifo));
				nextFFTBlockReady = true;
			}
			fifoIndex = 0;
		}
		fifo[fifoIndex++] = sample;
	}

	void timerCallback() override
	{
		if (nextFFTBlockReady)
		{
			drawNextLineOfSpectrogram();
			nextFFTBlockReady = false;
			repaint();
		}
	}

	void drawNextLineOfSpectrogram()
	{
		auto rightHandEdge = spectrogramImage.getWidth() - 1;
		auto imageHeight = spectrogramImage.getHeight();

		spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);

		forwardFFT.performFrequencyOnlyForwardTransform(fftData);

		auto maxLevel = FloatVectorOperations::findMinAndMax(fftData, fftSize / 2);

		for (auto y = 1; y < imageHeight; ++y)
		{
			auto skewedProportionY = 1.0f - std::exp(std::log(y / (float)imageHeight) * 0.2f);
			auto fftDataIndex = jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
			auto level = jmap(fftData[fftDataIndex], 0.0f, jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);

			spectrogramImage.setPixelAt(rightHandEdge, y, Colour::fromHSV(level, 1.0f, level, 1.0f));
		}
	}

	enum
	{
		fftOrder = 10,
		fftSize = 1 << fftOrder
	};

private:
	dsp::FFT forwardFFT;
	Image spectrogramImage;

	float fifo[fftSize];
	float fftData[2 * fftSize];
	int fifoIndex = 0;
	bool nextFFTBlockReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFTComponent)
};
