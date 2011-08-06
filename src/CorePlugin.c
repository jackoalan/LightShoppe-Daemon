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

/* This is a specially privledged plugin for the purpose of
 * providing */
/* various built-in classes and types */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h> /* For constants */

#include "PluginAPI.h"
#include "NodeInstAPI.h"
#include "DBOps.h"
#include "CorePlugin.h"
#include "cJSON.h"
#include "Node.h"
#include "Logging.h"

/* Gettext stuff */
#ifndef HW_RVL
#include <libintl.h>
#define _(String) gettext (String)
#else
#define _(String) String
#endif

/* Name of this component for logging */
static const char LOG_COMP[] = "CorePlugin.c";

static struct LSD_ScenePlugin const* corePlugin;

/* ******  Types  ****** */

static int rgbType;
static int integerType;
static int floatType;
static int triggerType;

/* Type accessors */
int
core_getRGBTypeID ()
{
    return rgbType;
}


int
core_getIntegerTypeID ()
{
    return integerType;
}


int
core_getFloatTypeID ()
{
    return floatType;
}


int
core_getTriggerTypeID ()
{
    return triggerType;
}


/* ****** Classes ****** */

/****** INT GEN STUFF ******/

static struct LSD_SceneNodeClass* intGenClass;

/* Stmts */
static unsigned int intGenSelectStmt;
static unsigned int intGenInsertStmt;
static unsigned int intGenUpdateStmt;
static unsigned int intGenDeleteStmt;

/* Plug funcs */
void
intGenBufferOut (struct LSD_SceneNodeOutput const* output)
{
    /* Nothing to do */
}


void*
intGenPtrOut (struct LSD_SceneNodeOutput const* output)
{
    return output->parentNode->data;
}


/* Plug func tables */
static const bfFunc intGenBfFuncs[] =
{intGenBufferOut};

static const bpFunc intGenBpFuncs[] =
{intGenPtrOut};

int
intGenMake (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugininst_addInstOutput (inst, integerType, "Int Out", 0, 0, NULL);

    plugindb_reset (corePlugin, intGenInsertStmt);
    plugindb_bind_int (corePlugin, intGenInsertStmt, 1, inst->dbId);
    if (plugindb_step (corePlugin, intGenInsertStmt) != SQLITE_DONE)
        return -1;
    return 0;
}


int
intGenRestore (struct LSD_SceneNodeInst const* inst, void* instData)
{
    int* castData = (int*)instData;
    plugindb_reset (corePlugin, intGenSelectStmt);
    plugindb_bind_int (corePlugin, intGenSelectStmt, 1, inst->dbId);
    if (plugindb_step (corePlugin, intGenSelectStmt) == SQLITE_ROW)
    {
        *castData = plugindb_column_int (corePlugin, intGenSelectStmt, 0);
        return 0;
    }

    return -1;
}


void
intGenClean (struct LSD_SceneNodeInst const* inst, void* instData)
{
    /* Nothing to do (LightShoppe handles the freeing of
     * instData) */
}


void
intGenDelete (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugindb_reset (corePlugin, intGenDeleteStmt);
    plugindb_bind_int (corePlugin, intGenDeleteStmt, 1, inst->dbId);
    plugindb_step (corePlugin, intGenDeleteStmt);
}


/****** INT VIEW STUFF ******/

static struct LSD_SceneNodeClass* intViewClass;

/* Stmts */
static unsigned int intViewSelectStmt;
static unsigned int intViewInsertStmt;
static unsigned int intViewDeleteStmt;

int
intViewMake (struct LSD_SceneNodeInst const* inst, void* instData)
{
    int inId;
    plugininst_addInstInput (inst, integerType, "Int In", &inId);

    plugindb_reset (corePlugin, intViewInsertStmt);
    plugindb_bind_int (corePlugin, intViewInsertStmt, 1, inst->dbId);
    plugindb_bind_int (corePlugin, intViewInsertStmt, 2, inId);
    plugindb_step (corePlugin, intViewInsertStmt);

    return 0;
}


int
intViewRestore (struct LSD_SceneNodeInst const* inst, void* instData)
{
    /* Nothing to do */
    return 0;
}


void
intViewClean (struct LSD_SceneNodeInst const* inst, void* instData)
{
    /* Nothing to do */
}


void
intViewDelete (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugindb_reset (corePlugin, intViewDeleteStmt);
    plugindb_bind_int (corePlugin, intViewDeleteStmt, 1, inst->dbId);
    plugindb_step (corePlugin, intViewDeleteStmt);
}


