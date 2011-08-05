bin_PROGRAMS = lsd

# Check to see if OLA is enabled
if YESOLA
OLAOBJ = OLAWrapper.cpp
OLALINK = -lola -lprotobuf
OLAFLAG = -DUSING_OLA
else !YESOLA
OLAOBJ = 
OLALINK = 
OLAFLAG = 
endif !YESOLA

lsd_SOURCES = cJSON.c GarbageCollector.c DBArr.c Array.c DBArrOps.c \
DBOps.c Node.c PluginAPI.c NodeInstAPI.c CoreRPC.c CorePlugin.c \
$(OLAOBJ) DMX.c PluginLoader.c Logging.c SceneCore.c

lsd_LDADD = $(OLALINK) @LTLIBINTL@

include $(top_srcdir)/PluginStaticLinks.m4

AM_CPPFLAGS = -DPLUGIN_DIR='"$(plugindir)"' -DWEB_PLUGIN_DIR='"$(webplugindir)"' -DWEB_DIR='"$(webdir)"' $(OLAFLAG) -DLOCALEDIR=\"$(localedir)\"
AM_LDFLAGS = -rdynamic