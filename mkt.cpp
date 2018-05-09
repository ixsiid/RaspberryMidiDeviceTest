#include <iostream>
#include <cstdlib>
#include <smf.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include <unistd.h>
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

typedef struct _Note {
	int time;
	unsigned char message[3];
} Note;

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

typedef struct _Smf {
	std::string filepath;
	Note * notes;
	int count;
} Smf;


typedef struct _Key {
	bool major;
	int  diff; // count from ハ(長短)調
} Key;

std::ostream & operator<<(std::ostream & Str, const Key & key) {
	std::string majorName[12] = {
		"ハ長調", "ト長調", "ニ長調", "イ長調", "ホ長調", "ロ長調",
		"嬰ヘ長調", "嬰ハ長調", "変イ長調", "変ホ長調", "変ロ長調", "ヘ長調"
	};
	std::string minorName[12] = {
		"ハ短調", "ト短調", "ニ短調", "イ短調", "ホ短調", "ロ短調",
		"嬰ヘ短調", "嬰ハ短調", "嬰ト短調", "嬰二短調", "変ロ短調", "ヘ短調"
	};

	printf("Key is %s key, diff %d; %s", 
		key.major ? "major" : "minor",
		key.diff, 
		(key.major ? majorName[key.diff] : minorName[key.diff]).c_str()
	);

	return Str;
}

Key get_key(int * noteCount, int * startNoteCount = NULL, bool major = true) {
	Key result = { major, 0 };
	int score = -1;

	int baseKey [12] = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 };
	int majorKey[12] = { 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0 };
	int minorKey[12] = { 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0 };

	for (int d=0; d<12; d++) {
		int _score = 0;
		for (int x=0; x<128; x++) {
			_score += noteCount[x] * baseKey[(x + d) % 12];
		}
		if (_score > score) {
			score = _score;
			result.diff = d;
		}
	}

	if (startNoteCount != NULL) {
		int majorScore = 0;
		int minorScore = 0;
		for (int x=0; x<128; x++) {
			majorScore += startNoteCount[x] * majorKey[(x + result.diff) % 12];
			minorScore += startNoteCount[x] * minorKey[(x + result.diff) % 12];
		}
		if (majorScore < minorScore) major = false;
	}

	if (!(result.major = major)) {
		result.diff = (result.diff + 3) % 12;
	}

	return result;
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
	std::vector<Smf> files = {
		{"magic_music.mid", NULL, -1}
	};

	for (int i=0; i<1; i++) {
		Smf * x = &(files[i]);
		std::cout << "parse: " << x->filepath << std::endl;
		SmfParser smf = SmfParser(x->filepath.c_str());
		smf.printcredits();
		smf.parse(0);
		smf.abstract();
		std::cout << std::endl;
		std::cout << "ticks per beat: " << smf.getTicks_per_beat() << std::endl;

		int noteCount = 0;
		for (auto y : smf.events) {
			if (y.event_type == 9 || y.event_type == 8) noteCount++;
		}

		std::sort(smf.events.begin(), smf.events.end(), compare_event);
		x->notes = new Note[noteCount];
		std::cout << "notes: " << x->notes << std::endl;
		x->count = 0;
		int tempo = smf.getBpm();
		int tickPerBeat = smf.getTicks_per_beat();
		float k = 60000.0f / (tempo * tickPerBeat);
		std::cout << "tempo " << tempo << ", tpb " << tickPerBeat << ", k=" << k << std::endl;
		for (auto y : smf.events) {
			if (y.event_type == 9) {
				x->notes[x->count].time = (int)(y.acumulate_time * k);
				x->notes[x->count].message[0] = 144;
				x->notes[x->count].message[1] = y.par1;
				x->notes[x->count].message[2] = y.par2;
				x->count++;
			} else if (y.event_type == 8) {
				x->notes[x->count].time = (int)(y.acumulate_time * k);
				x->notes[x->count].message[0] = 128;
				x->notes[x->count].message[1] = y.par1;
				x->notes[x->count].message[2] = 0;
				x->count++;
			}
		}

		std::cout << "count: " << x->count << std::endl;
	}

	Smf * play = &(files[0]);

	int noteCount[128];
	int startNoteCount[128];
	for (int i=0; i<128; i++) startNoteCount[i] = noteCount[i] = 0;
	for (int i=0; i<play->count; i++) {
		if (i < 7) startNoteCount[play->notes[i].message[1]]++;
		noteCount[play->notes[i].message[1]]++;
	}
	Key k = get_key(noteCount, startNoteCount);
	std::cout << k << std::endl;


	// Don't ignore sysex, timing, or active sensing messages.
	midiin->ignoreTypes( false, false, false );
	std::cout << "\nReading MIDI input ... press <enter> to quit.\n";


	std::thread thr([](Smf * smf, RtMidiOut * midi) {
		std::cout << "notes addr: " << smf->notes << std::endl;
		std::cout << "Play notes count: " << smf->count << std::endl;
		auto start = std::chrono::system_clock::now();
		int i = -1;
		long j = 0;
		Note * note = smf->notes;
		while(true) {
			int acumulate = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now() - start
			).count();

			if (j++ % 100000 == 0) std::cout << "\racumlatetime: " << acumulate << "[msec] (" << j << ")";
			fflush(stdout);
			while(true) {
				if (i >= smf->count) {
					std::cout << "play end" << std::endl;
					return;
				}

				if (note->time <= acumulate) {
					i++;
					midi->sendMessage(note->message, 3);
					std::cout << "\rtime=" << acumulate << ", " <<  *note << std::endl;
					note++;
//					std::cout << "next: " << *note << std::endl;
				} else {
					break;
				}
				usleep(1666);
			}
		}
		std::cout << "unknown finish to play" << std::endl;
		return;
	}, play, midiout);
	thr.detach();

	char input;
	std::cin.get(input);

	delete midiout;
	delete midiin;
	return 0;
}

