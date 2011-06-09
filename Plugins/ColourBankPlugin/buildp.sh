#!/bin/sh

echo "Compiling Fader Plugin"
gcc -o Server.so -shared -O2 -Wall Server.c
