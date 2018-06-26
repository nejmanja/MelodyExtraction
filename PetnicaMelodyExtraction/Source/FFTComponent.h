#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#define _F0 440 //A4=440Hz
#define _A 1.0594630943592952645618252949463 //2^(1/12) 
/*
Fn = F0*a^n (n = num of semitones from F0)
from here:
n = log a (fn/f0)
*/


class PitchContour
{
public:
	PitchContour(int newIndex, double smpRate, int fftSize) : fftIndex(newIndex)
	{
		frequency = (fftIndex * smpRate) / fftSize;
	}

	PitchContour()
	{

	}

	double getFreq()
	{
		return frequency;
	}

private:
	int fftIndex;
	double frequency;
};

//==============================================================================
class FFTComponent : public Component, private Timer
{
public:
	FFTComponent() :forwardFFT(fftOrder), spectrogramImage(Image::RGB, 512, 512, true)
	{
		addAndMakeVisible(textLog);
		textLog.setMultiLine(true);
		textLog.setReturnKeyStartsNewLine(true);
		textLog.setReadOnly(true);
		textLog.setScrollbarsShown(true);
		textLog.setCaretVisible(false);
		textLog.setPopupMenuEnabled(true);
		textLog.setColour(TextEditor::backgroundColourId, Colour(0x32ffffff));
		textLog.setColour(TextEditor::outlineColourId, Colour(0x1c000000));
		textLog.setColour(TextEditor::shadowColourId, Colour(0x16000000));
	}

	~FFTComponent()
	{
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colours::black);

		Rectangle<int> halfWindow(getWidth() / 2, 0, getWidth() / 2, getHeight());

		g.setOpacity(1.0f);
		g.drawImage(spectrogramImage, halfWindow.toFloat());
	}

	void resized() override
	{
		Rectangle<int> window = getLocalBounds();
		textLog.setBounds(window.removeFromLeft(getWidth() / 2));

	}

	void pushNextSampleIntoFifo(float sample, double smpRate) noexcept
	{
		if (fifoIndex == fftSize)
		{
			zeromem(fftData, sizeof(fftData));
			memcpy(fftData, fifo, sizeof(fifo));

			forwardFFT.performFrequencyOnlyForwardTransform(fftData);

			pushContourIntoArray(smpRate);
			textLog.moveCaretToEnd();
			textLog.insertTextAtCaret("\n MaxFreq:" + (String)songContour.getLast().getFreq());

			int roundedDistance = roundToInt(log(songContour.getLast().getFreq() / _F0) / log(_A));

			textLog.moveCaretToEnd();
			textLog.insertTextAtCaret("\nDist in semitones: " + (String)roundedDistance
				+ " Note: " + findNoteFromDistance(roundedDistance));
			fifoIndex = 0;
		}
		fifo[fifoIndex++] = sample;
	}

	String findNoteFromDistance(int distance)
	{
		short octNum;
		if (distance < -33)
			octNum = 1;
		else if (distance < -21)
			octNum = 2;
		else if (distance < -9)
			octNum = 3;
		else if (distance < 3)
			octNum = 4;
		else if (distance < 15)
			octNum = 5;
		else if (distance < 27)
			octNum = 6;
		else if (distance < 39)
			octNum = 7;

		int distMod = distance % 12;
		switch (distMod)
		{
		case 0:
			return "A" + (String)octNum;
		case -11:
		case 1:
			return "A#" + (String)octNum;
		case -10:
		case 2:
			return "B" + (String)octNum;
		case -9:
		case 3:
			return "C" + (String)octNum;
		case -8:
		case 4:
			return "C#" + (String)octNum;
		case -7:
		case 5:
			return "D" + (String)octNum;
		case -6:
		case 6:
			return "D#" + (String)octNum;
			break;
		case -5:
		case 7:
			return "E" + (String)octNum;
		case -4:
		case 8:
			return "F" + (String)octNum;
		case -3:
		case 9:
			return "F#" + (String)octNum;
		case -2:
		case 10:
			return "G" + (String)octNum;
		case -1:
		case 11:
			return "G#" + (String)octNum;
		default:
			return "Koj kurac druze";
		}
	}

	void pushContourIntoArray(double smpRate)
	{
		//TODO: add harmonic sum instead of pure max
		float maxFreq = findMaximum(fftData, fftSize / 2);
		for (int i = 0; i < fftSize / 2; ++i)
		{
			if (fftData[i] == maxFreq)
			{
				PitchContour newContour(i, smpRate, fftSize);
				songContour.add(newContour);
				break;
			}
		}
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

		float datablock[2 * fftSize];
		auto maxLevel = FloatVectorOperations::findMinAndMax(datablock, fftSize / 2);

		for (auto y = 1; y < imageHeight; ++y)
		{
			auto skewedProportionY = 1.0f - std::exp(std::log(y / (float)imageHeight) * 0.2f);
			auto fftDataIndex = jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
			auto level = jmap(datablock[fftDataIndex], 0.0f, jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);

			spectrogramImage.setPixelAt(rightHandEdge, y, Colour::fromHSV(level, 1.0f, level, 1.0f));
		}
		
	}

	enum
	{
		fftOrder = 12,
		fftSize = 1 << fftOrder
	};

	Array<PitchContour> songContour;

private:
	dsp::FFT forwardFFT;
	Image spectrogramImage;
	TextEditor textLog;

	float fifo[fftSize];
	float fftData[2 * fftSize];
	int fifoIndex = 0;
	bool nextFFTBlockReady = false;

	

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFTComponent)
};