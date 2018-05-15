#pragma once

#include <chrono>
#include "RtMidi.h"
#include "midi.hpp"

typedef struct _Note
{
	int time;
	unsigned char message[3];
} Note;

enum struct SmfStatus {
	START, PLAY, STOP, PAUSE,
};

using std::chrono::system_clock;
class Smf {
private:
	const char * filepath;
	Note * notes;
	Note * endpoint;
	Note * current;
	SmfStatus status;
	system_clock::time_point startTime;
	RtMidiOut * midi;

public:
	Smf(const char * filepath, RtMidiOut * midi);
	bool start();
	bool play();
	bool pause();
	bool stop();
	bool seek(int milliseconds);
};


