#!/bin/sh

echo "Compiling Colour Bank Plugin"
gcc -o Server.so -shared -O2 -Wall Server.c
