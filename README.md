# Ports
Ports is an OSC to CV/Gate and MIDI converter.
Built to run on raspberry pi zero w (or other variants). 
It listen for incoming OSC messages on port 5000.

It will convert them into CV/Trigger and gate on the MAX11300 chip or MIDI messages.


## Install

get your raspberry pi booted up and connected to internet, in order to download the needed packages. Then on your raspberry pi run `git clone https://github.com/hdavid/ports`

Then from the ports directory `run sudo bash ./install.sh`

This will:
- install the SPI broadcom drivers
- install liblo osc lib
- install needed packages
- compile ports
- configure fall back access point ssid: `ports`, psk: `portsports`
- configure ports to start at boot time automatically

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


## Runing
Port must run as root on order to access SPI bus used to communicate with the MAX11300.
Note that ports is started automatically at boot time. (done by ./install.sh)

you can however start manually from 
```
sudo /usr/local/bin/ports
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

for now the MIDI device is hardcoded in the code to be the first usb device plugged.

`/midi/<channel>/<cc>/<midiCC> <value>`

`/midi/<channel]>/noteOn/<noteNumber> <velocity>`

`/midi/<channel>/noteOff/<noteNumber> <velocity>`

channel: 1-16

note, cc, velocity, value : 1-127


## Possible Improvements

- mdns http://avahi.org/doxygen/html/client-publish-service_8c-example.html
- web server : https://github.com/cesanta/mongoose
- Ableton link : https://github.com/shaduzlabs/pink-0 http://shaduzlabs.com/blog/23/pink0-an-ableton-link-to-clockreset-hardware-converter.html