/****** FLOAT GEN STUFF ******/

static struct LSD_SceneNodeClass* floatGenClass;

/* Stmts */
static unsigned int floatGenSelectStmt;
static unsigned int floatGenInsertStmt;
static unsigned int floatGenUpdateStmt;
static unsigned int floatGenDeleteStmt;

/* Plug funcs */
void
floatGenBufferOut (struct LSD_SceneNodeOutput const* output)
{
    /* Nothing to do */
}


void*
floatGenPtrOut (struct LSD_SceneNodeOutput const* output)
{
    return output->parentNode->data;
}


/* Plug func tables */
static const bfFunc floatGenBfFuncs[] =
{floatGenBufferOut};

static const bpFunc floatGenBpFuncs[] =
{floatGenPtrOut};

int
floatGenMake (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugininst_addInstOutput (inst, floatType, "Float Out", 0, 0, NULL);

    plugindb_reset (corePlugin, floatGenInsertStmt);
    plugindb_bind_int (corePlugin, floatGenInsertStmt, 1, inst->dbId);
    if (plugindb_step (corePlugin, floatGenInsertStmt) != SQLITE_DONE)
        return -1;
    return 0;
}


int
floatGenRestore (struct LSD_SceneNodeInst const* inst, void* instData)
{
    double* castData = (double*)instData;
    plugindb_reset (corePlugin, floatGenSelectStmt);
    plugindb_bind_int (corePlugin, floatGenSelectStmt, 1, inst->dbId);
    if (plugindb_step (corePlugin, floatGenSelectStmt) == SQLITE_ROW)
    {
        *castData = plugindb_column_double (corePlugin, floatGenSelectStmt, 0);
        return 0;
    }

    return -1;
}


void
floatGenClean (struct LSD_SceneNodeInst const* inst, void* instData)
{

}


void
floatGenDelete (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugindb_reset (corePlugin, floatGenDeleteStmt);
    plugindb_bind_int (corePlugin, floatGenDeleteStmt, 1, inst->dbId);
    plugindb_step (corePlugin, floatGenDeleteStmt);
}


/****** FLOAT VIEW STUFF ******/

static struct LSD_SceneNodeClass* floatViewClass;

/* Stmts */
static unsigned int floatViewSelectStmt;
static unsigned int floatViewInsertStmt;
static unsigned int floatViewDeleteStmt;

int
floatViewMake (struct LSD_SceneNodeInst const* inst, void* instData)
{
    int inId;
    plugininst_addInstInput (inst, floatType, "Float In", &inId);

    plugindb_reset (corePlugin, floatViewInsertStmt);
    plugindb_bind_int (corePlugin, floatViewInsertStmt, 1, inst->dbId);
    plugindb_bind_int (corePlugin, floatViewInsertStmt, 2, inId);
    plugindb_step (corePlugin, floatViewInsertStmt);

    return 0;
}


int
floatViewRestore (struct LSD_SceneNodeInst const* inst, void* instData)
{
    return 0;
}


void
floatViewClean (struct LSD_SceneNodeInst const* inst, void* instData)
{

}


void
floatViewDelete (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugindb_reset (corePlugin, floatViewDeleteStmt);
    plugindb_bind_int (corePlugin, floatViewDeleteStmt, 1, inst->dbId);
    plugindb_step (corePlugin, floatViewDeleteStmt);
}


/****** RGB GEN STUFF ******/

static struct LSD_SceneNodeClass* rgbGenClass;

/* Stmts */
static unsigned int rgbGenSelectStmt;
static unsigned int rgbGenInsertStmt;
static unsigned int rgbGenUpdateStmt;
static unsigned int rgbGenDeleteStmt;

/* Plug funcs */
void
rgbGenBufferOut (struct LSD_SceneNodeOutput const* output)
{
    /* Nothing to do */
}


void*
rgbGenPtrOut (struct LSD_SceneNodeOutput const* output)
{
    return output->parentNode->data;
}


/* Plug func tables */
static const bfFunc rgbGenBfFuncs[] =
{rgbGenBufferOut};

static const bpFunc rgbGenBpFuncs[] =
{rgbGenPtrOut};

int
rgbGenMake (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugininst_addInstOutput (inst, rgbType, "RGB Out", 0, 0, NULL);

    plugindb_reset (corePlugin, rgbGenInsertStmt);
    plugindb_bind_int (corePlugin, rgbGenInsertStmt, 1, inst->dbId);
    if (plugindb_step (corePlugin, rgbGenInsertStmt) != SQLITE_DONE)
        return -1;
    return 0;
}


