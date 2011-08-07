/*
 **    This file is part of LightShoppe. Copyright 2011 Jack Andersen
 **
 **    LightShoppe is free software: you can redistribute it
 **    and/or modify it under the terms of the GNU General
 **    Public License as published by the Free Software
 **    Foundation, either version 3 of the License, or (at your
 **    option) any later version.
 **
 **    LightShoppe is distributed in the hope that it will
 **    be useful, but WITHOUT ANY WARRANTY; without even the
 **    implied warranty of MERCHANTABILITY or FITNESS FOR A
 **    PARTICULAR PURPOSE.  See the GNU General Public License
 **    for more details.
 **
 **    You should have received a copy of the GNU General
 **    Public License along with LightShoppe.  If not, see
 **    <http://www.gnu.org/licenses/>.
 **
 **    @author Jack Andersen <jackoalan@gmail.com>
 */

#include "SceneCore.h"
#include "PluginAPI.h"
#include "PluginAPICore.h"
#include "DBArrOps.h"
#include "CoreRPC.h"
#include "CorePlugin.h"
#include "GarbageCollector.h"
#include "DMX.h"
#include "PluginLoader.h"
#include "Node.h"
#include "Logging.h"
#include "DBOps.h"
#include "cJSON.h"

#include <stdio.h>
#include <ltdl.h>
#include <event.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef HW_RVL
#include <pwd.h>
#else
#include <gctypes.h>
#include <wiiuse/wpad.h>
#endif

#include "../config.h"

/* Gettext stuff */
#ifndef HW_RVL
#include <libintl.h>
#define _(String) gettext (String)
#else
#define _(String) String
#endif


/* Name of this component for logging */
static const char LOG_COMP[] = "SceneCore.c";


void
destruct_Univ (void* univ)
{
    if (univ)
    {
        struct LSD_Univ* castUniv = univ;
        if (castUniv->buffer)
        {
            free (castUniv->buffer);
            castUniv->buffer = NULL;
        }
    }
}


/* Flag governing whether a loop_break should loop into
 * itself */
static int reload = 0;

/* Interval in microseconds representing ideal refresh rate */
static const int UPDATE_INT = 20000; /* 50 FPS */

/* Event base for LSD's main thread */
static struct event_base* ebMain;

/* Event which is constantly rescheduled in order */
/* to ensure buffering occurs at a consistent interval */
static struct event* updEv;

/* Absolute timeval indicating when last update began */
static struct timeval lastUpdLi;

/* Generates the default path to save the database */
char const * 
getHomeDBPath ()
{
#ifndef HW_RVL
    struct passwd *pw = getpwuid(getuid());
    if(!pw)
        return NULL;
    
    char* path = malloc (256*sizeof(char));
    if (path)
    {
        snprintf (path, 256, "%s/.lsd.db", pw->pw_dir);
        return path;
    }
    else
        return NULL;
#else
    return "sd:/lsd.db";
#endif
}

/* Function which handles the calculation of remaining time */
/* to next processing interval and updates an event to
 * trigger */
/* in that remaining time. */
/* If the update is behind schedule, it will be executed
 * immediately instead */
void
rescheduleUpdate (struct event* ev, struct timeval* lastUpd, void ( *handler )(
                      evutil_socket_t,
                      short int,
                      void*), int interval)
{
    struct timeval remTime;
    struct timeval curTime;

    gettimeofday (&curTime, NULL);
    remTime.tv_sec = 0;
    remTime.tv_usec = lastUpd->tv_usec - curTime.tv_usec + interval;

    if (remTime.tv_usec < 0)  /* If we're behind schedule */
    {
        doLog (WARNING, LOG_COMP, _("Lighting update behind schedule."));
        handler (0, 0, NULL);
    }
    else
        evtimer_add (ev, &remTime);
}

void
handleInterrupt (evutil_socket_t one, short int two, void* three)
{
    reload = 0;
    event_base_loopbreak (ebMain);
}


/* Function which ensures each partition's output buffer is
 * available */
void
updateBuffers (evutil_socket_t one, short int two, void* three)
{
#ifdef HW_RVL
    /* We're on Wii, check to see if Home button is pressed */
    WPAD_ScanPads();
    u32 pressed = WPAD_ButtonsDown(0);
    if ( pressed & WPAD_BUTTON_HOME ) 
        handleInterrupt (0,0,NULL);
#endif
    
    evtimer_del (updEv);
    gettimeofday (&lastUpdLi, NULL);

    /* Do per-frame shite here */
    node_incFrameCount ();
    bufferUnivs ();
    writeUnivs ();

    /* Update timer for next interval occurance relative to
     * buffer start time */
    rescheduleUpdate (updEv, &lastUpdLi, updateBuffers, UPDATE_INT);
}



