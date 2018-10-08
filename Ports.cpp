#include "Ports.h"

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <lo/lo.h>
#include <lo/lo_cpp.h>

Ports::Ports() {
}

Ports::~Ports() {
}

lo::ServerThread oscServer(5000);

void Ports::start() {
	printf("Ports: Starting\n");
	gettimeofday(&started, NULL);
	gettimeofday(&lastReset, NULL);
	
	// open midi device
	this->midiOutput.openDevice("/dev/snd/midiC1D0");
	this->stop = false;
	this->pixi.configure();
	
	// OSC
	this->startOSC(5000);

	// TIMER
	signal(SIGALRM, pixiTimerCallback); 
	ualarm(PORTS_TIMER_PERIOD, PORTS_TIMER_PERIOD);
}

void Ports::startOSC(int port) {
	//register method for all floats messages
	oscServer.add_method(
		NULL,
		NULL,
		[this](const char *path, const char *types, lo_arg ** argv, int argc) {
			float value = 1;
			if (argc==1 && types[0]=='f'){
				value = (float)argv[0]->f;
			}
			this->oscMessage(path, value);
		}
	);

	// start
	oscServer.start();
	printf("osc started\n");
}





inline void Ports::pixiTimer() {

	//check timer accuracy
	gettimeofday(&now, NULL);
	timersub(&now, &lastTimer, &elapsed);
	lastTimer = now;
	if (elapsed.tv_sec < 100000 && elapsed.tv_usec>PORTS_TIMER_PERIOD * (1+PORTS_TIMER_PERIOD_TOLERANCE)) {
		printf("timer underrun. last timer : %f ms ago.\n", elapsed.tv_usec/1000.0);
	}
	long interval = elapsed.tv_usec;// interval since last pixiTimer execution

	//update channels values.
	for(int channel = 19; channel>=0; channel--){  //must do in reverse order as channel 20 is clock
		//trigger and synched trigger modes
	  	if (PORTS_OUTPUT_MODE_TRIG == channelModes[channel] || PORTS_OUTPUT_MODE_SYNCTRIG == channelModes[channel]) {
			if (channelSyncTriggerRequested[channel] && clockFrame) {
				//trigger requested and lfo just warped : trigger !
				//printf("synched trigger exec %d\n",channel);
				channelTrigCycles[channel] = PORTS_TRIGGER_CYCLES;
				channelSyncTriggerRequested[channel] = false;
			}
			if (channelTrigCycles[channel] > 0) {
				if (channelTrigCycles[channel] == PORTS_TRIGGER_CYCLES){
					//printf("trig start %d\n", channel);
				}
				channelTrigCycles[channel]--;
				if (channelValues[channel] != PORTS_TRIGGER_LEVEL) {
			  		//printf("trig set to one : %d\n",channel);
					channelValues[channel] = PORTS_TRIGGER_LEVEL;
					pixi.setChannelValue(channel, channelValues[channel]);
				}
			} else {
				if (channelValues[channel] != 0) {
					//printf("trig end %d\n", channel);
					channelValues[channel] = 0;
					pixi.setChannelValue(channel, channelValues[channel]);
				}
			}
		

		//check if mode is LFO kind
		} else if (channelIsLfo(channel)) {

			if (channelLFOFrequencies[channel]>0) { //ignore zero frequency
				//compute phase increment
				double lfoPeriod = (1000000.0 / channelLFOFrequencies[channel]);
				channelLFOPhases[channel] += interval/lfoPeriod;
				//lfo-19 just warped. clock fram !
				if (channelLFOPhases[channel]>1) {
					channelLFOPhases[channel] -= 1;
					if (channel == 19){
						clockFrame=true;
					}
				}
			}
			// range and offset
			float lfo_offset = BIPOLAR_POWER ? 0.0 : 0.5; // bipolar values are [-1 1], unipolar [0, 1]
			float lfo_range = BIPOLAR_POWER ? 1.0 : 0.5; // bipolar values are [-1 1], unipolar [0, 1]

			double phase = channelLFOPhases[channel];
			switch (channelModes[channel]) {
			  case PORTS_OUTPUT_MODE_LFO_SINE:
				channelValues[channel] = sin(phase * 2 * M_PI) * lfo_range + lfo_offset; //TODO: use lookup table
				break;
			  case PORTS_OUTPUT_MODE_LFO_SAW:
				channelValues[channel] = (1 - phase * 2) * lfo_range + lfo_offset;
				break;
			  case PORTS_OUTPUT_MODE_LFO_RAMP:
				channelValues[channel] = (phase * 2 - 1) * lfo_range + lfo_offset;
				break;
			  case PORTS_OUTPUT_MODE_LFO_TRI:
				channelValues[channel] = ((phase < 0.5 ? (phase  * 2) : (2 - phase * 2) ) * 2 - 1 ) * lfo_range + lfo_offset;
				break;
			  case PORTS_OUTPUT_MODE_LFO_SQUARE:
				channelValues[channel] = (phase < channelLFOPWMs[channel] ? 1 : -1) * lfo_range + lfo_offset;
				break;
			}
			pixi.setChannelValue(channel, channelValues[channel]);
		}
	}
	
	// update pixi
	pixi.update();

}


