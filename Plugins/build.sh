#!/bin/bash

for file in ./*/
do
  cd $file
  ./build.sh
  cd ..
done
