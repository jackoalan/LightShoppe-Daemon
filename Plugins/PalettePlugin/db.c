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
 ** This file contains all database operations for the palette sampler
 ** plugin. It's init function must be ran at plugin init.
 */

#include "db.h"
#include "PaletteSampler.h"
#include "../../NodeInstAPI.h"

// Main plugin object
static struct LSD_ScenePlugin const * palettePlugin;

// Stmt index bindings
static unsigned int selectNodeSettingStmt;
static unsigned int insertNodeSettingStmt;
static unsigned int updateNodeSettingSelModeStmt;
static unsigned int updateNodeSettingManualPaletteStmt;
static unsigned int updateNodeSettingNumOutsStmt;
static unsigned int updateNodeSettingSampleModeStmt;
static unsigned int updateNodeSettingRepeatModeStmt;
static unsigned int deleteNodeSettingStmt;

static unsigned int selectPaletteStmt;
static unsigned int insertPaletteStmt;
static unsigned int updatePaletteStmt;
static unsigned int deletePaletteStmt;

static unsigned int selectSwatchStmt;
static unsigned int insertSwatchStmt;
static unsigned int updateSwatchPosStmt;
static unsigned int updateSwatchColourStmt;
static unsigned int deleteSwatchStmt;
static unsigned int deleteSwatchParentStmt;

static unsigned int selectSelectInStmt;
static unsigned int insertSelectInStmt;
static unsigned int deleteSelectInStmt;

static unsigned int selectSampleStopInStmt;
static unsigned int insertSampleStopInStmt;
static unsigned int deleteSampleStopInStmt;

//static unsigned int selectRGBOutStmt;
//static unsigned int insertRGBOutStmt;
//static unsigned int deleteRGBOutStmt;

static unsigned int selectManualStopStmt;
static unsigned int insertManualStopStmt;
static unsigned int updateManualStopPosStmt;
static unsigned int deleteManualStopStmt;

// RPC methods

int getPaletteSwatches(cJSON* target, int paletteId){
	if(!target || target->type != cJSON_Object)
		return -1;
	
	cJSON* swatchArr = cJSON_CreateArray();
	plugindb_reset(palettePlugin,selectSwatchStmt);
	plugindb_bind_int(palettePlugin,selectSwatchStmt,1,paletteId);
	while(plugindb_step(palettePlugin,selectSwatchStmt) == SQLITE_ROW){
		cJSON* swatchObj = cJSON_CreateObject();
		cJSON_AddNumberToObject(swatchObj,"swatchId",
								plugindb_column_int(palettePlugin,selectSwatchStmt,0));
		cJSON_AddNumberToObject(swatchObj,"rVal",
								plugindb_column_double(palettePlugin,selectSwatchStmt,1));
		cJSON_AddNumberToObject(swatchObj,"gVal",
								plugindb_column_double(palettePlugin,selectSwatchStmt,2));
		cJSON_AddNumberToObject(swatchObj,"bVal",
								plugindb_column_double(palettePlugin,selectSwatchStmt,3));
		
		cJSON_AddItemToArray(swatchArr,swatchObj);
	}
	
	cJSON_AddItemToObject(target,"paletteSwatches",swatchArr);
	
	return 0;
}

