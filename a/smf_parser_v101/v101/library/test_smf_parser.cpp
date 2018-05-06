// g++ -o test_smf_parser test_smf_parser.cpp SmfParser.cpp
// ./test_smf_parser ../../midi/canco_bressol.midi

#include "SmfParser.hpp"

#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "math.h"
#include <string.h>

using namespace std;

static void usage(void);

int main (int argc, char* argv[]) {

	char *smffile=(char *)"";
	
	if (argc==1) usage();
	smffile = argv[1];
	
	SmfParser midi_file = SmfParser(smffile);
	midi_file.printcredits();
	#if 0
		midi_file.dump();
	#endif
	midi_file.parse(1);

	midi_file.abstract(); //només es pot cridar després de parse
	
	printf("\n");
	printf("ticks per beat: %d\n",midi_file.getTicks_per_beat());
	
return 0;

} 

static void usage () {
	cout << "test_smf_parser is a test program that uses the SmfParser library, a Standard Midi File (SMF) Parser. It analyzes the structure and content of a SMF file, searching for tracks and midi events.\n";
	cout << "At this stage smf_parser has been tested with midi files in the midi/ folder, considering different type of events and circumstances... but is not fully tested\n\n";
	
	cout << "usage: test_smf_parser <midi_file>\n\n";
	cout << "remember you can redirect the standard output to a file: test_smf_parser <midi_file> > midi_file.txt\n\n";
	exit(0);
}
