#!/bin/sh
echo "Compiling cJSON"
gcc -o cJSON.o -c -O2 -Wall cJSON.c

echo "Compiling Garbage Collector"
gcc -o GarbageCollector.o -c -O2 -Wall GarbageCollector.c

echo "Compiling Array Instances"
gcc -o DBArr.o -c -O2 -Wall DBArr.c

echo "Compiling Array"
gcc -o Array.o -c -O2 -Wall Array.c

echo "Compiling LSD DB Array Operatives"
gcc -o DBArrOps.o -c -O2 -Wall DBArrOps.c

echo "Compiling DB Operatives"
gcc -o DBOps.o -c -O2 -Wall DBOps.c

echo "Compiling Node"
gcc -o Node.o -c -O2 -Wall Node.c

echo "Compiling PluginAPI"
gcc -o PluginAPI.o -c -O2 -Wall PluginAPI.c

echo "Compiling NodeInstAPI"
gcc -o NodeInstAPI.o -c -O2 -Wall NodeInstAPI.c

echo "Compiling Core RPC"
gcc -o CoreRPC.o -c -O2 -Wall CoreRPC.c

echo "Compiling Core Plugin"
gcc -o CorePlugin.o -c -O2 -Wall CorePlugin.c

echo "Compiling OLA Wrapper"
g++ -o OLAWrapper.o -c -O2 -Wall OLAWrapper.cpp

echo "Compiling DMX"
gcc -o DMX.o -c -O2 -Wall DMX.c

echo "Compiling Plugin Loader"
gcc -o PluginLoader.o -c -O2 -Wall PluginLoader.c

echo "Compiling SceneCore target"
gcc -o lsd -O2 -Wall -rdynamic -lm -ldl -lmagic -lsqlite3 -levent -lola /usr/lib/libprotobuf.so cJSON.o GarbageCollector.o DBArr.o Array.o DBArrOps.o DBOps.o Node.o PluginAPI.o NodeInstAPI.o CoreRPC.o CorePlugin.o OLAWrapper.o DMX.o PluginLoader.o SceneCore.c

echo "Compiling Plugins"
cd Plugins
./buildp.sh
cd ..

