g++ -Wall -I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include -lglib-2.0 -lsmf -D__LINUX_ALSA__ -o mkt.o mkt.cpp RtMidi.cpp -lasound -lpthread
