#include "RtMidi.h"
#include "midi.hpp"
#include "smf.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>

int main() {
  MidiInterface *port = new MidiInterface("Multi");
  RtMidiOut *midiout  = (RtMidiOut *)port->connect("FLUID", MidiDirection::OUT);
  if (midiout == NULL) {
	 std::cout << "Not found target port name" << std::endl;
	 exit(1);
  }

  Smf *smf = new Smf("magic_music.mid", midiout);
  smf->start();

  while (true) {
	 std::cout << "\r> ";
	 fflush(stdout);

	 int c = std::cin.get();
	 if (c == 'p') {
		smf->play();
	 } else if (c == 's') {
		smf->pause();
	 } else if (c == 'r') {
		smf->seek(0);
	 } else if (c == 'q' || std::cin.eof()) {
		break;
	 }
  }

  delete midiout;
  delete port;
  return 0;
}
