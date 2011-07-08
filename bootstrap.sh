#!/bin/bash

type libtoolize
if [ $? -gt 0 ]; then
echo "Using glibtoolize"
TOOLCMD=glibtoolize
else
echo "Using libtoolize"
TOOLCMD=libtoolize
fi

$TOOLCMD -v
for plugin in Plugins/*/
do
pushd $plugin
$TOOLCMD -v
popd
done

autoreconf -vif