int paletteDBGetSampler(cJSON* target, int nodeId){
	if(!target || target->type != cJSON_Object)
		return -1;
	
	// settings
	cJSON* settingObj = cJSON_CreateObject();
	
	plugindb_reset(palettePlugin,selectNodeSettingStmt);
	plugindb_bind_int(palettePlugin,selectNodeSettingStmt,1,nodeId);
	if(plugindb_step(palettePlugin,selectNodeSettingStmt) == SQLITE_ROW){
		cJSON_AddNumberToObject(settingObj,"selMode",
								plugindb_column_int(palettePlugin,selectNodeSettingStmt,0));
		cJSON_AddNumberToObject(settingObj,"manualPaletteId",
								plugindb_column_int(palettePlugin,selectNodeSettingStmt,1));
		cJSON_AddNumberToObject(settingObj,"numOuts",
								plugindb_column_int(palettePlugin,selectNodeSettingStmt,2));
		cJSON_AddNumberToObject(settingObj,"sampleMode",
								plugindb_column_int(palettePlugin,selectNodeSettingStmt,3));
		cJSON_AddNumberToObject(settingObj,"paramSampleRepeatMode",
								plugindb_column_int(palettePlugin,selectNodeSettingStmt,4));
	}
	else{
		cJSON_AddStringToObject(target,"error","entry not in db");
		return -1;
	}
	cJSON_AddItemToObject(target,"settings",settingObj);
	
	// palette list
	cJSON* paletteArr = cJSON_CreateArray();
	
	plugindb_reset(palettePlugin,selectPaletteStmt);
	while(plugindb_step(palettePlugin,selectPaletteStmt) == SQLITE_ROW){
		cJSON* paletteObj = cJSON_CreateObject();
		int paletteId = plugindb_column_int(palettePlugin,selectPaletteStmt,0);
		cJSON_AddNumberToObject(paletteObj,"paletteId",paletteId);
		cJSON_AddStringToObject(paletteObj,"paletteName",
								(const char*)plugindb_column_text(palettePlugin,selectPaletteStmt,1));
		
		getPaletteSwatches(paletteObj,paletteId);
		
		cJSON_AddItemToArray(paletteArr,paletteObj);
	}
	cJSON_AddItemToObject(target,"palettes",paletteArr);

	
	
	// cur grad colours
	// Get inst data for this
	struct PaletteSamplerInstData* instData;
	plugin_getInstById(palettePlugin,nodeId,(void**)&instData);
	
	int i;

	if(instData){
		cJSON* swatchArr = cJSON_CreateArray();
		
		for(i=0; i<instData->swatchCount; ++i){
			cJSON* swatchObj = cJSON_CreateObject();
			struct RGB_TYPE* swatch;
			swatch = &(instData->swatchArr[i]);
			cJSON_AddNumberToObject(swatchObj,"r",swatch->r);
			cJSON_AddNumberToObject(swatchObj,"g",swatch->g);
			cJSON_AddNumberToObject(swatchObj,"b",swatch->b);
			cJSON_AddItemToArray(swatchArr,swatchObj);
		}
		
		cJSON_AddItemToObject(target,"curPalette",swatchArr);
	}
	else{
		cJSON_AddStringToObject(target,"error","error getting current palette");
		return -1;
	}
	
	
	// Sampler stops
	cJSON* stopArr = cJSON_CreateArray();
	
	for(i=0; i<instData->numOuts; ++i){
		cJSON* stopObj = cJSON_CreateObject();
		struct PaletteSamplerOutputData* outData = &(instData->outDataArr[i]);
		
		cJSON_AddNumberToObject(stopObj,"stopId",outData->outId);
		cJSON_AddNumberToObject(stopObj,"pos",outData->samplePos);
		
		cJSON_AddItemToArray(stopArr,stopObj);
	}
	
	cJSON_AddItemToObject(target,"sampleStops",stopArr);
	
	return 0;
}

int paletteDBSetSelMode(int nodeId, enum SourceModeEnum mode){
	plugindb_reset(palettePlugin,updateNodeSettingSelModeStmt);
	plugindb_bind_int(palettePlugin,updateNodeSettingSelModeStmt,1,nodeId);
	plugindb_bind_int(palettePlugin,updateNodeSettingSelModeStmt,2,mode);
	if(plugindb_step(palettePlugin,updateNodeSettingSelModeStmt) == SQLITE_DONE)
		return 0;
	return -1;
}

int paletteDBInsertPalette(const char* name){
	plugindb_reset(palettePlugin,insertPaletteStmt);
	plugindb_bind_text(palettePlugin,insertPaletteStmt,1,name,-1,NULL);
	if(plugindb_step(palettePlugin,insertPaletteStmt) == SQLITE_DONE)
		return 0;
	return -1;
}

