/*
  Pixi.cpp - Library Maxim PIXI A/D chip.
  (heavily) inspired by work of Wolfgang Friedrich
*/

#include "Pixi.h"
#include <stdio.h>
#include <iostream>
#include <bcm2835.h>
#include <unistd.h>


Pixi::Pixi() {

}


Pixi::~Pixi(){
	bcm2835_spi_end();
	bcm2835_close();
}


/*
Read register and return value
if the debug parameter is set to TRUE, the register value is printed in format
SPI read adress: 0x0 : h 0x4 : l 0x24
SPI read result 0x424
*/
inline uint16_t Pixi::readRegister(uint8_t address) {
    char out[3] = {(char)((address << 0x01) | PIXI_READ), 0x00, 0x00};
	char in[3]  = {0x00, 0x00, 0x00};
	bcm2835_spi_transfernb(out, in, 3);
	uint16_t result = (in[1] << 8) + in[2];
	return (result);
}


/*
write value into register
Address needs to be shifted up 1 bit, LSB is read/write flag
hi data byte is sent first, lo data byte last.
*/
inline void Pixi::writeRegister(uint8_t address, uint16_t value) {
 	char out[3] = { 
		(char) ((address << 0x01) | PIXI_WRITE), 
		(char) (value >> 8), 
		(char) (value & 0xFF)
	};
	bcm2835_spi_writenb(out, 3);
}


/*
General Config for the PixiShield
Read ID register to make sure the shield is connected.
*/
void Pixi::configure() {
	if (!bcm2835_init()) {
		std::cout << "Pixi: could not init bcm2835\n";
	} else{
		bcm2835_spi_begin();
		bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);		// The default
		bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);						// The default
		bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);		// ~ 4 MHz
		//bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_16);		// ~20 MHz
		bcm2835_spi_chipSelect(BCM2835_SPI_CS0);						// The default
		bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);		// the default
		
	}
  	uint16_t result = 0;	
  	uint16_t deviceId = readRegister(PIXI_DEVICE_ID);

	if (deviceId == 0x0424 || true) {
		std::cout << "Pixi: id: " << deviceId << "\n";
		// enable default burst, thermal shutdown, leave conversion rate at 200k
		writeRegister (PIXI_DEVICE_CTRL, !BRST | THSHDN); // ADCCONV = 00 default.
		// enable internal temp sensor
		// disable series resistor cancelation
		uint16_t deviceControlRegister = readRegister (PIXI_DEVICE_CTRL);
		writeRegister (PIXI_DEVICE_CTRL, deviceControlRegister | !RS_CANCEL);
		// keep TMPINTMONCFG at default 4 samples
		// Set int temp hi threshold
		writeRegister (PIXI_TEMP_INT_HIGH_THRESHOLD, 0x0230 );		// 70 deg C in .125 steps
		// Keep int temp lo threshold at 0 deg C, negative values need function to write a two's complement number.
		// enable internal and both external temp sensors
		deviceControlRegister = readRegister ( PIXI_DEVICE_CTRL);
		writeRegister (PIXI_DEVICE_CTRL, deviceControlRegister | TMPCTLINT | TMPCTLEXT1 | TMPCTLEXT2);
		
		
		// set device control register
		// MAX3100 PDF page 39
    	deviceControlRegister = readRegister(PIXI_DEVICE_CTRL);
    	uint16_t adc_ctl  = 0b11;	// ADCCTL  [0:1] ADC conversion mode selection. 
									//	00:Idle mode. 01:Single sweep. 10:Single conversion one port at a time, 11:Continuous sweep This mode is not controlled by CNVT.
    	uint16_t dac_ctl  = 0b00;	// DACCTL  [3:2] DAC mode selection.  
									//	00:Sequential update mode for DAC-configured ports. 01:Immediate update mode for DAC-configured ports.
    	uint16_t adc_conv = 0b00;	// ADCCONV [5:4] ADC conversion rate selection. 
									//	00:200ksps (default), 01:250ksps, 10:333ksps, 11:400ksps
    	uint16_t dac_ref  = 0b1;	// DACREF  [6]   DAC voltage reference 
									//	1:external, 0:internal

    	writeRegister(PIXI_DEVICE_CTRL, deviceControlRegister
                       | ADC_MODE_CONT
                       | DACREF
                       | !DACCTL
                      );
		
		usleep(1000);
		float temperature = readTemperature ( TEMP_CHANNEL_INT );
    	std::cout << "Pixi: internal temperature:" << temperature << "\n";
	
	} else {
		std::cout << "Pixi: max11300 not found\n";
		//do a loopback test on the SPI
		char out[3] = {0x01, 0x02, 0x03};
		char in[3]  = {0x00, 0x00, 0x00};
		bcm2835_spi_transfernb(out, in, 3);
		//uint16_t result = (in[1] << 8) + in[2];
		std::cout << "Pixi: loopback test:" << (int)in[0] << " " << (int)in[1] << " " << (int)in[2] << "\n";

	}
}


