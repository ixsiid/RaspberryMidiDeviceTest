// g++ -o smf_parser smf_parser_101.cpp
// ./smf_parser ../midi/canco_bressol.midi

// http://ubuntuforums.org/showthread.php?t=857511
// http://www.cplusplus.com/doc/tutorial/files/
// llegir (i escriure) un fitxer binari
// http://en.kioskea.net/faq/978-c-language-handling-64-bit-integers
// bitwise operators: http://www.cprogramming.com/tutorial/bitwise_operators.html
// format MIDI: http://www.sonicspot.com/guide/midifiles.html

// TEST
// Let_it_be.mid -> cançó completa. Funciona
// Super_Trouper.mid -> funciona
// Sequencer-Specific Meta-Event: midi/204a.mid: X0 X0 X41
// Sysex event: CHOIR1.MID
// 4.midi: hi ha 11 compassos. El Time Signature canvia amb el temps, s'ha de fer més càlculs per calcular el número de compassos. El núm de compassos no és un càlcul que s'hagi de fer al final, sinó que s'ha de fer a mida que canvien els compassos.
// 1.midi i 1b.midi. Canvi de tempo. El tempo no té res a veure amb el núm de compassos, ho fa bé. Dóna 22.25 compassos en comptes de 23 perquè no té en compte els silencis finals de l'últim compàs, que no generen events midi.

// TODO
// -informació del num d'events que té cada pista
// -fer una classe i un fitxer de directives
//considerar que Key Signature 
// trobar un exemple de time division in frames per second: /home/joan/midi_files/pop_rock/candlein.mid
// trobar un exemple de format type !=0
// 4.midi: hi ha 11 compassos. El Time Signature canvia amb el temps, s'ha de fer més càlculs per calcular el número de compassos. El núm de compassos no és un càlcul que s'hagi de fer al final, sinó que s'ha de fer a mida que canvien els compassos.
// es pot millorar el Key Signature per dir quina és la clau.

#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "math.h"
#include <string.h>

using namespace std;

typedef struct ch_event { //estructura per inquivir els channel events
   char *ch_event_type; 
   int ch_event_channel;
	int par1;
	char *desc_par1;
	int par2;
} CH_EVENT_STRUCT;

static void usage(void);
bool StringIsEqual(char*, char*);
void mostrarBits ( long int );
char * llista_metaevents ( int );
char * agafar_info_meta (int,int);
char * agafar_info_sysex (int);
//function that returns a struct: http://bytes.com/topic/c/answers/530470-can-functions-return-struct
CH_EVENT_STRUCT agafar_info_channel (int);
char * agafar_info_controller (int);

int format_type;
int time_division; //0: ticks per beat; 1: frames per second
int num_tracks;
int num_tracks_detectats=0;
int ticks_per_beat;
int bpm = 120;
int num_time_signature = 4,den_time_signature = 4;
int key = 0, scale = 0;
double num_bars;
int tempo_has_changed = 0;
int time_signature_has_changed = 0;

ifstream ofs;

