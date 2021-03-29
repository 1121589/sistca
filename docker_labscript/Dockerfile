FROM alpine

# Install GCC
RUN apk add build-base

COPY src cpuinfo/src
WORKDIR cpuinfo

RUN gcc -o ./which-cpu ./src/cpuinfo.c
RUN rm -R ./src