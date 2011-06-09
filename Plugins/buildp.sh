#!/bin/bash

for file in ./*/
do
  cd $file
  ./buildp.sh
  cd ..
done
