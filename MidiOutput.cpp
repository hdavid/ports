
#include "MidiOutput.h"
#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

//https://www.midi.org/specifications/item/table-1-summary-of-midi-message

MidiOutput::MidiOutput(){
}


MidiOutput::~MidiOutput(){
}

void MidiOutput::openDevice(const char* device) {
	// open midi device
	midifd = open(device, O_WRONLY, 0);
	if (midifd < 0) {
		printf("MidiOutput: Error: cannot open midi device %s\n", device);
		//exit(1);
	} else {
		std::cout << "MidiOutput: opened midi device " << device << "\n";
	}
}



int MidiOutput::parseInt(const char* a, int offset) {
	int sign, n, c;
	c = 0;
	n = 0;
	if (a[offset] == '-') {  // Handle negative integers
		sign = -1;
     	offset++;
  	} else {
  		sign = 1;
  	}
  	while(a[offset]!='/' && offset<strlen(a)){
  		n = n * 10 + a[offset] - '0';
  		offset++;
  		c++;
  	}
  	if (sign == -1) {
	    n = -n;
  	}
  	return n;
}



void MidiOutput::message(const char* path, int offset, float value) {
	if (midifd>0 && false) {
		std::cout << "ignoring midi message, no output";
	}
	//parse midi channel
	int channel = parseInt(path, offset);
	offset += channel<10?1:channel<100?2:3;
	if (strncmp(path+offset, "/cc/", 3)==0) {
		offset += 4;
		int number = parseInt(path, offset);
		//std::cout << "channel:" << channel << " cc:" << number << " value:"<< value << " \n" ;
		cc(channel,number,value);
	} else if (strncmp(path+offset, "/noteOn/", 8)==0) {
		offset += 8;
		int note = parseInt(path, offset);
		//std::cout << "channel:" << channel << " noteOn:" << note << " value:"<< value << " \n" ;
		noteOn(channel,note,value);
	} else if (strncmp(path+offset, "/noteOff/", 9)==0) {
		offset += 9;
		int note = parseInt(path, offset);
		//std::cout << "channel:" << channel << " noteOff:" << note << " value:"<< value << " \n" ;
		noteOff(channel,note,value);
	}
}




void MidiOutput::noteOn(int channel, int note, int velocity) {
    unsigned char packet[3];
	packet[0] = 0b10010000+(char)(channel-1); //1011=cc. 1001=noteOn, 1000=noteOff 
	packet[1] = (unsigned int) (note-1); //note / cc
	packet[2] = (unsigned int) velocity; //(unsigned int)value
	write(midifd, &packet, 3);
}

void MidiOutput::noteOff(int channel, int note, int velocity){
	unsigned char packet[3];
	packet[0] = 0b10000000+(char)(channel-1); //1011=cc. 1001=noteOn, 1000=noteOff 
	packet[1] = (unsigned int) (note-1); //note / cc
	packet[2] = (unsigned int) velocity; //(unsigned int)value
	write(midifd, &packet, 3);
}

void MidiOutput::cc(int channel, int number, int value){
	unsigned char packet[3];
	packet[0] = 0b10110000+(char)(channel-1); //1011=cc. 1001=noteOn, 1000=noteOff 
	packet[1] = (unsigned int) (number-1); //note / cc
	packet[2] = (unsigned int) value; //(unsigned int)value
	write(midifd, &packet, 3);
}
