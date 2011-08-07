#!/bin/bash

# Gather plugin with Autoconf present for building
echo "AC_CONFIG_SUBDIRS([" > Plugins/pluginlist.m4

for plugin in Plugins/*/
do
test -e "$plugin/configure.ac"
if [ $? -eq 0 ]; then
echo "`echo $plugin` " >> Plugins/pluginlist.m4
fi
done

echo "])" >> Plugins/pluginlist.m4

# Now for Automake
echo -ne "SUBDIRS =" > Plugins/Makefile.am

for plugin in Plugins/*/
do
test -e "$plugin/Makefile.am"
if [ $? -eq 0 ]; then
echo -e " \\" >> Plugins/Makefile.am
echo -n `echo -n $plugin | sed 's/Plugins\/\(.*\)\//\1/g'` >> Plugins/Makefile.am
fi
done

echo " " >> Plugins/Makefile.am
echo "nobase_webplugin_DATA = CORE/CorePlugin.css CORE/CorePlugin.js CORE/Client.json" >> Plugins/Makefile.am


echo -n "lsd_LDFLAGS +=" > PluginStaticLinks.m4
for plugin in Plugins/*/
do
test -e "$plugin/Makefile.am"
if [ $? -eq 0 ]; then
echo -e " \\" >> PluginStaticLinks.m4
echo -n "-dlpreopen ../Plugins/`echo -n $plugin | sed 's/Plugins\/\(.*\)\//\1/g'`/src/`echo -n $plugin | sed 's/Plugins\/\(.*\)\//\1/g'`.la" >> PluginStaticLinks.m4
fi
done


# Autoreconf everything
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

aclocal
intltoolize --force
