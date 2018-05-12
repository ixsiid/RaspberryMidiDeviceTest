#include <iostream>
#include <cstdlib>
#include <smf.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include <unistd.h>
#include "RtMidi.h"
#include "SmfParser.hpp"
#include "midi.hpp"

void mycallback(double deltatime, std::vector<unsigned char> *message, void *userData)
{
	/**
		48 -> base De

		Target De -> 0, De# -> 1, Re -> 2, .... Si -> 11
		below Target is 1, 3, 6, 8, 10, only black key.
	*/
	const unsigned char target[5] = { 1, 3, 6, 8, 10 };

	if ( message->size() > 0 ) {
//		std::cout << "stamp = " << deltatime << std::endl;
//		printf("type = %d\n", (*message)[0]);
//		(*message)[1] = 64;

		printf("%d -> ", (*message)[1]);

		int key = (*message)[1] / 12;
		int a   = (*message)[1] % 12;
		if (a > 10) a -= 5;
		else if (a > 8) a -= 4;
		else if (a > 6) a -= 3;
		else if (a > 3) a -= 2;
		else if (a > 1) a -= 1;
		int numWhite = (key - 60 / 12) * 7 + a;

		int tOct = sizeof(target);
		int tKey = numWhite / tOct;
		int tA   = numWhite % tOct;
		if (tA < 0) {
			tA += sizeof(target);
			tKey -= 1;
		}

		(*message)[1] = 60 + (tKey * 12) + target[tA];

		printf("%d\n", (*message)[1]);
		((RtMidiOut *)userData)->sendMessage(message);
	}
}

int main()
{
	MidiInterface *port = new MidiInterface("MKT");

	RtMidiOut *midiout = (RtMidiOut *)port->connect("FLUID", MidiDirection::OUT);
	if (midiout == NULL) {
		std::cout << "Not found out port" << std::endl;
		exit(1);
	}

	RtMidiIn  *midiin  = (RtMidiIn  *)port->connect("microKEY", MidiDirection::IN);
	if (midiin == NULL) {
		std::cout << "Not found in port" << std::endl;
		exit(1);
	}
	midiin->setCallback(&mycallback, midiout);

	// Don't ignore sysex, timing, or active sensing messages.
	midiin->ignoreTypes( false, false, false );
	std::cout << "\nReading MIDI input ... press <enter> to quit.\n";


	char input;
	std::cin.get(input);

	delete midiout;
	delete midiin;
	delete port;
	return 0;
}

