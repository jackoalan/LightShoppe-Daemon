#!/bin/sh

echo "Compiling Colour Bank Plugin"
gcc -o Server.so -shared -g -O0 -Wall Server.c
