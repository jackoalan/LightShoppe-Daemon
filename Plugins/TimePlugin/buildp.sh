#!/bin/sh

echo "Compiling Time Plugin"
gcc -o Server.so -shared -O2 -Wall Server.c
