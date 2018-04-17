#ifndef PORTS_H
#define PORTS_H

#include "Pixi.h"
#include "MidiOutput.h"

#include <stdlib.h>
#include <sys/time.h>


#define PORTS_OUTPUT_MODE_GATE 1
#define PORTS_OUTPUT_MODE_TRIG 2
#define PORTS_OUTPUT_MODE_SYNCTRIG 3
#define PORTS_OUTPUT_MODE_CVUNI 4

#define PORTS_OUTPUT_MODE_FLIPFLOP 10

#define PORTS_OUTPUT_MODE_RANDOM_SH 40

#define PORTS_OUTPUT_MODE_CVBI 50

#define PORTS_OUTPUT_MODE_LFO_SINE 71
#define PORTS_OUTPUT_MODE_LFO_SAW 81
#define PORTS_OUTPUT_MODE_LFO_RAMP 82
#define PORTS_OUTPUT_MODE_LFO_TRI 83
#define PORTS_OUTPUT_MODE_LFO_SQUARE 91

#define PORTS_INPUT_MODE_GATE 100
#define PORTS_INPUT_MODE_TRIG 101
#define PORTS_INPUT_MODE_CVUNI 102

#define PORTS_INPUT_MODE_CVBI 150

#define PORTS_TIMER_PERIOD 2000 //in microseconds
#define PORTS_TIMER_PERIOD_TOLERANCE 0.3 //
#define PORTS_TRIGGER_CYCLES 10000 / PORTS_TIMER_PERIOD // 


#define BIPOLAR_POWER false

//forward delcaration
inline void pixiTimerCallback(int sig_num);


class Ports {

  public:
    Ports();
    ~Ports();

	void start();
    void startOSC(int port);
	void pixiTimer();
    void oscMessage(const char *path, const float value);
    bool stop;
    bool restart;
  private:
  	Pixi pixi;
  	MidiOutput midiOutput;
	int channelModes[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int channelTrigCycles[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	bool channelSyncTriggerRequested[20] = {false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false};
	double channelValues[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	double channelLFOPhases[20]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // 0 to 1
	double channelLFOFrequencies[20]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	double channelLFOPWMs[20]= {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5}; 
	bool clockFrame=false;
	struct timeval lastTimer;
	struct timeval lastReset;
	struct timeval now;
	struct timeval elapsed;
	struct timeval started;
	int parseChannel(const char* path, int offset);
	int parseOutputMode(const char* path, int offset);
	int parseInputMode(const char* path, int offset);
	bool channelIsInput(int channel);
	bool channelIsLfo(int channel);
	bool channelIsBipolar(int channel);
	int parseInt(const char* a, int offset);
  
};

extern Ports portsInstance;

#endif  // PORTS_H