int paletteDBUpdatePalette(int paletteId, const char* name){
	plugindb_reset(palettePlugin,updatePaletteStmt);
	plugindb_bind_int(palettePlugin,updatePaletteStmt,1,paletteId);
	plugindb_bind_text(palettePlugin,updatePaletteStmt,2,name,-1,NULL);
	if(plugindb_step(palettePlugin,updatePaletteStmt) == SQLITE_DONE)
		return 0;
	return -1;
}

int paletteDBDeletePalette(int paletteId){
	plugindb_reset(palettePlugin,deletePaletteStmt);
	plugindb_bind_int(palettePlugin,deletePaletteStmt,1,paletteId);
	if(plugindb_step(palettePlugin,deletePaletteStmt) == SQLITE_DONE)
		return 0;
	return -1;
}

int paletteDBInsertSwatch(int paletteId, cJSON* colour){
	if(!colour || colour->type != cJSON_Object)
		return -1;
	
	cJSON* rVal = cJSON_GetObjectItem(colour,"r");
	if(!rVal || rVal->type != cJSON_Number)
		return -1;
	
	cJSON* gVal = cJSON_GetObjectItem(colour,"g");
	if(!gVal || gVal->type != cJSON_Number)
		return -1;
	
	cJSON* bVal = cJSON_GetObjectItem(colour,"b");
	if(!bVal || bVal->type != cJSON_Number)
		return -1;
	
	plugindb_reset(palettePlugin,insertSwatchStmt);
	plugindb_bind_int(palettePlugin,insertSwatchStmt,1,paletteId);
	plugindb_bind_double(palettePlugin,insertSwatchStmt,2,rVal->valuedouble);
	plugindb_bind_double(palettePlugin,insertSwatchStmt,3,gVal->valuedouble);
	plugindb_bind_double(palettePlugin,insertSwatchStmt,4,bVal->valuedouble);
	
	if(plugindb_step(palettePlugin,insertSwatchStmt) == SQLITE_DONE)
		return 0;
	return -1;

}

int paletteDBUpdateSwatchColour(int swatchId, cJSON* colour){
	if(!colour || colour->type != cJSON_Object)
		return -1;
	
	cJSON* rVal = cJSON_GetObjectItem(colour,"r");
	if(!rVal || rVal->type != cJSON_Number)
		return -1;
	
	cJSON* gVal = cJSON_GetObjectItem(colour,"g");
	if(!gVal || gVal->type != cJSON_Number)
		return -1;
	
	cJSON* bVal = cJSON_GetObjectItem(colour,"b");
	if(!bVal || bVal->type != cJSON_Number)
		return -1;
	
	plugindb_reset(palettePlugin,updateSwatchColourStmt);
	plugindb_bind_int(palettePlugin,updateSwatchColourStmt,1,swatchId);
	plugindb_bind_double(palettePlugin,updateSwatchColourStmt,2,rVal->valuedouble);
	plugindb_bind_double(palettePlugin,updateSwatchColourStmt,3,gVal->valuedouble);
	plugindb_bind_double(palettePlugin,updateSwatchColourStmt,4,bVal->valuedouble);
	
	if(plugindb_step(palettePlugin,updateSwatchColourStmt) == SQLITE_DONE)
		return 0;
	return -1;
}

int paletteDBDeleteSwatch(int swatchId){
	plugindb_reset(palettePlugin,deleteSwatchStmt);
	plugindb_bind_int(palettePlugin,deleteSwatchStmt,1,swatchId);
	if(plugindb_step(palettePlugin,deleteSwatchStmt) == SQLITE_DONE)
		return 0;
	return -1;
}

int paletteDBUpdateSwatchPos(int swatchId, int posIdx){
	plugindb_reset(palettePlugin,updateSwatchPosStmt);
	plugindb_bind_int(palettePlugin,updateSwatchPosStmt,1,swatchId);
	plugindb_bind_int(palettePlugin,updateSwatchPosStmt,2,posIdx);
	if(plugindb_step(palettePlugin,updateSwatchPosStmt) == SQLITE_DONE)
		return 0;
	return -1;
}
				   
