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

/*
 ** This file is implements the Palette Sampler plugin.
 ** It is designed to be a flexible way of maintaining a database
 ** of colour palettes and sampling them with an arbitraty number
 ** of outputs.
 **
 ** The basic principle involves two main parts:
 **
 ** 1) A database to record palette entries and swatches (individual colours
 **    bound to the palette object.
 **
 ** 2) A gradient generator to take any sized palette and evenly
 **    distribute its colours across a common domain ranged [0,1]
 **
 ** The palette database is edited in a similar manner to LightShoppe's
 ** low-level patch. Add and remove buttons allow the insertion and
 ** removal of palettes and underneath that is the list of swatches bound
 ** to the palette.
 **
 ** All instances of the palette sampler have access to the same common
 ** database, so palettes can be used in multiple places at once with seperate
 ** sampling characteristics. 
 **
 ** Each node instance has a setting to determine the palette selection method
 ** in manual mode, a 'use' button underneath the palette list is enabled.
 ** Selecting a palette from the list and activating this button will make
 ** that palette the source. In parameter mode, an integer input is added to
 ** the node that will be used to select the palette instead.
 **
 ** Each node instance has a #out setting that determines the number of
 ** RGB outputs and the number of sample stops the gradient will have
 ** 
 ** The gradient sampler of a node has two modes of operation: manual and
 ** parameterised. Manual mode enables a gradient viewer strip in the
 ** node's configuration dialog. The strip has photoshop-style stops,
 ** except the stops aren't used to set the gradient's colours. Instead
 ** they're used to define points at which to sample the gradient along
 ** it's domain. Stops may cross over eachother without incident. 
 **
 ** Parameterised mode will add inputs to the node to accept the sample
 ** position from an external source as a floating point value.
 ** Values [0,1] will behave just like the manual sampler, however,
 ** there is another setting enabled by Parameterisation mode that determines
 ** the behaviour of other values outside this range.
 ** - Clamp mode is the default and will output a static colour of either
 **   end of the gradient
 ** - Auto loop mode will take the domain delta between two swatches in the
 **   gradient and insert a special stop at 1+(delta). This stop will be
 **   coloured the same as the First colour in the gradient, therefore
 **   making a seamless loop. Any sampling beyond 1+(delta) will be 
 **   automatically wrapped to the beginning of the gradient.
 ** - Auto blackout will use the delta as described above to insert a
 **   black stop in the gradient. This will give a smooth fade to black
 **   at either end of the gradient.
 **
 ** Please note that all modes behave exactly the same in [0,1]
 ** 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h> // For constants

#include "../../PluginAPI.h"
#include "../../NodeInstAPI.h"
#include "../../CorePlugin.h"
#include "../../cJSON.h"

#include "db.h"
#include "PaletteSampler.h"
#include "Gradient.h"

// Main plugin object
static struct LSD_ScenePlugin const * palettePlugin;

// RGB Type
static int rgbTypeId;
// Integer Type
static int intTypeId;
// Float Type
static int floatTypeId;


// Sampler (main) Class
static struct LSD_SceneNodeClass* paletteSamplerClass;



// Plug funcs

void paletteRgbBuffer(struct LSD_SceneNodeOutput const * output){
	paletteGradientOut(output->parentNode, output->dbId);
}

void* paletteRgbPtr(struct LSD_SceneNodeOutput const * output){
	struct PaletteSamplerInstData* instData = (struct PaletteSamplerInstData*)output->parentNode->data;
	
	int i;
	for(i=0;i<instData->numOuts;++i){
		if(instData->outDataArr[i].outId == output->dbId){
			return (void*)&(instData->outDataArr[i].outVal);
		}
	}
	
    return NULL;
}


static const bfFunc bfFuncs[] =
{paletteRgbBuffer};

static const bpFunc bpFuncs[] =
{paletteRgbPtr};



int paletteSamplerMake(struct LSD_SceneNodeInst const * inst, void* instData){
    // Create Default entries in DB
	newPaletteInst(inst->dbId);
	return 0;
}

int paletteSamplerRestore(struct LSD_SceneNodeInst const * inst, void* instData){
    return restorePaletteInst(inst);
}

void paletteSamplerClean(struct LSD_SceneNodeInst const * inst, void* instData){
    cleanPaletteInst(inst);
}

void paletteSamplerDelete(struct LSD_SceneNodeInst const * inst, void* instData){
    // The great database massacre
    deletePaletteInst(inst->dbId);
}




/* RPC handler
 * -getSampler //palette list, settings, active palette colour stops (for SVG rendering)//
 * -setSelMode (manual, param)
 * -insertPalette
 * -updatePalette
 * -deletePalette
 * -insertSwatch
 * -updateSwatch
 * -deleteSwatch 
 * -reorderSwatches (paletteId, orderedSwatchIdArr)
 * -activatePalette (id)[manual selMode]
 * -setNumOut (number)
 * -setSampleMode (manual, param)
 * -setSamplePos (idx,pos[0,1]) [manual SampleMode]
 * -setRepeatMode [param SampleMode]
 */
