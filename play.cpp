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
	Note * endpoint;
	Note * current;
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
	MidiInterface * port = new MidiInterface("Multi");
	RtMidiOut *midiout = (RtMidiOut *)port->connect("FLUID", MidiDirection::IN);
	if (midiout == NULL) {
		std::cout << "Not found target port name" << std::endl;
		exit(1);
	}

	//
	std::vector<Smf> files = {
		{"magic_music.mid", NULL, NULL, NULL}
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
		int count = 0;
		int tempo = smf.getBpm();
		int tickPerBeat = smf.getTicks_per_beat();
		float k = 60000.0f / (tempo * tickPerBeat);
		std::cout << "tempo " << tempo << ", tpb " << tickPerBeat << ", k=" << k << std::endl;
		for (auto y : smf.events) {
			if (y.event_type == 9) {
				x->notes[count].time = (int)(y.acumulate_time * k);
				x->notes[count].message[0] = 144;
				x->notes[count].message[1] = y.par1;
				x->notes[count].message[2] = y.par2;
				count++;
			} else if (y.event_type == 8) {
				x->notes[count].time = (int)(y.acumulate_time * k);
				x->notes[count].message[0] = 128;
				x->notes[count].message[1] = y.par1;
				x->notes[count].message[2] = 0;
				count++;
			}
		}

		x->endpoint = x->notes + count;
		x->current = x->notes;

		std::cout << "count: " << count << std::endl;
	}


	for (unsigned int x=0; x<files.size(); x++) {
		int noteCount[128];
		int startNoteCount[128];
		for (int i=0; i<128; i++) startNoteCount[i] = noteCount[i] = 0;
		Note * current = files[x].notes;
		for (int i=0; current < files[x].endpoint; i++) {
			if (i < 7) startNoteCount[current->message[1]]++;
			noteCount[current->message[1]]++;
			current++;
		}
		Key k = get_key(noteCount, startNoteCount);
		std::cout << k << std::endl;
	}

	bool playing = false;
	auto start = std::chrono::system_clock::now();
	unsigned int playIndex = 0;

	std::thread thr([&playing, &start, &files, &playIndex](RtMidiOut * midi) {
		int pausedTime = -1;
		long j = 0;
		while(true) {
			int acumulate = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now() - start
			).count();

			if (playing) {
				if (pausedTime >= 0) {
					start += std::chrono::milliseconds(acumulate - pausedTime);
					acumulate = pausedTime;
					pausedTime = -1;
				}
			} else {
				if (pausedTime < 0) pausedTime = acumulate;
				acumulate = -1;
			}

			if (j++ > 100000) {
				std::cout << "\racumlate time: " << acumulate << "[msec]";
				fflush(stdout);
				j = 0;
			}

			while(true) {
				for (unsigned int x=0; x<files.size(); x++) {
					Smf * play = &files[x];
					if (play->current < play->endpoint && play->current->time <= acumulate) {
						if (playIndex == x) midi->sendMessage(play->current->message, 3);
						play->current++;
//						std::cout << "next: " << play -> current << std::endl;
					} else {
						break;
					}
				}
				break;
			}
			usleep(1500);
		}
		std::cout << "unknown finish to play" << std::endl;
		return;
	}, midiout);
	thr.detach();

	while (true) {
		std::cout << "\r> ";
		fflush(stdout);

		int c = std::cin.get();
		if (c >= '0' && c <= '9') {
			playIndex = c - '0';
		} else if (c == 'p') {
			std::cout << "ppp" << std::endl;
			playing = true;
		} else if (c == 's') {
			playing = false;
		} else if (c == 'r') {
			for (unsigned int i=0; i<files.size(); i++) {
				files[i].current = files[i].notes;
				start = std::chrono::system_clock::now();
			}
		} else if (c == 'q' || std::cin.eof()) {
			break;
		}
	}

	delete midiout;
	delete port;
	return 0;
}