int main(int argc, char* argv[])
{
	//http://www.cplusplus.com/doc/tutorial/variables/
	//char: 			1 byte
	//short int: 	2 bytes
	//int:			4 bytes
   //long int (long) 4 bytes
	//long long: 	8 bytes

	char SOFTWARE[] = "smf_parser";
	char DATE[] = "January";
	char YEAR[] = "2012";
	char AUTHOR[] = "Joan Quintana Compte (joanillo)";
	char LICENSE[] = "GPL v.3";
	char WEB[] = "joanqc arroba gmail.com - www.joanillo.org";
	char VERSION[] = "v1.01";

	char *smffile=(char *)"";
	unsigned char rec; //char és 1 byte
	char *cad; //cad serà de longitud variable (és un punter a caràcters)
	unsigned long int track_size = 0;
	int event_type=0; //1: channel_event; 2: meta_event; 3: sysex_event
	int metaevent_type;
	int length = 0;

	int delta_time = 0;
	int delta_time_acumulat = 0;
	int max_delta_time_acumulat = 0; //Serveix per calcular el número de compassos. Pot ser que no totes les pistes tinguin el mateix tamany.

	CH_EVENT_STRUCT ChEvent2;

	int i=0; //comptador de bytes

	//system("clear"); -> ho comento perquè provoca uns caràcters estranys quan volco sobre fitxer
	cout << SOFTWARE << " " << VERSION << "\n"
		<< "Created by " << AUTHOR << "\n"
		<< "Licensed under " << LICENSE << "\n";

	if (argc==1) usage();
	smffile = argv[1];

#if 0 //volem o no imprimir tota la ristra de bytes?
	//obrim per llegir
	//ofs.open( "../midi/canco_bressol.midi", ios::in | ios::binary );
	//ofs.open( "../midi/en_joan_petit_quan_balla.mid", ios::in | ios::binary );
	ofs.open( smffile, ios::in | ios::binary );
	if (ofs == NULL) {
		printf("Loading SMF file failed.\n");
		exit(-1);
	}

	//bucle principal, recorrem tot el fitxer byte a byte, i l'escrivim per pantalla
	while(!ofs.eof()) {
		ofs.read( (char *) &rec, sizeof(char) );
		printf("%x ",rec);
	}
	ofs.close();
#endif

	ofs.open( smffile, ios::in | ios::binary );
	if (ofs == NULL) {
		printf("Loading SMF file failed.\n");
		exit(-1);
	}

	i=0;
	printf("\n");
	//tornem al principi
	ofs.seekg( 0* sizeof(char),std::ofstream::beg );

	//bucle principal, recorrem tot el fitxer byte a byte
	//header chunk
	if (i==0) {
		//format type
		ofs.seekg( 9 * sizeof(char), std::ofstream::beg );
		ofs.read( (char *) &rec, sizeof(char) );
		format_type = (int)rec;
		printf("format type: %d\n",format_type);
		//number of tracks (considero només la possibilitat de número de pistes <=255, 1 byte)
		ofs.seekg( 11 * sizeof(char), std::ofstream::beg );
		ofs.read( (char *) &rec, sizeof(char) );
		num_tracks = (int)rec;
		printf("number of tracks: %d\n",num_tracks);
		//Time Division (bytes 13 i 14)
		ofs.read( (char *) &rec, sizeof(char) );
		if (!(rec>>7)&1) { //el bit més significatiu és un 0: time division in ticks per beat
			time_division = 0;
			printf("time division in ticks per beat (%d)\n", time_division);
			ticks_per_beat = (rec & 127)*256;
			ofs.read( (char *) &rec, sizeof(char) );
			ticks_per_beat += (int)rec;
			printf("Ticks per beat: %d\n",ticks_per_beat);
		} else {
			time_division = 1;
			printf("time division in frames per second (%d)\n", time_division);
			printf("Exit\n");
			exit(0);
		}
		i=i+14;
	}

	for(;;) {

		//track chunk - chunk ID 	"MTrk" (0x4D54726B)
		cad = new char[4];
		ofs.read (cad, 4);
		if (ofs.eof()) break;
		if (StringIsEqual(cad,(char *)"MTrk")) {
			num_tracks_detectats++;
			if (delta_time_acumulat > max_delta_time_acumulat) max_delta_time_acumulat=delta_time_acumulat; 
			delta_time_acumulat=0;
			printf("track #%d\n",num_tracks_detectats);
			i=i+4;
			//chunk size
			cad = new char[4];
			ofs.read (cad, 4);
			//char és 1 byte, i com es pot veure amb el hex2, no és el mateix considerar 8 bits amb signe (char o signed char, que agafa valor negatius), que 8 bits sense signe (unsigned char, 0-255)
			track_size= (unsigned long int)((unsigned char)cad[0]*256*256*256+(unsigned char)cad[1]*256*256+(unsigned char)cad[2]*256+(unsigned char)cad[3]);
			printf("track size: %ld\n",track_size);

			i+=4;
		} else {
			//recuperar la posició anterior
			ofs.seekg( i* sizeof(char),std::ofstream::beg );
		}

		//llegeixo un nou valor
		ofs.read( (char *) &rec, sizeof(char) );
		//printf("%x ",rec);
		i++;

		//detecció Meta Event (0xFF)
		if ((int)rec==255) { //he trobat un 0xFF
			event_type=2;
			ChEvent2.ch_event_type=(char *)"";
			ChEvent2.ch_event_channel=0;
			ofs.read( (char *) &rec, sizeof(char) );
			metaevent_type=(int)rec;
			printf("Meta Event. Type: 0x%x (%ddec), %s",rec,rec,llista_metaevents(metaevent_type));
			i++;
			//ara ve la length, que és de tamany variable-length
			length=0;
			for(;;) {
				ofs.read( (char *) &rec, sizeof(char) );
				i++;
				if ((rec>>7)&1) {
					length = (length +(rec & 127))<<7; //multiplico per 128

				} else {
					length += (rec & 127);
					printf(" (%d bytes): ",length);
					printf("%s\n",agafar_info_meta (metaevent_type,length));
					i += length;
					break; //el bit més significatiu és un 0, podem sortir del bucle
				}
			}
		} 
		//detecció SysEx Event (0xF0 0xF7).
		else if ((int)rec==240 || (int)rec==247) { //he trobat un 0xF0 o 0xF7
			event_type=3;
			ChEvent2.ch_event_type=(char *)"";
			ChEvent2.ch_event_channel=0;
			printf("SysEx Event. ");
			//ara ve la length, que és de tamany variable-length
			length=0;
			for(;;) {
				ofs.read( (char *) &rec, sizeof(char) );
				i++;
				if ((rec>>7)&1) {
					length = (length +(rec & 127))<<7; //multiplico per 128

				} else {
					length += (rec & 127);
					printf(" (%d bytes): ",length);
					printf("%s\n",agafar_info_sysex (length));
					i += length;
					break; //el bit més significatiu és un 0, podem sortir del bucle
				}
			}
		} 
		else {
			//vull mirar si és un delta_time. recuperar la posició anterior per entrar al bucle
			//channel event. hi ha dues possibilitats: un channel event real (delta time, event type, midi channel, par1, par2), o un channel event 'virtual' (0xFF, delta_time=0 i ja està) 
			i--;
			ofs.seekg( (i)* sizeof(char),std::ofstream::beg );
			printf("Channel Event. ");
			//ara ve el Delta Time, que és de tamany variable-length
			delta_time=0;
			for(;;) {
				ofs.read( (char *) &rec, sizeof(char) );
				i++;
				if ((rec>>7)&1) {
					delta_time = (delta_time +(rec & 127))<<7; //multiplico per 128
					} else {
					delta_time += (rec & 127);
					delta_time_acumulat += delta_time;
					printf("t= %d (%d) ",delta_time, delta_time_acumulat);
					break; //el bit més significatiu és un 0, podem sortir del bucle
				}
			}
			//mirem si és un channel event real o virtual -> això és el que falla!!
			ofs.read( (char *) &rec, sizeof(char) );
			//recuperar la posició anterior
			ofs.seekg( (i)* sizeof(char),std::ofstream::beg );
			if ((int)rec >= 128 && (int)rec <= 239) {
				event_type==1; //seguim endavant per extreure la informació d'aquest event
				CH_EVENT_STRUCT ChEvent;
				length=3;
				ChEvent = agafar_info_channel(length);
				printf("%s ch=%d par1=%d ",ChEvent.ch_event_type,ChEvent.ch_event_channel,ChEvent.par1);
				if (ChEvent.ch_event_type=="Controller") printf("(%s) ",ChEvent.desc_par1);
				printf("par2=%d\n",ChEvent.par2);
				i += length;
				ChEvent2.ch_event_type=ChEvent.ch_event_type;
				ChEvent2.ch_event_channel=ChEvent.ch_event_channel;
			} 
			// MIDI streams support a rudimentary form of compression in which
			// successive events with the same “status” (event type and channel) may
			// omit the status byte. És el cas, per ex, de midi/Super_Trouper.mid
			// Es fa necessari la declaració de ChEvent2 per guardar el valor de l'anterior event.
			else if ((int)rec!=255 && (int)rec!=240 && (int)rec!=247) { //n és metaevent ni sysex, per tant ha de continuar sent un channel event
			//else if (ChEvent2.ch_event_type!="" && ChEvent2.ch_event_channel>0) { //o bé
				event_type==1; //seguim endavant per extreure la informació d'aquest event
				CH_EVENT_STRUCT ChEvent;
				length=2;
				ChEvent = agafar_info_channel(length);
				ChEvent.ch_event_type=ChEvent2.ch_event_type;
				ChEvent.ch_event_channel=ChEvent2.ch_event_channel;
				printf("%s ch=%d par1=%d ",ChEvent.ch_event_type,ChEvent.ch_event_channel,ChEvent.par1);
				if (ChEvent.ch_event_type=="Controller") printf("(%s) ",ChEvent.desc_par1);
				printf("par2=%d\n",ChEvent.par2);
				i += length;
			} else {
				event_type==0;
				ChEvent2.ch_event_type=(char *)"";
				ChEvent2.ch_event_channel=0;
				printf("\n");
			}
		}

	}

	printf("End of file\n\n");
	if (delta_time_acumulat > max_delta_time_acumulat) max_delta_time_acumulat=delta_time_acumulat;

	// RESUM ***********************
	// -ticks_per_beat = 384
	// -beats_per_minute = 80
	// -beats_per_bar = 2

	//         9216 t 
	// ------------------------ = 12 bar (compassos) (independent dels bpm)
	// 384 t/beat * 2 beat/bar 
	if (time_signature_has_changed<=1) num_bars = ((double)max_delta_time_acumulat) / ((double)ticks_per_beat * (double)num_time_signature);
		
	printf("ABSTRACT: \n");
	printf("---------\n");
	printf("SMF: %s\n",smffile);
	printf("Format type: %d\n",format_type);
	printf("Time division: %d",time_division);
	(time_division==0) ? printf(" (ticks_per_beat)\n") : printf(" (frames_per_second)\n");
	printf("Number of tracks: %d\n",num_tracks);
	printf("ticks_per_beat: %d\n",ticks_per_beat);
	if (time_signature_has_changed<=1) printf("Time Signature: %d / %d\n",num_time_signature, den_time_signature); //Time Signature pot canviar amb el temps, no és un atribut global
	if (tempo_has_changed<=1) printf("Tempo: %d\n",bpm); //Tempo pot canviar amb el temps, no és un atribut global
	if (time_signature_has_changed<=1) {
		printf("Number of bars: %f\n",num_bars);
	} else {
		printf("Number of bars: Time Signature changes over time (TO DO)\n");
	}
	ofs.close();
	return 0;
}

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////

