g++ -Wall -I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include -lglib-2.0 -lsmf -D__LINUX_ALSA__ -o create_midi_interface.o create_midi_interface.cpp RtMidi.cpp SmfParser.cpp midi.cpp -lasound -lpthread
# g++ -Wall -I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include -lglib-2.0 -lsmf -D__LINUX_ALSA__ -o play.o play.cpp RtMidi.cpp SmfParser.cpp midi.cpp -lasound -lpthread

