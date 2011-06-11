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


#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#include "PluginAPICore.h"
#include "PluginAPI.h"
#include "DBOps.h"
#include "NodeInstAPI.h"

static const int DB_MUL = 50;

static enum API_STATE apistate;

void destruct_ScenePlugin(void* plugin){
    if(plugin){
        struct LSD_ScenePlugin* castPlugin = plugin;
        if(castPlugin->cleanupFunc){
            castPlugin->cleanupFunc(castPlugin);
        }
		if(castPlugin->dlObj){
			dlclose(castPlugin->dlObj);
			printf("Closed Plugin SO\n");
		}
    }
}

// State setter
int lsdapi_setState(enum API_STATE state){
    apistate = state;
    return 0;
}



// Object construction functions

int plugininit_registerNodeClass(struct LSD_ScenePlugin const * key,
                                 struct LSD_SceneNodeClass** ptrToBind,
                                 int(*nodeMakeFunc)(struct LSD_SceneNodeInst const *, void* instData),
								 int(*nodeRestoreFunc)(struct LSD_SceneNodeInst const *, void* instData),
                                 void(*nodeCleanFunc)(struct LSD_SceneNodeInst const *, void* instData),
                                 void(*nodeDeleteFunc)(struct LSD_SceneNodeInst const *, void* instData),
                                 size_t nodeDataSize,
                                 const char* name, const char* desc,
								 int classIdx,
								 bfFunc* bfFuncTbl, bpFunc* bpFuncTbl){
    if(apistate != STATE_PINIT)
        return -10;
    
    if(!key || !ptrToBind || !nodeMakeFunc || !nodeRestoreFunc || !nodeCleanFunc || !nodeDeleteFunc || !name){
        fprintf(stderr,"Improper use of registerNodeClass(), all args must be filled\n");
        return -1;
    }
    
    struct LSD_SceneNodeClass* tempClass;
    
    // Backend function
    if(lsddb_addNodeClass(&tempClass,key->dbId,name,desc,classIdx)<0){
        fprintf(stderr,"Error while running DB and array portion of registerNodeClass()\n");
        return -1;
    }
    
	tempClass->plugin = key;
    tempClass->nodeMakeFunc = nodeMakeFunc;
	tempClass->nodeRestoreFunc = nodeRestoreFunc;
    tempClass->nodeCleanFunc = nodeCleanFunc;
	tempClass->nodeDeleteFunc = nodeDeleteFunc;
    tempClass->instDataSize = nodeDataSize;
	tempClass->bfFuncTbl = bfFuncTbl;
	tempClass->bpFuncTbl = bpFuncTbl;
    
    *ptrToBind = tempClass;
    
    return 0;
}

int plugininit_registerDataType(struct LSD_ScenePlugin const * key,
                                int* ptrToBind,
                                const char* name, const char* desc){
    if(apistate != STATE_PINIT)
        return -10;
    
    if(!key || !name){
        fprintf(stderr,"Improper use of registerDataType()\n");
        return -1;
    }
    
    
    int typeId;
    
    // Backend function
    if(lsddb_addDataType(&typeId,key->dbId,name,desc)<0){
        fprintf(stderr,"Error while running DB and Array portion of registerDataType()\n");
        return -1;
    }
    
    
    *ptrToBind = typeId;
    
    return 0;

}


struct LSD_SceneNodeInst const * plugin_getInstById(struct LSD_ScenePlugin const * key,int nodeId,void** dataBind){
	struct LSD_SceneNodeInst const * inst;
	if(lsddb_resolveInstFromId(&inst, nodeId, dataBind) == 0){
		if(inst->nodeClass->plugin == key){
			return inst;
		}
		fprintf(stderr,"Inst request failed ownership test\n");
	}
	return NULL;
}


// Initialiser's database api

int plugininit_createTable(struct LSD_ScenePlugin const * key,
                           const char* name,
                           const char* coldef){
    if(apistate != STATE_PINIT)
        return -10;
    
    if(lsddbapi_createTable(key->dbId, name, coldef)<0){
        fprintf(stderr,"error while creating table in plugin API\n");
        return -1;
    }
    
    return 0;
}



// Database statement Prep

int plugindb_prepSelect(struct LSD_ScenePlugin const * key,
                        unsigned int* stmtBinding,
                        const char* tblName,
                        const char* colPortion,
                        const char* wherePortion){
    if(apistate != STATE_PINIT)
        return -10;
    
    if(lsddbapi_prepSelect(key, stmtBinding, 
                           tblName, colPortion, wherePortion)<0){
        fprintf(stderr,"Error while prepping SELECT in plugin API\n");
        return -1;
    }
    
    return 0;

}

int plugindb_prepInsert(struct LSD_ScenePlugin const * key,
                        unsigned int* stmtBinding,
                        const char* tblName,
                        const char* colPortion,
                        const char* valuesPortion){
    if(apistate != STATE_PINIT)
        return -10;
    
    if(lsddbapi_prepInsert(key, stmtBinding, 
                           tblName, colPortion, valuesPortion)<0){
        fprintf(stderr,"Error while prepping INSERT in plugin API\n");
        return -1;
    }
    
    return 0;

}

int plugindb_prepUpdate(struct LSD_ScenePlugin const * key,
                        unsigned int* stmtBinding,
                        const char* tblName,
                        const char* setPortion,
                        const char* wherePortion){
    if(apistate != STATE_PINIT)
        return -10;
    
    if(lsddbapi_prepUpdate(key, stmtBinding, 
                           tblName, setPortion, wherePortion)<0){
        fprintf(stderr,"Error while prepping UPDATE in plugin API\n");
        return -1;
    }
    
    return 0;

}

