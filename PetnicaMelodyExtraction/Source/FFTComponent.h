#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiOutputComponent.h"

#define _F0 440 //A4=440Hz
#define _A 1.0594630943592952645618252949463 //2^(1/12) 
#define _FFTWINDOWSIZE 16

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
		int distanceFromA4 = roundToInt(log(frequency / _F0) / log(_A));
		midiNote = distanceFromA4 + 69;
	}

	PitchContour()
	{
	}

	double getFreq()
	{
		return frequency;
	}
	int getMidiNote()
	{
		return midiNote;
	}

private:
	int fftIndex;
	int midiNote;
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
		textLog.setReadOnly(false);
		textLog.setScrollbarsShown(true);
		textLog.setCaretVisible(false);
		textLog.setPopupMenuEnabled(true);
		textLog.setColour(TextEditor::backgroundColourId, Colour(0x32ffffff));
		textLog.setColour(TextEditor::outlineColourId, Colour(0x1c000000));
		textLog.setColour(TextEditor::shadowColourId, Colour(0x16000000));

		addAndMakeVisible(deletionThresSlider);
		deletionThresSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
		deletionThresSlider.setRange(0.0, 1.0);
		deletionThresSlider.setValue(0.3);

		addAndMakeVisible(fftScanThresSlider);
		fftScanThresSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
		fftScanThresSlider.setRange(0.0, 1.0);
		fftScanThresSlider.setValue(0.5);

		addAndMakeVisible(midiComp);
		midiComp.setUpTrack(4, 4, 500000);
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
		Rectangle<int> window(getWidth() / 2, getHeight());
		textLog.setBounds(window.removeFromTop(getHeight() * 0.8));
		deletionThresSlider.setBounds(window.removeFromTop(window.getHeight() / 2));
		fftScanThresSlider.setBounds(window);
	}

	void pushNextSampleIntoFifo(float sample, double smpRate) noexcept
	{
		if (fifoIndex == fftSize)
		{
			zeromem(fftData, sizeof(fftData));
			memcpy(fftData, fifo, sizeof(fifo));

			forwardFFT.performFrequencyOnlyForwardTransform(fftData);

			pushContourIntoArray(smpRate);

			if (windowIndex < _FFTWINDOWSIZE)
			{
				for (int i = 0; i < fftSize / 2; ++i)
				{
					fftWindow[windowIndex][i] = fftData[i];
				}
				windowIndex++;
			}
			else
			{
				processWindow();
				//shift the matrix left one time
				for (int y = 0; y < fftSize / 2; ++y)
				{
					for (int x = 1; x < _FFTWINDOWSIZE; ++x)
					{
						fftWindow[x - 1][y] = fftWindow[x][y]; //this should probably work
					}
				}
				for (int y = 0; y < fftSize / 2; ++y)
				{
					fftWindow[_FFTWINDOWSIZE - 1][y] = fftData[y];
				}
			}
			

			drawNextLineOfSpectrogram();
			fifoIndex = 0;
		}
		fifo[fifoIndex++] = sample;
	}

	void processWindow()
	{
		for (int i = 0; i < _FFTWINDOWSIZE; ++i)//for each block
		{
			//find peak for each block and push them all into an array or something
		}
	}

	void findMelodyRange()
	{
		Array<int> midiNotes;
		Array<int> midiNoteStarts;
		/*
		Find where the average melody lies, then setup threshold
		and remove mistaken notes or switch octaevs when there are
		the same notes/diff octaves present next to the mistakes
		*/
		int histogramArr[128] = { 0 };
		int prevNote = -1;

		for (int i = 0; i < songContour.size(); ++i)
		{
			if (songContour[i].getMidiNote() != prevNote)
			{
				histogramArr[songContour[i].getMidiNote()]++; //currently passess every block, might try to only place changes
				prevNote = songContour[i].getMidiNote();
				midiNotes.add(songContour[i].getMidiNote());
				midiNoteStarts.add(i * fftSize * 3);

				textLog.moveCaretToEnd();
				textLog.insertTextAtCaret((String)songContour[i].getMidiNote() + "\n");
			}
		}

		int mostPresentNote = findMaximum(histogramArr, 128);
		float deletionThres = mostPresentNote * deletionThresSlider.getValue(); //around 0.3 seems to give the best results

		for (int i = 0; i < 128; ++i)
		{
			if ((histogramArr[i] > 0) && (histogramArr[i] < deletionThres))
			{
				for (int j = 1; j < midiNotes.size() - 1; ++j)
				{
					if (midiNotes[j] == i)
					{
						midiNotes.remove(j);
						midiNoteStarts.remove(j);
					}
				}
			}
			//textLog.moveCaretToEnd();
			//textLog.insertTextAtCaret((String)i + ": " + (String)histogramArr[i] + "\n");
		}

		int mostPresentIndex;
		for (int i = 0; i < 128; ++i)
		{
			if (histogramArr[i] == mostPresentNote)
				mostPresentIndex = i;
		}

		for (int i = 0; i < mostPresentIndex - 7; ++i) //oktava i jos malo preko
		{
			for (int j = 1; j < midiNotes.size() - 1; ++j)
			{
				if (midiNotes[j] == i)
				{
					midiNotes.remove(j);
					midiNoteStarts.remove(j);
				}
			}
		}

		for (int i = 1; i < midiNotes.size() - 1; ++i)
		{
			if (midiNotes[i] == (midiNotes[i + 1] + 12))
			{
				midiNotes.setUnchecked(i, midiNotes[i + 1]);
				
			}
			else if (midiNotes[i] == (midiNotes[i - 1] + 12))
			{
				midiNotes.setUnchecked(i, midiNotes[i - 1]);
			}
		}
		textLog.moveCaretToEnd();
		textLog.insertTextAtCaret("Most present note: " + (String)mostPresentNote);

		//Writing the notes to file
		int prevNoteBeginning = 0;
		for (int i = 0; i < midiNotes.size() - 1; ++i)
		{
			midiComp.addNoteToSequence(midiNotes[i], midiNoteStarts[i], midiNoteStarts[i+1] - midiNoteStarts[i]);
		}
		midiComp.addNoteToSequence(midiNotes[midiNotes.size() - 1], midiNoteStarts[midiNotes.size() - 1], songContour.size()*fftSize * 3 - midiNoteStarts[midiNotes.size() - 1]);
		midiComp.finishTrack(songContour.size()*fftSize*3);
		
		
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
		float maxFreq = findMaximum(fftData, fftSize / 2);
		float minFreq = findMinimum(fftData, fftSize / 2);
		float thresFactor = fftScanThresSlider.getValue(); //0.5f seems to give the best results
		float threshold = (maxFreq - minFreq) * thresFactor;

		
		//for each freq over the tresh, find all present notable harmonics, take one with largest sum
		
		int mostPresentFreq = 0;
		float largestSum = 0;

		for (int i = 1; i < fftSize / 2; ++i)
		{
			if (fftData[i] >= threshold) //if it's present enough
			{
				float sum = fftData[i];

				
				sum += fftData[i * 3 / 2];

				int x = 2;
				int FIndex = x*i;
				float baseFreq = i * smpRate / fftSize;
				float freq = 2 * baseFreq;
				while (freq < 10000) //i guess
				{
					if (fftData[FIndex] >= threshold)
					{
						sum += fftData[FIndex];
					}
					
					if (sum >= largestSum)
					{
						largestSum = sum; //this is the most present
						mostPresentFreq = i; //at this index
					}

					x++;
					FIndex = x * i;
					freq = baseFreq * FIndex;
				}
			}
		}
		PitchContour newContour(mostPresentFreq, smpRate, fftSize);
		songContour.add(newContour);
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

		auto maxLevel = FloatVectorOperations::findMinAndMax(fftData, fftSize / 2);

		for (auto y = 1; y < imageHeight; ++y)
		{
			auto skewedProportionY = 1.0f - std::exp(std::log(y / (float)imageHeight) * 0.05f);
			auto fftDataIndex = jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
			auto level = jmap(fftData[fftDataIndex], 0.0f, jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);

			spectrogramImage.setPixelAt(rightHandEdge, y, Colour::fromHSV(level, 1.0f, level, 1.0f));
		}
		repaint();
	}

	enum
	{
		fftOrder = 12,
		fftSize = 1 << fftOrder
	};

	Array<PitchContour> songContour;
	MidiOutputComponent midiComp;

	Slider deletionThresSlider, fftScanThresSlider;

private:
	dsp::FFT forwardFFT;
	Image spectrogramImage;
	TextEditor textLog;

	/*
	create an fft Window, essentially a fifo structure with a bunch of fftBlocks
	*/
	float fftWindow[_FFTWINDOWSIZE][fftSize / 2];
	int windowIndex; //used when filling for the first time

	float fifo[fftSize];
	float fftData[2 * fftSize];
	int fifoIndex = 0;
	bool nextFFTBlockReady = false;

	String prevNote = "X"; int prevNoteDistance = 0; int prevNoteBeginning = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFTComponent)
};