void Ports::oscMessage(const char* path, float v) {
	float value = v;
	int offset = 0;
	if (strncmp(path, "/in/", 3)==0) {
		offset += 4;
		//TODO: input handling....

	} else if (strncmp(path, "/reset", 6)==0) {
		offset += 6;
		gettimeofday(&now, NULL);
		timersub(&now, &lastReset, &elapsed);
		if (elapsed.tv_sec > 10) {
		   	lastReset = now;
			std::cout << "resetting pixi.\n";
			pixi.configure();
		}

	} else if (strncmp(path, "/restart", 8)==0) {
		offset += 8;
	 	gettimeofday(&now, NULL);
		timersub(&now, &lastReset, &elapsed);
		if (elapsed.tv_sec > 10) {
			timersub(&now, &started, &elapsed);
			if (elapsed.tv_sec > 10) { //only restart if we started more than 30 sec ago
				lastReset = now;
				std::cout << "restarting\n";
				restart = true;
				stop = true;
				//system("killall portsd; sleep 1;/usr/sbin/portsd no-daemon");
				//exec("/usr/sbin/portsd no-daemon");
				//execl("/usr/sbin/portsd", "/usr/sbin/portsd", "no-daemon", (char *) 0);
				//
			}
		}
	} else if (strncmp(path, "/reboot", 7)==0) {
		offset += 7;
		gettimeofday(&now, NULL);
		timersub(&now, &lastReset, &elapsed);
		if (elapsed.tv_sec > 10){
			lastReset = now;
			std::cout << "rebooting";
			system("reboot now\n");
		}
		
		
	} else if (strncmp(path, "/a/", 3)==0 
		|| strncmp(path, "/b/", 3)==0 
		|| strncmp(path, "/c/", 3)==0
		|| strncmp(path, "/d/", 3)==0
		|| strncmp(path, "/e/", 3)==0
		|| strncmp(path, "/f/", 3)==0
		|| strncmp(path, "/g/", 3)==0
		|| strncmp(path, "/h/", 3)==0
	) {

		int bank = 0;
	

		if (strncmp(path, "/a/", 3)==0){
			bank = 0;
		} else if (strncmp(path, "/b/", 3)==0){
			bank = 1;
		} else if (strncmp(path, "/c/", 3)==0){
			bank = 2;
		} else if (strncmp(path, "/d/", 3)==0){
			bank = 3;
		} else if (strncmp(path, "/e/", 3)==0){
			bank = 4;
		} else if (strncmp(path, "/f/", 3)==0){
			bank = 5;
		} else if (strncmp(path, "/g/", 3)==0){
			bank = 6;
		} else if (strncmp(path, "/h/", 3)==0){
			bank = 7;
		}
		int numBanks = 3;
		if (bank + 1 > currentBank + numBanks) {
			return;
		}
		offset += 3;
		int channel = parseInt(path, offset);
		offset += channel<10?2:channel<100?3:4;
		channel += bank*8;
		
		channel -= 1;
		if (channel >= 0 && channel<20){
			handleChannelMsg(path,offset,channel, value);
		} else {
			std::cout << "invalid channel : " << path << "\n";
		}
	} else if (strncmp(path, "/out/", 5)==0)  {
		offset += 5;
		int channel = parseInt(path, offset);
		offset += channel<10?2:channel<100?3:4;
		channel -= 1;
		if (channel >= 0 && channel<20){
			handleChannelMsg(path,offset,channel, value);
		} else {
			std::cout << "invalid channel : " << path << "\n";
		}
	} else if (strncmp(path, "/midi/", 6)==0) {
		offset += 6;
		//std::cout << "OSC -> " << path << " : " << value << "\n";
		midiOutput.message(path,offset, value);
	}			
}