int paletteDBReorderSwatches(int paletteId, cJSON* swatchIdArr){
	if(!swatchIdArr || swatchIdArr->type != cJSON_Array)
		return -1;
	
	int i;
	for(i=0; i<cJSON_GetArraySize(swatchIdArr); ++i){
		cJSON* idItem = cJSON_GetArrayItem(swatchIdArr,i);
		paletteDBUpdateSwatchPos(idItem->valueint,i);
	}
	
	return 0;
}

int paletteDBActivatePalette(int nodeId, int paletteId){
	plugindb_reset(palettePlugin,updateNodeSettingManualPaletteStmt);
	plugindb_bind_int(palettePlugin,updateNodeSettingManualPaletteStmt,1,nodeId);
	plugindb_bind_int(palettePlugin,updateNodeSettingManualPaletteStmt,2,paletteId);
	if(plugindb_step(palettePlugin,updateNodeSettingManualPaletteStmt) == SQLITE_DONE)
		return 0;
	return -1;
}

int paletteDBSetNumOut(int nodeId, int numOuts){
	plugindb_reset(palettePlugin,updateNodeSettingNumOutsStmt);
	plugindb_bind_int(palettePlugin,updateNodeSettingNumOutsStmt,1,nodeId);
	plugindb_bind_int(palettePlugin,updateNodeSettingNumOutsStmt,2,numOuts);
	if(plugindb_step(palettePlugin,updateNodeSettingNumOutsStmt) == SQLITE_DONE)
		return 0;
	return -1;
}

int paletteDBSampleMode(int nodeId, enum SourceModeEnum mode){
	plugindb_reset(palettePlugin,updateNodeSettingSampleModeStmt);
	plugindb_bind_int(palettePlugin,updateNodeSettingSampleModeStmt,1,nodeId);
	plugindb_bind_int(palettePlugin,updateNodeSettingSampleModeStmt,2,mode);
	if(plugindb_step(palettePlugin,updateNodeSettingSampleModeStmt) == SQLITE_DONE)
		return 0;
	return -1;
}

int paletteDBManualSampleStopPos(int stopId, double pos){
	plugindb_reset(palettePlugin,updateManualStopPosStmt);
	plugindb_bind_int(palettePlugin,updateManualStopPosStmt,1,stopId);
	plugindb_bind_double(palettePlugin,updateManualStopPosStmt,2,pos);
	if(plugindb_step(palettePlugin,updateManualStopPosStmt) == SQLITE_DONE)
		return 0;
	return -1;
}

int paletteDBRepeatMode(int nodeId, enum RepeatMode mode){
	plugindb_reset(palettePlugin,updateNodeSettingRepeatModeStmt);
	plugindb_bind_int(palettePlugin,updateNodeSettingRepeatModeStmt,1,nodeId);
	plugindb_bind_int(palettePlugin,updateNodeSettingRepeatModeStmt,2,mode);
	if(plugindb_step(palettePlugin,updateNodeSettingRepeatModeStmt) == SQLITE_DONE)
		return 0;
	return -1;
}

