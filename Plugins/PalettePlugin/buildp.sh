#!/bin/sh
echo "Compiling Palette Sampler Plugin"
gcc -o Server.so -Wall -shared -O2 db.c Gradient.c Server.c
