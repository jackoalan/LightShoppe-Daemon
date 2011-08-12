if BUILD_RVL
SUBDIRS = wii
boot.dol : lsd
	elf2dol lsd $@
wiiapp_PROGRAMS = boot.dol
else
bin_PROGRAMS = lsd
endif

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

# Add WiiMain if building for Wii
if BUILD_RVL
WIIOBJ = WiiMain.c
else
WIIOBJ = 
endif

lsd_SOURCES = cJSON.c GarbageCollector.c DBArr.c Array.c DBArrOps.c \
DBOps.c Node.c PluginAPI.c NodeInstAPI.c CoreRPC.c CorePlugin.c \
$(OLAOBJ) DMX.c PluginLoader.c Logging.c SceneCore.c $(WIIOBJ)

lsd_LDFLAGS = 
if BUILD_RVL
lsd_LDFLAGS += -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float \
-lltdlc -lsqlite3 -levent -lwiiuse -lfat -lbte -logc
else
lsd_LDFLAGS += $(OLALINK) @LTLIBINTL@ -lsqlite3
endif

include $(top_srcdir)/PluginStaticLinks.m4

if BUILD_RVL
AM_CPPFLAGS = -DWEB_PLUGIN_DIR='"sd:/lsd/webplugins"' -DWEB_DIR='"sd:/lsd/www"'
AM_LDFLAGS = 
else
AM_CPPFLAGS = -DPLUGIN_DIR='"$(plugindir)"' -DWEB_PLUGIN_DIR='"$(webplugindir)"' -DWEB_DIR='"$(webdir)"' $(OLAFLAG) -DLOCALEDIR=\"$(localedir)\"
AM_LDFLAGS = -rdynamic
endif