int restorePaletteInst(struct LSD_SceneNodeInst const * inst){
	if(!inst)
		return -1;
	
	struct PaletteSamplerInstData* instData = (struct PaletteSamplerInstData*)inst->data;
	if(!instData)
		return -1;
	// selMode,manualPaletteId,numOuts,sampleMode,paramSampleRepeatMode
	plugindb_reset(palettePlugin,selectNodeSettingStmt);
	plugindb_bind_int(palettePlugin,selectNodeSettingStmt,1,inst->dbId);
	if(plugindb_step(palettePlugin,selectNodeSettingStmt) == SQLITE_ROW){
		// Settings
		instData->selMode = plugindb_column_int(palettePlugin,selectNodeSettingStmt,0);
		instData->curPaletteId = plugindb_column_int(palettePlugin,selectNodeSettingStmt,1);
		instData->numOuts = plugindb_column_int(palettePlugin,selectNodeSettingStmt,2);
		instData->sampleMode = plugindb_column_int(palettePlugin,selectNodeSettingStmt,3);
		instData->paramSampleRepeatMode = plugindb_column_int(palettePlugin,
															  selectNodeSettingStmt,4);
		
		// Output data
		struct PaletteSamplerOutputData* outDataArr = malloc(sizeof(struct PaletteSamplerOutputData)*
															 instData->numOuts);
		
		if(!outDataArr)
			return -1;
		
		instData->outDataArr = outDataArr;
		
		// Repopulate swatches
        if(instData->selMode == MANUAL){
            // Lookup swatch as referenced in the struct
            plugindb_reset(palettePlugin,selectSwatchStmt);
            plugindb_bind_int(palettePlugin,selectSwatchStmt,1,instData->curPaletteId);
            int swatchCount = 0;
            while(plugindb_step(palettePlugin,selectSwatchStmt) == SQLITE_ROW){
                // Count swatches in palette
                ++swatchCount;
            }
            
            instData->swatchCount = swatchCount;
            
            struct RGB_TYPE* swatchArr = malloc(sizeof(struct RGB_TYPE)*swatchCount);
            
            if(!swatchArr)
                return -1;
            
            instData->swatchArr = swatchArr;
            
            // Copy swatches into struct
            int i = 0;
            plugindb_reset(palettePlugin,selectSwatchStmt);
            plugindb_bind_int(palettePlugin,selectSwatchStmt,1,instData->curPaletteId);
            while(plugindb_step(palettePlugin,selectSwatchStmt) == SQLITE_ROW && i<swatchCount){
                instData->swatchArr[i].r = plugindb_column_double(palettePlugin,selectSwatchStmt,1);
                instData->swatchArr[i].g = plugindb_column_double(palettePlugin,selectSwatchStmt,2);
                instData->swatchArr[i].b = plugindb_column_double(palettePlugin,selectSwatchStmt,3);
                ++i;
            }
            
        }
        else if(instData->selMode == PARAM){
            // Remove curPalette Reference and wait for buffering
            instData->curPaletteId = -1;
			
			// Set SelIn
			plugindb_reset(palettePlugin,selectSelectInStmt);
			plugindb_bind_int(palettePlugin,selectSelectInStmt,1,inst->dbId);
			if(plugindb_step(palettePlugin,selectSelectInStmt) == SQLITE_ROW){
				plugininst_getInputStruct(inst, &(instData->selIn), 
										  plugindb_column_int(palettePlugin,selectSelectInStmt,0));
			}
        }
		
		if(instData->sampleMode == PARAM){
			struct LSD_SceneNodeInput const ** sampleStopInArr = 
			malloc(sizeof(struct LSD_SceneNodeInput*)*instData->numOuts);
			if(!sampleStopInArr)
				return -1;
			
			instData->sampleStopInArr = sampleStopInArr;
			
			plugindb_reset(palettePlugin,selectSampleStopInStmt);
			plugindb_bind_int(palettePlugin,selectSampleStopInStmt,1,inst->dbId);
			int i = 0;
			while(plugindb_step(palettePlugin,selectSampleStopInStmt) == SQLITE_ROW && i < instData->numOuts){
				plugininst_getInputStruct(inst, &(instData->sampleStopInArr[i]), 
										  plugindb_column_int(palettePlugin,selectSampleStopInStmt,0));
				++i;
			}
		}
		else if(instData->sampleMode == MANUAL){
			plugindb_reset(palettePlugin,selectManualStopStmt);
			plugindb_bind_int(palettePlugin,selectManualStopStmt,1,inst->dbId);
			int i = 0;
			while(plugindb_step(palettePlugin,selectManualStopStmt) == SQLITE_ROW){
				instData->outDataArr[i].samplePos = plugindb_column_double(palettePlugin,selectManualStopStmt,2);
				++i;
			}
		}
	}
	else{
		return -1;
	}
	
	return 0;
}


// Tracks individual instance's parameters
static const char NODE_SETTING[] = "NodeSetting";
static const char NODE_SETTING_COLS[] =
"nodeId INTEGER PRIMARY KEY, selMode INTEGER, manualPaletteId INTEGER, "
"numOuts INTEGER, sampleMode INTEGER, paramSampleRepeatMode INTEGER";

// Shared Palette DB
static const char PALETTE[] = "Palette";
static const char PALETTE_COLS[] =
"id INTEGER PRIMARY KEY, name TEXT";

