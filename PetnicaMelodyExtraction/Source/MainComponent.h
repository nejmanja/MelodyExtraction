#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

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

		formatManager.registerBasicFormats();
		transportSource.addChangeListener(this);

        setSize (400, 300);
        setAudioChannels (0, 2);
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
		if (source == &transportSource)
		{
			if (transportSource.isPlaying())
				changeState(Playing);
			else
				changeState(Stopped);
		}
	}

	void buttonClicked(Button* button) override
	{
		if (button == &openButton)
		{
			FileChooser chooser("Select a Wave file to play...",
				File::nonexistent,
				"*.wav; *.mp3");
			if (chooser.browseForFileToOpen())
			{
				auto file = chooser.getResult();
				auto* reader = formatManager.createReaderFor(file);
				if (reader != nullptr)
				{
					std::unique_ptr<AudioFormatReaderSource> newSource(new AudioFormatReaderSource(reader, true));
					transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
					playButton.setEnabled(true);
					readerSource.reset(newSource.release());
				}
			}
		}
		else if (button == &playButton)
		{
			changeState(Starting);
		}
		else if (button == &stopButton)
		{
			changeState(Stopping);
		}
	}

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
		transportSource.prepareToPlay(samplesPerBlockExpected,sampleRate);
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
		if (readerSource.get() == nullptr)
		{
			bufferToFill.clearActiveBufferRegion();
			return;
		}
		transportSource.getNextAudioBlock(bufferToFill);
    }

    void releaseResources() override
	{
		transportSource.releaseResources();
    }

    //==============================================================================
    void paint (Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

        // You can add your drawing code here!
    }

    void resized() override
    {
		Rectangle<int> window = getLocalBounds();
		openButton.setBounds(window.removeFromTop(getHeight() / 3).reduced(5));
		playButton.setBounds(window.removeFromTop(getHeight() / 3).reduced(5));
		stopButton.setBounds(window.reduced(5));
    }


private:
    //==============================================================================
    // Your private member variables go here...

	

	enum TransportState
	{
		Starting,
		Stopping,
		Playing,
		Stopped
	};

	void changeState(TransportState newState)
	{
		if (state != newState)
		{
			state = newState;

			switch(state)
			{
			case Stopped:
				stopButton.setEnabled(false);
				playButton.setEnabled(true);
				transportSource.setPosition(0.0);
				break;
			case Playing:
				stopButton.setEnabled(true);
				break;
			case Starting:
				playButton.setEnabled(false);
				transportSource.start();
				break;
			case Stopping:
				transportSource.stop();
				break;
			}
		}
	}

	TextButton openButton, playButton, stopButton;

	AudioFormatManager formatManager;
	std::unique_ptr<AudioFormatReaderSource> readerSource;
	AudioTransportSource transportSource;
	TransportState state;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
