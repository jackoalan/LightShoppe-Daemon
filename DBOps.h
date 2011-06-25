/*
 **    This file is part of LightShoppe. Copyright 2011 Jack Andersen
 **
 **    LightShoppe is free software: you can redistribute it
 **    and/or modify it under the terms of the GNU General
 **    Public License as published by the Free Software
 **    Foundation, either version 3 of the License, or (at your
 **    option) any later version.
 **
 **    LightShoppe is distributed in the hope that it will
 **    be useful, but WITHOUT ANY WARRANTY; without even the
 **    implied warranty of MERCHANTABILITY or FITNESS FOR A
 **    PARTICULAR PURPOSE.  See the GNU General Public License
 **    for more details.
 **
 **    You should have received a copy of the GNU General
 **    Public License along with LightShoppe.  If not, see
 **    <http://www.gnu.org/licenses/>.
 **
 **    @author Jack Andersen <jackoalan@gmail.com>
 */


/**
  * Core database operations for Node architecture
  */

#ifndef DBOPS_H
#define DBOPS_H

#include <sqlite3.h>
#include <evhttp.h>

#include "Node.h"
#include "cJSON.h"
#include "PluginAPI.h"

enum RGBOPT
{
    SINGLE,
    RED,
    GREEN,
    BLUE
};

struct LSD_RGB
{
    double r;
    double g;
    double b;
};

struct LSD_Tuple
{
    double x;
    double y;
};


/**
  * Each SceneDBStmt stores a pointer to the plugin
  *structure that is verified against the 'key' provided by
  *the plugin accessing the statement.
  */
struct LSD_SceneDBStmt
{
    struct LSD_ScenePlugin const* pluginPtr;
    sqlite3_stmt* stmt;
};

void
destruct_SceneDBStmt (void* dbStmt);


int
lsddb_openDB (const char* path);


int
lsddb_saveDB (const char* origPath);


int
lsddb_autoSaveDB (const char* origPath);


int
lsddb_emptyDB ();


int
lsddb_closeDB ();


int
lsddb_initDB ();


int
lsddb_resetDB ();


/* Called by partition creator and facade class to create
 * node patch spaces */
int
lsddb_createPatchSpace (const char* name, int* idBinding, int parentPatchSpace);


int
lsddb_removePatchSpace (int psid);


int
lsddb_updatePatchSpaceName (int psId, const char* name);


int
lsddb_setPatchSpaceColour (int psId, cJSON* colour);


int
lsddb_createPatchSpaceIn (int patchSpaceId, const char* name, int* idBinding);


int
lsddb_createPatchSpaceOut (int patchSpaceId, const char* name, int* idBinding);


int
lsddb_removePatchSpaceOut (int outId);


int
lsddb_removePatchSpaceIn (int inId);


int
lsddb_updatePatchSpaceInName (int inId, const char* name);


int
lsddb_updatePatchSpaceOutName (int outId, const char* name);


/* May only be called in a series immediately before a state
 * reload */
int
lsddb_createPartition (const char* name, int* idBinding);


int
lsddb_updatePartitionImage (int partId, const char* imageUrl);


int
lsddb_removePartition (int partId);


int
lsddb_updatePartitionName (int partId, const char* name);


int
lsddb_addNodeClass (struct LSD_SceneNodeClass** ptrToBind,
                    int pluginId,
                    const char* name,
                    const char* desc,
                    int classIdx);


int
lsddb_addDataType (int* ptrToBind, int pluginId, const char* name,
                   const char* desc);


int
lsddb_structUnivArr ();


int
lsddb_structChannelArr ();


/* A kind of array structure bootstrapping function */
int
lsddb_structPartitionArr ();


/* May only be called in a series immediately before a state
 * reload */
int
lsddb_deletePartition (int partId);


int
lsddb_addPatchChannel (int partId, cJSON* opts);


int
lsddb_updatePatchChannel (int chanId, cJSON* opts);


int
lsddb_deletePatchChannel (int chanId);


int
lsddb_addNodeInstInput (struct LSD_SceneNodeInst const* node,
                        int typeId,
                        const char* name,
                        struct LSD_SceneNodeInput** ptrToBind,
                        int* idBinding);


int
lsddb_addNodeInstOutput (struct LSD_SceneNodeInst const* node,
                         int typeId,
                         const char* name,
                         struct LSD_SceneNodeOutput** ptrToBind,
                         int* idBinding,
                         int bfFuncIdx,
                         int bpFuncIdx);


int
lsddb_removeNodeInstInput (int inputId);


int
lsddb_removeNodeInstOutput (int outputId);


int
lsddb_addNodeInst (int patchSpaceId, struct LSD_SceneNodeClass* nc,
                   int* idBinding, struct LSD_SceneNodeInst** ptrToBind);


int
lsddb_removeNodeInst (int nodeId);


int
lsddb_updateNodeName (int nodeId, const char* name);


int
lsddb_addFacadeNodeInst (int childPatchSpaceId,
                         int* idBinding,
                         int parentPatchSpace);


int
lsddb_indexHtmlGen (const char* pluginsDir, struct evbuffer* target);


int
lsddb_jsonPlugins (cJSON* target);


int
lsddb_disablePlugin (int pluginId);


int
lsddb_enablePlugin (int pluginId);


/* This function type is defined to enable secure passing */
/* of a function to retreive the head, rather than the head */
/* itself. */
typedef const struct LSD_ScenePluginHEAD* ( *ghType )(void);

int
lsddb_pluginHeadLoader (ghType phGet,
                        int enable,
                        const char* parentDirectoryName,
                        const char* pluginSha,
                        void* dlObj);


int
lsddb_resolvePluginFromNodeId (struct LSD_ScenePlugin** pluginBind, int nodeId);


