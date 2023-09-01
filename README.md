# Build
+ mkdir build && cd build
+ cmake ..
+ make

# Docker build
+ docker build -t sy-netsim .

## Deploy run
+ docker run -p 127.0.0.1:12123:12123 -dit sy-netsim

## Development run
+ docker run -p 127.0.0.1:12123:12123 --mount type=bind,src="$(pwd)",target=/app -dit sy-netsim
+ docker attach <ID>