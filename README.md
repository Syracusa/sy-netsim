# Installation(Ubuntu)
## Install pkg-config
+ sudo apt install pkg-config
## Install openssl-dev
+ sudo apt-get install libssl-dev
## Install libwebsocket
+ git clone https://github.com/warmcat/libwebsockets.git
+ cd libwebsockets
+ mkdir build && cd build
+ cmake ..
+ make
+ sudo make install