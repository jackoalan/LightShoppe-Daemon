#!/bin/sh

#echo "Compiling PCMPipe Utility"
#gcc -o PCMPipe -g -O0 -Wall -lfftw3 PCMPipe.c

echo "Compiling Music Visualiser Plugin"
gcc -o Server.so -shared -O2 -Wall -lfftw3 -lasound PCMPipe.c Server.c
