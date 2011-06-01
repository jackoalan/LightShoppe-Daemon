#!/bin/bash

for file in ./*/*.so
do
  rm $file
done