int
rgbGenRestore (struct LSD_SceneNodeInst const* inst, void* instData)
{
    struct RGB_TYPE* castData = (struct RGB_TYPE*)instData;
    plugindb_reset (corePlugin, rgbGenSelectStmt);
    plugindb_bind_int (corePlugin, rgbGenSelectStmt, 1, inst->dbId);
    if (plugindb_step (corePlugin, rgbGenSelectStmt) == SQLITE_ROW)
    {
        castData->r = plugindb_column_double (corePlugin, rgbGenSelectStmt, 0);
        castData->g = plugindb_column_double (corePlugin, rgbGenSelectStmt, 1);
        castData->b = plugindb_column_double (corePlugin, rgbGenSelectStmt, 2);

        return 0;
    }

    return -1;
}


void
rgbGenClean (struct LSD_SceneNodeInst const* inst, void* instData)
{

}


void
rgbGenDelete (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugindb_reset (corePlugin, rgbGenDeleteStmt);
    plugindb_bind_int (corePlugin, rgbGenDeleteStmt, 1, inst->dbId);
    plugindb_step (corePlugin, rgbGenDeleteStmt);
}


/****** RGB VIEW STUFF ******/

static struct LSD_SceneNodeClass* rgbViewClass;

/* Stmts */
static unsigned int rgbViewSelectStmt;
static unsigned int rgbViewInsertStmt;
static unsigned int rgbViewDeleteStmt;

int
rgbViewMake (struct LSD_SceneNodeInst const* inst, void* instData)
{
    int inId;
    plugininst_addInstInput (inst, rgbType, "RGB In", &inId);

    plugindb_reset (corePlugin, rgbViewInsertStmt);
    plugindb_bind_int (corePlugin, rgbViewInsertStmt, 1, inst->dbId);
    plugindb_bind_int (corePlugin, rgbViewInsertStmt, 2, inId);
    plugindb_step (corePlugin, rgbViewInsertStmt);

    return 0;
}


int
rgbViewRestore (struct LSD_SceneNodeInst const* inst, void* instData)
{
    return 0;
}


void
rgbViewClean (struct LSD_SceneNodeInst const* inst, void* instData)
{

}


void
rgbViewDelete (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugindb_reset (corePlugin, rgbViewDeleteStmt);
    plugindb_bind_int (corePlugin, rgbViewDeleteStmt, 1, inst->dbId);
    plugindb_step (corePlugin, rgbViewDeleteStmt);
}


/****** TRIGGER GEN STUFF ******/

static struct LSD_SceneNodeClass* triggerGenClass;

/* Plug funcs */
void
triggerGenBufferOut (struct LSD_SceneNodeOutput const* output)
{
    /* Nothing to do */
}


void*
triggerGenPtrOut (struct LSD_SceneNodeOutput const* output)
{
    return output->parentNode->data;
}


/* Plug func tables */
static const bfFunc triggerGenBfFuncs[] =
{triggerGenBufferOut};

static const bpFunc triggerGenBpFuncs[] =
{triggerGenPtrOut};

int
triggerGenMake (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugininst_addInstOutput (inst, triggerType, "Trigger Out", 0, 0, NULL);

    return 0;
}


int
triggerGenRestore (struct LSD_SceneNodeInst const* inst, void* instData)
{
    int* castData = (int*)instData;
    *castData = 0;
    return 0;
}


void
triggerGenClean (struct LSD_SceneNodeInst const* inst, void* instData)
{

}


void
triggerGenDelete (struct LSD_SceneNodeInst const* inst, void* instData)
{

}


/****** TRIGGER COUNT STUFF ******/

struct TriggerCounter
{
    int phase;
    struct RGB_TYPE out;
    int triggerKnown;
    int inId;
};

static struct LSD_SceneNodeClass* rgbTriggerClass;

static unsigned int rgbTriggerSelectStmt;
static unsigned int rgbTriggerInsertStmt;
static unsigned int rgbTriggerDeleteStmt;

