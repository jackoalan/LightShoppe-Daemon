#!/bin/sh
echo "Compiling cJSON"
gcc -o cJSON.o -c -g -O0 -Wall cJSON.c

echo "Compiling Garbage Collector"
gcc -o GarbageCollector.o -c -g -O0 -Wall GarbageCollector.c

echo "Compiling Array Instances"
gcc -o DBArr.o -c -g -O0 -Wall DBArr.c

echo "Compiling Array"
gcc -o Array.o -c -g -O0 -Wall Array.c

echo "Compiling LSD DB Array Operatives"
gcc -o DBArrOps.o -c -g -O0 -Wall DBArrOps.c

echo "Compiling DB Operatives"
gcc -o DBOps.o -c -g -O0 -Wall DBOps.c

echo "Compiling Node"
gcc -o Node.o -c -g -O0 -Wall Node.c

echo "Compiling PluginAPI"
gcc -o PluginAPI.o -c -g -O0 -Wall PluginAPI.c

echo "Compiling NodeInstAPI"
gcc -o NodeInstAPI.o -c -g -O0 -Wall NodeInstAPI.c

echo "Compiling Core RPC"
gcc -o CoreRPC.o -c -g -O0 -Wall CoreRPC.c

echo "Compiling Core Plugin"
gcc -o CorePlugin.o -c -g -O0 -Wall CorePlugin.c

echo "Compiling OLA Wrapper"
g++ -o OLAWrapper.o -c -g -O0 -Wall OLAWrapper.cpp

echo "Compiling DMX"
gcc -o DMX.o -c -g -O0 -Wall DMX.c

echo "Compiling Fader Plugin"
gcc -o Plugins/faderPlugin.o -c -g -O0 -Wall Plugins/faderPlugin.c

echo "Compiling SceneCore target"
gcc -o lsd -g -O0 -Wall -lm -lsqlite3 -levent -lola /usr/lib/libprotobuf.so cJSON.o GarbageCollector.o DBArr.o Array.o DBArrOps.o DBOps.o Node.o PluginAPI.o NodeInstAPI.o CoreRPC.o CorePlugin.o OLAWrapper.o DMX.o Plugins/faderPlugin.o SceneCore.c
