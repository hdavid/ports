# Ports
Ports is an OSC to CV/Gate and MIDI converter.
Built to run on raspberry pi zero w (or other raspberry pi variants). 
It listens for incoming OSC messages on port 5000.

It will convert them into CV/Trigger and gate on the MAX11300 chip or MIDI messages (see API section of this readme)


## Installation

During this procedure we assume that you have access to your pi, using a screen + keyboard, or over SSH.

### get your raspberry pi setup:
- download the raspbian lite image. you only need the lite version (no need for X windows and similar things)  https://www.raspberrypi.org/downloads/raspbian
- Install raspbian on your pi. https://www.raspberrypi.org/documentation/installation/installing-images/
- Enable sshd by putting an empty file called ssh on the boot partition. `touch /Volumes/boot/ssh` on mac.
- get your pi connected on the internet
  - use a wired ethernet connection. just plug the pi into your router.
  - or setup wifi as described https://www.raspberrypi.org/documentation/configuration/wireless/wireless-cli.md

Ports could work on other linux flavours such as osmc etc, but the automatic WLAN fallback might not work as expected as WLAN is distro specific.

`/install.sh` (below) assumes the system is a raspbian jessie.


### install ports

Clone ports repository `git clone https://github.com/hdavid/ports.git`

- if git is not installed, just run `sudo apt-get update && sudo apt-get install git`

Then from the ports directory run `./install.sh`

This will:
- download and install the SPI broadcom drivers
- install liblo osc lib package
- install dnsmsaq package (dns, dhcp for accesspoint mode) 
- compile ports
- configure fall back access point ssid: `ports`, psk: `portsports`
- configure ports to start at boot time automatically

you can also install pink, to provide ableton link. run `./install-pink.sh` 

## Accessing

If no known wifi network is in range, ports will create an access point : 
- ssid: ports
- psk: portsports

You can add your own wifi networks in `/etc/wpa_supplicant/wpa_supplicant.conf`. 
Make sure to enter your network _before_ the one added by ports (identified by ssid="ports" and mode=2), as wpa_supplicant try the networks in the order they appear in this configuration file.

Example of network configuration :
```
network={
	str_id="myNetworkFriendlyName"
    ssid="myNetwork"
    psk="MyPassword"
}
```


## Running
Port must run as root on order to access SPI bus used to communicate with the MAX11300.
Note that ports is started automatically at boot time. (done by ./install.sh)

you can however start manually from 
```
sudo /usr/local/bin/portsd no-daemon
```


## OSC API

OSC Port: `5000`


### CV Outputs

`/out/<[1-20]>/<mode> <float>`

mode is one of : trig, gate, flipflop, cvuni, cvbi, lfosine, lfosaw, lfotri, lfosquare

unipolar is [0v 10v]. bipolar is [-5v +5v]. 
LFO rates are in Hertz.
other input values are in the range [0 1] and mapped to the cv voltage.

### MIDI Output

for now the MIDI device is hardcoded in the code to be the first usb device plugged. `/dev/snd/midiC1D0` is easily changeable in Ports.cpp.

`/midi/<channel>/cc/<midiCC> <value>`

`/midi/<channel>/noteOn/<noteNumber> <velocity>`

`/midi/<channel>/noteOff/<noteNumber> <velocity>`

channel: 1-16

noteNumber, midiCC, velocity, value : 1-127


## Possible Improvements

- mdns http://avahi.org/doxygen/html/client-publish-service_8c-example.html
- web server : https://github.com/cesanta/mongoose
- Ableton link : https://github.com/shaduzlabs/pink-0 http://shaduzlabs.com/blog/23/pink0-an-ableton-link-to-clockreset-hardware-converter.html