/* Plug funcs */
void
rgbTriggerBufferOut (struct LSD_SceneNodeOutput const* output)
{
    struct TriggerCounter* trigCount = output->parentNode->data;

    struct LSD_SceneNodeInput const* trigIn;
    plugininst_getInputStruct (output->parentNode, &trigIn, trigCount->inId);

    if (trigIn && trigIn->connection)
    {
        int* curTrig = node_bufferOutput (trigIn->connection);
        if (curTrig)
        {
            if (( *curTrig - trigCount->triggerKnown ) == 1)
            {
                ++trigCount->phase;
                if (trigCount->phase > 2)
                    trigCount->phase = 0;
            }
            trigCount->triggerKnown = *curTrig;
        }
    }

    if (trigCount->phase == 0)
    {
        trigCount->out.r = 1.0;
        trigCount->out.g = 0.0;
        trigCount->out.b = 0.0;
    }
    else if (trigCount->phase == 1)
    {
        trigCount->out.r = 0.0;
        trigCount->out.g = 1.0;
        trigCount->out.b = 0.0;
    }
    else if (trigCount->phase == 2)
    {
        trigCount->out.r = 0.0;
        trigCount->out.g = 0.0;
        trigCount->out.b = 1.0;
    }
    else
    {
        trigCount->phase = 0;
        trigCount->out.r = 1.0;
        trigCount->out.g = 0.0;
        trigCount->out.b = 0.0;
    }
}


void*
rgbTriggerPtrOut (struct LSD_SceneNodeOutput const* output)
{
    struct TriggerCounter* trigCount = output->parentNode->data;
    return (void*)&( trigCount->out );
}


/* Plug func tables */
static const bfFunc rgbTriggerBfFuncs[] =
{rgbTriggerBufferOut};

static const bpFunc rgbTriggerBpFuncs[] =
{rgbTriggerPtrOut};

int
rgbTriggerMake (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugininst_addInstOutput (inst, rgbType, "RGB Out", 0, 0, NULL);

    int inId;
    plugininst_addInstInput (inst, triggerType, "Trigger In", &inId);

    plugindb_reset (corePlugin, rgbTriggerInsertStmt);
    plugindb_bind_int (corePlugin, rgbTriggerInsertStmt, 1, inst->dbId);
    plugindb_bind_int (corePlugin, rgbTriggerInsertStmt, 2, inId);
    plugindb_step (corePlugin, rgbTriggerInsertStmt);

    return 0;
}


int
rgbTriggerRestore (struct LSD_SceneNodeInst const* inst, void* instData)
{
    struct TriggerCounter* castData = (struct TriggerCounter*)instData;
    castData->phase = 0;
    castData->triggerKnown = 0;

    plugindb_reset (corePlugin, rgbTriggerSelectStmt);
    plugindb_bind_int (corePlugin, rgbTriggerSelectStmt, 1, inst->dbId);
    if (plugindb_step (corePlugin, rgbTriggerSelectStmt) == SQLITE_ROW)
    {
        castData->inId = plugindb_column_int (corePlugin,
                                              rgbTriggerSelectStmt,
                                              0);
        return 0;
    }
    return -1;
}


void
rgbTriggerClean (struct LSD_SceneNodeInst const* inst, void* instData)
{

}


void
rgbTriggerDelete (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugindb_reset (corePlugin, rgbTriggerDeleteStmt);
    plugindb_bind_int (corePlugin, rgbTriggerDeleteStmt, 1, inst->dbId);
    plugindb_step (corePlugin, rgbTriggerDeleteStmt);
}


/* ******  Plugin members ****** */