bool StringIsEqual(char* Str1, char* Str2) // Compare two strings and tell us if they are equal.
{
 if(strlen(Str1) != strlen(Str2))
   return false;
 for(int i = 0; i < strlen(Str2); i++)
   if(Str1[i] != Str2[i])
    return false;
 return true;
}

void mostrarBits ( long int decimal ) //per fer proves
{
	unsigned c, muestraMascara = 1 << 15;
	printf( "%7li = ", decimal );

	for ( c = 1; c <= 16; c++ ) 
	{
		putchar ( decimal & muestraMascara ? '1' : '0' );
		decimal <<= 1;
		if ( c % 8 == 0 )
			putchar ( ' ' );
	}
	printf("\n");
}

char * llista_metaevents (int valor) {
	char *tipus;

	switch ( valor ) {
		case 0 : 
			tipus=(char *)"Sequence Number";
			break;
		case 1 : 
			tipus=(char *)"Text Event";
			break;
		case 2 : 
			tipus=(char *)"Copyright Notice";
			break;
		case 3 : 
			tipus=(char *)"Sequence/Track Name";
			break;
		case 4 : 
			tipus=(char *)"Instrument Name";
			break;
		case 5 : 
			tipus=(char *)"Lyric";
			break;
		case 6 : 
			tipus=(char *)"Marker";
			break;
		case 7 : 
			tipus=(char *)"Cue Point";
			break;
		case 32 : 
			tipus=(char *)"MIDI Channel Prefix";
			break;
		case 33 : //http://www.piclist.com/techref/io/serial/midi/midifile.html
			tipus=(char *)"MIDI Port";
			break;
		case 47 : 
			tipus=(char *)"End of Track";
			break;
		case 81 : 
			tipus=(char *)"Set Tempo";
			tempo_has_changed++;
			break;
		case 84 : 
			tipus=(char *)"SMPTE Offset";
			break;
		case 88 : 
			tipus=(char *)"Time Signature";
			time_signature_has_changed++;
			break;
		case 89 : 
			tipus=(char *)"Key Signature";
			break;
		case 127 : 
			tipus=(char *)"Sequencer-Specific Meta-Event";
			break;
		default:
			tipus=(char *)"";
			break;
	}
	return (tipus);
}

