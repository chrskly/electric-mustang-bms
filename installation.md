# Installation

git clone <this_repo>
cd <this_repo>/src
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
cd ../
export PICO_SDK_PATH=${PWD}/pico-sdk
cd build
cmake ..
make