int
coreInit (struct LSD_ScenePlugin const* plugin)
{

    corePlugin = plugin;

    /* Register RGB Type */
    plugininit_registerDataType (plugin, &rgbType, "RGB Type", "");

    /* Register Float Type */
    plugininit_registerDataType (plugin, &floatType, "Float Type", "");

    /* Register Integer Type */
    plugininit_registerDataType (plugin, &integerType, "Integer Type", "");

    /* Register Trigger Type */
    plugininit_registerDataType (plugin, &triggerType, "Trigger Type", "");

    /* Register int generator */
    plugininit_registerNodeClass (plugin,
                                  &intGenClass,
                                  intGenMake,
                                  intGenRestore,
                                  intGenClean,
                                  intGenDelete,
                                  sizeof( int ),
                                  "Integer Generator",
                                  "Desc",
                                  1,
                                  intGenBfFuncs,
                                  intGenBpFuncs);

    /* Register int viewer */
    plugininit_registerNodeClass (plugin,
                                  &intViewClass,
                                  intViewMake,
                                  intViewRestore,
                                  intViewClean,
                                  intViewDelete,
                                  0,
                                  "Integer Viewer",
                                  "Desc",
                                  2,
                                  NULL,
                                  NULL);

    /* Register float generator */
    plugininit_registerNodeClass (plugin,
                                  &floatGenClass,
                                  floatGenMake,
                                  floatGenRestore,
                                  floatGenClean,
                                  floatGenDelete,
                                  sizeof( double ),
                                  "Float Generator",
                                  "Desc",
                                  3,
                                  floatGenBfFuncs,
                                  floatGenBpFuncs);

    /* Register float viewer */
    plugininit_registerNodeClass (plugin,
                                  &floatViewClass,
                                  floatViewMake,
                                  floatViewRestore,
                                  floatViewClean,
                                  floatViewDelete,
                                  0,
                                  "Float Viewer",
                                  "Desc",
                                  4,
                                  NULL,
                                  NULL);

    /* Register rgb generator */
    plugininit_registerNodeClass (plugin,
                                  &rgbGenClass,
                                  rgbGenMake,
                                  rgbGenRestore,
                                  rgbGenClean,
                                  rgbGenDelete,
                                  sizeof( struct RGB_TYPE ),
                                  "RGB Generator",
                                  "Desc",
                                  5,
                                  rgbGenBfFuncs,
                                  rgbGenBpFuncs);

    /* Register rgb viewer */
    plugininit_registerNodeClass (plugin,
                                  &rgbViewClass,
                                  rgbViewMake,
                                  rgbViewRestore,
                                  rgbViewClean,
                                  rgbViewDelete,
                                  0,
                                  "RGB Viewer",
                                  "Desc",
                                  6,
                                  NULL,
                                  NULL);

    /* Register trigger generator */
    plugininit_registerNodeClass (plugin,
                                  &triggerGenClass,
                                  triggerGenMake,
                                  triggerGenRestore,
                                  triggerGenClean,
                                  triggerGenDelete,
                                  sizeof( int ),
                                  "Trigger Generator",
                                  "Desc",
                                  7,
                                  triggerGenBfFuncs,
                                  triggerGenBpFuncs);

    /* Register rgb trigger */
    plugininit_registerNodeClass (plugin,
                                  &rgbTriggerClass,
                                  rgbTriggerMake,
                                  rgbTriggerRestore,
                                  rgbTriggerClean,
                                  rgbTriggerDelete,
                                  sizeof( struct TriggerCounter ),
                                  "RGB Trigger",
                                  "Desc",
                                  8,
                                  rgbTriggerBfFuncs,
                                  rgbTriggerBpFuncs);

    /* Create Int Gen DB stuff */
    plugininit_createTable (plugin,
                            "intGen",
                            "id INTEGER PRIMARY KEY, value INTEGER DEFAULT 0");
    plugindb_prepSelect (plugin, &intGenSelectStmt, "intGen", "value", "id=?1");
    plugindb_prepInsert (plugin, &intGenInsertStmt, "intGen", "id", "?1");
    plugindb_prepUpdate (plugin,
                         &intGenUpdateStmt,
                         "intGen",
                         "value=?2",
                         "id=?1");
    plugindb_prepDelete (plugin, &intGenDeleteStmt, "intGen", "id=?1");

    /* Create Int View DB stuff */
    plugininit_createTable (plugin,
                            "intView",
                            "id INTEGER PRIMARY KEY, inId INTEGER NOT NULL");
    plugindb_prepSelect (plugin, &intViewSelectStmt, "intView", "inId", "id=?1");
    plugindb_prepInsert (plugin,
                         &intViewInsertStmt,
                         "intView",
                         "id,inId",
                         "?1,?2");
    plugindb_prepDelete (plugin, &intViewDeleteStmt, "intView", "id=?1");

    /* Create Float Gen DB stuff */
    plugininit_createTable (plugin,
                            "floatGen",
                            "id INTEGER PRIMARY KEY, value REAL DEFAULT 0.0");
    plugindb_prepSelect (plugin,
                         &floatGenSelectStmt,
                         "floatGen",
                         "value",
                         "id=?1");
    plugindb_prepInsert (plugin, &floatGenInsertStmt, "floatGen", "id", "?1");
    plugindb_prepUpdate (plugin,
                         &floatGenUpdateStmt,
                         "floatGen",
                         "value=?2",
                         "id=?1");
    plugindb_prepDelete (plugin, &floatGenDeleteStmt, "floatGen", "id=?1");

    /* Create Float View DB stuff */
    plugininit_createTable (plugin,
                            "floatView",
                            "id INTEGER PRIMARY KEY, inId INTEGER NOT NULL");
    plugindb_prepSelect (plugin,
                         &floatViewSelectStmt,
                         "floatView",
                         "inId",
                         "id=?1");
    plugindb_prepInsert (plugin,
                         &floatViewInsertStmt,
                         "floatView",
                         "id,inId",
                         "?1,?2");
    plugindb_prepDelete (plugin, &floatViewDeleteStmt, "floatView", "id=?1");

    /* Create RGB Gen DB stuff */
    plugininit_createTable (plugin,
                            "rgbGen",
                            "id INTEGER PRIMARY KEY, rVal REAL DEFAULT 0.0, "
                            "gVal REAL DEFAULT 0.0, bVal REAL DEFAULT 0.0");
    plugindb_prepSelect (plugin,
                         &rgbGenSelectStmt,
                         "rgbGen",
                         "rVal,gVal,bVal",
                         "id=?1");
    plugindb_prepInsert (plugin, &rgbGenInsertStmt, "rgbGen", "id", "?1");
    plugindb_prepUpdate (plugin,
                         &rgbGenUpdateStmt,
                         "rgbGen",
                         "rVal=?2,gVal=?3,bVal=?4",
                         "id=?1");
    plugindb_prepDelete (plugin, &rgbGenDeleteStmt, "rgbGen", "id=?1");

    /* Create RGB View DB stuff */
    plugininit_createTable (plugin,
                            "rgbView",
                            "id INTEGER PRIMARY KEY, inId INTEGER NOT NULL");
    plugindb_prepSelect (plugin, &rgbViewSelectStmt, "rgbView", "inId", "id=?1");
    plugindb_prepInsert (plugin,
                         &rgbViewInsertStmt,
                         "rgbView",
                         "id,inId",
                         "?1,?2");
    plugindb_prepDelete (plugin, &rgbViewDeleteStmt, "rgbView", "id=?1");

    /* Create RGB Trigger DB stuff */
    plugininit_createTable (plugin,
                            "rgbTrigger",
                            "id INTEGER PRIMARY KEY, inId INTEGER NOT NULL");
    plugindb_prepSelect (plugin,
                         &rgbTriggerSelectStmt,
                         "rgbTrigger",
                         "inId",
                         "id=?1");
    plugindb_prepInsert (plugin,
                         &rgbTriggerInsertStmt,
                         "rgbTrigger",
                         "id,inId",
                         "?1,?2");
    plugindb_prepDelete (plugin, &rgbTriggerDeleteStmt, "rgbTrigger", "id=?1");

    return 0;
}


