#include <iostream>
#include <cstdlib>
#include "RtMidi.h"

static std::string outClientName = std::string("MKT Output Client");
static std::string inClientName = std::string("MKT Input Client");
static std::string outPortName = std::string("MKT Output Port");
static std::string inPortName  = std::string("MKT Input Port");

static std::string targetOut = std::string("FLUID");
static std::string targetIn  = std::string("microKEY");

void mycallback(double deltatime, std::vector<unsigned char> *message, void *userData)
{
	if ( message->size() > 0 ) {
//		std::cout << "stamp = " << deltatime << std::endl;
		(*message)[1] = 64;
		((RtMidiOut *)userData)->sendMessage(message);
	}
}

void listMidiPort (std::string inPortName, int * inPortIndex, std::string outPortName, int * outPortIndex) {
	try {
		RtMidiIn *midiin = new RtMidiIn();
		unsigned int nPorts = midiin->getPortCount();
		for (unsigned int i=0; i<nPorts; i++) {
			std::string portName = midiin->getPortName(i);
			std::string::size_type loc = portName.find(inPortName, 0);
			if (loc != std::string::npos) {
				*inPortIndex = i;
				std::cout << "found input: " << i << std::endl;
				break;
			}
		}
	} catch (RtMidiError &error) {
		error.printMessage();
	}

	try {
		RtMidiOut *midiout = new RtMidiOut();
		unsigned int nPorts = midiout->getPortCount();
		for (unsigned int i=0; i<nPorts; i++) {
			std::string portName = midiout->getPortName(i);
			std::string::size_type loc = portName.find(outPortName, 0);
			if (loc != std::string::npos) {
				*outPortIndex = i;
				std::cout << "found output: " << i << std::endl;
				break;
			}
		}
	} catch (RtMidiError &error) {
		error.printMessage();
	}
}


int main()
{
	int outPort = 0, inPort = 0;
	listMidiPort(targetIn, &inPort, targetOut, &outPort);
	

	RtMidiOut *midiout = new RtMidiOut(RtMidi::UNSPECIFIED, outClientName);
	unsigned int outPorts = midiout->getPortCount();
	if (outPorts == 0) {
		std::cout << "No out ports available!\n";
		delete midiout;
		return 0;
	}
	midiout->openPort(outPort, outPortName);

	RtMidiIn *midiin = new RtMidiIn(RtMidi::UNSPECIFIED, inClientName);
	unsigned int inPorts = midiin->getPortCount();
	if (inPorts == 0) {
		std::cout << "No in ports available!\n";
		delete midiout;
		delete midiin;
		return 0;
	}
	midiin->openPort(inPort, inPortName);
	midiin->setCallback(&mycallback, midiout);

	// Don't ignore sysex, timing, or active sensing messages.
	midiin->ignoreTypes( false, false, false );
	std::cout << "\nReading MIDI input ... press <enter> to quit.\n";

	char input;
	std::cin.get(input);

	delete midiout;
	delete midiin;
	return 0;
}

