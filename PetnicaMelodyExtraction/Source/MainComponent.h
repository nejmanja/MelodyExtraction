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

		addAndMakeVisible(despacito);
		despacito.setText((String)123, dontSendNotification);

		formatManager.registerBasicFormats();

		addAndMakeVisible(fftComp);

		

        setSize (1200, 300);
    }

    ~MainComponent()
    {
		openButton.removeListener(this);
		playButton.removeListener(this);
		stopButton.removeListener(this);

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

					filterCoefficients = IIRCoefficients(IIRCoefficients::makeLowPass(__FILESAMPLERATE, 2500, 1.5));
					filter.setCoefficients(filterCoefficients);
					getBlockButton.setEnabled(true);
				}
			}
		}
		else if (button == &getBlockButton)
		{
			preformFFTOnBlock();
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
		preformFFTOnBlock();
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

		openButton.setBounds(window.removeFromTop(getHeight() / 4).reduced(5));
		despacito.setBounds(window.removeFromTop(getHeight() / 4).reduced(5));
		getBlockButton.setBounds(window.reduced(5));
    }

	void preformFFTOnBlock()
	{
		//todo change to preform the whole thing at once i done did it lol should delet dis comment
		filter.processSamples(fileBuffer.getWritePointer(0), fileBuffer.getNumSamples());
		for (int t = 0; t < fileBuffer.getNumSamples(); ++t)
		{
			fftComp.pushNextSampleIntoFifo(fileBuffer.getSample(0, t), __FILESAMPLERATE);
		}

		fftComp.midiComp.finishTrack(fftComp.songContour.size()*fftComp.fftSize*2);
		fftComp.midiComp.writeToFile("C:\\Users\\Milanovic\\Music\\output.mid");

		despacito.setText(despacito.getText() + " \nAmount of samples: " + (String)fileBuffer.getNumSamples(),
			dontSendNotification);
		despacito.setText(despacito.getText() + " \n Amount of contours: " + (String)fftComp.songContour.size(), dontSendNotification);
		//fftComp.drawNextLineOfSpectrogram();
	}

private:
    //==============================================================================
    // Your private member variables go here...
	double __FILESAMPLERATE;
	TextButton openButton, playButton, stopButton, getBlockButton;

	AudioFormatManager formatManager;
	std::unique_ptr<AudioFormatReaderSource> readerSource;
	AudioSampleBuffer fileBuffer;

	IIRFilter filter;
	IIRCoefficients filterCoefficients;

	Label despacito;

	FFTComponent fftComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