/* RPC handler function */
void
rpcHandler (cJSON* in, cJSON* out)
{

    cJSON* coreMethod = cJSON_GetObjectItem (in, "coreMethod");
    if (!coreMethod || coreMethod->type != cJSON_String)
        return;

    cJSON* nodeId = cJSON_GetObjectItem (in, "nodeId");
    if (!nodeId || nodeId->type != cJSON_Number)
        return;

    cJSON* val = NULL;

    /* Determine which RPC operation to perform */
    if (strcasecmp (coreMethod->valuestring, "setIntGenVal") == 0)
    {
        val = cJSON_GetObjectItem (in, "val");
        if (!val || val->type != cJSON_Number)
            return;

        plugindb_reset (corePlugin, intGenUpdateStmt);
        plugindb_bind_int (corePlugin, intGenUpdateStmt, 1, nodeId->valueint);
        plugindb_bind_int (corePlugin, intGenUpdateStmt, 2, val->valueint);
        if (plugindb_step (corePlugin, intGenUpdateStmt) == SQLITE_DONE)
            cJSON_AddStringToObject (out, "success", "success");
        else
            cJSON_AddStringToObject (out, "error", _("Unable to update val"));

        int* intGenInt = NULL;
        plugin_getInstById (corePlugin, nodeId->valueint, (void**)&intGenInt);
        if (intGenInt)
            *intGenInt = val->valueint;

    }
    else if (strcasecmp (coreMethod->valuestring, "getIntGenVal") == 0)
    {

        plugindb_reset (corePlugin, intGenSelectStmt);
        plugindb_bind_int (corePlugin, intGenSelectStmt, 1, nodeId->valueint);
        if (plugindb_step (corePlugin, intGenSelectStmt) == SQLITE_ROW)
            cJSON_AddNumberToObject (out, "val",
                                     plugindb_column_int (corePlugin,
                                                          intGenSelectStmt, 0));
        else
            cJSON_AddStringToObject (out, "error", _("Unable to get val"));

    }
    else if (strcasecmp (coreMethod->valuestring, "getIntViewVal") == 0)
    {
        plugindb_reset (corePlugin, intViewSelectStmt);
        plugindb_bind_int (corePlugin, intViewSelectStmt, 1, nodeId->valueint);
        int inId;
        if (plugindb_step (corePlugin, intViewSelectStmt) == SQLITE_ROW)
            inId = plugindb_column_int (corePlugin, intViewSelectStmt, 0);
        else
        {
            cJSON_AddStringToObject (out,
                                     "error",
                                     _("Unable to find nodeInst in DB"));
            return;
        }

        struct LSD_SceneNodeInst const* viewInst = plugin_getInstById (
            corePlugin,
            nodeId->valueint,
            NULL);

        struct LSD_SceneNodeInput const* intViewerIn = NULL;
        plugininst_getInputStruct (viewInst, &intViewerIn, inId);

        if (intViewerIn && intViewerIn->connection)
        {
            int* theVal = node_bufferOutput (intViewerIn->connection);
            if (theVal)
                cJSON_AddNumberToObject (out, "val", *theVal);
        }
        else
            cJSON_AddStringToObject (out,
                                     "error",
                                     _("Unable to resolve viewer's connection"));
    }
    else if (strcasecmp (coreMethod->valuestring, "setFloatGenVal") == 0)
    {
        val = cJSON_GetObjectItem (in, "val");
        if (!val || val->type != cJSON_Number)
            return;

        plugindb_reset (corePlugin, floatGenUpdateStmt);
        plugindb_bind_int (corePlugin, floatGenUpdateStmt, 1, nodeId->valueint);
        plugindb_bind_double (corePlugin,
                              floatGenUpdateStmt,
                              2,
                              val->valuedouble);
        if (plugindb_step (corePlugin, floatGenUpdateStmt) == SQLITE_DONE)
            cJSON_AddStringToObject (out, "success", "success");
        else
            cJSON_AddStringToObject (out, "error", _("Unable to update val"));

        double* floatGenDbl = NULL;
        plugin_getInstById (corePlugin, nodeId->valueint, (void**)&floatGenDbl);
        if (floatGenDbl)
            *floatGenDbl = val->valuedouble;

    }
    else if (strcasecmp (coreMethod->valuestring, "getFloatGenVal") == 0)
    {

        plugindb_reset (corePlugin, floatGenSelectStmt);
        plugindb_bind_int (corePlugin, floatGenSelectStmt, 1, nodeId->valueint);
        if (plugindb_step (corePlugin, floatGenSelectStmt) == SQLITE_ROW)
            cJSON_AddNumberToObject (out, "val",
                                     plugindb_column_double (corePlugin,
                                                             floatGenSelectStmt,
                                                             0));
        else
            cJSON_AddStringToObject (out, "error", _("Unable to get val"));

    }
    else if (strcasecmp (coreMethod->valuestring, "getFloatViewVal") == 0)
    {
        plugindb_reset (corePlugin, floatViewSelectStmt);
        plugindb_bind_int (corePlugin, floatViewSelectStmt, 1, nodeId->valueint);
        int inId;
        if (plugindb_step (corePlugin, floatViewSelectStmt) == SQLITE_ROW)
            inId = plugindb_column_int (corePlugin, floatViewSelectStmt, 0);
        else
        {
            cJSON_AddStringToObject (out,
                                     "error",
                                     _("Unable to find nodeInst in DB"));
            return;
        }

        struct LSD_SceneNodeInst const* viewInst = plugin_getInstById (
            corePlugin,
            nodeId->valueint,
            NULL);

        struct LSD_SceneNodeInput const* floatViewerIn = NULL;
        plugininst_getInputStruct (viewInst, &floatViewerIn, inId);

        if (floatViewerIn && floatViewerIn->connection)
        {
            double* theVal = node_bufferOutput (floatViewerIn->connection);
            if (theVal)
                cJSON_AddNumberToObject (out, "val", *theVal);
        }
        else
            cJSON_AddStringToObject (out,
                                     "error",
                                     _("Unable to resolve viewer's connection"));
    }
    else if (strcasecmp (coreMethod->valuestring, "setRgbGenVal") == 0)
    {
        val = cJSON_GetObjectItem (in, "val");
        if (!val || val->type != cJSON_Object)
            return;

        cJSON* rVal = cJSON_GetObjectItem (val, "r");
        if (!rVal || rVal->type != cJSON_Number)
            return;

        cJSON* gVal = cJSON_GetObjectItem (val, "g");
        if (!gVal || gVal->type != cJSON_Number)
            return;

        cJSON* bVal = cJSON_GetObjectItem (val, "b");
        if (!bVal || bVal->type != cJSON_Number)
            return;

        plugindb_reset (corePlugin, rgbGenUpdateStmt);
        plugindb_bind_int (corePlugin, rgbGenUpdateStmt, 1, nodeId->valueint);
        plugindb_bind_double (corePlugin,
                              rgbGenUpdateStmt,
                              2,
                              rVal->valuedouble);
        plugindb_bind_double (corePlugin,
                              rgbGenUpdateStmt,
                              3,
                              gVal->valuedouble);
        plugindb_bind_double (corePlugin,
                              rgbGenUpdateStmt,
                              4,
                              bVal->valuedouble);

        if (plugindb_step (corePlugin, rgbGenUpdateStmt) == SQLITE_DONE)
            cJSON_AddStringToObject (out, "success", "success");
        else
            cJSON_AddStringToObject (out, "error", _("Unable to update val"));

        struct RGB_TYPE* rgbGenObj = NULL;
        plugin_getInstById (corePlugin, nodeId->valueint, (void**)&rgbGenObj);
        if (rgbGenObj)
        {
            rgbGenObj->r = rVal->valuedouble;
            rgbGenObj->g = gVal->valuedouble;
            rgbGenObj->b = bVal->valuedouble;
        }
    }
    else if (strcasecmp (coreMethod->valuestring, "getRgbGenVal") == 0)
    {

        plugindb_reset (corePlugin, rgbGenSelectStmt);
        plugindb_bind_int (corePlugin, rgbGenSelectStmt, 1, nodeId->valueint);
        if (plugindb_step (corePlugin, rgbGenSelectStmt) == SQLITE_ROW)
        {
            cJSON* rgbObj = cJSON_CreateObject ();
            cJSON_AddNumberToObject (rgbObj, "r",
                                     plugindb_column_double (corePlugin,
                                                             rgbGenSelectStmt,
                                                             0));
            cJSON_AddNumberToObject (rgbObj, "g",
                                     plugindb_column_double (corePlugin,
                                                             rgbGenSelectStmt,
                                                             1));
            cJSON_AddNumberToObject (rgbObj, "b",
                                     plugindb_column_double (corePlugin,
                                                             rgbGenSelectStmt,
                                                             2));
            cJSON_AddItemToObject (out, "val", rgbObj);
        }
        else
            cJSON_AddStringToObject (out, "error", _("Unable to get val"));

    }
    else if (strcasecmp (coreMethod->valuestring, "getRgbViewVal") == 0)
    {
        plugindb_reset (corePlugin, rgbViewSelectStmt);
        plugindb_bind_int (corePlugin, rgbViewSelectStmt, 1, nodeId->valueint);
        int inId;
        if (plugindb_step (corePlugin, rgbViewSelectStmt) == SQLITE_ROW)
            inId = plugindb_column_int (corePlugin, rgbViewSelectStmt, 0);
        else
        {
            cJSON_AddStringToObject (out,
                                     "error",
                                     _("Unable to find nodeInst in DB"));
            return;
        }

        struct LSD_SceneNodeInst const* viewInst = plugin_getInstById (
            corePlugin,
            nodeId->valueint,
            NULL);

        struct LSD_SceneNodeInput const* rgbViewerIn = NULL;
        plugininst_getInputStruct (viewInst, &rgbViewerIn, inId);

        if (rgbViewerIn && rgbViewerIn->connection)
        {
            struct RGB_TYPE* theVal = node_bufferOutput (
                rgbViewerIn->connection);
            if (theVal)
            {
                cJSON* rgbObj = cJSON_CreateObject ();
                cJSON_AddNumberToObject (rgbObj, "r", theVal->r);
                cJSON_AddNumberToObject (rgbObj, "g", theVal->g);
                cJSON_AddNumberToObject (rgbObj, "b", theVal->b);
                cJSON_AddItemToObject (out, "val", rgbObj);
            }
        }
        else
            cJSON_AddStringToObject (out,
                                     "error",
                                     _("Unable to resolve viewer's connection"));
    }
    else if (strcasecmp (coreMethod->valuestring, "triggerGen") == 0)
    {
        int* trigVal = NULL;
        plugin_getInstById (corePlugin, nodeId->valueint, (void**)&trigVal);
        if (trigVal)
            *trigVal += 1;
    }
}


void
coreClean (struct LSD_ScenePlugin const* plugin)
{

}


/* Core Stuff Below - must remain in order to be loaded */
static const struct LSD_ScenePluginHEAD pluginHead = {
    coreInit,
    coreClean,
    rpcHandler
};

const struct LSD_ScenePluginHEAD*
getCoreHead ()
{
    return &pluginHead;
}


