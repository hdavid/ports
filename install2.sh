#!/bin/bash

# Install needed packages
echo Install needed packages
sudo apt-get update > /dev/null && sudo apt-get install -y --force-yes liblo-dev  > /dev//null


# Broadcom driver
echo install bcm2835 lib
rm -rf bcm2835-1.52*
wget --quiet http://www.airspayce.com/mikem/bcm2835/bcm2835-1.52.tar.gz > /dev/null
tar -xzf bcm2835-1.52.tar.gz > /dev/null
cd bcm2835-1.52
./configure > /dev/null && make > /dev/null && make install > /dev/null
cd ..
rm -rf bcm2835-1.52*

# Compile
echo compile ports
g++ \
	-o ports \
	Pixi.cpp Ports.cpp MidiOutput.cpp \
	-l bcm2835 \
	-l lo \
	-I /usr/include/ \
	-std=c++11 \
	-std=c++0x \
	-pthread && \
	sudo mv ports /usr/sbin

# Install
echo Install ports
sudo cp support/init.d/ports /etc/init.d/ports
echo Enable port at boot
sudo update-rc.d ports defaults
 
 
# Pink
echo Install pink
#git clone https://github.com/hdavid/pink-0.git
#cd pink-0
#git checkout ports
#./install.sh
#cd ..
