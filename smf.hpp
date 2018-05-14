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

typedef struct _Note
{
	int time;
	unsigned char message[3];
} Note;

enum struct SmfStatus {
	Start, Play, Stop, Pause,
};

class Smf
{
 private:
	const char * filepath;
	Note * notes;
	Note * endpoint;
	SmfStatus status;
	std::chrono::time_point startTime;

 public:
	Smf(const char * filepath, RtMidiOut * midi);
	bool play(RtMidiOut * midi);
	bool pause();
	bool stop();
	bool seek(int milliseconds);
};
