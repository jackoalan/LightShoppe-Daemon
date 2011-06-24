#!/bin/sh

echo "Compiling Animation Plugin"
gcc -o Server.so -Wall -g -O0 -shared attackDecay.c db.c Server.c
