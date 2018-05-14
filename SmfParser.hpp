#pragma once

//SmfParser.hpp

//using namespace std;
//#include <iostream>
#include <vector>

struct ch_event {
	int event_type;
	char *ch_event_type;
	int acumulate_time;
	int ch_event_channel;
	int par1;
	char *desc_par1;
	int par2;
};

class SmfParser {
public:
	SmfParser(const char * smffile);
	~SmfParser();
	void printcredits();
	void parse(int output);
	void dump();
	void abstract();
	int getFormat_type() { return format_type; }
	int getTime_division() { return time_division; }
	int getTicks_per_beat() { return ticks_per_beat; }
	int getBpm() { return bpm; }
	int getNum_time_signature() { return num_time_signature; }
	int getDen_time_signature() { return den_time_signature; }
	int getNum_tracks() { return num_tracks; }
	double getNum_bars() { return num_bars; }
	int getKey() { return key; }
	int getScale() { return scale; }
	std::vector<ch_event> events;
						
private:
	const char *smffile;
	int format_type;
	int time_division; //0: ticks per beat; 1: frames per second
	int ticks_per_beat;
	int bpm;
	int num_time_signature;
	int den_time_signature;
	int num_tracks;
	double num_bars;
	int key, scale;
	
	int tempo_has_changed;
	int time_signature_has_changed;
};


