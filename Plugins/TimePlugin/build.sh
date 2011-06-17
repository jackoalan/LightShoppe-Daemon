#!/bin/sh

echo "Compiling Time Plugin"
gcc -o Server.so -shared -g -O0 -Wall Server.c