void
handleReload (evutil_socket_t ont, short int two, void* three)
{
    reload = 1;
    event_base_loopbreak (ebMain);
}

void
handleLogReopen (evutil_socket_t ont, short int two, void* three)
{
    reloadLogging ();
}


int
lsdSceneEntry (const char* dbpath, int rpcPort, const char* pathPrefix)
{
    char const * HOME_DB = getHomeDBPath ();
    
    if (!dbpath)
    {
        doLog (NOTICE, LOG_COMP, _("Checking for DB in home."));
        if (lsddb_openDB (HOME_DB) < 0){
            doLog (NOTICE, LOG_COMP, _("Establishing empty DB."));
            if (lsddb_emptyDB () < 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to initialise new DB."));
                return -1;
            }
        }
    }
    else
    {
        doLog (NOTICE, LOG_COMP, _("Opening DB from file."));
        if (lsddb_openDB (dbpath) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to open specified DB from file."));
            return -1;
        }
    }

    lsdapi_setState (STATE_PINIT);

    /** INIT EVENT BASE **/
    ebMain = event_base_new ();

#ifndef HW_RVL
    /** REGISTER INTERRUPT HANDLER **/
    doLog (NOTICE, LOG_COMP, _("Registering signal handlers."));
    struct event* intEv = evsignal_new (ebMain, SIGINT, handleInterrupt, NULL);
    evsignal_add (intEv, NULL);

    /** REGISTER RELOAD HANDLER **/
    struct event* relEv = evsignal_new (ebMain, SIGUSR1, handleReload, NULL);
    evsignal_add (relEv, NULL);
    
    /** REGISTER LOG REOPEN HANDLER **/
    struct event* logEv = evsignal_new (ebMain, SIGUSR2, handleLogReopen, NULL);
    evsignal_add (logEv, NULL);
#endif

    /** OPEN HTTP RPC **/
    doLog (NOTICE, LOG_COMP, _("Opening HTTP RPC."));
    if (openRPC (ebMain, rpcPort, pathPrefix) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to open RPC on port %d."), rpcPort);
        return -1;
    }

    /** OPEN OLA **/
    doLog (NOTICE, LOG_COMP, _("Starting OLA connection."));
    if (initDMX () < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to open OLA connection."));
        return -1;
    }

    /** REGISTER PERIODIC LIGHTING UPDATE **/
    doLog (NOTICE, LOG_COMP, _("Registering periodic lighting update."));
    updEv = evtimer_new (ebMain, updateBuffers, NULL);

    /** Reloader Loop **/

    reload = 1;
    while (reload)
    {

        /** INIT GARBAGE COLLECTOR **/
        doLog (NOTICE, LOG_COMP, _("Initialising Garbage Collector."));
        lsdgc_prepGCOps ();

        /** RESET DATABASE FOR FRESH STATE **/
        doLog (NOTICE, LOG_COMP, _("Resetting DB."));
        lsddb_resetDB ();

        /** INIT TIME **/
        lsdapi_setState (STATE_PINIT);

        /** ALLOCATE STATE ARRAYS **/

        doLog (NOTICE, LOG_COMP, _("Establishing System ArrayHeads."));
        if (initLsdArrays () < 0)
        {
            doLog (ERROR, LOG_COMP, _("Error while establishing ArrayHeads."));
            return -1;
        }

        /** LOAD PLUGINS HERE **/

        /* Core Plugin */
        doLog (NOTICE, LOG_COMP, _("Loading core plugin head."));
        if (lsddb_pluginHeadLoader (getCoreHead, 1, "CORE", "0", NULL) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to properly load core plugin."));
            return -1;
        }
        
        /* Statically linked plugins */
        doLog (NOTICE, LOG_COMP, _("Loading static plugins."));
        if (loadPlugins_static () < 0)
            doLog (ERROR, LOG_COMP, _("Unable to get preloaded (static) plugins."));

#ifndef HW_RVL
        /* Dynamically shared plugins */
        doLog (NOTICE, LOG_COMP, _("Loading shared plugins."));
        if (lt_dlinit ())
            doLog (ERROR, LOG_COMP, _("Unable to initialise ltdl: %s."), lt_dlerror());
        else
            loadPluginsDirectory ();
#endif


        /** STRUCT PARTITION ARRAY **/

        doLog (NOTICE, LOG_COMP, _("Structing partition array."));
        if (lsddb_structPartitionArr () < 0)
            doLog (ERROR, LOG_COMP, _("Unable to structPartitionArr()."));

        /** STRUCT UNIV ARRAY **/
        doLog (NOTICE, LOG_COMP, _("Structing Universe array."));
        if (lsddb_structUnivArr () < 0)
        {
            doLog (ERROR, LOG_COMP, _("Problem structing universe array."));
            return -1;
        }

        /** STRUCT CHANNEL ARRAY **/
        doLog (NOTICE, LOG_COMP, _("Structing Channel array."));
        if (lsddb_structChannelArr () < 0)
        {
            doLog (ERROR, LOG_COMP, _("Problem structing channel array."));
            return -1;
        }

        /** Curtain Up **/
        lsdapi_setState (STATE_PRUN);

        /** BEGIN PARTITION BUFFER LOOP **/
        node_resetFrameCount ();
        updateBuffers (0, 0, NULL);
        doLog (NOTICE, LOG_COMP, _("Dispatching(Ctrl-c to quit)..."));
        event_base_dispatch (ebMain);

        /** CLEAN UP SHITE **/
        lsdapi_setState (STATE_PCLEAN);

        doLog (NOTICE, LOG_COMP, _("Cleaning up Arrays."));
        if (clearLsdArrays () < 0)
            doLog (WARNING, LOG_COMP, 
                   _("There was a problem cleaning up arrays. Continuing anyway."));
        
#ifndef HW_RVL
        /** DONE WITH LTDL **/
        lt_dlexit ();
#endif

        /** CLOSE GARBAGE COLLECTOR **/
        doLog (NOTICE, LOG_COMP, _("Closing Garbage Collector."));
        lsdgc_finalGCOps ();

    }

    /** Close OLA **/
    doLog (NOTICE, LOG_COMP, _("Closing OLA connection."));
    closeDMX ();

    /** Close RPC **/
    doLog (NOTICE, LOG_COMP, _("Closing HTTP RPC."));
    closeRPC ();

    /* Update Cleanup */
    doLog (NOTICE, LOG_COMP, _("Cleaning Lighting Update."));
    evtimer_del (updEv);
    event_free (updEv);

    /* Signal cleanup */
#ifndef HW_RVL
    doLog (NOTICE, LOG_COMP, _("Cleaning signal handlers."));
    evsignal_del (intEv);
    event_free (intEv);
    evsignal_del (relEv);
    event_free (relEv);
    evsignal_del (logEv);
    event_free (logEv);
#endif

    /* Done with libevent */
    event_base_free (ebMain);

    /* Save */
    doLog (NOTICE, LOG_COMP, _("Saving DB to file."));
    if (dbpath)
        lsddb_saveDB (dbpath);
    else /* Save In Home Directory */
        lsddb_saveDB (HOME_DB);

    /* Finialise DB */
    doLog (NOTICE, LOG_COMP, _("Cleaning up DB."));
    lsddb_closeDB ();

#ifndef HW_RVL
    free((void*)HOME_DB);
#endif
    
    return 0;
}

