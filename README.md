# Build
+ mkdir build && cd build
+ cmake ..
+ make

# Docker deploy
## Deploy build
+ docker build -t sy-netsim . -f Dockerfile.deploy
## Deploy run
+ docker run --rm -p 127.0.0.1:12123:12123 -dit sy-netsim

# Docker development
## Development build
+ docker build -t sy-netsim-dev . -f Dockerfile.development
## Development run
+ docker run -p 127.0.0.1:12123:12123 --mount type=bind,src="$(pwd)",target=/src -dit sy-netsim-dev
+ docker attach <ID>