// Shared Swatch DB
static const char SWATCH[] = "Swatch";
static const char SWATCH_COLS[] =
"id INTEGER PRIMARY KEY, parentPalette INTEGER, posIdx INTEGER, "
"rVal REAL, gVal REAL, bVal REAL";

// Tracks selector inputs bound to instances
static const char SELECT_IN[] = "SelectIn";
static const char SELECT_IN_COLS[] =
"inId INTEGER PRIMARY KEY, nodeId INTEGER";

// Tracks stop position inputs bound to instances
static const char SAMPLE_STOP_IN[] = "SampleStopIn";
static const char SAMPLE_STOP_IN_COLS[] =
"inId INTEGER PRIMARY KEY, nodeId INTEGER";

// Tracks outputs bound to instances
static const char RGB_OUT[] = "RGBOut";
static const char RGB_OUT_COLS[] = 
"outId INTEGER PRIMARY KEY, nodeId INTEGER, outIdx INTEGER";

// When an instance is in manual sampling mode,
// this is where static stop positions are stored
// This table is emptied for an instance when switched to param mode
// and repopulated as a default linear spread when switched to manual
static const char MANUAL_STOP[] = "ManualStop";
static const char MANUAL_STOP_COLS[] =
"outId INTEGER PRIMARY KEY, nodeId INTEGER, outIdx INTEGER, pos REAL";

