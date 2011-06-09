#!/bin/sh

echo "Compiling Fader Plugin"
gcc -o Server.so -shared -g -O0 -Wall Server.c