void Ports::handleChannelMsg(const char* path, int offset, int channel, float value){
	int mode = parseOutputMode(path, offset);
	bool force = false;
	if (channelModes[channel] != mode) {
		channelModes[channel] = mode;
		force = true;
	}
	bool isBipolar = channelIsBipolar(channel);
	if (channelIsLfo(channel)){
		//lfo frequency clipping
		if (value<0){
			value = 0;
		}
		if (value>10000){
			value = 10000;
		}
		channelLFOFrequencies[channel] = value;
		pixi.setChannelMode(channel, false, isBipolar, force);
	} else {
		//value scaling
		if (value > 1) {
			value = 1;
		}
		if (channelIsBipolar(channel)){
			if (value <= -1){
				value = -1;
			}
		} else {
			if (value < 0) {
				value = 0;
			}	
		}
		channelValues[channel] = value;
		//change pixi mode
		pixi.setChannelMode(channel, false, isBipolar, force);
		pixi.setChannelValue(channel, value);
		if (PORTS_OUTPUT_MODE_TRIG == channelModes[channel]) {
			channelTrigCycles[channel] = PORTS_TRIGGER_CYCLES;
		}else if (PORTS_OUTPUT_MODE_SYNCTRIG == channelModes[channel]) {
			channelSyncTriggerRequested[channel] = true;
		}
		
	}
}

int Ports::parseOutputMode(const char* str, int offset){
	if (strncmp(str+offset, "gate", 4)==0) {
		return PORTS_OUTPUT_MODE_GATE;
	} else if (strncmp(str+offset, "trig", 4)==0) {
		return PORTS_OUTPUT_MODE_TRIG;
	} else if (strncmp(str+offset, "synctrig", 8)==0) {
		return PORTS_OUTPUT_MODE_SYNCTRIG;
	} else if (strncmp(str+offset, "flipflop", 8)==0) {
		return PORTS_OUTPUT_MODE_FLIPFLOP;
	} else if (strncmp(str+offset, "cvuni", 5)==0 || strncmp(str+offset, "cv", 2)==0) {
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
	} else if (strncmp(str+offset, "cvuni", 5)==0 || strncmp(str+offset, "cv", 2)==0) {
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
  int modee = channelModes[channel];
  return BIPOLAR_POWER && (!(modee < 50 || (modee >= 100 && modee < 150)));
}


Ports portsInstance;

inline void pixiTimerCallback(int sig_num){
	portsInstance.pixiTimer();
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

/*main() {

	ports.start();

	while(true){
	  std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	//std::thread threadLibLo(startOSCLibLo, "5002");
	//std::thread threadOSCPACK(startOSCPACK, 1);
	//signal(SIGALRM, timerCallback);   
	//this one makes udp server to stop working
	//ualarm(PORTS_TIMER_PERIOD, PORTS_TIMER_PERIOD);
	//startOSCLibLo("5000");
 	std::cout << "Ports ending.\n";
	return 0;
}*/



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