int paletteSamplerDBInit(struct LSD_ScenePlugin const * plugin){
	palettePlugin = plugin;
    
    
	
	plugininit_createTable(palettePlugin, NODE_SETTING, NODE_SETTING_COLS);
	plugindb_prepSelect(palettePlugin, &selectNodeSettingStmt, NODE_SETTING, 
						"selMode,manualPaletteId,numOuts,sampleMode,paramSampleRepeatMode",
						"nodeId=?1");
	plugindb_prepInsert(palettePlugin, &insertNodeSettingStmt, NODE_SETTING, 
						"nodeId,selMode,manualPaletteId,numOuts,sampleMode,paramSampleRepeatMode",
						"?1,0,0,0,0,0");
	plugindb_prepUpdate(palettePlugin, &updateNodeSettingSelModeStmt, NODE_SETTING,
						"selMode=?2","nodeId=?1");
	plugindb_prepUpdate(palettePlugin, &updateNodeSettingManualPaletteStmt, NODE_SETTING,
						"manualPaletteId=?2","nodeId=?1");
	plugindb_prepUpdate(palettePlugin, &updateNodeSettingNumOutsStmt, NODE_SETTING,
						"numOuts=?2","nodeId=?1");
	plugindb_prepUpdate(palettePlugin, &updateNodeSettingSampleModeStmt, NODE_SETTING,
						"sampleMode=?2","nodeId=?1");
	plugindb_prepUpdate(palettePlugin, &updateNodeSettingRepeatModeStmt, NODE_SETTING,
						"paramSampleRepeatMode=?2","nodeId=?1");
	plugindb_prepDelete(palettePlugin, &deleteNodeSettingStmt, NODE_SETTING,
						"nodeId=?1");
	
	plugininit_createTable(palettePlugin,PALETTE,PALETTE_COLS);
	plugindb_prepSelect(palettePlugin, &selectPaletteStmt, PALETTE,
						"id,name","1=1");
	plugindb_prepInsert(palettePlugin, &insertPaletteStmt, PALETTE,
						"name","?1");
	plugindb_prepUpdate(palettePlugin, &updatePaletteStmt, PALETTE,
						"name=?2","id=?1");
	plugindb_prepDelete(palettePlugin, &deletePaletteStmt, PALETTE,
						"id=?1");
	
	plugininit_createTable(palettePlugin,SWATCH,SWATCH_COLS);
	// Make index with parentPalette and posIdx
	plugininit_createIndex(palettePlugin,"SwatchIndex",SWATCH,"parentPalette,posIdx");
	plugindb_prepSelect(palettePlugin, &selectSwatchStmt, SWATCH,
						"id,rVal,gVal,bVal","parentPalette=?1");
	plugindb_prepInsert(palettePlugin, &insertSwatchStmt, SWATCH,
						"parentPalette,rVal,gVal,bVal","?1,?2,?3,?4");
	plugindb_prepUpdate(palettePlugin, &updateSwatchPosStmt, SWATCH,
						"posIdx=?2","id=?1");
	plugindb_prepUpdate(palettePlugin, &updateSwatchColourStmt, SWATCH,
						"rVal=?2,gVal=?3,bVal=?4","id=?1");
	plugindb_prepDelete(palettePlugin, &deleteSwatchStmt, SWATCH,
						"id=?1");
	plugindb_prepDelete(palettePlugin, &deleteSwatchParentStmt, SWATCH,
						"parentPalette=?1");
	
	
	plugininit_createTable(palettePlugin,SELECT_IN,SELECT_IN_COLS);
	plugindb_prepSelect(palettePlugin, &selectSelectInStmt, SELECT_IN,
						"inId","nodeId=?1");
	plugindb_prepInsert(palettePlugin, &insertSelectInStmt, SELECT_IN,
						"nodeId,inId","?1,?2");
	plugindb_prepDelete(palettePlugin, &deleteSelectInStmt, SELECT_IN,
						"nodeId=?1");
	
	plugininit_createTable(palettePlugin,SAMPLE_STOP_IN,SAMPLE_STOP_IN_COLS);
	plugindb_prepSelect(palettePlugin, &selectSampleStopInStmt, SAMPLE_STOP_IN,
						"inId","nodeId=?1");
	plugindb_prepInsert(palettePlugin, &insertSampleStopInStmt, SAMPLE_STOP_IN,
						"nodeId,inId","?1,?2");
	plugindb_prepDelete(palettePlugin, &deleteSampleStopInStmt, SAMPLE_STOP_IN,
						"nodeId=?1");
	
	//plugininit_createTable(palettePlugin, RGB_OUT, RGB_OUT_COLS);
	//plugininit_createIndex(palettePlugin, "RgbOutIdx", RGB_OUT, "nodeId,outIdx");
	//plugindb_prepSelect(palettePlugin, &selectRGBOutStmt, RGB_OUT,
	
	plugininit_createTable(palettePlugin, MANUAL_STOP, MANUAL_STOP_COLS);
	plugininit_createIndex(palettePlugin, "ManualStopIdx", MANUAL_STOP, "nodeId,outIdx");
	plugindb_prepSelect(palettePlugin, &selectManualStopStmt, MANUAL_STOP,
						"outId,outIdx,pos","nodeId=?1");
	plugindb_prepInsert(palettePlugin, &insertManualStopStmt, MANUAL_STOP,
						"nodeId,outIdx,pos","?1,?2,?3");
	plugindb_prepUpdate(palettePlugin, &updateManualStopPosStmt, MANUAL_STOP,
						"pos=?2","outId=?1");
	plugindb_prepDelete(palettePlugin, &deleteManualStopStmt, MANUAL_STOP,
						"nodeId=?1");
    
    return 0;
	
}

int deletePaletteInst(int instId){
	plugindb_reset(palettePlugin,deleteNodeSettingStmt);
	plugindb_bind_int(palettePlugin,deleteNodeSettingStmt,1,instId);
	plugindb_step(palettePlugin,deleteNodeSettingStmt);
	
	plugindb_reset(palettePlugin,deleteSelectInStmt);
	plugindb_bind_int(palettePlugin,deleteSelectInStmt,1,instId);
	plugindb_step(palettePlugin,deleteSelectInStmt);
	
	plugindb_reset(palettePlugin,deleteSampleStopInStmt);
	plugindb_bind_int(palettePlugin,deleteSampleStopInStmt,1,instId);
	plugindb_step(palettePlugin,deleteSampleStopInStmt);
	
	plugindb_reset(palettePlugin,deleteManualStopStmt);
	plugindb_bind_int(palettePlugin,deleteManualStopStmt,1,instId);
	plugindb_step(palettePlugin,deleteManualStopStmt);
	
	return 0;
}

