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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h> /* For constants */

#include "../../PluginAPI.h"
#include "../../NodeInstAPI.h"
#include "../../CorePlugin.h"
#include "../../cJSON.h"

/** SYMBOL ALIAS FOR LIBTOOL **/
#define getPluginHead  ColourBankPlugin_LTX_getPluginHead

/* Main plugin object */
static struct LSD_ScenePlugin const* pickerBankPlugin;

/* RGB Type */
static int rgbTypeId;

/* Stmt index bindings */
static unsigned int selectPickerNodeStmt;
static unsigned int insertPickerStmt;
static unsigned int updatePickerStmt;
static unsigned int deletePickerStmt;
static unsigned int deletePickerNodeStmt;

static struct LSD_SceneNodeClass* colourBankClass;

/* Struct for individual picker */
struct PickerData
{
    int dbId;
    struct RGB_TYPE pickerVal;
};

/* Struct for instData */
struct PickerInstData
{
    int numPickers;
    struct PickerData* pickerArr;
};

/* Plug funcs */

void
pickerBufferOut (struct LSD_SceneNodeOutput const* output)
{
    /* Nothing needs to be done */
}


void*
pickerPointerOut (struct LSD_SceneNodeOutput const* output)
{
    struct LSD_SceneNodeInst const* nodeInst = output->parentNode;
    struct PickerInstData* instData = (struct PickerInstData*)nodeInst->data;

    int i;
    for (i = 0; i < instData->numPickers; ++i)
        if (instData->pickerArr[i].dbId == output->dbId)
            return (void*)&( instData->pickerArr[i].pickerVal );
    return NULL;
}


/* Plug function tables */

static const bfFunc bfFuncs[] =
{pickerBufferOut};

static const bpFunc bpFuncs[] =
{pickerPointerOut};

/* Node Init funcs */

int
colourBankNodeMake (struct LSD_SceneNodeInst const* inst, void* instData)
{
    /* Nothing to be done here */
    printf ("Node Make ran\n");

    return 0;
}


int
colourBankNodeRestore (struct LSD_SceneNodeInst const* inst, void* instData)
{
    struct PickerInstData* castData = (struct PickerInstData*)instData;

    /* First get a count of pickers in bank */
    plugindb_reset (pickerBankPlugin, selectPickerNodeStmt);
    plugindb_bind_int (pickerBankPlugin, selectPickerNodeStmt,
                       1, inst->dbId);

    int cnt = 0;
    while (plugindb_step (pickerBankPlugin, selectPickerNodeStmt) == SQLITE_ROW)
        ++cnt;

    /* Allocate a PickerData array of length cnt */
    struct PickerData* newArr = malloc (sizeof( struct PickerData ) * cnt);
    if (!newArr)
    {
        fprintf (stderr, "Unable to allocate memory for picker arr\n");
        return -1;
    }

    /* Copy count into data */
    castData->numPickers = cnt;

    /* Assign new memory to instData */
    castData->pickerArr = newArr;

    /* Rerun query insterting data */
    plugindb_reset (pickerBankPlugin, selectPickerNodeStmt);
    plugindb_bind_int (pickerBankPlugin, selectPickerNodeStmt,
                       1, inst->dbId);

    int i = 0;
    while (plugindb_step (pickerBankPlugin,
                          selectPickerNodeStmt) == SQLITE_ROW && i < cnt)
    {

        castData->pickerArr[i].dbId = plugindb_column_int (pickerBankPlugin,
                                                           selectPickerNodeStmt,
                                                           0);
        castData->pickerArr[i].pickerVal.r = plugindb_column_double (
            pickerBankPlugin,
            selectPickerNodeStmt,
            1);
        castData->pickerArr[i].pickerVal.g = plugindb_column_double (
            pickerBankPlugin,
            selectPickerNodeStmt,
            2);
        castData->pickerArr[i].pickerVal.b = plugindb_column_double (
            pickerBankPlugin,
            selectPickerNodeStmt,
            3);

        ++i;
    }

    printf ("Node Restore ran\n");

    return 0;
}


