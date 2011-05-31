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



#ifndef PLUGINAPI_H
#define PLUGINAPI_H

#include <sqlite3.h>
#include "cJSON.h"

#include "Node.h"


/**
 * LSD_ScenePlugin
 * 
 * Semi-private data structure where data is const for ALL plugin
 * implementations. This structure is pointer-kept by plugin's static
 * members and behaves as a "key" to authenticate and identify plugin
 * operations.
 */
struct LSD_ScenePlugin {
    int dbId;
    void(*cleanupFunc)(struct LSD_ScenePlugin const * plugin);
    void(*handleRPC)(cJSON* in, cJSON* out);
	void* dlObj;
};

void destruct_ScenePlugin(void* plugin);

/**
 * LSD_ScenePluginHEAD
 *
 * The head structure defines a static singleton that must be present in 
 * every plugin's shared object. Its two members are function pointers for
 * plugin initialisation and cleanup.
 *
 * The initialisation is ran at LSD startup when plugins are loaded.
 * It should register classes, their plugs, types, and perform any necessary
 * database operations to get up and going.
 *
 * The cleanup function will do anyhting it needs to in order to free up memory
 * and clean up or commit temporary database entries. And is called on partition
 * deallocation (From a restart or shutdown)
 */
struct LSD_ScenePluginHEAD {
    int(*initFunc)(struct LSD_ScenePlugin const * plugin, cJSON const * confjson);
    void(*cleanupFunc)(struct LSD_ScenePlugin const * plugin);
	void(*handler)(cJSON* in, cJSON* out);
    const char* pluginDomain;
    const char* pluginVersion;
};



// Object construction functions

int plugininit_registerNodeClass(struct LSD_ScenePlugin const * key,
                                 struct LSD_SceneNodeClass** ptrToBind,
                                 int(*nodeInitFunc)(struct LSD_SceneNodeInst const *, void* instData),
								 int(*nodeRestoreFunc)(struct LSD_SceneNodeInst const *, void* instData),
                                 void(*nodeCleanFunc)(struct LSD_SceneNodeInst const *, void* instData),
                                 size_t nodeDataSize,
                                 const char* name, const char* desc,
								 int classIdx,
								 bfFunc* bfFuncTbl, bpFunc* bpFuncTbl);

int plugininit_registerDataType(struct LSD_ScenePlugin const * key,
                                int* ptrToBind,
                                unsigned short int isArray,
                                const char* name, const char* desc);




// Initialiser's database api

int plugininit_createTable(struct LSD_ScenePlugin const * key,
                                  const char* name,
                                  const char* coldef);


int plugininit_createIndex(struct LSD_ScenePlugin const * key,
                                  const char* name,
                                  const char* tblName,
                                  const char* coldef);

int pluginclean_dropTable(struct LSD_ScenePlugin const * key,
                                 const char* tblName);

// Database statement Prep

int plugininit_prepSelect(struct LSD_ScenePlugin const * key,
                               unsigned int* stmtBinding,
                               const char* tblName,
                               const char* wherePortion);

int plugininit_prepInsert(struct LSD_ScenePlugin const * key,
                               unsigned int* stmtBinding,
                               const char* tblName,
                               const char* valuesPortion);

int plugininit_prepUpdate(struct LSD_ScenePlugin const * key,
                               unsigned int* stmtBinding,
                               const char* tblName,
                               const char* setPortion);

int plugininit_prepDelete(struct LSD_ScenePlugin const * key,
                               unsigned int* stmtBinding,
                               const char* tblName,
                               const char* wherePortion);

// Database Flow Control

int plugindb_reset(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding);

int plugindb_step(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding);

// Database binding functions

int plugindb_bind_blob(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                              int sqlBinding, const void* data, int length,
                              void(*destructor)(void*));

int plugindb_bind_double(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                              int sqlBinding, double data);

int plugindb_bind_int(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                                int sqlBinding, int data);

int plugindb_bind_int64(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                             int sqlBinding, sqlite3_int64 data);

int plugindb_bind_null(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                              int sqlBinding);

int plugindb_bind_text(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                              int sqlBinding, const char* data, int length,
                              void(*destructor)(void*));

int plugindb_bind_text16(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                                int sqlBinding, const void* data, int length,
                                void(*destructor)(void*));

// Used to free memory associated with prepared statements
int plugindb_finalizeStmt(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding);

// Database column return functions

const void* plugindb_column_blob(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                                int colIdx);

int plugindb_column_bytes(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                                 int colIdx);

int plugindb_column_bytes16(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                                   int colIdx);

double plugindb_column_double(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                                  int colIdx);

int plugindb_column_int(struct LSD_ScenePlugin const * key,unsigned int* stmtBinding,
                               int colIdx);

sqlite3_int64 plugindb_column_int64(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                                           int colIdx);

const unsigned char* plugindb_column_text(struct LSD_ScenePlugin const * key,
                                                 unsigned int* stmtBinding,
                                                 int colIdx);

const void* plugindb_column_text16(struct LSD_ScenePlugin const * key, unsigned int* stmtBinding,
                                          int colIdx);

#endif // PLUGINAPI_H
