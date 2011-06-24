#!/bin/sh

echo "Compiling Animation Plugin"
gcc -o Server.so -Wall -O2 -shared attackDecay.c db.c Server.c
