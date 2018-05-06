#include <iostream>
#include <cstdlib>
#include <smf.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include "RtMidi.h"
#include "SmfParser.hpp"

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
//		printf("type = %d\n", (*message)[0]);
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

struct Note {
	int time;
	unsigned char message[3];
};

bool compare_event(const ch_event & left, const ch_event & right) {
	if (left.acumulate_time == right.acumulate_time) {
		if (left.event_type == right.event_type) {
			return left.par1 < right.par2;
		}
		return left.event_type < right.event_type;
	}
	return left.acumulate_time < right.acumulate_time;
}

std::ostream & operator<<(std::ostream & Str, const Note & note) {
	if (note.message[0] == 144) {
		printf("timing %d[msec], note %d on, velocity = %d", note.time, note.message[1], note.message[2]);
	} else if (note.message[0] == 128) {
		printf("timing %d[msec], note %d off", note.time, note.message[1]);
	} else {
		printf("unknown note; %d %d %d %d", note.time, note.message[0], note.message[1], note.message[2]);
	}
	return Str;
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

	//
	std::vector<std::string> files = {
		"magic_music.mid"
	};

	Note * playNotes = NULL;
	int playNotesCount = 0;
	
	for (auto x : files) {
		SmfParser smf = SmfParser(x.c_str());
		smf.printcredits();
		std::cout << "parse" << std::endl;
		smf.parse(0);
		std::cout << "abstract" << std::endl;
		smf.abstract();
		std::cout << std::endl;
		std::cout << "ticks per beat: " << smf.getTicks_per_beat() << std::endl;

		int noteCount = 0;
		for (auto y : smf.events) {
			if (y.event_type == 9 || y.event_type == 8) noteCount++;
		}

		std::sort(smf.events.begin(), smf.events.end(), compare_event);
		Note notes[noteCount];
		int i = 0;
		int tempo = smf.getBpm();
		int tickPerBeat = smf.getTicks_per_beat();
		float k = 60000.0f / (tempo * tickPerBeat);
		std::cout << "tempo " << tempo << ", tpb " << tickPerBeat << ", k=" << k << std::endl;
		for (auto y : smf.events) {
			if (y.event_type == 9) {
				notes[i].time = (int)(y.acumulate_time * k);
				notes[i].message[0] = 144;
				notes[i].message[1] = y.par1;
				notes[i].message[2] = y.par2;
				i++;
			} else if (y.event_type == 8) {
				notes[i].time = (int)(y.acumulate_time * k);
				notes[i].message[0] = 128;
				notes[i].message[1] = y.par1;
				notes[i].message[2] = 0;
				i++;
			}
		}

		playNotes = &(notes[0]);
		playNotesCount = noteCount;
	}

	// Don't ignore sysex, timing, or active sensing messages.
	midiin->ignoreTypes( false, false, false );
	std::cout << "\nReading MIDI input ... press <enter> to quit.\n";

	std::thread thr([](Note * notes, int count, RtMidiOut * midi) {
		std::cout << "Play notes count: " << count << std::endl;
		auto start = std::chrono::system_clock::now();
		int i = -1;
		while(true) {
			int acumulate = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now() - start
			).count();
			while(true) {
				if (i++ >= count) return;
				Note note = notes[i];
				if (note.time <= acumulate) {
					midi->sendMessage(note.message, 3);
					std::cout << "time=" << acumulate << ", " <<  note << std::endl;
				} else {
					break;
				}
			}
		}
		std::cout << "play end" << std::endl;
		return;
	}, playNotes, playNotesCount, midiout);
	thr.detach();

	char input;
	std::cin.get(input);

	delete midiout;
	delete midiin;
	return 0;
}

