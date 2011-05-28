/*
**    This file is part of LightShoppe.
**    Copyright 2011 Jack Andersen
**
**    LightShoppe is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    LightShoppe is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with LightShoppe.  If not, see <http://www.gnu.org/licenses/>.
**
**    @author Jack Andersen <jackoalan@gmail.com>
*/



#include "SceneCore.h"
#include <stdio.h>
#include <dlfcn.h>
#include "PluginAPI.h"
#include "PluginAPICore.h"
#include "DBArrOps.h"
#include "CoreRPC.h"
#include "CorePlugin.h"
#include "GarbageCollector.h"
#include "DMX.h"

#include <event.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "DBOps.h"
#include "cJSON.h"

#include "Plugins/faderPlugin.h"

// Basic runtime configuration stuff
static int verbose;
static int dbfile;
static char dbpath[256];

void destruct_Univ(void* univ){
    if(univ){
        struct LSD_Univ* castUniv = univ;
        if(castUniv->buffer){
            free(castUniv->buffer);
            castUniv->buffer = NULL;
        }
    }
}

// Plugin SO loader - NOT READY FOR USE!
/*
int lsdLoadPlugin(const char* path, struct LSD_ScenePluginHEAD** headBind){
    if(!headBind){
        fprintf(stderr,"headBind must not be NULL in lsdLoadPlugin()\n");
        return -1;
    }
    
    void* soHandle;
    soHandle = dlopen(path,RTLD_LAZY);
    
    if(!soHandle){
        fprintf(stderr,"Unable to load specified plugin: %s\n",dlerror());
        return -1;
    }
    
    struct LSD_ScenePluginHEAD* ph;
    ph = dlsym(soHandle,"pluginHead");
    
    if(!ph){
        fprintf(stderr,"Unable to resolve 'pluginHead' symbol in plugin\n");
        dlclose(soHandle);
        return -1;
    }
    
    *headBind = ph;
    
    return 0;
}*/


// Flag governing whether a loop_break should loop into itself
static int reload = 0;

// Interval in microseconds representing ideal refresh rate
static const int UPDATE_INT = 20000; // 50 FPS

// Event base for LSD's main thread
static struct event_base* ebMain;

// Event which is constantly rescheduled in order
// to ensure buffering occurs at a consistent interval
static struct event* updEv;

// Absolute timeval indicating when last update began
static struct timeval lastUpdLi;




// Function which handles the calculation of remaining time
// to next processing interval and updates an event to trigger
// in that remaining time.
// If the update is behind schedule, it will be executed immediately instead
void rescheduleUpdate(struct event* ev, struct timeval* lastUpd,void(*handler)(int,short int,void*),int interval){
	struct timeval remTime;
	struct timeval curTime;
	
	
	gettimeofday(&curTime,NULL);
	remTime.tv_sec = 0;
	remTime.tv_usec = lastUpd->tv_usec - curTime.tv_usec + interval;

	
	if(remTime.tv_usec < 0){ // If we're behind schedule
		printf("Behind Schedule\n");
		handler(0,0,NULL);
	}
	else{
		evtimer_add(ev,&remTime);
	}
}

// Function which ensures each partition's output buffer is available
void updateBuffers(int one,short int two, void* three){
	evtimer_del(updEv);
    gettimeofday(&lastUpdLi,NULL);
    
    
    // Do per-frame shite here
    //printf("Update...\n");
	bufferUnivs();
	writeUnivs();
    
    // Update timer for next interval occurance relative to buffer start time
	rescheduleUpdate(updEv,&lastUpdLi,updateBuffers,UPDATE_INT);
}


void handleInterrupt(int one,short int two, void* three){
    reload = 0;
	event_base_loopbreak(ebMain);
}

void handleReload(int ont,short int two, void* three){
    reload = 1;
    event_base_loopbreak(ebMain);
}