int
lsddb_setNodeColour (int nodeId, cJSON* colour);


int
lsddb_nodeInstPos (int nodeId, int x, int y);


int
lsddb_facadeInstPos (int nodeId, int xVal, int yVal);


int
lsddb_panPatchSpace (int psId, int xVal, int yVal, double scale);


int
lsddb_nodeInstComment (int nodeID, const char* comment);


int
lsddb_removeNodeInst (int nodeId);


int
lsddb_wireNodes (int srcFacadeInt,
                 int srcId,
                 int destFacadeInt,
                 int destId,
                 int* idBinding);


int
lsddb_unwireNodes (int wireID);


int
lsddb_jsonClassLibrary (cJSON* target);


int
lsddb_resolveClassFromId (struct LSD_SceneNodeClass** ptrToBind, int classId);


int
lsddb_resolveInstFromId (struct LSD_SceneNodeInst const** target,
                         int nodeId,
                         void** dataBind);


int
lsddb_resolveInstFromInId (struct LSD_SceneNodeInst const** target, int inId);


int
lsddb_resolveInstFromOutId (struct LSD_SceneNodeInst const** target, int outId);


int
lsddb_jsonNodes (int patchSpaceId, cJSON* target);


int
lsddb_jsonFacade (int psId, cJSON* resp);


int
lsddb_jsonWires (int patchSpaceId, cJSON* target);


int
lsddb_jsonPatchSpace (int patchSpaceId, cJSON* resp);


int
lsddb_jsonParts (cJSON* target);


int
lsddb_jsonPlugins (cJSON* target);


int
lsddb_enablePlugin (int plugId);


int
lsddb_disablePlugin (int plugId);


int
lsddb_purgePlugin (int plugId);


int
lsddb_structPluginArr ();


int
lsddb_destructPluginArr ();


int
lsddb_resolveInputFromId (struct LSD_SceneNodeInput** inBind, int inId);


int
lsddb_getPatchChannels (cJSON* target);


int
lsddb_addPatchChannel (int partId, cJSON* opts);


/* Plugin API stuff */
int
lsddbapi_createTable (int pluginId, const char* subName, const char* colDefs);


int
lsddbapi_createIndex (int pluginId,
                      const char* idxName,
                      const char* subName,
                      const char* colDefs);


int
lsddbapi_prepSelect (struct LSD_ScenePlugin const* pluginObj,
                     unsigned int* stmtIdxBind,
                     const char* tblName,
                     const char* colPortion,
                     const char* wherePortion);


int
lsddbapi_prepInsert (struct LSD_ScenePlugin const* pluginObj,
                     unsigned int* stmtIdxBind,
                     const char* tblName,
                     const char* colPortion,
                     const char* valuesPortion);


int
lsddbapi_prepUpdate (struct LSD_ScenePlugin const* pluginObj,
                     unsigned int* stmtIdxBind,
                     const char* tblName,
                     const char* setPortion,
                     const char* wherePortion);


int
lsddbapi_prepDelete (struct LSD_ScenePlugin const* pluginObj,
                     unsigned int* stmtIdxBind,
                     const char* tblName,
                     const char* wherePortion);


int
lsddbapi_stmtReset (struct LSD_ScenePlugin const* plugin,
                    unsigned int stmtIdx,
                    int* result);


int
lsddbapi_stmtStep (struct LSD_ScenePlugin const* plugin,
                   unsigned int stmtIdx,
                   int* result);


int
lsddbapi_stmtBindDouble (struct LSD_ScenePlugin const* plugin,
                         unsigned int stmtIdx,
                         unsigned int sqlIdx,
                         double data,
                         int* result);


int
lsddbapi_stmtBindInt (struct LSD_ScenePlugin const* plugin,
                      unsigned int stmtIdx,
                      unsigned int sqlIdx,
                      int data,
                      int* result);


int
lsddbapi_stmtBindInt64 (struct LSD_ScenePlugin const* plugin,
                        unsigned int stmtIdx,
                        unsigned int sqlIdx,
                        sqlite3_int64 data,
                        int* result);


int
lsddbapi_stmtBindNull (struct LSD_ScenePlugin const* plugin,
                       unsigned int stmtIdx,
                       unsigned int sqlIdx,
                       int* result);


int lsddbapi_stmtBindText (struct LSD_ScenePlugin const* plugin,
                           unsigned int stmtIdx,
                           unsigned int sqlIdx,
                           const char* data,
                           int length,
                           void ( *destructor )(void*),
                           int* result);

int lsddbapi_stmtBindText16 (struct LSD_ScenePlugin const* plugin,
                             unsigned int stmtIdx,
                             unsigned int sqlIdx,
                             const void* data,
                             int length,
                             void ( *destructor )(void*),
                             int* result);

int
lsddbapi_stmtColDouble (struct LSD_ScenePlugin const* plugin,
                        unsigned int stmtIdx,
                        unsigned int colIdx,
                        double* result);


int
lsddbapi_stmtColInt (struct LSD_ScenePlugin const* plugin, unsigned int stmtIdx,
                     unsigned int colIdx, int* result);


int
lsddbapi_stmtColInt64 (struct LSD_ScenePlugin const* plugin,
                       unsigned int stmtIdx,
                       unsigned int colIdx,
                       sqlite3_int64* result);


int
lsddbapi_stmtColText (struct LSD_ScenePlugin const* plugin,
                      unsigned int stmtIdx,
                      unsigned int colIdx,
                      const unsigned char** result);


int
lsddbapi_stmtColText16 (struct LSD_ScenePlugin const* plugin,
                        unsigned int stmtIdx,
                        unsigned int colIdx,
                        const void** result);


int
lsddbapi_getLastInsertRowId ();


#endif /* DBOPS_H */
