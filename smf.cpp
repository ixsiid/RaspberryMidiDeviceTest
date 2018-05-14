#include "smf.hpp"
#include "RtMidi.h"
#include "SmfParser.hpp"
#include "midi.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <smf.h>
#include <thread>
#include <unistd.h>

std::ostream &operator<<(std::ostream &Str, const Note &note) {
	if (note.message[0] == 144) {
		printf("timing %d[msec], note %d on, velocity = %d", note.time, note.message[1], note.message[2]);
	} else if (note.message[0] == 128) {
		printf("timing %d[msec], note %d off", note.time, note.message[1]);
	} else {
		printf("unknown note; %d %d %d %d", note.time, note.message[0], note.message[1], note.message[2]);
	}
	return Str;
}

bool compare_event(const ch_event &left, const ch_event &right) {
	if (left.acumulate_time == right.acumulate_time) {
		if (left.event_type == right.event_type) {
		return left.par1 < right.par2;
		}
		return left.event_type < right.event_type;
	}
	return left.acumulate_time < right.acumulate_time;
}

Smf::Smf(const char *_filepath, RtMidiOut * _midi) {
	midi = _midi;

	filepath = _filepath;
	std::cout << "parse: " << filepath << std::endl;

	SmfParser smf = SmfParser(filepath);
	smf.printcredits();
	smf.parse(0);
	smf.abstract();
	std::cout << std::endl << "ticks per beat: " << smf.getTicks_per_beat() << std::endl;

	int noteCount = 0;
	for (auto x : smf.events)
		if (x.event_type == 9 || x.event_type == 8) noteCount++;

	std::sort(smf.events.begin(), smf.events.end(), compare_event);
	notes = new Note[noteCount];
	std::cout << "notes: " << notes << std::endl;

	int tempo			= smf.getBpm();
	int tickPerBeat = smf.getTicks_per_beat();
	float k			= 60000.0f / (tempo * tickPerBeat);
	std::cout << "tempo " << tempo << ", tpb " << tickPerBeat << ", k=" << k << std::endl;

	int count = 0;
	for (auto x : smf.events) {
		if (x.event_type == 9) {
		notes[count].time			= (int)(x.acumulate_time * k);
		notes[count].message[0] = 144;
		notes[count].message[1] = x.par1;
		notes[count].message[2] = x.par2;
		count++;
		} else if (x.event_type == 8) {
		notes[count].time			= (int)(x.acumulate_time * k);
		notes[count].message[0] = 128;
		notes[count].message[1] = x.par1;
		notes[count].message[2] = 0;
		count++;
		}
	}

	endpoint = notes + count;

	status = SmfStatus::STOP;
	startTime = std::chrono::system_clock::now();

	std::cout << "count: " << count << std::endl;
}

bool Smf::start() {
	if (status != SmfStatus::STOP) return false;

	std::thread thr([&](RtMidiOut *midi, Note *begin, Note *end) {
		Note *current	= begin;
		int pausedTime = -1;

		long j = 0;
		while (status != SmfStatus::STOP) {
			int acumulate = std::chrono::duration_cast<std::chrono::milliseconds> (
			                std::chrono::system_clock::now() - startTime
			                ).count();

			if (status == SmfStatus::PAUSE) {
				// Pause直後は、現在の経過時間を保存する
				if (pausedTime < 0) pausedTime = acumulate;
				usleep(5000);
				continue;
			} else {
				// Play直後は、Pauseしていた時間を再生開始時間に継ぎ足す
				if (pausedTime >= 0) {
					startTime += std::chrono::milliseconds(acumulate - pausedTime);
					acumulate  = pausedTime;
					pausedTime = -1;
				}
			}

			// たまに状況表示、後で消す
			if (j++ > 100000) {
				std::cout << "\racumlate time: " << acumulate << "[msec]";
				fflush(stdout);
				j = 0;
			}

			while (current < end) {
				if (current->time <= acumulate) {
					if (status == SmfStatus::PLAY) {
						midi->sendMessage(current->message, 3);
					}
					current++;
				} else {
					break;
				}
			}
			usleep(1500);
		}
		std::cout << "Thread destroy" << std::endl;
		if (status != SmfStatus::STOP) std::cout << "Unknown reason for stopped" << std::endl;
		return;
	}, midi, notes, endpoint);
	thr.detach();

	status = SmfStatus::PAUSE;
	return true;
}

bool Smf::play() {
	if (status == SmfStatus::STOP) Smf::start();
	status = SmfStatus::PLAY;
	return true;
}

bool Smf::pause() {
	if (status == SmfStatus::STOP) return false;
	status = SmfStatus::PAUSE;
	return true;
}

bool Smf::stop() {
	if (status == SmfStatus::STOP) return false;
	status = SmfStatus::STOP;
	return true;
}

bool Smf::seek(int milliseconds) {
	return false;
}

