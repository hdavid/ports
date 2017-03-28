
rm -rf pa_stable_v190600_20161030.tgz portaudio
wget http://www.portaudio.com/archives/pa_stable_v190600_20161030.tgz
tar -xzf pa_stable_v190600_20161030.tgz
cd portaudio
./configure --with-alsa --without-oss --without-jack
make
sudo make install

rm -rf pink-0
git clone https://github.com/shaduzlabs/pink-0.git
git checkout develop
cd pink-0


echo -e "[ \033[1m\033[96mpink\033[m ] Update submodules -----------------------------------------------------"
git submodule update --init --recursive

echo -e "[ \033[1m\033[96mpink\033[m ] Install dependencies --------------------------------------------------"
sudo apt-get update && sudo apt-get install -y --force-yes cmake wiringpi

#echo -e "[ \033[1m\033[96mpink\033[m ] Build and install portaudio -------------------------------------------"
#cd modules/portaudio
#./configure --with-alsa --without-oss --without-jack
#make
#sudo make install
#cd ../../
#echo a

echo ""
echo ""
echo ""
echo -e "[ \033[1m\033[96mpink\033[m ] Build and install nanomsg ---------------------------------------------"
cd modules/nanomsg
mkdir build
cd build
cmake ..
make
sudo make install
sudo ldconfig
cd ../../../



echo ""
echo ""
echo ""
echo -e "[ \033[1m\033[96mpink\033[m ] Build pink and install the binary and the daemon configuration file ---"
rm -rf build
mkdir build
cd build
#if [ $1 == "no-ui" ]
#  then
    cmake -DCMAKE_BUILD_TYPE=Release -DUSE_PI_ZERO=OFF -DUSE_WEBSOCKET=ON -DJUST_INSTALL_CEREAL=ON ..
#  else
#    echo -e "[ \033[1m\033[96mpink\033[m ] Raspberry Pi Zero shield support is enabled ---------------------------"
#    cmake -DCMAKE_BUILD_TYPE=Release -DUSE_PI_ZERO=ON -DUSE_WEBSOCKET=ON -DJUST_INSTALL_CEREAL=ON ..
#fi
make
#if [ $platform == "linux-rpi" ];
#  then
    sudo make install
    sudo update-rc.d pink defaults
#fi
cd ..

#echo ""
#echo ""
#echo ""
#echo -e "[ \033[1m\033[96mpink\033[m ] Update device configuration -------------------------------------------"
#if [ $platform == "linux-rpi" ];
#  then
#    sudo cp --backup=numbered support/hostapd.conf /etc/hostapd/.
#    sudo cp -r support/html/* /var/www/html/.
#fi

cd ..


