#!/bin/bash
# ports
# setup network 


MAC=`cat /sys/class/net/wlan0/address`
PORTS_HOSTNAME="ports-${MAC: -2}"
echo hostname is $PORTS_HOSTNAME
echo $PORTS_HOSTNAME | sudo tee /etc/hostname
#sudo sed -i 's/127.0.0.1/127.0.0.1 $PORTS_HOSTNAME/' /etc/hosts 
#echo "127.0.0.1 $PORTS_HOSTNAME" | sudo tee -a /etc/hosts

### install needed packages
echo Install needed packages
#sudo apt-get update > /dev/null && sudo apt-get install -y --force-yes dnsmasq git liblo-dev  > /dev//null
### prevent dhcp to start at boot
sudo update-rc.d -f dnsmasq remove > /dev/null


### update wifi config to enable fallback on ports ssid 
if sudo grep -q "mode=2" /etc/wpa_supplicant/wpa_supplicant.conf; then
	echo wpa already configured with accesspoint mode.
else
	echo configure network fall back onto ports ssid
	
	#cat >> /etc/wpa_supplicant/wpa_supplicant.conf <<\DELIM
	sudo tee -a /etc/wpa_supplicant/wpa_supplicant.conf >/dev/null <<DELIM
# put your own client networks above this access point mode
network={
    ssid="$PORTS_HOSTNAME"
    mode=2
    key_mgmt=WPA-PSK
    psk="portsports"
    frequency=2437
}
DELIM
fi

### dnsmasq configuration
echo configure dnsmasq
sudo tee /etc/dnsmasq.conf >/dev/null <<DELIM
domain-needed
interface=wlan0
dhcp-range=192.168.99.50,192.168.99.255,255.255.255.0,12h
no-hosts
addn-hosts=/etc/hosts.dnsmasq
DELIM
### dnsmasq hosts 
sudo tee /etc/hosts.dnsmasq >/dev/null <<DELIM
192.168.99.1	$PORTS_HOSTNAME
DELIM


### boot script
echo create boot script
sudo tee /etc/rc.local >/dev/null <<DELIM
#!/bin/bash
echo "performance" | sudo tee  /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
sleep 5 # todo wait for ip.
#inet=\`hostname -I\`
inet=\`/sbin/ifconfig wlan0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'\`
if [[ \$inet == 169*  ]];
then
    # noconnection. access point
    echo "Creating WIFI Access Point"
    ifconfig wlan0 down
    ifconfig wlan0 192.168.99.1 netmask 255.255.255.0 up
    sudo /etc/init.d/dnsmasq start
    echo "wifi network created ssid: $PORTS_HOSTNAME key: portsports"
else
    sudo /etc/init.d/dnsmasq stop
fi

/usr/sbin/ports > /var/log/ports.log 2>&1 &

exit 0
DELIM

### Pink
# echo install pink-0
#git clone https://github.com/hdavid/pink-0.git
#cp install-pink.sh pink-0
#cd pink-0
#git checkout ports
#../install-pink.sh no-ui
#cd ..


### BCM2835
echo install bcm2835 lib
rm -rf bcm2835-1.52*
wget --quiet http://www.airspayce.com/mikem/bcm2835/bcm2835-1.52.tar.gz > /dev/null
tar -xzf bcm2835-1.52.tar.gz > /dev/null
cd bcm2835-1.52
./configure > /dev/null && make > /dev/null && sudo make install > /dev/null
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
	sudo mv ports /usr/sbin



