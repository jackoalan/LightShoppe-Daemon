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
#include <string.h>

#include "PluginAPI.h"
#include "NodeInstAPI.h"
#include "DBOps.h"
#include "CorePlugin.h"
#include "cJSON.h"
#include "Node.h"

// Types
static int rgbType;
static int integerType;
static int floatType;
static int triggerType;

// Type accessors
int core_getRGBTypeID(){
	return rgbType;
}
int core_getIntegerTypeID(){
	return integerType;
}
int core_getFloatTypeID(){
	return floatType;
}
int core_getTriggerTypeID(){
	return triggerType;
}

// Classes
static struct LSD_SceneNodeClass* testClass;
static struct LSD_SceneNodeClass* intGenClass;
//static struct LSD_SceneNodeClass* intViewClass;


static struct RGB_TYPE testOut;



/****** TEST STUFF ******/

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

static const bfFunc testBfFuncs[] =
{testNodeBufferTestOut};

static const bpFunc testBpFuncs[] =
{testNodePtrTestOut};


int testNodeMake(struct LSD_SceneNodeInst const * inst, void* instData){
	if(plugininst_addInstOutput(inst,rgbType,"Amber Out",0,0,NULL)<0){
		fprintf(stderr,"Problem while adding instOutput in test node constructor\n");
		return -1;
	}

	return 0;
}

int testNodeRestore(struct LSD_SceneNodeInst const * inst, void* instData){
	return 0;
}

void testNodeClean(struct LSD_SceneNodeInst const * inst, void* instData){
	
}

void testNodeDelete(struct LSD_SceneNodeInst const * inst, void* instData){
	
}


/****** INT GEN STUFF ******/

static int intGenInt;

// Plug funcs
void intGenBufferOut(struct LSD_SceneNodeOutput const * output){
	intGenInt = 42;
}

void* intGenPtrOut(struct LSD_SceneNodeOutput const * output){
	return (void*)&intGenInt;
}

static const bfFunc intGenBfFuncs[] =
{intGenBufferOut};

static const bpFunc intGenBpFuncs[] =
{intGenPtrOut};

int intGenMake(struct LSD_SceneNodeInst const * inst, void* instData){
    return 0;
}

int intGenRestore(struct LSD_SceneNodeInst const * inst, void* instData){
    return 0;
}

void intGenClean(struct LSD_SceneNodeInst const * inst, void* instData){
    
}

void intGenDelete(struct LSD_SceneNodeInst const * inst, void* instData){
    
}


// Plugin init funcs

int coreInit(struct LSD_ScenePlugin const * plugin){
	
    
    // Register RGB Type
    if(plugininit_registerDataType(plugin,&rgbType,1,"RGB Type","RGB value")<0){
        fprintf(stderr,"Error while registering RGB data type\n");
        return -1;
    }
    
    // Register test class
    if(plugininit_registerNodeClass(plugin,&testClass,testNodeMake,testNodeRestore,testNodeClean,
									testNodeDelete,0,"TestTest","Test Class",0,testBfFuncs,testBpFuncs)<0){
        fprintf(stderr, "Error while registering core's test class\n");
        return -1;
    }
    
    // Register int generator
    if(plugininit_registerNodeClass(plugin,&intGenClass,intGenMake,intGenRestore,intGenClean,intGenDelete,
								 0,"Integer Generator","Desc",1,intGenBfFuncs,intGenBpFuncs)<0){
		fprintf(stderr, "Error while registering core's integer generator class\n");
        return -1;
    }
    
    return 0;
}

// RPC handler function
void rpcHandler(cJSON* in, cJSON* out){
    
	cJSON* coreMethod = cJSON_GetObjectItem(in,"coreMethod");
    if(!coreMethod || coreMethod->type != cJSON_String)
        return;
    
    cJSON* nodeId = cJSON_GetObjectItem(in,"nodeId");
    if(!nodeId || nodeId->type != cJSON_Number)
        return;
    
    // Determine which RPC operation to perform
    if(strcasecmp(coreMethod->valuestring,"setIntGenVal") == 0){
        
    }
    else if(strcasecmp(coreMethod->valuestring,"getIntViewVal") == 0){
        
    }
}

void coreClean(struct LSD_ScenePlugin const * plugin){
    
}

// Core Stuff Below - must remain in order to be loaded
static const struct LSD_ScenePluginHEAD pluginHead = {
    coreInit,
    coreClean,
	rpcHandler
};

const struct LSD_ScenePluginHEAD* getCoreHead(){
    return &pluginHead;
}