char * agafar_info_meta (int metaevent_type, int length) {
	long long mpqn;
	char *cad=(char *)"";
	cad = new char[length];
	ofs.read (cad, length);


	char *cad2=(char *)"";

	char *cad3=(char *)"";
	cad3 = new char[3];

	switch ( metaevent_type ) {
	case 0 : //Sequence Number, 2 bytes
		break;
	case 1 : case 2: case 3 : case 4: case 5 : case 6: case 7 : //Text Event, Copyright Notice, Sequence/Track Name, Instrument Name, Lyric, Marker, Cue Point (ASCII text)
		cad2 = new char[length];
		cad2=cad;
		//només he trobat un cas en què és necessària aquesta línia. El problema ve del terminating null string. Sik no és correcte, apareixen caràcters estranys. Meta Event. Type: 0x6 (6dec), Marker (12 bytes): SuperTrouperÁrha
		cad2[length]='\0';
		break;
	case 32 : //MIDI Channel Prefix, 1 bytes
		cad2 = new char[length];
		sprintf(cad2,"%d",cad[0]);
		break;
	case 33 : //MIDI Port, 1 bytes ////http://www.piclist.com/techref/io/serial/midi/midifile.html
		cad2 = new char[length];
		sprintf(cad2,"%d",cad[0]);
		break;
	case 47 : //End of Track, 0 bytes
		cad2 = new char[length];
		cad2=cad;
		break;
	case 81 : //Set Tempo, 3 bytes
		//Meta Event 	Type 	Length 	Microseconds/Quarter-Note
		//255 (0xFF) 	81 (0x51) 	3 	0-8355711
		//MICROSECONDS_PER_MINUTE = 60000000
		//BPM = MICROSECONDS_PER_MINUTE / MPQN
		//MPQN = MICROSECONDS_PER_MINUTE / BPM
		mpqn=(long long)((unsigned char)cad[0])*256*256;
		mpqn+=(long long)((unsigned char)cad[1])*256;
		mpqn+=(long long)((unsigned char)cad[2]);
		//printf("MPQN = %lld\n",mpqn);
		bpm = (int) ((long long)60000000 / mpqn);
		cad2 = new char[8];
		sprintf(cad2,"%d bpm",bpm); //converts to decimal base (http://www.cplusplus.com/reference/clibrary/cstdlib/itoa/)
		break;
	case 84 : //SMPTE Offset, 5 bytes
		//cad=(char *)"not implemented at this moment...";
		cad2 = new char[5*3];
		sprintf(cad2,"X%x X%x X%x X%x X%x",cad[0],cad[1],cad[2],cad[3],cad[4]); 
		break;
	case 88 : //Time Signature, 4 bytes
		//el num i den de la time_signature són els dos primers bytes. Els altres dos no interessen;
		num_time_signature=(int)cad[0];
		den_time_signature = pow(2,cad[1]);
		cad2 = new char[4+3+4];
		sprintf(cad2,"%d / %d",num_time_signature,den_time_signature);
		break;
	case 89 : //Key Signature, 2 bytes
		key=(int)cad[0];
		scale=(int)cad[1];
		cad2 = new char[4+4+51];
		sprintf(cad2,"%d (number of sharps or flats); %d (0: major, 1: minor)",key,scale);
		break;
	case 127 : //Sequencer-Specific Meta-Event, variable bytes
		cad2 = new char[length*10]; //per justificar el valor de 10 (i no 3), veure agafar_info_sysex
		for (int i=0;i<length;i++) {
			sprintf(cad3,"X%x ",cad[i]); 
			strcat(cad2,cad3);
		} 
		break;
	}

	return (cad2);
}

