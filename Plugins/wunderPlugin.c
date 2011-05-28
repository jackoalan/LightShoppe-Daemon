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


#include "wunderPlugin.h"

#include <event.h>
#include <evhttp.h>

#include <unistd.h> // For fork()

// Interval in seconds representing ideal update rate
static const int UPDATE_INT = 30;

// Event base for wunder plugin
static struct event_base* ebMain;

// Event which is constantly rescheduled in order
// to ensure updating occurs at a consistent interval
static struct event* updEv;

// Epoch timeval indicating when last update began
static struct timeval lastUpdLi;


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

void wunderUpdate(int one,short int two, void* three){
	evtimer_del(updEv);
    gettimeofday(&lastUpdLi,NULL);
    
    
    // Do per-update shite here

    
    // Update timer for next interval occurance relative to buffer start time
	rescheduleUpdate(updEv,&lastUpdLi,updateBuffers,UPDATE_INT);
}


// Wunder Interface

struct WeatherDay {
	
};

int wunderProc(){
	
}

void wunderStop(){
	event_base_loopbreak(ebMain);
}


int wunderStart(){
	// Allocate Shared Memory
	
	
	// Start Child Process
	pid_t procId;
	procId = fork();
	if(procId){ // parent
		
	}
	else{ // child
		
	}
}