void
colourBankNodeClean (struct LSD_SceneNodeInst const* inst, void* instData)
{
    struct PickerInstData* castData = (struct PickerInstData*)instData;
    free (castData->pickerArr);
    printf ("Node Clean ran\n");
}


void
colourBankNodeDelete (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugindb_reset (pickerBankPlugin, deletePickerNodeStmt);
    plugindb_bind_int (pickerBankPlugin, deletePickerNodeStmt,
                       1, inst->dbId);
    if (plugindb_step (pickerBankPlugin, deletePickerNodeStmt) != SQLITE_DONE)
        fprintf (stderr, "Problem while removing picker Bank from DB\n");
    printf ("Node Delete ran\n");
}


void
colourBankRPCHandler (cJSON* in, cJSON* out)
{

    /* Pointers to data that may need to be resolved */
    void* instData = NULL;
    struct LSD_SceneNodeInst const* inst = NULL;

    /* get nodeId */
    cJSON* nodeId = cJSON_GetObjectItem (in, "nodeId");

    cJSON* cbMethod = cJSON_GetObjectItem (in, "cbMethod");
    if (cbMethod && cbMethod->type == cJSON_String)
    {
        /* First find out the requested method */
        if (strcasecmp (cbMethod->valuestring, "getNodePickers") == 0)
        {

            if (!nodeId || nodeId->type != cJSON_Number)
            {
                cJSON_AddStringToObject (out, "error", "nodeId not provided");
                return;
            }

            cJSON* pickerArr = cJSON_CreateArray ();
            plugindb_reset (pickerBankPlugin, selectPickerNodeStmt);
            plugindb_bind_int (pickerBankPlugin,
                               selectPickerNodeStmt,
                               1,
                               nodeId->valueint);
            while (plugindb_step (pickerBankPlugin,
                                  selectPickerNodeStmt) == SQLITE_ROW)
            {
                int pickerId = plugindb_column_int (pickerBankPlugin,
                                                    selectPickerNodeStmt,
                                                    0);
                double lastR = plugindb_column_double (pickerBankPlugin,
                                                       selectPickerNodeStmt,
                                                       1);
                double lastG = plugindb_column_double (pickerBankPlugin,
                                                       selectPickerNodeStmt,
                                                       2);
                double lastB = plugindb_column_double (pickerBankPlugin,
                                                       selectPickerNodeStmt,
                                                       3);

                cJSON* pickerObject = cJSON_CreateObject ();
                cJSON_AddNumberToObject (pickerObject, "pickerId", pickerId);
                cJSON* colourObject = cJSON_CreateObject ();
                cJSON_AddNumberToObject (colourObject, "r", lastR);
                cJSON_AddNumberToObject (colourObject, "g", lastG);
                cJSON_AddNumberToObject (colourObject, "b", lastB);
                cJSON_AddItemToObject (pickerObject, "rgb", colourObject);
                cJSON_AddItemToArray (pickerArr, pickerObject);
            }

            cJSON_AddItemToObject (out, "pickers", pickerArr);

        }
        else if (strcasecmp (cbMethod->valuestring, "updatePicker") == 0)
        {
            cJSON* pickerId = cJSON_GetObjectItem (in, "pickerId");
            if (!pickerId || pickerId->type != cJSON_Number)
            {
                cJSON_AddStringToObject (out, "error", "pickerId not available");
                return;
            }

            cJSON* rVal = cJSON_GetObjectItem (in, "r");
            cJSON* gVal = cJSON_GetObjectItem (in, "g");
            cJSON* bVal = cJSON_GetObjectItem (in, "b");
            if (!rVal || rVal->type != cJSON_Number ||
                !gVal || gVal->type != cJSON_Number ||
                !bVal || bVal->type != cJSON_Number)
            {
                cJSON_AddStringToObject (out,
                                         "error",
                                         "Invalid colour provided");
                return;
            }

            plugindb_reset (pickerBankPlugin, updatePickerStmt);
            plugindb_bind_int (pickerBankPlugin,
                               updatePickerStmt,
                               1,
                               pickerId->valueint);
            plugindb_bind_double (pickerBankPlugin,
                                  updatePickerStmt,
                                  2,
                                  rVal->valuedouble);
            plugindb_bind_double (pickerBankPlugin,
                                  updatePickerStmt,
                                  3,
                                  gVal->valuedouble);
            plugindb_bind_double (pickerBankPlugin,
                                  updatePickerStmt,
                                  4,
                                  bVal->valuedouble);

            if (plugindb_step (pickerBankPlugin,
                               updatePickerStmt) != SQLITE_DONE)
            {
                cJSON_AddStringToObject (out, "error", "Error updating picker");
                return;
            }

            /* Update buffered values */
            inst = plugin_getInstById (pickerBankPlugin,
                                       nodeId->valueint,
                                       &instData);
            if (instData)
            {
                struct PickerInstData* castData =
                    (struct PickerInstData*)instData;
                int i;
                for (i = 0; i < castData->numPickers; ++i)
                    if (castData->pickerArr[i].dbId == pickerId->valueint)
                    {
                        castData->pickerArr[i].pickerVal.r = rVal->valuedouble;
                        castData->pickerArr[i].pickerVal.g = gVal->valuedouble;
                        castData->pickerArr[i].pickerVal.b = bVal->valuedouble;
                        break;
                    }
            }

            cJSON_AddStringToObject (out, "success", "success");

        }
        else if (strcasecmp (cbMethod->valuestring, "addPicker") == 0)
        {
            if (!nodeId || nodeId->type != cJSON_Number)
            {
                cJSON_AddStringToObject (out, "error", "nodeId not available");
                return;
            }

            /* Get inst object to add output */
            int outId;
            inst = plugin_getInstById (pickerBankPlugin,
                                       nodeId->valueint,
                                       &instData);
            if (inst)
            {

                if (plugininst_addInstOutput (inst, rgbTypeId, "Output", 0, 0,
                                              &outId) < 0)
                {
                    cJSON_AddStringToObject (out,
                                             "error",
                                             "Couldn't add output to node");
                    return;
                }

                cJSON_AddStringToObject (out, "success", "success");
            }
            else
                cJSON_AddStringToObject (out,
                                         "error",
                                         "Couldn't get inst by id");

            plugindb_reset (pickerBankPlugin, insertPickerStmt);
            plugindb_bind_int (pickerBankPlugin, insertPickerStmt, 1, outId);
            plugindb_bind_int (pickerBankPlugin,
                               insertPickerStmt,
                               2,
                               nodeId->valueint);

            if (plugindb_step (pickerBankPlugin,
                               insertPickerStmt) != SQLITE_DONE)
            {
                cJSON_AddStringToObject (out, "error", "Error inserting picker");
                return;
            }

            /* Reload inst data */
            colourBankNodeClean (inst, instData);
            colourBankNodeRestore (inst, instData);

        }
        else if (strcasecmp (cbMethod->valuestring, "deletePicker") == 0)
        {
            cJSON* pickerId = cJSON_GetObjectItem (in, "pickerId");
            if (!pickerId || pickerId->type != cJSON_Number)
            {
                cJSON_AddStringToObject (out, "error", "pickerId not available");
                return;
            }

            /* Get inst object to remove output */
            inst = plugin_getInstById (pickerBankPlugin,
                                       nodeId->valueint,
                                       &instData);
            if (inst)
            {

                if (plugininst_removeInstOutput (inst, pickerId->valueint) < 0)
                {
                    cJSON_AddStringToObject (out,
                                             "error",
                                             "Couldn't remove output from node");
                    return;
                }

            }
            else
                cJSON_AddStringToObject (out,
                                         "error",
                                         "Couldn't get inst by id");

            plugindb_reset (pickerBankPlugin, deletePickerStmt);
            plugindb_bind_int (pickerBankPlugin,
                               deletePickerStmt,
                               1,
                               pickerId->valueint);

            if (plugindb_step (pickerBankPlugin,
                               deletePickerStmt) != SQLITE_DONE)
            {
                cJSON_AddStringToObject (out, "error", "Error deleting picker");
                return;
            }

            /* Reload inst data */
            colourBankNodeClean (inst, instData);
            colourBankNodeRestore (inst, instData);

            cJSON_AddStringToObject (out, "success", "success");

        }
        else
            cJSON_AddStringToObject (out, "error", "unhandled cbMethod");
    }
    else
        cJSON_AddStringToObject (out, "error", "cbMethod not provided");
}


