#!/bin/sh

#echo "Compiling PCMPipe Utility"
#gcc -o PCMPipe -g -O0 -Wall -lfftw3 PCMPipe.c

echo "Compiling Music Visualiser Plugin"
gcc -o Server.so -shared -g -O0 -Wall -lfftw3 PCMPipe.c Server.c