int plugindb_prepDelete(struct LSD_ScenePlugin const * key,
                        unsigned int* stmtBinding,
                        const char* tblName,
                        const char* wherePortion){
    if(apistate != STATE_PINIT)
        return -10;
    
    if(lsddbapi_prepDelete(key, stmtBinding, tblName, wherePortion)<0){
        fprintf(stderr,"Error while prepping DELETE in plugin API\n");
        return -1;
    }
    
    return 0;

}

// Database Control Flow

int plugindb_reset(struct LSD_ScenePlugin const * key, unsigned int stmtBinding){
    
	int result;
    if(lsddbapi_stmtReset(key, stmtBinding, &result)<0){
        fprintf(stderr,"Unable to reset stmt\n");
        return -1;
    }
    
    return result;
}

int plugindb_step(struct LSD_ScenePlugin const * key, unsigned int stmtBinding){
	
	int result;
	if(lsddbapi_stmtStep(key, stmtBinding, &result)<0){
        fprintf(stderr,"Unable to step stmt\n");
        return -1;
    }
	
    return result;
}

// Database binding functions


int plugindb_bind_double(struct LSD_ScenePlugin const * key, unsigned int stmtBinding,
                         unsigned int sqlBinding, double data){
	
	int result;
	if(lsddbapi_stmtBindDouble(key, stmtBinding, sqlBinding, data, &result)<0){
        fprintf(stderr,"Unable to bind double to stmt\n");
        return -1;
    }
	
    return result;
}

int plugindb_bind_int(struct LSD_ScenePlugin const * key, unsigned int stmtBinding,
                      unsigned int sqlBinding, int data){
	
	int result;
	if(lsddbapi_stmtBindInt(key, stmtBinding, sqlBinding, data, &result)<0){
        fprintf(stderr,"Unable to bind int to stmt\n");
        return -1;
    }
	
    return result;
}

int plugindb_bind_int64(struct LSD_ScenePlugin const * key, unsigned int stmtBinding,
                        unsigned int sqlBinding, sqlite3_int64 data){
	
	int result;
	if(lsddbapi_stmtBindInt64(key, stmtBinding, sqlBinding, data, &result)<0){
        fprintf(stderr,"Unable to bind int64 to stmt\n");
        return -1;
    }
	
    return result;
}

int plugindb_bind_null(struct LSD_ScenePlugin const * key, unsigned int stmtBinding,
                       unsigned int sqlBinding){
	
	int result;
	if(lsddbapi_stmtBindNull(key, stmtBinding, sqlBinding, &result)<0){
        fprintf(stderr,"Unable to bind null to stmt\n");
        return -1;
    }
	
    return result;
}

int plugindb_bind_text(struct LSD_ScenePlugin const * key, unsigned int stmtBinding,
                       unsigned int sqlBinding, const char* data, int length,
                       void(*destructor)(void*)){
	
	int result;
	if(lsddbapi_stmtBindText(key, stmtBinding, sqlBinding, data, length, destructor, &result)<0){
        fprintf(stderr,"Unable to bind text to stmt\n");
        return -1;
    }
	
    return result;
}

int plugindb_bind_text16(struct LSD_ScenePlugin const * key, unsigned int stmtBinding,
                         unsigned int sqlBinding, const void* data, int length,
                         void(*destructor)(void*)){
	
	int result;
	if(lsddbapi_stmtBindText16(key, stmtBinding, sqlBinding, data, length, destructor, &result)<0){
        fprintf(stderr,"Unable to bind text16 to stmt\n");
        return -1;
    }
	
    return result;
}


// Database column return functions

double plugindb_column_double(struct LSD_ScenePlugin const * key, unsigned int stmtBinding,
                              int colIdx){
	
	double result;
	if(lsddbapi_stmtColDouble(key, stmtBinding, colIdx, &result)<0){
        fprintf(stderr,"Unable to get column from stmt\n");
        return 0.0;
    }
	
    return result;
}

int plugindb_column_int(struct LSD_ScenePlugin const * key, unsigned int stmtBinding,
                        int colIdx){
	
	int result;
	if(lsddbapi_stmtColInt(key, stmtBinding, colIdx, &result)<0){
        fprintf(stderr,"Unable to get column from stmt\n");
        return 0;
    }
	
    return result;
}

sqlite3_int64 plugindb_column_int64(struct LSD_ScenePlugin const * key, unsigned int stmtBinding,
                                    int colIdx){
	
	sqlite3_int64 result;
	if(lsddbapi_stmtColInt64(key, stmtBinding, colIdx, &result)<0){
        fprintf(stderr,"Unable to get column from stmt\n");
        return 0;
    }
	
    return result;
}

const unsigned char* plugindb_column_text(struct LSD_ScenePlugin const * key,
                                          unsigned int stmtBinding, int colIdx){
	
	const unsigned char* result;
	if(lsddbapi_stmtColText(key, stmtBinding, colIdx, &result)<0){
        fprintf(stderr,"Unable to get column from stmt\n");
        return NULL;
    }
	
    return result;
}

const void* plugindb_column_text16(struct LSD_ScenePlugin const * key, unsigned int stmtBinding,
                                   int colIdx){
	
	const void* result;
	if(lsddbapi_stmtColText16(key, stmtBinding, colIdx, &result)<0){
        fprintf(stderr,"Unable to get column from stmt\n");
        return NULL;
    }
	
    return result;
}

