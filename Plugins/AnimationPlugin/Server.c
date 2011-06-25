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

#include <string.h>

#include "../../PluginAPI.h"
#include "../../cJSON.h"
#include "../../CorePlugin.h"
#include "../../Node.h"

#include "attackDecay.h"
#include "db.h"

/* Plugin object */
static struct LSD_ScenePlugin const* animationPlugin;

/* Types */
int floatTypeId;

/* Attack Decay plug funcs */
void
adBufferOut (struct LSD_SceneNodeOutput const* output)
{
    struct AttackDecayState* castData =
        (struct AttackDecayState*)output->parentNode->data;
    double* srcVal;
    if (castData->srcIn->connection)
        srcVal = (double*)node_bufferOutput (castData->srcIn->connection);
    else
        return;

    procAttackDecay (srcVal, castData);
}


void*
adPointerOut (struct LSD_SceneNodeOutput const* output)
{
    struct AttackDecayState* castData =
        (struct AttackDecayState*)output->parentNode->data;
    return &( castData->lastVal );
}


/* AttackDecay Plug func table */
static const bfFunc bfFuncs[] =
{adBufferOut};

static const bpFunc bpFuncs[] =
{adPointerOut};

/* Plugin Events */
int
animationPluginInit (struct LSD_ScenePlugin const* plugin)
{
    animationPlugin = plugin;
    floatTypeId = core_getFloatTypeID ();
    ad_publishFloatId (floatTypeId);

    initAnimDB (plugin);

    plugininit_registerNodeClass (plugin, NULL,
                                  attackDecayNodeMake,
                                  attackDecayNodeRestore,
                                  attackDecayNodeClean,
                                  attackDecayNodeDelete,
                                  sizeof( struct AttackDecayState ),
                                  "Attack/Decay", "Desc", 0,
                                  bfFuncs, bpFuncs);

    return 0;
}


void
animationPluginClean (struct LSD_ScenePlugin const* plugin)
{
}


void
animationPluginRPC (cJSON* in, cJSON* out)
{
    cJSON* nodeId = cJSON_GetObjectItem (in, "nodeId");
    if (!nodeId || nodeId->type != cJSON_Number)
        return;

    cJSON* animationMethod = cJSON_GetObjectItem (in, "animationMethod");
    if (!animationMethod || animationMethod->type != cJSON_String)
    {
        cJSON_AddStringToObject (out, "error", "animationMethod not provided");
        return;
    }

    if (strcasecmp (animationMethod->valuestring, "getAttackDecay") == 0)
    {
        double attack, decay;
        getAttackDecayVals (nodeId->valueint, &attack, &decay);
        cJSON_AddNumberToObject (out, "attackVal", attack);
        cJSON_AddNumberToObject (out, "decayVal", decay);
    }
    else if (strcasecmp (animationMethod->valuestring, "updateAttack") == 0)
    {
        cJSON* attackAmt = cJSON_GetObjectItem (in, "attackAmt");
        if (!attackAmt || attackAmt->type != cJSON_Number)
        {
            cJSON_AddStringToObject (out, "error", "attackAmt not provided");
            return;
        }
        updateAttackDecayAttack (nodeId->valueint, attackAmt->valuedouble);
    }
    else if (strcasecmp (animationMethod->valuestring, "updateDecay") == 0)
    {
        cJSON* decayAmt = cJSON_GetObjectItem (in, "decayAmt");
        if (!decayAmt || decayAmt->type != cJSON_Number)
        {
            cJSON_AddStringToObject (out, "error", "decayAmt not provided");
            return;
        }
        updateAttackDecayDecay (nodeId->valueint, decayAmt->valuedouble);
    }
    else
        cJSON_AddStringToObject (out,
                                 "error",
                                 "animationPlugin does not handle this method");

    cJSON_AddStringToObject (out, "success", "success");
}


/* HEAD Stuff */
static const struct LSD_ScenePluginHEAD pluginHead = {
    animationPluginInit,
    animationPluginClean,
    animationPluginRPC
};


/*
  * In order for LightShoppe to properly traverse scopes and
  *connect to the Plugin HEAD, an accessor function named
  *`getPluginHead`
  * must be included by the plugin author that returns a
  *reference to the Plugin HEAD
  */

extern const struct LSD_ScenePluginHEAD*
getPluginHead ()
{
    return &pluginHead;
}