# syntax=docker/dockerfile:1

FROM gcc
WORKDIR /app
RUN apt-get update
RUN apt-get install -y cmake

## Deploy only
# COPY . .
# RUN rm -rf build
# RUN mkdir -p build
# WORKDIR /app/build
# RUN cmake ..
# RUN make
# CMD ./bin/simulator

## Development only
CMD /bin/bash 

EXPOSE 12123
