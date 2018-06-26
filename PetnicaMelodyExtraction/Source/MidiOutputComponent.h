#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#define _QTR 59200

#define _BAR _QTR*4
#define _HALF _QTR*2
#define _8TH _QTR/2
#define _16TH _QTR/4
#define _32ND _QTR/8

#define _QTR_D _QTR+_8TH
#define _8TH_D _8TH+_16TH


class MidiOutputComponent    : public Component
{
public:
    MidiOutputComponent()
    {
    }

    ~MidiOutputComponent(){}

    void paint (Graphics& g) override{}

    void resized() override{}

	void setUpTrack(int tSigNum, int tSigDen, int tempoInMiS)
	{
		msgSequence.addEvent(MidiMessage::timeSignatureMetaEvent(tSigNum, tSigDen));
		msgSequence.addEvent(MidiMessage::tempoMetaEvent(tempoInMiS));
	}

	void addNoteToSequence(int note, int offset, int lenInTicks)
	{
		msgSequence.addEvent(MidiMessage::noteOn(1, note, (uint8)100), offset);
		msgSequence.updateMatchedPairs();
		msgSequence.addEvent(MidiMessage::noteOff(1, note, (uint8)100), offset + lenInTicks);
	}

	void finishTrack(int EndTime)
	{
		msgSequence.addEvent(MidiMessage::endOfTrack(), EndTime);
		msgSequence.addEvent(MidiMessage::midiStart());
		msgSequence.addEvent(MidiMessage::midiStop(), EndTime);
	}

	void writeToFile(String destFileAbsPath)
	{
		midiFile.setSmpteTimeFormat(25, 40); //millisec format

		midiFile.addTrack(msgSequence); //add the finished trackm

		File oFile(destFileAbsPath);
		auto oStream = oFile.createOutputStream();
		midiFile.writeTo(*oStream);
	}
	
private:

	MidiMessageSequence msgSequence;
	MidiFile midiFile;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiOutputComponent)
};
