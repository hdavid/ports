#!/bin/bash
# ports
# setup network 

### install needed packages
echo Install needed packages
sudo apt-get update > /dev/null && sudo apt-get install -y --force-yes dnsmasq git liblo-dev  > /dev//null
### prevent dhcp to start at boot
update-rc.d -f dnsmasq remove /dev/null


### update wifi config to enable fallback on ports ssid 
if grep -q "mode=2" /etc/wpa_supplicant/wpa_supplicant.conf; then
	echo wpa already configured with accesspoint mode.
else
	echo configure network fall back onto ports ssid
	cat >> /etc/wpa_supplicant/wpa_supplicant.conf <<\DELIM
# put your own client networks above this access point mode
network={
    ssid="ports"
    mode=2
    key_mgmt=WPA-PSK
    psk="portsports"
    frequency=2437
}
DELIM
fi

### dnsmasq configuration
echo configure dnsmasq
cat > /etc/dnsmasq.conf <<\DELIM
domain-needed
interface=wlan0
dhcp-range=192.168.1.50,192.168.1.255,255.255.255.0,12h
no-hosts
addn-hosts=/etc/hosts.dnsmasq
DELIM
### dnsmasq hosts 
sudo cat > /etc/hosts.dnsmasq <<\DELIM
192.168.1.1	ports
DELIM



### boot script
echo create boot script
cat > /etc/rc.local <<\DELIM
#!/bin/bash
echo "performance" | tee  /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor > /home/pi/boot.log 2>&1
inet=`hostname -I`
if [[ $inet == 169*  ]];
then
    # noconnection. access point
    echo "Creating WIFI Access Point"
    ifconfig wlan0 down
    ifconfig wlan0 192.168.1.1 netmask 255.255.255.0 up
    sudo /etc/init.d/dnsmasq start
    echo "wifi network created ssid: ports key: portsports"
fi

/usr/local/bin/ports > /home/pi/ports.log 2>&1 &

exit 0
DELIM

echo install bcm2835 lib
rm -rf bcm2835-1.52*
wget --quiet http://www.airspayce.com/mikem/bcm2835/bcm2835-1.52.tar.gz > /dev/null
tar -xzf bcm2835-1.52.tar.gz > /dev/null
cd bcm2835-1.52
./configure > /dev/null && make > /dev/null && make install > /dev/null
cd ..
rm -rf bcm2835-1.52*

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
	mv ports /usr/local/bin