char * agafar_info_sysex (int length) {
	char *cad=(char *)"";
	cad = new char[length];
	ofs.read (cad, length);

	char *cad2=(char *)"";
	cad2 = new char[length*10]; //com es veu en l'exemple peter.mid, el desplegament dels bytes del Sysex event a vegades es desplega bé X3b però a vegades es desplega com Xffffff81. Per tant, he de considerar 8 bytes + 2 caràcters (X i espai): 10 bytes (TBD)

	char *cad3=(char *)"";
	cad3 = new char[3];

	for (int i=0;i<length;i++) {
		sprintf(cad3,"X%x ",cad[i]); 
		strcat(cad2,cad3);
	} 

	return (cad2);
}

CH_EVENT_STRUCT agafar_info_channel (int length) {

	CH_EVENT_STRUCT ChEvent;
	int channel_event_type;
	int channel_number;

	char *cad=(char *)"";
	cad = new char[length];
	ofs.read (cad, length);

	if (length==3) { //cas normal
		ChEvent.par1=cad[1];
		ChEvent.par2=cad[2];

		channel_event_type = ((cad[0]>>4)&15) ;
		channel_number = (int)cad[0]&15 ;

		switch (channel_event_type) {
		case 8: //Note Off
			cad=(char *)"Note Off";
			break;
		case 9: //Note On
			cad=(char *)"Note On";
			break;
		case 10: //Note Aftertouch
			cad=(char *)"Note Aftertouch";
			break;
		case 11: //Controller
			cad=(char *)"Controller";
			break;
		case 12: //Program Change
			cad=(char *)"Program Change";
			break;
		case 13: //Channel Aftertouch
			cad=(char *)"Channel Aftertouch";
			break;
		case 14: //Pitch Bend
			cad=(char *)"Pitch Bend";
			break;
		}
	} else if (length==2) { //compressió activada
		ChEvent.par1=cad[0];
		ChEvent.par2=cad[1];
		cad=(char *)"";
	}

	ChEvent.ch_event_type=cad;
	ChEvent.ch_event_channel=channel_number;

	if (channel_event_type==11) ChEvent.desc_par1=agafar_info_controller (ChEvent.par1);
	return (ChEvent);

}

