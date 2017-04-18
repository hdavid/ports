#ifndef MIDIOUTPUT_H
#define MIDIOUTPUT_H

class MidiOutput {

	public:
  		MidiOutput();
    	~MidiOutput();
    	void openDevice(const char* device);
		void message(const char* path, int offset, float value);

	private:
  	 	int midifd = -1; 
  		void noteOn(int channel, int note, int velocity);
  		void noteOff(int channel, int note, int velocity);
  		void cc(int channel, int number, int value);
  		int parseInt(const char* a, int offset);

};


#endif  // MIDIOUTPUT_H