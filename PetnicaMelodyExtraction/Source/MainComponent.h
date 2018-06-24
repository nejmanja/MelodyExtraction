#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "FFTComponent.h"
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public AudioAppComponent, public Button::Listener, public ChangeListener
{
public:
    //==============================================================================
    MainComponent()
    {
		addAndMakeVisible(openButton);
		openButton.setButtonText("Open...");
		openButton.addListener(this);

		addAndMakeVisible(playButton);
		playButton.setButtonText("Play");
		playButton.setColour(TextButton::buttonColourId, Colours::green);
		playButton.addListener(this);
		playButton.setEnabled(false);

		addAndMakeVisible(stopButton);
		stopButton.setButtonText("Stop");
		stopButton.setColour(TextButton::buttonColourId, Colours::red);
		stopButton.addListener(this);
		stopButton.setEnabled(false);

		addAndMakeVisible(getBlockButton);
		getBlockButton.setButtonText("preform fft on block");
		getBlockButton.setColour(TextButton::buttonColourId, Colours::purple);
		getBlockButton.setEnabled(false);

		formatManager.registerBasicFormats();

		addAndMakeVisible(fftComp);

        setSize (800, 300);
        //setAudioChannels (0, 1);
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
					position = 0;
					setAudioChannels(0, reader->numChannels);
				}
			}
		}
		else if (button == &playButton)
		{
		}
		else if (button == &stopButton)
		{
		}
		else if (button == &getBlockButton)
		{
			preformFFTOnBlock();
		}
	}

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
		//transportSource.prepareToPlay(samplesPerBlockExpected,sampleRate);
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
		int numInputChannels = fileBuffer.getNumChannels();
		int numOutputChannels = bufferToFill.buffer->getNumChannels();

		int outputSamplesRemaining = bufferToFill.numSamples;
		int outputSamplesOffset = bufferToFill.startSample;

		while (outputSamplesRemaining > 0)
		{
			int bufferSamplesRemaining = fileBuffer.getNumSamples() - position;
			int samplesThisTime = jmin(outputSamplesRemaining, bufferSamplesRemaining);

			for (int channel = 0; channel < numOutputChannels; ++channel)
			{
				bufferToFill.buffer->copyFrom(channel,
					outputSamplesOffset,
					fileBuffer,
					channel % numInputChannels,
					position,
					samplesThisTime);

			}

			outputSamplesRemaining -= samplesThisTime;
			outputSamplesOffset += samplesThisTime;
			position += samplesThisTime;

			if (position == fileBuffer.getNumSamples())
				position = 0;
		}
    }

    void releaseResources() override
	{
		//transportSource.releaseResources();
    }

    //==============================================================================
    void paint (Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
		Rectangle<int> window = getLocalBounds();

		fftComp.setBounds(window.removeFromRight(getWidth() / 2));

		openButton.setBounds(window.removeFromTop(getHeight() / 4).reduced(5));
		playButton.setBounds(window.removeFromTop(getHeight() / 4).reduced(5));
		stopButton.setBounds(window.removeFromTop(getHeight() / 4).reduced(5));
		getBlockButton.setBounds(window.reduced(5));
    }

	void preformFFTOnBlock()
	{
		//ovde ce radis stvari
	}

private:
    //==============================================================================
    // Your private member variables go here...

	TextButton openButton, playButton, stopButton, getBlockButton;

	AudioFormatManager formatManager;
	std::unique_ptr<AudioFormatReaderSource> readerSource;

	AudioSampleBuffer fileBuffer;

	int position = 0;

	FFTComponent fftComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
