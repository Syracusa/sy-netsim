# syntax=docker/dockerfile:1

FROM gcc
WORKDIR /app
RUN apt-get update
RUN apt-get install -y cmake
COPY . .
RUN rm -rf build
RUN mkdir -p build
WORKDIR /app/build
RUN cmake ..
RUN make

CMD ./bin/simulator
EXPOSE 12123
