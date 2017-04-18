cd pink-0

rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_PI_ZERO=ON -DUSE_WEBSOCKET=ON -DJUST_INSTALL_CEREAL=ON ..
make

sudo make install
sudo update-rc.d pink defaults
cd ../..