#ifndef HW_RVL
int
main (int argc, const char** argv)
{

    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
    
    /* Parse Command Line */
    int i;
    int verbose = 0;
    int rpcPort = 9196;
    const char* dbpath = NULL;
    const char* pathPrefix = "/lightshoppe";
    if (argc > 0)
        for (i = 0; i < argc; ++i)
        {
            if (strncmp (argv[i], "-h", 2) == 0)
            {
                printf (_("Usage: lsd [-hv] [-p port] [-P \"Path Prefix\"] [-d dbfile]\n"));
                return 0;
            }
            else if (strncmp (argv[i], "-v", 2) == 0)
                verbose = 1;
            else if (strncmp (argv[i], "-d", 2) == 0)
            {
                if (strlen(argv[i]) > 2)
                    dbpath = argv[i]+2;
                else if (i+1 < argc)
                    dbpath = argv[i+1];
                else
                {
                    printf (_("Missing path value for -d.\n"));
                    return -1;
                }
            }
            else if (strncmp (argv[i], "-p", 2) == 0)
            {
                const char* portStr;
                if (strlen(argv[i]) > 2)
                    portStr = argv[i]+2;
                else if (i+1 < argc)
                    portStr = argv[i+1];
                else
                {
                    printf (_("Missing port value for -p.\n"));
                    return -1;
                }
                
                int portNum = atoi (portStr);
                if (!portNum)
                {
                    printf (_("Unable to parse port value.\n"));
                    return -1;
                }
                
                rpcPort = portNum;
            }
            else if (strncmp (argv[i], "-P", 2) == 0)
            {
                if (strlen(argv[i]) > 2)
                    pathPrefix = argv[i]+2;
                else if (i+1 < argc)
                    pathPrefix = argv[i+1];
                else
                {
                    printf (_("Missing string for path prefix.\n"));
                    return -1;
                }
                
                if(pathPrefix[0] != '/')
                {
                    printf (_("Path must begin with a leading slash.\n"));
                    return -1;
                }
            }

        }

    /* Begin Logging */
    initLogging (verbose);
    
    /* Start LSD! */
    int exitCode = lsdSceneEntry (dbpath, rpcPort, pathPrefix);
    
    /* End Logging */
    finishLogging ();
    
    return exitCode;
}
#endif
