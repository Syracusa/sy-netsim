# Build
+ mkdir build && cd build
+ cmake ..
+ make

# Docker
+ docker build -t sy-netsim .
+ docker run -p 127.0.0.1:12123:12123 -dit sy-netsim