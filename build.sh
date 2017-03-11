#sudo apt-get update && sudo apt-get install 

#sudo apt-get update && sudo apt-get install -y --force-yes liblo-dev 

g++ \
	-o ports \
	Pixi.cpp Ports.cpp MidiOutput.cpp \
	-l bcm2835 \
	-l lo \
	-I /usr/include/ \
	-std=c++11 \
	-std=c++0x \
	-pthread && \
	sudo ./ports
