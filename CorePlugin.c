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


// This is a specially privledged plugin for the purpose of providing
// various built-in classes and types 

#include <stdio.h>
#include <stdlib.h>

#include "PluginAPI.h"
#include "NodeInstAPI.h"
#include "DBOps.h"
#include "CorePlugin.h"
#include "cJSON.h"

static int rgbType;
static struct LSD_SceneNodeClass* testClass;

static struct RGB_TYPE testOut;

extern int core_getRGBTypeID(){
	return rgbType;
}

// Plug funcs

void testNodeBufferTestOut(struct LSD_SceneNodeOutput const * output){
	testOut.r = 1.0;
	testOut.g = 0.5;
	testOut.b = 0.1;
}

void* testNodePtrTestOut(struct LSD_SceneNodeOutput const * output){
	return (void*)&testOut;
}

// Plug function tables

static const bfFunc bfFuncs[] =
{testNodeBufferTestOut};

static const bpFunc bpFuncs[] =
{testNodePtrTestOut};


// Node Init funcs

int testNodeInit(struct LSD_SceneNodeInst const * inst, void* instData){
	if(plugininst_addInstOutput(inst,rgbType,"Amber Out",0,-1,0)<0){
		fprintf(stderr,"Problem while adding instOutput in test node constructor\n");
		return -1;
	}

	return 0;
}

// TODO: Add node restore func

void testNodeClean(struct LSD_SceneNodeInst const * inst, void* instData){
	
}

// RPC handler function (Currently an echo of key 'hello')
void rpcHandler(cJSON* in, cJSON* out){
	cJSON* helloObject = cJSON_GetObjectItem(in,"hello");
	cJSON_AddStringToObject(out,"hello",helloObject->valuestring);
	//printf("RPC Called on core plugin\n");
}

// Plugin init funcs

int coreInit(struct LSD_ScenePlugin const * plugin, cJSON const * confjson){
	
    
    // Register RGB Type
    if(plugininit_registerDataType(plugin,&rgbType,1,"RGB Type","RGB value")<0){
        fprintf(stderr,"Error while registering RGB data type\n");
        return -1;
    }
    
    // Register test class
    if(plugininit_registerNodeClass(plugin,&testClass,testNodeInit,NULL,testNodeClean,0,"TestTest","Test Class",0,bfFuncs,bpFuncs)<0){
        fprintf(stderr, "Error while registering core's test class\n");
        return -1;
    }
    
    
    
    return 0;
}

void coreClean(struct LSD_ScenePlugin const * plugin){
    
}

static const struct LSD_ScenePluginHEAD pluginHead = {
    coreInit,
    coreClean,
	rpcHandler,
    "org.resinbros.lsd.core",
    "0.1"
};

const struct LSD_ScenePluginHEAD* getCoreHead(){
    return &pluginHead;
}