void paletteRPCHandler(cJSON* in, cJSON* out){
	
	cJSON* nodeId = cJSON_GetObjectItem(in,"nodeId");
	if(!nodeId || nodeId->type != cJSON_Number){
		cJSON_AddStringToObject(out,"error","nodeId not provided");
		return;
	}
	
	cJSON* paletteMethod = cJSON_GetObjectItem(in,"paletteMethod");
	if(!paletteMethod || paletteMethod->type != cJSON_String){
		cJSON_AddStringToObject(out,"error","paletteMethod not provided");
		return;
	}
	
	if(strcasecmp(paletteMethod->valuestring,"getSampler") == 0){
		paletteDBGetSampler(out,nodeId->valueint);
	}
	else if(strcasecmp(paletteMethod->valuestring,"setSelMode") == 0){
        cJSON* mode = cJSON_GetObjectItem(in,"mode");
        if(!mode || mode->type != cJSON_Number)
            return;
        
		paletteDBSetSelMode(nodeId->valueint, mode->valueint);
	}
	else if(strcasecmp(paletteMethod->valuestring,"insertPalette") == 0){
        cJSON* name = cJSON_GetObjectItem(in,"name");
        if(!name || name->type != cJSON_String)
            return;
        
		paletteDBInsertPalette(name->valuestring);
	}
	else if(strcasecmp(paletteMethod->valuestring,"updatePalette") == 0){
		cJSON* paletteId = cJSON_GetObjectItem(in,"paletteId");
		if(!paletteId || paletteId->type != cJSON_Number){
			cJSON_AddStringToObject(out,"error","paletteId not provided");
			return;
		}
		
        cJSON* name = cJSON_GetObjectItem(in,"name");
        if(!name || name->type != cJSON_String)
            return;
        
		paletteDBUpdatePalette(paletteId->valueint, name->valuestring);
	}	
	else if(strcasecmp(paletteMethod->valuestring,"deletePalette") == 0){
		cJSON* paletteId = cJSON_GetObjectItem(in,"paletteId");
		if(!paletteId || paletteId->type != cJSON_Number){
			cJSON_AddStringToObject(out,"error","paletteId not provided");
			return;
		}
		
		paletteDBDeletePalette(paletteId->valueint);
	}	
	else if(strcasecmp(paletteMethod->valuestring,"insertSwatch") == 0){
        cJSON* colour = cJSON_GetObjectItem(in,"colour");
        if(!colour || colour->type != cJSON_Object)
            return;
		
		cJSON* paletteId = cJSON_GetObjectItem(in,"paletteId");
		if(!paletteId || paletteId->type != cJSON_Number){
			cJSON_AddStringToObject(out,"error","paletteId not provided");
			return;
		}
        
		paletteDBInsertSwatch(paletteId->valueint, colour);
	}	
	else if(strcasecmp(paletteMethod->valuestring,"updateSwatch") == 0){
        cJSON* colour = cJSON_GetObjectItem(in,"colour");
        if(!colour || colour->type != cJSON_Object)
            return;
		
		cJSON* swatchId = cJSON_GetObjectItem(in,"swatchId");
		if(!swatchId || swatchId->type != cJSON_Number){
			cJSON_AddStringToObject(out,"error","swatchId not provided");
			return;
		}
        
		paletteDBUpdateSwatchColour(swatchId->valueint, colour);
	}	
	else if(strcasecmp(paletteMethod->valuestring,"deleteSwatch") == 0){
		cJSON* swatchId = cJSON_GetObjectItem(in,"swatchId");
		if(!swatchId || swatchId->type != cJSON_Number){
			cJSON_AddStringToObject(out,"error","swatchId not provided");
			return;
		}
		
		paletteDBDeleteSwatch(swatchId->valueint);
	}
    else if(strcasecmp(paletteMethod->valuestring,"reorderSwatches") == 0){
        cJSON* swatchIdArr = cJSON_GetObjectItem(in,"swatchIdArr");
        if(!swatchIdArr || swatchIdArr->type != cJSON_Array)
            return;
        
        paletteDBReorderSwatches(nodeId->valueint, swatchIdArr);
    }
	else if(strcasecmp(paletteMethod->valuestring,"activatePalette") == 0){
        cJSON* paletteId = cJSON_GetObjectItem(in,"paletteId");
        if(!paletteId || paletteId->type != cJSON_Number)
            return;
        
		paletteDBActivatePalette(nodeId->valueint, paletteId->valueint);
	}	
	else if(strcasecmp(paletteMethod->valuestring,"setNumOut") == 0){
        cJSON* numOut = cJSON_GetObjectItem(in,"numOut");
        if(!numOut || numOut->type != cJSON_Number)
            return;
        
		paletteDBSetNumOut(nodeId->valueint, numOut->valueint);
	}	
	else if(strcasecmp(paletteMethod->valuestring,"setSampleMode") == 0){
        cJSON* mode = cJSON_GetObjectItem(in,"mode");
        if(!mode || mode->type != cJSON_Number)
            return;
        
		paletteDBSampleMode(nodeId->valueint, mode->valueint);
	}	
	else if(strcasecmp(paletteMethod->valuestring,"setSamplePos") == 0){
        cJSON* pos = cJSON_GetObjectItem(in,"pos");
        if(!pos || pos->type != cJSON_Number)
            return;
		
		cJSON* outId = cJSON_GetObjectItem(in,"outId");
		if(!outId || outId->type != cJSON_Number)
			return;
        
		paletteDBManualSampleStopPos(nodeId->valueint, outId->valueint, pos->valuedouble);
	}	
	else if(strcasecmp(paletteMethod->valuestring,"setRepeatMode") == 0){
        cJSON* mode = cJSON_GetObjectItem(in,"mode");
        if(!mode || mode->type != cJSON_Number)
            return;
        
		paletteDBRepeatMode(nodeId->valueint, mode->valueint);
	}	
}

// Plugin handlers

int palettePluginInit(struct LSD_ScenePlugin const * plugin){
	palettePlugin = plugin;
	rgbTypeId = core_getRGBTypeID();
	intTypeId = core_getIntegerTypeID();
	floatTypeId = core_getFloatTypeID();
	
	plugininit_registerNodeClass(plugin,&paletteSamplerClass,paletteSamplerMake,
                                 paletteSamplerRestore,paletteSamplerClean,
                                 paletteSamplerDelete, sizeof(struct PaletteSamplerInstData),
                                 "Palette Sampler","Desc",0,bfFuncs,bpFuncs);
    
    paletteSamplerDBInit(plugin);
	
	return 0;
}

void palettePluginClean(struct LSD_ScenePlugin const * plugin){}



// Standard plugin shite

static const struct LSD_ScenePluginHEAD pluginHead = {
    palettePluginInit,
    palettePluginClean,
	paletteRPCHandler
};

extern const struct LSD_ScenePluginHEAD* getPluginHead(){
    return &pluginHead;
}