char * agafar_info_controller (int controller_type) {
	char *cad=(char *)"";

	switch (controller_type) {
		case 0: //Bank Select
			cad=(char *)"Bank Select";
			break;
		case 1: //Modulation
			cad=(char *)"Modulation";
			break;
		case 2: //Breath Controller
			cad=(char *)"Breath Controller";
			break;
		case 4: //Foot Controller
			cad=(char *)"Foot Controller";
			break;
		case 5: //Portamento Time
			cad=(char *)"Portamento Time";
			break;
		case 6: //Data Entry (MSB)
			cad=(char *)"Data Entry (MSB)";
			break;
		case 7: //Main Volume
			cad=(char *)"Main Volume";
			break;
		case 8: //Balance
			cad=(char *)"Balance";
			break;
		case 10: //Pan
			cad=(char *)"Pan";
			break;
		case 11: //Expression Controller
			cad=(char *)"Expression Controller";
			break;
		case 12: //Effect Control 1
			cad=(char *)"Effect Control 1";
			break;
		case 13: //Effect Control 2
			cad=(char *)"Effect Control 2";
			break;
		case 16: //General-Purpose Controllers 1
			cad=(char *)"General-Purpose Controllers 1";
			break;
		case 17: //General-Purpose Controllers 2
			cad=(char *)"General-Purpose Controllers 2";
			break;
		case 18: //General-Purpose Controllers 3
			cad=(char *)"General-Purpose Controllers 3";
			break;
		case 19: //General-Purpose Controllers 4
			cad=(char *)"General-Purpose Controllers 4";
			break;
		case 64: //Damper pedal (sustain)
			cad=(char *)"Damper pedal (sustain)";
			break;
		case 65: //Portamento
			cad=(char *)"Portamento)";
			break;
		case 66: //Sostenuto
			cad=(char *)"Sostenuto";
			break;
		case 67: //Soft Pedal
			cad=(char *)"Soft Pedal";
			break;
		case 68: //Legato Footswitch
			cad=(char *)"Legato Footswitch";
			break;
		case 69: //Hold 2
			cad=(char *)"Hold 2";
			break;
		case 70: //Sound Controller 1 (default: Timber Variation)
			cad=(char *)"Sound Controller 1 (default: Timber Variation)";
			break;
		case 71: //Sound Controller 2 (default: Timber/Harmonic Content)
			cad=(char *)"Sound Controller 2 (default: Timber/Harmonic Content)";
			break;
		case 72: //Sound Controller 3 (default: Release Time)
			cad=(char *)"Sound Controller 3 (default: Release Time)";
			break;
		case 73: //Sound Controller 4 (default: Attack Time)
			cad=(char *)"Sound Controller 4 (default: Attack Time)";
			break;
		case 74: //Sound Controller 5
			cad=(char *)"Sound Controller 5";
			break;
		case 75: //Sound Controller 6
			cad=(char *)"Sound Controller 6";
			break;
		case 76: //Sound Controller 7
			cad=(char *)"Sound Controller 7";
			break;
		case 77: //Sound Controller 8
			cad=(char *)"Sound Controller 8";
			break;
		case 78: //Sound Controller 9
			cad=(char *)"Sound Controller 9";
			break;
		case 79: //Sound Controller 10
			cad=(char *)"Sound Controller 10";
			break;
		case 80: //General-Purpose Controllers 5
			cad=(char *)"General-Purpose Controllers 5";
			break;
		case 81: //General-Purpose Controllers 6
			cad=(char *)"General-Purpose Controllers 6";
			break;
		case 82: //General-Purpose Controllers 7
			cad=(char *)"General-Purpose Controllers 7";
			break;
		case 83: //General-Purpose Controllers 8
			cad=(char *)"General-Purpose Controllers 8";
			break;
		case 84: //Portamento Control
			cad=(char *)"Portamento Control";
			break;
		case 91: //Effects 1 Depth (formerly External Effects Depth)
			cad=(char *)"Effects 2 Depth (formerly Tremolo Depth)";
			break;
		case 92: //Effects 2 Depth (formerly Tremolo Depth)
			cad=(char *)"Effects 2 Depth (formerly Tremolo Depth)";
			break;
		case 93: //Effects 3 Depth (formerly Chorus Depth)
			cad=(char *)"Effects 3 Depth (formerly Chorus Depth)";
			break;
		case 94: //Effects 4 Depth (formerly Celeste Detune)
			cad=(char *)"Effects 4 Depth (formerly Celeste Detune)";
			break;
		case 95: //Effects 5 Depth (formerly Phaser Depth)
			cad=(char *)"Effects 5 Depth (formerly Phaser Depth)";
			break;
		case 96: //Data Increment
			cad=(char *)"Data Increment";
			break;
		case 97: //Data Decrement
			cad=(char *)"Data Decrement";
			break;
		case 98: //Non-Registered Parameter Number (LSB)
			cad=(char *)"Non-Registered Parameter Number (LSB)";
			break;
		case 99: //Non-Registered Parameter Number (MSB)
			cad=(char *)"Non-Registered Parameter Number (MSB)";
			break;
		case 100: //Registered Parameter Number (LSB)
			cad=(char *)"Registered Parameter Number (LSB)";
			break;
		case 101: //Registered Parameter Number (MSB)
			cad=(char *)"Registered Parameter Number (MSB)";
			break;
	}

	if (controller_type>=32 && controller_type<=63) { //LSB for controllers 0-31
		//
	}

	if (controller_type>=121 && controller_type<=127) { //Mode Messages
		//
	}

	return (cad);
}

static void usage () {
	cout << "smf_parser is a Standar Midi File (SMF) Parser. It analyzes the structure and content of a SMF file, searching for tracks and midi events.\n";
	cout << "At this stage smf_parser has been tested with midi files in the midi/ folder, considering different type of events and circumstances... but is not fully tested\n\n";
	
	cout << "usage: smf_parser <midi_file>\n\n";
	cout << "remember you can redirect the standard output to a file: smf_parser <midi_file> > midi_file.txt\n\n";
	exit(0);
}