#include "Ports.h"
#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <lo/lo.h>
#include <thread>


Ports::Ports() {
	std::cout << "Ports::Ports\n";
	midiOutput.openDevice("/dev/snd/midiC1D0");
	pixi.configure();
}

Ports::~Ports() {
	
}


int Ports::parseInt(const char* a, int offset) {
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


inline void Ports::timer() {
	//check timer accuracy
    gettimeofday(&now, NULL);
    timersub(&now, &lastTimer, &elapsed);
    lastTimer = now;
	if (elapsed.tv_sec < 100000 && elapsed.tv_usec>PORTS_TIMER_PERIOD * (1+PORTS_TIMER_PERIOD_TOLERANCE)) {
    	printf("timer underrun. last timer : %f ms ago.\n", elapsed.tv_usec/1000.0);
    }
    
    //update channels
    long interval = elapsed.tv_usec;
	for(int channel=0; channel<20; channel++){
		//trigger mode
	  	if (PORTS_OUTPUT_MODE_TRIG == channelModes[channel]) {
			if (channelTrigCyles[channel] > 0) {
			  	channelValues[channel] =  1;
			  	channelTrigCyles[channel]--;
			} else {
			  	channelValues[channel] = 0;
			}
			pixi.setChannelValue(channel, channelValues[channel], false, channelIsBipolar(channel));
		}

	  //check if mode is LFO kind
	  if (channelIsLfo(channel)) {
		double lfoPeriod = (1000000.0 / channelLFOFrequencies[channel]);
		channelLFOPhases[channel] += interval/lfoPeriod;
		if (channelLFOPhases[channel]>1) {
			channelLFOPhases[channel] -= 1;
		}
		double phase = channelLFOPhases[channel];
		//std::cout << "LFO " << channel << " : " << phase << "\n";
		switch (channelModes[channel]) {
		  case PORTS_OUTPUT_MODE_LFO_SINE:
			channelValues[channel] = sin(phase * 2 * M_PI) * 0.5 + 0.5;//TODO: use lookup table
			break;
		  case PORTS_OUTPUT_MODE_LFO_SAW:
			channelValues[channel] = (1 - phase);
			break;
		  case PORTS_OUTPUT_MODE_LFO_RAMP:
			channelValues[channel] = phase;
			break;
		  case PORTS_OUTPUT_MODE_LFO_TRI:
			channelValues[channel] = (phase < 0.5 ? phase * 2 : (1 - phase) * 2) * 0.5 + 0.5;
			break;
		  case PORTS_OUTPUT_MODE_LFO_SQUARE:
			channelValues[channel] = (phase < channelLFOPWMs[channel]) ? 1 : 0;
			break;
		}
		pixi.setChannelValue(channel, channelValues[channel], false, channelIsBipolar(channel));
	  }
	}
    
    //update pixi
	pixi.update();

}


void Ports::oscMessage(const char *path, float value) {
	int offset = 0;
	if (strncmp(path, "/in/", 3)==0) {
		offset += 4;
		//std::cout << "OSC message : " << path << " : " << value << "\n";
		//pixi.setChannel(0,  value, true, false);
	} else if (strncmp(path, "/out/", 5)==0) {
		offset += 5;
		//value scaling

		int channel = parseInt(path, offset);
		if (channel > 0 && channel<=20){
			offset += channel<10?2:channel<100?3:4;
			channel -= 1;
			int mode = parseOutputMode(path, offset);
			bool force = false;
			if (channelModes[channel] != mode) {
				channelModes[channel] = mode;
				force = true;
			}
			bool isBipolar = channelIsBipolar(channel);
			std::cout << "OSC -> " << path << " : " << value << "\n";
			if (channelIsLfo(channel)){
				if (value<=0){
					value = 0.01;
				}
				if (value>1000){
					value = 1000;
				}
				channelLFOFrequencies[channel] = value;
				pixi.setChannelMode(channel, false, isBipolar, force);
			}else{
				if (value > 1) {
					value = 1;
				}
				if (value < 0) {
					value = 0;
				}
				channelValues[channel] = value;
				//change pixi mode
				pixi.setChannelMode(channel, false, isBipolar, force);
				pixi.setChannelValue(channel, value, false, isBipolar);
			}
			
		} else {
			std::cout << "invalid channel : " << path << "\n";
		}
	} else if (strncmp(path, "/midi/", 6)==0) {
		offset += 6;
		std::cout << "OSC -> " << path << " : " << value << "\n";
		midiOutput.message(path,offset, value);
	}			
}


int Ports::parseOutputMode(const char* str, int offset){
	if (strncmp(str+offset, "gate", 4)==0) {
		return PORTS_OUTPUT_MODE_GATE;
	} else if (strncmp(str+offset, "trig", 4)==0) {
		return PORTS_OUTPUT_MODE_TRIG;
	} else if (strncmp(str+offset, "flipflop", 8)==0) {
		return PORTS_OUTPUT_MODE_FLIPFLOP;
	} else if (strncmp(str+offset, "cvuni", 5)==0) {
		return PORTS_OUTPUT_MODE_CVUNI;
	} else if (strncmp(str+offset, "cvbi", 4)==0) {
		return PORTS_OUTPUT_MODE_CVBI;
	} else if (strncmp(str+offset, "sh", 2)==0) {
		return PORTS_OUTPUT_MODE_RANDOM_SH;
	} else if (strncmp(str+offset, "lfosine", 7)==0) {
		return PORTS_OUTPUT_MODE_LFO_SINE;
	} else if (strncmp(str+offset, "lfosaw", 6)==0) {
		return PORTS_OUTPUT_MODE_LFO_SAW;
	} else if (strncmp(str+offset, "lforamp", 7)==0) {
		return PORTS_OUTPUT_MODE_LFO_RAMP;
	}else if (strncmp(str+offset, "lfotri", 6)==0) {
		return PORTS_OUTPUT_MODE_LFO_TRI;
	} else if (strncmp(str+offset, "lfosquare", 9)==0) {
		return PORTS_OUTPUT_MODE_LFO_SQUARE;
	}
	return -1;
}


int Ports::parseInputMode(const char* str, int offset){
	if (strncmp(str+offset, "gate", 4)==0) {
		return PORTS_INPUT_MODE_GATE;
	} else if (strncmp(str+offset, "trig", 4)==0) {
		return PORTS_INPUT_MODE_TRIG;
	//} else if (strncmp(str+offset, "flipflop", 8)==0) {
	//	return PORTS_INPUT_MODE_FLIPFLOP;
	} else if (strncmp(str+offset, "cvuni", 5)==0) {
		return PORTS_INPUT_MODE_CVUNI;
	} else if (strncmp(str+offset, "cvbi", 4)==0) {
		return PORTS_INPUT_MODE_CVBI;
	}
	return -1;
}


bool Ports::channelIsInput(int channel) {
  return  channelModes[channel] >= 100;
}


bool Ports::channelIsLfo(int channel) {
  return channelModes[channel] > 70 && channelModes[channel] < 100;
}


bool Ports::channelIsBipolar(int channel) {
	return false;
  int modee = channelModes[channel];
  return (modee < 50 || modee >= 100 && modee < 150) ? false : true;
}





// MAIN



Ports ports;


inline void timerCallback(int sig_num){
	ports.timer();
}

void libloError(int num, const char *msg, const char *path) {
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
}

/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int libloHandler(const char *path, const char *types, lo_arg ** argv, 
int argc, void *data, void *user_data) {
    float value = 1;
    if (argc==1 && types[0]=='f'){
    	value = (float)argv[0]->f;
    }
    ports.oscMessage(path, value);
    return 0;
}

int startOSCLibLo(const char* port) {
	int done = 0;
	void error(int num, const char *m, const char *path);
	int lo_fd;
	fd_set rfds;
	int retval;
	lo_server s = lo_server_new(port, libloError); 
  
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000*1000;
    lo_server_add_method(s, NULL, NULL, libloHandler, NULL);
    /* get the file descriptor of the server socket, if supported */
    lo_fd = lo_server_get_socket_fd(s);
    if (lo_fd > 0) {
    	while(!done){
			FD_ZERO(&rfds);
			FD_SET(lo_fd, &rfds);
			retval = select(lo_fd + 1, &rfds, NULL, NULL, NULL);  //&tv       /* no timeout */
			if (retval == -1) {
				//printf("select() error\n");
				//exit(1);
			} else if (retval > 0) {
				//if (FD_ISSET(0, &rfds)) {
				//    read_stdin();
				//}
				if (FD_ISSET(lo_fd, &rfds)) {
					lo_server_recv_noblock(s, 0);
				}
			}
		}
	} else {
		std::cout << "lo fd is zero !!! \n";
	}
    //if (lo_fd > 0) {
        /* select() on lo_server fd is supported, so we'll use select()
         * to watch both stdin and the lo_server fd. */
        //do {
           
        //} while (!done);
    //}
    //return 0;
}



main() {
	std::cout << "Ports starting\n";
    //std::thread threadLibLo(startOSCLibLo, "5002");
	//std::thread threadOSCPACK(startOSCPACK, 1);
	signal(SIGALRM, timerCallback);   
	//this one makes udp server to stop working
    ualarm(PORTS_TIMER_PERIOD, PORTS_TIMER_PERIOD);
    startOSCLibLo("5000");
 	std::cout << "Ports ending.\n";
	return 0;
}









/*
String Ports::channelGetModeName(int channel) {
  String out = "";
  switch (channelModes[channel]) {
    case OUTPUT_MODE_GATE:
      out = "gate";
      break;
    case OUTPUT_MODE_TRIG:
      out = "trig";
      break;
    case OUTPUT_MODE_FLIPFLOP:
      out = "flipflop";
      break;
    case OUTPUT_MODE_CVUNI:
      out = "cvuni";
      break;
    case OUTPUT_MODE_CVBI:
      out = "cvbi";
      break;
    case OUTPUT_MODE_RANDOM_SH:
      out = "sh";
      break;
    case OUTPUT_MODE_LFO_SINE:
      out = "lfosine";
      break;
    case OUTPUT_MODE_LFO_SAW:
      out = "lfosaw";
      break;
    case OUTPUT_MODE_LFO_RAMP:
      out = "lforamp";
      break;
    case OUTPUT_MODE_LFO_TRI:
      out = "lfotri";
      break;
    case OUTPUT_MODE_LFO_SQUARE:
      out = "lfosquare";
      break;

    case INPUT_MODE_GATE:
      out = "gate";
      break;
    case INPUT_MODE_TRIG:
      out = "trig";
      break;
    case INPUT_MODE_CVUNI:
      out = "cvuni";
      break;
    case INPUT_MODE_CVBI:
      out = "cvbi";
      break;
  }
  return out;
}
*/









