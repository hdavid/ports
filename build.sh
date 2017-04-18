#!/bin/bash

echo compile ports

g++ \
	Pixi.cpp Ports.cpp MidiOutput.cpp main.cpp \
	-o portsd \
	-l bcm2835 \
	-l lo \
	-I /usr/include/ \
	-std=c++11 \
	-std=c++0x \
	-pthread && \
	sudo ./portsd no-daemon

