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
#include <sys/time.h>

#include "../../PluginAPI.h"
#include "../../NodeInstAPI.h"
#include "../../CorePlugin.h"
#include "../../cJSON.h"

/* Main plugin object */
static struct LSD_ScenePlugin const* timePlugin;

/* RGB Type */
static int floatTypeId;

static struct LSD_SceneNodeClass* timeClass;

/* Current timeofday */
static double curTime;

/* Plug funcs */

void
timeBufferOut (struct LSD_SceneNodeOutput const* output)
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    curTime = (double)tv.tv_sec;
    curTime += (double)tv.tv_usec / (double)1000000;
}


void*
timePointerOut (struct LSD_SceneNodeOutput const* output)
{
    return &curTime;
}


/* Plug function tables */

static const bfFunc bfFuncs[] =
{timeBufferOut};

static const bpFunc bpFuncs[] =
{timePointerOut};

/* Node Init funcs */

int
timeNodeMake (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugininst_addInstOutput (inst, floatTypeId, "Time Float", 0, 0, NULL);

    return 0;
}


int
timeNodeRestore (struct LSD_SceneNodeInst const* inst, void* instData)
{

    return 0;
}


void
timeNodeClean (struct LSD_SceneNodeInst const* inst, void* instData)
{

}


void
timeNodeDelete (struct LSD_SceneNodeInst const* inst, void* instData)
{

}


void
timeRPCHandler (cJSON* in, cJSON* out)
{

}


/* Plugin init funcs */

int
timePluginInit (struct LSD_ScenePlugin const* plugin)
{

    timePlugin = plugin;

    floatTypeId = core_getFloatTypeID ();

    /* Register Time class */
    plugininit_registerNodeClass (plugin,
                                  &timeClass,
                                  timeNodeMake,
                                  timeNodeRestore,
                                  timeNodeClean,
                                  timeNodeDelete,
                                  0,
                                  "Epoch Time",
                                  "Desc",
                                  0,
                                  bfFuncs,
                                  bpFuncs);

    return 0;
}


void
timePluginClean (struct LSD_ScenePlugin const* plugin)
{
    /* Nothing to be done here */
}


static const struct LSD_ScenePluginHEAD pluginHead = {
    timePluginInit,
    timePluginClean,
    timeRPCHandler
};

extern const struct LSD_ScenePluginHEAD*
getPluginHead ()
{
    return &pluginHead;
}


