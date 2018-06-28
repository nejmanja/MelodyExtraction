#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "FFTComponent.h"
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public AudioAppComponent, public Button::Listener, public ChangeListener, private Timer
{
public:
    //==============================================================================
    MainComponent()
    {
		addAndMakeVisible(openButton);
		openButton.setButtonText("Open...");
		openButton.addListener(this);

		addAndMakeVisible(getBlockButton);
		getBlockButton.setButtonText("preform fft on block");
		getBlockButton.setColour(TextButton::buttonColourId, Colours::purple);
		getBlockButton.addListener(this);
		getBlockButton.setEnabled(false);


		addAndMakeVisible(lpCutoff);
		lpCutoff.setSliderStyle(Slider::SliderStyle::LinearVertical);
		lpCutoff.setRange(0.0, 900.0);
		lpCutoff.setValue(225, dontSendNotification);
		lpCutoff.setSkewFactorFromMidPoint(275.0);
		lpCutoff.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 20);
		addAndMakeVisible(lpCutoffLabel);
		lpCutoffLabel.attachToComponent(&lpCutoff, false);
		lpCutoffLabel.setText("LP Cutoff", dontSendNotification);

		addAndMakeVisible(hpCutoff);
		hpCutoff.setSliderStyle(Slider::SliderStyle::LinearVertical);
		hpCutoff.setRange(1500.0, 10000.0);
		hpCutoff.setValue(4500.0, dontSendNotification);
		hpCutoff.setSkewFactorFromMidPoint(5000.0);
		hpCutoff.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 20);
		addAndMakeVisible(hpCutoffLabel);
		hpCutoffLabel.attachToComponent(&hpCutoff, false);
		hpCutoffLabel.setText("HP Cutoff", dontSendNotification);

		addAndMakeVisible(hpQ);
		hpQ.setSliderStyle(Slider::SliderStyle::LinearVertical);
		hpQ.setRange(0.1, 25.0);
		hpQ.setValue(1.5, dontSendNotification);
		hpQ.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 20);
		addAndMakeVisible(hpQLabel);
		hpQLabel.attachToComponent(&hpQ, false);
		hpQLabel.setText("HP Q", dontSendNotification);

		addAndMakeVisible(lpQ);
		lpQ.setSliderStyle(Slider::SliderStyle::LinearVertical);
		lpQ.setRange(0.1, 25.0);
		lpQ.setValue(0.8, dontSendNotification);
		lpQ.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 20);
		addAndMakeVisible(lpQLabel);
		lpQLabel.attachToComponent(&lpQ, false);
		lpQLabel.setText("LP Q", dontSendNotification);

		addAndMakeVisible(despacito);
		despacito.setText("Waiting for file input...", dontSendNotification);
		

		formatManager.registerBasicFormats();

		addAndMakeVisible(fftComp);

		

        setSize (1200, 300);
    }

    ~MainComponent()
    {
		openButton.removeListener(this);

        shutdownAudio();
    }

	void changeListenerCallback(ChangeBroadcaster* source) override
	{
	}

	void buttonClicked(Button* button) override
	{
		if (button == &openButton)
		{
			shutdownAudio();

			FileChooser chooser("Select a Wave file to play...", File::nonexistent, "*.wav; *.mp3");
			if (chooser.browseForFileToOpen())
			{
				auto file = chooser.getResult();
				std::unique_ptr<AudioFormatReader> reader (formatManager.createReaderFor(file));
				if (reader != nullptr)
				{
					fileBuffer.setSize(reader->numChannels, (int)reader->lengthInSamples);
					reader->read(&fileBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
					setAudioChannels(0, reader->numChannels);


					__FILESAMPLERATE = reader->sampleRate;
					despacito.setText("Sample rate: " + (String)__FILESAMPLERATE, dontSendNotification);

					//TODO: add peak filter and make variables adjustable in the UI
					
					getBlockButton.setEnabled(true);
				}
			}
		}
		else if (button == &getBlockButton)
		{
			preformFFTOnAudio();
		}
	}

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
    }

    void releaseResources() override
	{
    }
	void timerCallback() override
	{
		preformFFTOnAudio();
	}
    //==============================================================================
    void paint (Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
		Rectangle<int> window = getLocalBounds();

		fftComp.setBounds(window.removeFromRight(getWidth() * 2 / 3));
		int textBoxOffset = 10;

		Rectangle<int> sliderArea = window.removeFromRight(window.getWidth() / 2);
		Rectangle<int> lpArea = sliderArea.removeFromTop(sliderArea.getHeight() / 2);
		lpCutoff.setBounds(lpArea.removeFromLeft(lpArea.getWidth() / 2).reduced(5));
		lpCutoffLabel.setTopLeftPosition(lpCutoff.getPosition().getX(), 
			lpCutoff.getPosition().getY() - textBoxOffset);
		lpQ.setBounds(lpArea.reduced(5));
		lpQLabel.setTopLeftPosition(lpQ.getPosition().getX(),
			lpQ.getPosition().getY() - textBoxOffset);
		hpCutoff.setBounds(sliderArea.removeFromLeft(sliderArea.getWidth() / 2).reduced(5));
		hpCutoffLabel.setTopLeftPosition(hpCutoff.getPosition().getX(),
			hpCutoff.getPosition().getY() - textBoxOffset);
		hpQ.setBounds(sliderArea.reduced(5));
		hpQLabel.setTopLeftPosition(hpQ.getPosition().getX(),
			hpQ.getPosition().getY() - textBoxOffset);

		openButton.setBounds(window.removeFromTop(getHeight() / 4).reduced(5));
		despacito.setBounds(window.removeFromTop(getHeight() / 2).reduced(5));
		getBlockButton.setBounds(window.reduced(5));
    }

	void preformFFTOnAudio()
	{
		//filterCoefficients = IIRCoefficients(IIRCoefficients::makeLowPass(__FILESAMPLERATE, lpCutoff.getValue(), lpQ.getValue()));
		lpFilter.setCoefficients(IIRCoefficients(IIRCoefficients::makeLowPass(__FILESAMPLERATE, lpCutoff.getValue(), lpQ.getValue())));
		hpFilter.setCoefficients(IIRCoefficients(IIRCoefficients::makeHighPass(__FILESAMPLERATE, hpCutoff.getValue(), hpQ.getValue())));
		peakFilter.setCoefficients(IIRCoefficients::makePeakFilter(__FILESAMPLERATE, 590, 1.5, 1.5));

		lpFilter.processSamples(fileBuffer.getWritePointer(0), fileBuffer.getNumSamples());
		hpFilter.processSamples(fileBuffer.getWritePointer(0), fileBuffer.getNumSamples());
		peakFilter.processSamples(fileBuffer.getWritePointer(0), fileBuffer.getNumSamples());
		for (int t = 0; t < fileBuffer.getNumSamples(); ++t)
		{
			fftComp.pushNextSampleIntoFifo(fileBuffer.getSample(0, t), __FILESAMPLERATE);
		}
		Time currentTime;
		fftComp.findMelodyRange();

		String lpLog = (String)(lpCutoff.getValue());
		lpLog.replaceCharacter(lpLog[lpLog.lastIndexOfChar('.')], '-');
		lpLog += ("_LPQ" + (String)lpQ.getValue());
		lpLog.replaceCharacter(lpLog[lpLog.lastIndexOfChar('.')], '-');

		String hpLog = (String)(hpCutoff.getValue());
		hpLog.replaceCharacter(hpLog[hpLog.lastIndexOfChar('.')], '-');
		hpLog += ("_HPQ" + (String)hpQ.getValue());
		hpLog.replaceCharacter(hpLog[hpLog.lastIndexOfChar('.')], '-');

		fftComp.midiComp.writeToFile("C:\\Users\\Milanovic\\Music\\MExOutput\\" + (String)(currentTime.getMillisecondCounter() / 1000) + 
			"_LP" +  lpLog + "_HP" + hpLog + ".mid");

		despacito.setText(despacito.getText() + "\nSuccessfully exported MIDI file with filters: LP:" + (String)lpCutoff.getValue() + "/" + (String)lpQ.getValue() +
			" HP:" + (String)hpCutoff.getValue() + "/" + (String)hpQ.getValue() ,dontSendNotification);
		despacito.setText(despacito.getText() + " \nAmount of samples: " + (String)fileBuffer.getNumSamples(),
			dontSendNotification);
		despacito.setText(despacito.getText() + " \n Amount of contours: " + (String)fftComp.songContour.size(), dontSendNotification);
	}

private:
    //==============================================================================
    // Your private member variables go here...
	double __FILESAMPLERATE;
	TextButton openButton, getBlockButton;

	AudioFormatManager formatManager;
	std::unique_ptr<AudioFormatReaderSource> readerSource;
	AudioSampleBuffer fileBuffer;

	IIRFilter lpFilter;
	IIRFilter hpFilter;
	IIRFilter peakFilter;
	IIRCoefficients filterCoefficients;

	Label despacito, hpCutoffLabel, lpCutoffLabel, hpQLabel, lpQLabel;
	Slider hpCutoff, lpCutoff, hpQ, lpQ;

	FFTComponent fftComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
