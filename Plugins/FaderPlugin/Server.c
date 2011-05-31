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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../PluginAPI.h"
#include "../../NodeInstAPI.h"
#include "../../DBOps.h"
#include "../../CorePlugin.h"
#include "../../cJSON.h"


static struct LSD_SceneNodeClass* faderClass;

static struct RGB_TYPE faderOuts[5];
static struct RGB_TYPE faderRGBs[5];
static double faderInts[5];


// Plug funcs

void faderBufferOut(struct LSD_SceneNodeOutput const * output){
	int i;
	for(i=0;i<5;++i){
		faderOuts[i].r = faderRGBs[i].r * faderInts[i];
		faderOuts[i].g = faderRGBs[i].g * faderInts[i];
		faderOuts[i].b = faderRGBs[i].b * faderInts[i];
	}
}

void* faderPtr0(struct LSD_SceneNodeOutput const * output){
	//printf("Requested ptr0\n");
	return (void*)&(faderOuts[0]);
}

void* faderPtr1(struct LSD_SceneNodeOutput const * output){
	//printf("Requested ptr1\n");
	return (void*)&(faderOuts[1]);
}

void* faderPtr2(struct LSD_SceneNodeOutput const * output){
	return (void*)&(faderOuts[2]);
}

void* faderPtr3(struct LSD_SceneNodeOutput const * output){
	return (void*)&(faderOuts[3]);
}

void* faderPtr4(struct LSD_SceneNodeOutput const * output){
	return (void*)&(faderOuts[4]);
}

// Plug function tables

static const bfFunc bfFuncs[] =
{faderBufferOut};

static const bpFunc bpFuncs[] =
{faderPtr0,faderPtr1,faderPtr2,faderPtr3,faderPtr4};


// Node Init funcs

int faderNodeInit(struct LSD_SceneNodeInst const * inst, void* instData){
	int rgbType = core_getRGBTypeID();
	
	if(plugininst_addInstOutput(inst,rgbType,"Colour One",0,-1,0)<0){
		fprintf(stderr,"Problem while adding instOutput in test node constructor\n");
		return -1;
	}
	
	if(plugininst_addInstOutput(inst,rgbType,"Colour Two",0,-1,1)<0){
		fprintf(stderr,"Problem while adding instOutput in test node constructor\n");
		return -1;
	}
	
	if(plugininst_addInstOutput(inst,rgbType,"Colour Three",0,-1,2)<0){
		fprintf(stderr,"Problem while adding instOutput in test node constructor\n");
		return -1;
	}
	
	if(plugininst_addInstOutput(inst,rgbType,"Colour Four",0,-1,3)<0){
		fprintf(stderr,"Problem while adding instOutput in test node constructor\n");
		return -1;
	}
	
	if(plugininst_addInstOutput(inst,rgbType,"Colour Five",0,-1,4)<0){
		fprintf(stderr,"Problem while adding instOutput in test node constructor\n");
		return -1;
	}
	
	return 0;
}

void faderNodeClean(struct LSD_SceneNodeInst const * inst, void* instData){
	
}

void faderRPCHandler(cJSON* in, cJSON* out){

	
	cJSON* faderMethod = cJSON_GetObjectItem(in,"faderMethod");
	if(faderMethod && faderMethod->type==cJSON_String){
		// First find out the requested method
        if(strcasecmp(faderMethod->valuestring,"getState")==0){
			cJSON* faderArr = cJSON_CreateArray();
			
			int i;
			for(i=0;i<5;++i){
				cJSON* faderObject = cJSON_CreateObject();
				cJSON_AddNumberToObject(faderObject,"intensity",faderInts[i]);
				cJSON* colourObject = cJSON_CreateObject();
				cJSON_AddNumberToObject(colourObject,"r",faderRGBs[i].r);
				cJSON_AddNumberToObject(colourObject,"g",faderRGBs[i].g);
				cJSON_AddNumberToObject(colourObject,"b",faderRGBs[i].b);
				cJSON_AddItemToObject(faderObject,"rgb",colourObject);
				cJSON_AddItemToArray(faderArr,faderObject);
			}
			
			cJSON_AddItemToObject(out,"faders",faderArr);
		}
		else if(strcasecmp(faderMethod->valuestring,"setState")==0){
			cJSON* faderArr = cJSON_GetObjectItem(in,"faders");
			
			int i;
			for(i=0;i<5;++i){
				cJSON* faderObject = cJSON_GetArrayItem(faderArr,i);
				cJSON* intensity = cJSON_GetObjectItem(faderObject,"intensity");
				cJSON* colourObject = cJSON_GetObjectItem(faderObject,"rgb");
				cJSON* rObj = cJSON_GetObjectItem(colourObject,"r");
				cJSON* gObj = cJSON_GetObjectItem(colourObject,"g");
				cJSON* bObj = cJSON_GetObjectItem(colourObject,"b");
				
				faderInts[i] = intensity->valuedouble;
				faderRGBs[i].r = rObj->valuedouble;
				faderRGBs[i].g = gObj->valuedouble;
				faderRGBs[i].b = bObj->valuedouble;

			}
		}
		else{
			cJSON_AddStringToObject(out,"error","unhandled fader method");
		}
	}
	else{
		cJSON_AddStringToObject(out,"error","faderMethod not provided");
	}
}

// Plugin init funcs

int faderPluginInit(struct LSD_ScenePlugin const * plugin, cJSON const * confjson){
	printf("Fader Init Ran\n");
	// Init data
	int i;
	for(i=0;i<5;++i){
		faderInts[i] = 0.0;
		faderRGBs[i].r = 0.0;
		faderRGBs[i].g = 0.0;
		faderRGBs[i].b = 0.0;
	}
	
    // Register fader class
    if(plugininit_registerNodeClass(plugin,&faderClass,faderNodeInit,NULL,faderNodeClean,0,"Fader","Fader Class",0,bfFuncs,bpFuncs)<0){
        fprintf(stderr, "Error while registering fader class\n");
        return -1;
    }
    
    return 0;
}

void faderPluginClean(struct LSD_ScenePlugin const * plugin){
    
}

static const struct LSD_ScenePluginHEAD pluginHead = {
    faderPluginInit,
    faderPluginClean,
	faderRPCHandler,
    "org.resinbros.lsd.fader",
    "0.1"
};

extern const struct LSD_ScenePluginHEAD* getPluginHead(){
    return &pluginHead;
}