uint16_t Pixi::getOutput(int channel) {
	return outputs[channel];
}


uint16_t Pixi::getInput(int channel) {
  	return inputs[channel];
}


float Pixi::getOutputVoltage(int channel) {
	if (channelIsBipolar[channel]) {
		return outputs[channel] / 4095.0 * 10.0 - 5;
  	} else {
    	return outputs[channel] / 4095.0 * 10.0;
  	}
}


float Pixi::getInputVoltage(int channel) {
	if (channelIsBipolar[channel]) {
		return outputs[channel] / 4095.0 * 10.0 - 5;
	} else {
		return outputs[channel] / 4095.0 * 10.0;
	}
}


/*
Readout of given temperature channel and conversion into degC float return value
*/
float Pixi::readTemperature(int temp_channel) {
	float result = 0;
	uint16_t rawresult = 0;
	bool sign = 0;
	// INT_TEMP_DATA is the lowest temp data adress, channel runs from 0 to 2.
	rawresult =  readRegister( PIXI_INT_TEMP_DATA + temp_channel); 
	sign = ( rawresult & 0x0800 ) >> 11; 
	if (sign == 1) {
		rawresult = ( ( rawresult & 0x07FF ) xor 0x07FF ) + 1;	// calc absolut value from 2's comnplement
	}
	result = 0.125 * ( rawresult & 0x0007 ) ;	// pick only lowest 3 bit for value left of decimal point  
												// One LSB is 0.125 deg C
	result = result + ( ( rawresult >> 3) & 0x01FF ) ;
	if (sign == 1){
		result = result * -1;	// fix sign
	}
	return (result);
}  


/*
output analog value when channel is configured in mode 5
*/
inline void Pixi::writeAnalog(int channel, uint16_t value) {
    writeRegister(PIXI_DAC_DATA + channel, value);
}


inline uint16_t Pixi::readAnalog(int channel) {
	return readRegister(PIXI_ADC_DATA + channel);
}


void Pixi::setChannelMode(int channel, bool isInput, bool isBipolar, bool force) {
	if (channelIsInput[channel] != isInput || channelIsBipolar[channel] != isBipolar || force) {
		// update channel config
		channelIsInput[channel] = isInput;
		channelIsBipolar[channel] = isBipolar;
      	// MAX3100 pdf page 43
      	uint8_t channelMode = channelIsInput[channel] ? CH_MODE_ADC_P : CH_MODE_DAC_ADC_MON; // CH_MODE_DAC  CH_MODE_DAC_ADC_MON
      	uint8_t range = channelIsBipolar[channel] ? CH_5N_TO_5P : CH_0_TO_10P;	// no -10v ref CH_5N_TO_5P
      	writeRegister(PIXI_PORT_CONFIG + channel,
                    (
                      ( (channelMode << 12 ) & FUNCID )
                      //| ( (configuration.adcVoltageReference << 11) & FUNCPRM_AVR_INV) // adcVoltageReference 0 internal, 1 external
                      | ( (0x0b << 11) & FUNCPRM_AVR_INV)
                      | ( (range << 8 ) & FUNCPRM_RANGE )
                      //| ( (configuration.nbAvgADCSamples << 4) & FUNCPRM_NR_OF_SAMPLES) // number of adc samples. in power of two.
	                 | ( (0x1b << 4) & FUNCPRM_NR_OF_SAMPLES)
                    )
                   );
    	std::cout << "configured -> /" << (isInput?"in/":"out/") << channel << "/ " <<  (isBipolar?"bi":"uni") << (force?" forced":"") << "\n";
	}
}


void Pixi::setChannelValue(int channel, float value) {
	if (!channelIsInput[channel]){
		uint16_t out = 0;
		if (channelIsBipolar[channel]){
			out = (uint16_t)(value * 2047 + 2048);
		} else {
			out = (uint16_t)(value * 4095);
		}
		if (out > 4095){
			out = 4095;
		} else if (out < 0){
			out = 0;
		}
		outputs[channel] = out;
	}
}


void Pixi::update() {
	for (int channel=0; channel<PIXI_NUM_CHANNELS; channel++){
		if (!channelIsInput[channel]) {
			if (outputLastSentValues[channel] != outputs[channel]) {
				writeAnalog(channel, outputs[channel]);
				outputLastSentValues[channel] = outputs[channel];
			}
		}
	}
}
