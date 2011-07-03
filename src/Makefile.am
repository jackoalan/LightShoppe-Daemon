bin_PROGRAMS = lsd

lsd_SOURCES = cJSON.c GarbageCollector.c DBArr.c Array.c DBArrOps.c \
DBOps.c Node.c PluginAPI.c NodeInstAPI.c CoreRPC.c CorePlugin.c \
OLAWrapper.cpp DMX.c PluginLoader.c SceneCore.c

lsd_LDADD = -lola -lprotobuf

AM_CPPFLAGS = -DPLUGIN_DIR='"$(plugindir)"' -DWEB_PLUGIN_DIR='"$(webplugindir)"'