int lsdSceneEntry(){
    
	if(!dbfile){
		printf("Establishing empty DB.\n");
		if(lsddb_emptyDB()<0){
			fprintf(stderr,"\nError while opening DB\n");
			return -1;
		}
	}
	else{
		printf("Opening DB from file\n");
		if(lsddb_openDB(dbpath)<0){
			fprintf(stderr,"Unable to open DB from file\n");
			return -1;
		}
	}
 

    
    lsdapi_setState(STATE_PINIT);
    	

    
    /** INIT EVENT BASE **/
    ebMain = event_base_new();
    
    /** REGISTER INTERRUPT HANDLER **/
    printf("Registering signal handlers\n");
    struct event* intEv = evsignal_new(ebMain,SIGINT,handleInterrupt,NULL);
    evsignal_add(intEv,NULL);
    
    /** REGISTER RELOAD HANDLER **/
    struct event* relEv = evsignal_new(ebMain,SIGUSR1,handleReload,NULL);
    evsignal_add(relEv,NULL);
    

    
    /** OPEN HTTP RPC **/
    printf("Opening HTTP RPC\n");
    if(openRPC(ebMain,9196)<0){
        fprintf(stderr,"Unable to open RPC\n");
        return -1;
    }
	
	/** OPEN OLA **/
	printf("Starting OLA connection\n");
	if(initDMX()<0){
		fprintf(stderr,"Unable to open OLA connection\n");
		return -1;
	}
    
    /** REGISTER PERIODIC LIGHTING UPDATE **/
    printf("Registering periodic lighting update\n");
    updEv = evtimer_new(ebMain,updateBuffers,NULL);
    
    
    /** Reloader Loop **/
     
    reload = 1;
    while(reload){
		
		/** INIT GARBAGE COLLECTOR **/
		printf("Initialising Garbage Collector\n");
		lsdgc_prepGCOps();
        
		/** RESET DATABASE FOR FRESH STATE **/
		printf("Resetting DB\n");
		lsddb_resetDB();
		
        /** INIT TIME **/
        lsdapi_setState(STATE_PINIT);
        
        
        /** ALLOCATE STATE ARRAYS **/
         
        printf("Establishing System ArrayHeads.\n");
        if(initLsdArrays()<0){
            fprintf(stderr,"Error while establishing ArrayHeads\n");
            return -1;
        }
		
		/** LOAD PLUGINS HERE **/
		
		printf("Loading core plugin head\n");
		if(lsddb_pluginHeadLoader(getCoreHead(),1)<0){
			fprintf(stderr,"Unable to properly load core plugin\n");
			return -1;
		}
		
		printf("Loading fader bank plugin\n");
		if(lsddb_pluginHeadLoader(getFaderPluginHead(),1)<0){
			fprintf(stderr,"Unable to properly load fader plugin\n");
			return -1;
		}
		
		/** STRUCT PARTITION ARRAY **/
        
        printf("Structing partition array\n");
        if(lsddb_structPartitionArr()<0)
            fprintf(stderr,"Unable to structPartitionArr()\n");
		
		
		/** STRUCT UNIV ARRAY **/
		printf("Structing Universe array\n");
		if(lsddb_structUnivArr()<0){
			fprintf(stderr,"Problem structing universe array\n");
			return -1;
		}
		
		/** STRUCT CHANNEL ARRAY **/
		printf("Structing Channel array\n");
		if(lsddb_structChannelArr()<0){
			fprintf(stderr,"Problem structing channel array\n");
			return -1;
		}
        

        /** Curtain Up **/
        lsdapi_setState(STATE_PRUN);
		
		
        
        /** BEGIN PARTITION BUFFER LOOP **/
		updateBuffers(0,0,NULL);
        printf("Dispatching(Ctrl-c to quit)...\n");
        event_base_dispatch(ebMain);
        

        
        /** CLEAN UP SHITE **/
        lsdapi_setState(STATE_PCLEAN);
        
        printf("\nCleaning up Arrays\n");
        if(clearLsdArrays()<0){
            fprintf(stderr,"There was a problem cleaning up arrays. Continuing anyway\n");
        }
		
		/** CLOSE GARBAGE COLLECTOR **/
		printf("Closing Garbage Collector\n");
		lsdgc_finalGCOps();
        
    }
    
	/** Close OLA **/
	printf("Closing OLA connection\n");
	closeDMX();
	
    /** Close RPC **/
    printf("Closing HTTP RPC\n");
    closeRPC();
    
    // Update Cleanup
    printf("Cleaning Lighting Update\n");
    evtimer_del(updEv);
    event_free(updEv);
    
    // Signal cleanup
    printf("Cleaning signal handlers\n");
    evsignal_del(intEv);
    event_free(intEv);
    evsignal_del(relEv);
    event_free(relEv);
    
    // Done with libevent
    event_base_free(ebMain);
	
	// Save if necessary
	if(dbfile){
		printf("Saving DB to file\n");
		lsddb_saveDB(dbpath);
	}
    
    // Finialise DB    
    printf("Cleaning up DB\n");
    lsddb_closeDB();
    
    return 0;
}

int main(int argc, const char** argv){
	
	// Parse CommandLine
	int i;
	verbose = 0;
	dbfile = 0;
	if(argc>0){
		for (i=0; i<argc; ++i) {
			if(strcmp(argv[i],"-v")==0)
				verbose = 1;
			if(strcmp(argv[i],"-d")==0){
				strncpy(dbpath,argv[i+1],256);
				dbfile = 1;
			}
			if(strcmp(argv[i],"-f")==0)
				if(daemon(0,0)<0){printf("Unable to fork into BG\n");exit(-1);}
		}
	}
	
	// Start LSD!
    return lsdSceneEntry();
}
