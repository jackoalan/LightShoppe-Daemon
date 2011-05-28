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
                                 int(*nodeInitFunc)(struct LSD_SceneNodeInst const *, void* instData),
								 int(*nodeRestoreFunc)(struct LSD_SceneNodeInst const *, void* instData),
                                 void(*nodeCleanFunc)(struct LSD_SceneNodeInst const *, void* instData),
                                 size_t nodeDataSize,
                                 const char* name, const char* desc,
								 const char* nodeConfJSFunc,
								 bfFunc* bfFuncTbl, bpFunc* bpFuncTbl){
    if(apistate != STATE_PINIT)
        return -10;
    
    if(!key || !ptrToBind || !nodeInitFunc || !nodeCleanFunc || !name){
        fprintf(stderr,"Improper use of registerNodeClass(), all args must be filled\n");
        return -1;
    }
    
    struct LSD_SceneNodeClass* tempClass;
    
    // Backend function
    if(lsddb_addNodeClass(&tempClass,key->dbId,name,desc,nodeConfJSFunc)<0){
        fprintf(stderr,"Error while running DB and array portion of registerNodeClass()\n");
        return -1;
    }
    
    tempClass->nodeInitFunc = nodeInitFunc;
	tempClass->nodeRestoreFunc = nodeRestoreFunc;
    tempClass->nodeCleanFunc = nodeCleanFunc;
    tempClass->instDataSize = nodeDataSize;
	tempClass->bfFuncTbl = bfFuncTbl;
	tempClass->bpFuncTbl = bpFuncTbl;
    
    *ptrToBind = tempClass;
    
    return 0;
}

int plugininit_registerDataType(struct LSD_ScenePlugin const * key,
                                int* ptrToBind,
                                unsigned short int isArray,
                                const char* name, const char* desc){
    if(apistate != STATE_PINIT)
        return -10;
    
    if(!key || !name){
        fprintf(stderr,"Improper use of registerDataType()\n");
        return -1;
    }
    
    
    int typeId;
    
    // Backend function
    if(lsddb_addDataType(&typeId,key->dbId,name,desc,isArray)<0){
        fprintf(stderr,"Error while running DB and Array portion of registerDataType()\n");
        return -1;
    }
    
    
    *ptrToBind = typeId;
    
    return 0;

}





// Initialiser's database api

int plugininit_createTable(struct LSD_ScenePlugin const * key,
                           const char* name,
                           const char* coldef){
    if(apistate != STATE_PINIT)
        return -10;
    return 0;

}


int plugininit_createIndex(struct LSD_ScenePlugin const * key,
                           const char* name,
                           const char* tblName,
                           const char* coldef){
    if(apistate != STATE_PINIT)
        return -10;
    
    return 0;

}

int pluginclean_dropTable(struct LSD_ScenePlugin const * key,
                          const char* tblName){
    if(apistate != STATE_PCLEAN)
        return -10;
    
    return 0;

}

// Database statement Prep

int plugindb_prepSelect(struct LSD_ScenePlugin const * key,
                        unsigned int* stmtBinding,
                        const char* tblName,
                        const char* wherePortion){
    if(apistate != STATE_PINIT)
        return -10;
    return 0;

}

int plugindb_prepInsert(struct LSD_ScenePlugin const * key,
                        unsigned int* stmtBinding,
                        const char* tblName,
                        const char* valuesPortion){
    if(apistate != STATE_PINIT)
        return -10;
    
    return 0;

}

int plugindb_prepUpdate(struct LSD_ScenePlugin const * key,
                        unsigned int* stmtBinding,
                        const char* tblName,
                        const char* setPortion){
    if(apistate != STATE_PINIT)
        return -10;
    
    return 0;

}

int plugindb_prepDelete(struct LSD_ScenePlugin const * key,
                        unsigned int* stmtBinding,
                        const char* tblName,
                        const char* wherePortion){
    if(apistate != STATE_PINIT)
        return -10;
    
    return 0;

}

// Database Control Flow

int plugindb_reset(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding){
    return 0;

}

int plugindb_step(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding){
    return 0;

}

// Database binding functions

int plugindb_bind_blob(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                       int sqlBinding, const void* data, int length,
                       void(*destructor)(void*)){
    return 0;

}

int plugindb_bind_double(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                         int sqlBinding, double data){
    return 0;

}

int plugindb_bind_int(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                      int sqlBinding, int data){
    return 0;

}

int plugindb_bind_int64(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                        int sqlBinding, sqlite3_int64 data){
    return 0;

}

int plugindb_bind_null(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                       int sqlBinding){
    return 0;

}

int plugindb_bind_text(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                       int sqlBinding, const char* data, int length,
                       void(*destructor)(void*)){
    return 0;

}

int plugindb_bind_text16(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                         int sqlBinding, const void* data, int length,
                         void(*destructor)(void*)){
    return 0;

}

// Used to free memory associated with prepared statements
int plugindb_finalizeStmt(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding){
    if(apistate != STATE_PCLEAN)
        return -10;
    return 0;

}

// Database column return functions

/*const void* plugindb_column_blob(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                                 int colIdx){
    return 0;

}*/

int plugindb_column_bytes(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                          int colIdx){
    return 0;

}

int plugindb_column_bytes16(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                            int colIdx){
    return 0;

}

double plugindb_column_double(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                              int colIdx){
    return 0;

}

int plugindb_column_int(struct LSD_ScenePlugin const * key,unsigned int* stmtBinding,
                        int colIdx){
    return 0;

}

sqlite3_int64 plugindb_column_int64(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                                    int colIdx){
    return 0;

}

/*const unsigned char* plugindb_column_text(struct LSD_ScenePlugin const * key,
                                          unsigned int* stmtBinding,
                                          int colIdx){

}*/

/*const void* plugindb_column_text16(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                                   int colIdx){
    
}*/