/* Plugin init funcs */

int
colourBankPluginInit (struct LSD_ScenePlugin const* plugin)
{
    printf ("Colour Bank Init Ran\n");

    pickerBankPlugin = plugin;

    rgbTypeId = core_getRGBTypeID ();

    /* Register Colour Bank class */
    if (plugininit_registerNodeClass (plugin, &colourBankClass,
                                      colourBankNodeMake, colourBankNodeRestore,
                                      colourBankNodeClean, colourBankNodeDelete,
                                      sizeof( struct PickerInstData ),
                                      "Colour Bank", "Colour Bank Class", 0,
                                      bfFuncs, bpFuncs) < 0)
    {
        fprintf (stderr, "Error while registering Colour Bank class\n");
        return -1;
    }

    /* Register DB tables */
    if (plugininit_createTable (plugin, "picker",
                                "id INTEGER PRIMARY KEY, nodeId INTEGER NOT NULL, lastR REAL DEFAULT 0, "
                                "lastG REAL DEFAULT 0, lastB REAL DEFAULT 0") <
        0)
    {
        fprintf (stderr,
                 "Unable to register db tables for colour bank plugin\n");
        return -1;
    }

    /* Register DB stmts */

    /* Select pickers from nodeId */
    if (plugindb_prepSelect (plugin,
                             &selectPickerNodeStmt,
                             "picker", "id,lastR,lastG,lastB", "nodeId=?1") < 0)
    {
        fprintf (stderr, "Unable to prep SELECT for colour bank plugin\n");
        return -1;
    }

    /* Insert picker */
    if (plugindb_prepInsert (plugin, &insertPickerStmt,
                             "picker", "id,nodeId", "?1,?2") < 0)
    {
        fprintf (stderr, "Unable to prep INSERT for colour bank plugin\n");
        return -1;
    }

    /* Update picker */
    if (plugindb_prepUpdate (plugin, &updatePickerStmt,
                             "picker", "lastR=?2,lastG=?3,lastB=?4",
                             "id=?1") < 0)
    {
        fprintf (stderr, "Unable to prep UPDATE for colour bank plugin\n");
        return -1;
    }

    /* Delete picker */
    if (plugindb_prepDelete (plugin,
                             &deletePickerStmt, "picker", "id=?1") < 0)
    {
        fprintf (stderr, "Unable to prep DELETE for colour bank plugin\n");
        return -1;
    }

    /* Delete pickers from nodeId */
    if (plugindb_prepDelete (plugin,
                             &deletePickerNodeStmt, "picker", "nodeId=?1") < 0)
    {
        fprintf (stderr, "Unable to prep DELETE2 for colour bank plugin\n");
        return -1;
    }

    return 0;
}


void
colourBankPluginClean (struct LSD_ScenePlugin const* plugin)
{
    /* Nothing to be done here */
}


static const struct LSD_ScenePluginHEAD pluginHead = {
    colourBankPluginInit,
    colourBankPluginClean,
    colourBankRPCHandler
};

extern const struct LSD_ScenePluginHEAD*
getPluginHead ()
{
    return &pluginHead;
}


