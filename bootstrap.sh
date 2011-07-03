#!/bin/bash

libtoolize -v
for plugin in Plugins/*/
do
pushd $plugin
libtoolize -v
popd
done

autoreconf -vif
