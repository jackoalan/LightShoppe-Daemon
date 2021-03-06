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

#include "CoreRPC.h"
#include "cJSON.h"

#include <event.h>
#include <evhttp.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "DBOps.h"
#include "SceneCore.h"
#include "PluginAPI.h"
#include "Logging.h"

/* Gettext stuff */
#ifndef HW_RVL
#include <libintl.h>
#define _(String) gettext (String)
#else
#define _(String) String
#endif

/* Name of this component for logging */
static const char LOG_COMP[] = "CoreRPC.c";


void
lsdCustomRPC (cJSON* req, cJSON* resp)
{
    cJSON* nodeId = cJSON_GetObjectItem (req, "nodeId");
    if (nodeId && nodeId->type == cJSON_Number)
    {
        struct LSD_ScenePlugin* plugin;
        if (lsddb_resolvePluginFromNodeId (&plugin, nodeId->valueint) < 0)
        {
            cJSON_AddStringToObject (resp, "error", "error");
            return;
        }

        if (plugin->handleRPC)
            plugin->handleRPC (req, resp);
    }
    else
    {
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("nodeId not present or not a number"));
        return;
    }
}


void
lsdJsonLibrary (cJSON* req, cJSON* resp)
{
    lsddb_jsonClassLibrary (resp);
}


void
lsdJsonPartitions (cJSON* req, cJSON* resp)
{
    lsddb_jsonParts (resp);
}


void
lsdJsonPatchSpace (cJSON* req, cJSON* resp)
{
    cJSON* psId = cJSON_GetObjectItem (req, "psId");
    if (psId && psId->type == cJSON_Number)
        lsddb_jsonPatchSpace (psId->valueint, resp);
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("psId key not present or not a number"));
}


void
lsdAddNode (cJSON* req, cJSON* resp)
{
    cJSON* psId = cJSON_GetObjectItem (req, "psId");
    if (psId && psId->type == cJSON_Number)
    {
        cJSON* classId = cJSON_GetObjectItem (req, "classId");
        if (classId && classId->type == cJSON_Number)
        {
            struct LSD_SceneNodeClass* nc = NULL;
            if (lsddb_resolveClassFromId (&nc, classId->valueint) < 0)
            {
                cJSON_AddStringToObject (resp,
                                         "error",
                                         _("Problem while resolving class object"));
                return;
            }

            /* Get Coords */
            cJSON* xVal = cJSON_GetObjectItem (req, "x");
            int xValVal;
            if (!xVal || xVal->type != cJSON_Number)
                xValVal = 0;
            else
                xValVal = xVal->valueint;

            cJSON* yVal = cJSON_GetObjectItem (req, "y");
            int yValVal;
            if (!yVal || yVal->type != cJSON_Number)
                yValVal = 0;
            else
                yValVal = yVal->valueint;

            int instId;
            if (lsddb_addNodeInst (psId->valueint, nc, &instId, NULL) < 0)
                cJSON_AddStringToObject (resp,
                                         "error",
                                         _("Unable to add node instance to DB"));
            else
            {
                if (lsddb_nodeInstPos (instId, xValVal, yValVal) < 0)
                    cJSON_AddStringToObject (resp,
                                             "error",
                                             _("Error while positioning node"));
                cJSON_AddStringToObject (resp, "success", "success");
                cJSON_AddNumberToObject (resp, "instId", instId);
            }
        }
        else
            cJSON_AddStringToObject (resp,
                                     "error",
                                     _("classId not present or not a number"));
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("psId key not present or not a number"));
}


void
lsdDeleteNode (cJSON* req, cJSON* resp)
{
    cJSON* nodeId = cJSON_GetObjectItem (req, "nodeId");
    if (nodeId && nodeId->type == cJSON_Number)
    {
        if (lsddb_removeNodeInst (nodeId->valueint) < 0)
            cJSON_AddStringToObject (resp, "error", "error");
        else
            cJSON_AddStringToObject (resp, "success", "success");
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("nodeId key not present or not a number"));
}


void
lsdUpdateNodeName (cJSON* req, cJSON* resp)
{
    cJSON* nodeId = cJSON_GetObjectItem (req, "nodeId");
    if (nodeId && nodeId->type == cJSON_Number)
    {

        cJSON* name = cJSON_GetObjectItem (req, "name");
        if (!name || name->type != cJSON_String)
        {
            cJSON_AddStringToObject (resp,
                                     "error",
                                     _("name not present or not a string"));
            return;
        }

        if (lsddb_updateNodeName (nodeId->valueint, name->valuestring) < 0)
            cJSON_AddStringToObject (resp, "error", "error");
        else
            cJSON_AddStringToObject (resp, "success", "success");
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("nodeId key not present or not a number"));
}


void
lsdSetNodeColour (cJSON* req, cJSON* resp)
{
    cJSON* nodeId = cJSON_GetObjectItem (req, "nodeId");
    if (nodeId && nodeId->type == cJSON_Number)
    {

        cJSON* colour = cJSON_GetObjectItem (req, "colour");
        if (!colour || colour->type != cJSON_Object)
        {
            cJSON_AddStringToObject (resp,
                                     "error",
                                     _("name not present or not an object"));
            return;
        }

        if (lsddb_setNodeColour (nodeId->valueint, colour) < 0)
            cJSON_AddStringToObject (resp, "error", "error");
        else
            cJSON_AddStringToObject (resp, "success", "success");
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("nodeId key not present or not a number"));
}


void
lsdAddFacade (cJSON* req, cJSON* resp)
{
    cJSON* psId = cJSON_GetObjectItem (req, "psId");
    if (psId && psId->type == cJSON_Number)
    {
        int newPS;
        if (lsddb_createPatchSpace ("New PatchSpace", &newPS,
                                    psId->valueint) < 0)
            cJSON_AddStringToObject (resp,
                                     "error",
                                     _("Unable to add node instance to DB"));
        else
        {
            /* Get Coords */
            cJSON* xVal = cJSON_GetObjectItem (req, "x");
            int xValVal;
            if (!xVal || xVal->type != cJSON_Number)
                xValVal = 0;
            else
                xValVal = xVal->valueint;

            cJSON* yVal = cJSON_GetObjectItem (req, "y");
            int yValVal;
            if (!yVal || yVal->type != cJSON_Number)
                yValVal = 0;
            else
                yValVal = yVal->valueint;

            if (lsddb_facadeInstPos (newPS, xValVal, yValVal) < 0)
                cJSON_AddStringToObject (resp,
                                         "error",
                                         _("Error while positioning facade node"));

            cJSON_AddStringToObject (resp, "success", "success");
            cJSON_AddNumberToObject (resp, "psId", newPS);
        }
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("psId key not present or not a number"));
}


void
lsdDeleteFacade (cJSON* req, cJSON* resp)
{
    cJSON* nodeId = cJSON_GetObjectItem (req, "facNodeId");
    if (nodeId && nodeId->type == cJSON_Number)
    {
        if (lsddb_removePatchSpace (nodeId->valueint) < 0)
            cJSON_AddStringToObject (resp, "error", "error");
        else
            cJSON_AddStringToObject (resp, "success", "success");
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("facNodeId key not present or not a number"));
}


void
lsdUpdateFacadeName (cJSON* req, cJSON* resp)
{
    cJSON* nodeId = cJSON_GetObjectItem (req, "facNodeId");
    if (nodeId && nodeId->type == cJSON_Number)
    {

        cJSON* name = cJSON_GetObjectItem (req, "name");
        if (!name || name->type != cJSON_String)
        {
            cJSON_AddStringToObject (resp,
                                     "error",
                                     _("name not present or not a string"));
            return;
        }

        if (lsddb_updatePatchSpaceName (nodeId->valueint,
                                        name->valuestring) < 0)
            cJSON_AddStringToObject (resp, "error", "error");
        else
            cJSON_AddStringToObject (resp, "success", "success");
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("facNodeId key not present or not a number"));
}


void
lsdSetFacadeColour (cJSON* req, cJSON* resp)
{
    cJSON* nodeId = cJSON_GetObjectItem (req, "facNodeId");
    if (nodeId && nodeId->type == cJSON_Number)
    {

        cJSON* colour = cJSON_GetObjectItem (req, "colour");
        if (!colour || colour->type != cJSON_Object)
        {
            cJSON_AddStringToObject (resp,
                                     "error",
                                     _("name not present or not an object"));
            return;
        }

        if (lsddb_setPatchSpaceColour (nodeId->valueint, colour) < 0)
            cJSON_AddStringToObject (resp, "error", "error");
        else
            cJSON_AddStringToObject (resp, "success", "success");
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("nodeId key not present or not a number"));
}


void
lsdGetFacade (cJSON* req, cJSON* resp)
{
    cJSON* facNodeId = cJSON_GetObjectItem (req, "facNodeId");
    if (facNodeId && facNodeId->type == cJSON_Number)
    {
        if (lsddb_jsonFacade (facNodeId->valueint, resp) < 0)
            cJSON_AddStringToObject (resp, "error", "error");
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("facNodeId not present or not a number"));
}


void
lsdCreateFacadeIn (cJSON* req, cJSON* resp)
{
    cJSON* facNodeId = cJSON_GetObjectItem (req, "facNodeId");
    if (facNodeId && facNodeId->type == cJSON_Number)
    {
        cJSON* name = cJSON_GetObjectItem (req, "name");
        if (!name || name->type != cJSON_String)
        {
            cJSON_AddStringToObject (resp, "error", _("name not provided"));
            return;
        }

        int inId;
        if (lsddb_createPatchSpaceIn (facNodeId->valueint, name->valuestring,
                                      &inId) < 0)
        {
            cJSON_AddStringToObject (resp, "error", "error");
            return;
        }
        cJSON_AddStringToObject (resp, "success", "success");
        cJSON_AddNumberToObject (resp, "inId", inId);
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("facNodeId not present or not a number"));
}


void
lsdDeleteFacadeIn (cJSON* req, cJSON* resp)
{
    cJSON* inId = cJSON_GetObjectItem (req, "inId");
    if (inId && inId->type == cJSON_Number)
    {
        if (lsddb_removePatchSpaceIn (inId->valueint) < 0)
            cJSON_AddStringToObject (resp, "error", "error");
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("inId not present or not a number"));
}


void
lsdUpdateFacadeIn (cJSON* req, cJSON* resp)
{
    cJSON* inId = cJSON_GetObjectItem (req, "inId");
    if (inId && inId->type == cJSON_Number)
    {
        cJSON* name = cJSON_GetObjectItem (req, "name");
        if (!name || name->type != cJSON_String)
        {
            cJSON_AddStringToObject (resp, "error", _("name not provided"));
            return;
        }

        if (lsddb_updatePatchSpaceInName (inId->valueint,
                                          name->valuestring) < 0)
        {
            cJSON_AddStringToObject (resp, "error", "error");
            return;
        }
        cJSON_AddStringToObject (resp, "success", "success");
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("inId not present or not a number"));
}


void
lsdCreateFacadeOut (cJSON* req, cJSON* resp)
{
    cJSON* facNodeId = cJSON_GetObjectItem (req, "facNodeId");
    if (facNodeId && facNodeId->type == cJSON_Number)
    {
        cJSON* name = cJSON_GetObjectItem (req, "name");
        if (!name || name->type != cJSON_String)
        {
            cJSON_AddStringToObject (resp, "error", _("name not provided"));
            return;
        }

        int outId;
        if (lsddb_createPatchSpaceOut (facNodeId->valueint, name->valuestring,
                                       &outId) < 0)
        {
            cJSON_AddStringToObject (resp, "error", "error");
            return;
        }
        cJSON_AddStringToObject (resp, "success", "success");
        cJSON_AddNumberToObject (resp, "outId", outId);
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("facNodeId not present or not a number"));
}


void
lsdDeleteFacadeOut (cJSON* req, cJSON* resp)
{
    cJSON* outId = cJSON_GetObjectItem (req, "outId");
    if (outId && outId->type == cJSON_Number)
    {
        if (lsddb_removePatchSpaceOut (outId->valueint) < 0)
            cJSON_AddStringToObject (resp, "error", "error");
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("outId not present or not a number"));
}


void
lsdUpdateFacadeOut (cJSON* req, cJSON* resp)
{
    cJSON* outId = cJSON_GetObjectItem (req, "outId");
    if (outId && outId->type == cJSON_Number)
    {
        cJSON* name = cJSON_GetObjectItem (req, "name");
        if (!name || name->type != cJSON_String)
        {
            cJSON_AddStringToObject (resp, "error", _("name not provided"));
            return;
        }

        if (lsddb_updatePatchSpaceOutName (outId->valueint,
                                           name->valuestring) < 0)
        {
            cJSON_AddStringToObject (resp, "error", "error");
            return;
        }
        cJSON_AddStringToObject (resp, "success", "success");
    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("outId not present or not a number"));
}


void
lsdWireNodes (cJSON* req, cJSON* resp)
{
    cJSON* leftFacadeInterior = cJSON_GetObjectItem (req, "leftFacadeInterior");
    if (!leftFacadeInterior || leftFacadeInterior->type != cJSON_Number)
    {
        cJSON_AddStringToObject (
            resp,
            "error",
            _("leftFacadeInterior not present or not a number"));
        return;
    }

    cJSON* leftNodeId = cJSON_GetObjectItem (req, "leftNodeId");
    if (!leftNodeId || leftNodeId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("leftNodeId not present or not a number"));
        return;
    }

    cJSON* rightFacadeInterior = cJSON_GetObjectItem (req,
                                                      "rightFacadeInterior");
    if (!rightFacadeInterior || rightFacadeInterior->type != cJSON_Number)
    {
        cJSON_AddStringToObject (
            resp,
            "error",
            _("rightFacadeInterior not present or not a number"));
        return;
    }

    cJSON* rightNodeId = cJSON_GetObjectItem (req, "rightNodeId");
    if (!rightNodeId || rightNodeId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("rightNodeId not present or not a number"));
        return;
    }

    int wireId;
    if (lsddb_wireNodes (leftFacadeInterior->valueint, leftNodeId->valueint,
                         rightFacadeInterior->valueint, rightNodeId->valueint,
                         &wireId) < 0)
        cJSON_AddStringToObject (resp, "error", _("Error while wiring nodes"));
    else
    {
        cJSON_AddStringToObject (resp, "success", "success");
        cJSON_AddNumberToObject (resp, "wireId", wireId);
    }
}


void
lsdUnwire (cJSON* req, cJSON* resp)
{
    cJSON* wireId = cJSON_GetObjectItem (req, "wireId");
    if (!wireId || wireId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("wireId not present or not a number"));
        return;
    }

    if (lsddb_unwireNodes (wireId->valueint) < 0)
        cJSON_AddStringToObject (resp, "error", _("Error while unwiring nodes"));
    else
        cJSON_AddStringToObject (resp, "success", "success");
}


void
lsdPositionNode (cJSON* req, cJSON* resp)
{
    cJSON* nodeId = cJSON_GetObjectItem (req, "nodeId");
    if (!nodeId || nodeId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("nodeId not present or not a number"));
        return;
    }

    cJSON* xVal = cJSON_GetObjectItem (req, "x");
    if (!xVal || xVal->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("x not present or not a number"));
        return;
    }

    cJSON* yVal = cJSON_GetObjectItem (req, "y");
    if (!yVal || yVal->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("y not present or not a number"));
        return;
    }

    if (lsddb_nodeInstPos (nodeId->valueint, xVal->valueint,
                           yVal->valueint) < 0)
        cJSON_AddStringToObject (resp, "error", _("Error while positioning node"));
    else
        cJSON_AddStringToObject (resp, "success", "success");
}


void
lsdPositionFacade (cJSON* req, cJSON* resp)
{
    cJSON* nodeId = cJSON_GetObjectItem (req, "childPSId");
    if (!nodeId || nodeId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("childPSId not present or not a number"));
        return;
    }

    cJSON* xVal = cJSON_GetObjectItem (req, "x");
    if (!xVal || xVal->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("x not present or not a number"));
        return;
    }

    cJSON* yVal = cJSON_GetObjectItem (req, "y");
    if (!yVal || yVal->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("y not present or not a number"));
        return;
    }

    if (lsddb_facadeInstPos (nodeId->valueint, xVal->valueint,
                             yVal->valueint) < 0)
        cJSON_AddStringToObject (resp, "error", _("Error while positioning node"));
    else
        cJSON_AddStringToObject (resp, "success", "success");
}


void
lsdPanPatchSpace (cJSON* req, cJSON* resp)
{
    cJSON* psId = cJSON_GetObjectItem (req, "psId");
    if (!psId || psId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("psId not present or not a number"));
        return;
    }

    cJSON* xVal = cJSON_GetObjectItem (req, "x");
    if (!xVal || xVal->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("x not present or not a number"));
        return;
    }

    cJSON* yVal = cJSON_GetObjectItem (req, "y");
    if (!yVal || yVal->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("y not present or not a number"));
        return;
    }

    cJSON* scaleVal = cJSON_GetObjectItem (req, "scale");
    if (!scaleVal || scaleVal->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("scale not present or not a number"));
        return;
    }

    if (lsddb_panPatchSpace (psId->valueint, xVal->valueint, yVal->valueint,
                             scaleVal->valuedouble) < 0)
    {
        cJSON_AddStringToObject (resp, "error", "error");
        return;
    }

    cJSON_AddStringToObject (resp, "success", "success");
}


void
lsdGetChannelPatch (cJSON* req, cJSON* resp)
{
    if (lsddb_getPatchChannels (resp) < 0)
        cJSON_AddStringToObject (resp, "error", _("Unable to get patch"));
}


void
lsdCreatePartition (cJSON* req, cJSON* resp)
{
    cJSON* partName = cJSON_GetObjectItem (req, "name");
    if (!partName || partName->type != cJSON_String)
    {
        cJSON_AddStringToObject (resp, "error", _("Error while adding partition"));
        return;
    }

    int partId;
    if (lsddb_createPartition (partName->valuestring, &partId) < 0)
    {
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("Unable to insert partition into DB"));
        return;
    }

    cJSON* imageFile = cJSON_GetObjectItem (req, "imageFile");
    if (imageFile && imageFile->type == cJSON_String)
        lsddb_updatePartitionImage (partId, imageFile->valuestring);

    cJSON_AddStringToObject (resp, "success", "success");
}


void
lsdUpdatePartition (cJSON* req, cJSON* resp)
{
    cJSON* partId = cJSON_GetObjectItem (req, "partId");
    if (!partId || partId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("partId not valid"));
        return;
    }

    cJSON* partName = cJSON_GetObjectItem (req, "name");
    if (!partName || partName->type != cJSON_String)
    {
        cJSON_AddStringToObject (resp, "error", _("name not valid"));
        return;
    }

    if (lsddb_updatePartitionName (partId->valueint, partName->valuestring) < 0)
    {
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("Unable to update partition name"));
        return;
    }

    cJSON* imageFile = cJSON_GetObjectItem (req, "imageFile");
    if (imageFile && imageFile->type == cJSON_String)
        lsddb_updatePartitionImage (partId->valueint, imageFile->valuestring);

    cJSON_AddStringToObject (resp, "success", "success");
}


void
lsdDeletePartition (cJSON* req, cJSON* resp)
{
    cJSON* partId = cJSON_GetObjectItem (req, "partId");
    if (!partId || partId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("partId not a valid value"));
        return;
    }

    if (lsddb_removePartition (partId->valueint) < 0)
    {
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("Unable to remove partition from DB"));
        return;
    }

    cJSON_AddStringToObject (resp, "success", "success");
}


void
lsdUpdateChannel (cJSON* req, cJSON* resp)
{
    cJSON* chanId = cJSON_GetObjectItem (req, "chanId");
    if (!chanId || chanId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("chanId not a valid value"));
        return;
    }

    if (lsddb_updatePatchChannel (chanId->valueint, req) < 0)
        cJSON_AddStringToObject (resp, "error", "error");
    else
        cJSON_AddStringToObject (resp, "success", "success");
}


void
lsdDeleteChannel (cJSON* req, cJSON* resp)
{
    cJSON* chanId = cJSON_GetObjectItem (req, "chanId");
    if (!chanId || chanId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("chanId not a valid value"));
        return;
    }

    if (lsddb_deletePatchChannel (chanId->valueint) < 0)
        cJSON_AddStringToObject (resp, "error", "error");
    else
        cJSON_AddStringToObject (resp, "success", "success");
}


void
lsdCreateChannel (cJSON* req, cJSON* resp)
{
    cJSON* partId = cJSON_GetObjectItem (req, "partId");
    if (!partId || partId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("partId not a valid value"));
        return;
    }

    if (lsddb_addPatchChannel (partId->valueint, req) < 0)
        cJSON_AddStringToObject (resp, "error", "error");
    else
        cJSON_AddStringToObject (resp, "success", "success");
}


void
lsdJsonPlugins (cJSON* req, cJSON* resp)
{
    if (lsddb_jsonPlugins (resp) < 0)
        cJSON_AddStringToObject (resp, "error", _("unable to get plugins"));
}


void
lsdDisablePlugin (cJSON* req, cJSON* resp)
{
    cJSON* pId = cJSON_GetObjectItem (req, "pluginId");
    if (!pId || pId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("pluginId not a valid value"));
        return;
    }

    if (lsddb_disablePlugin (pId->valueint) < 0)
        cJSON_AddStringToObject (resp, "error", _("Unable to disable plugin"));
    else
        cJSON_AddStringToObject (resp, "success", "success");
}


void
lsdEnablePlugin (cJSON* req, cJSON* resp)
{
    cJSON* pId = cJSON_GetObjectItem (req, "pluginId");
    if (!pId || pId->type != cJSON_Number)
    {
        cJSON_AddStringToObject (resp, "error", _("pluginId not a valid value"));
        return;
    }

    if (lsddb_enablePlugin (pId->valueint) < 0)
        cJSON_AddStringToObject (resp, "error", _("Unable to enable plugin"));
    else
        cJSON_AddStringToObject (resp, "success", "success");
}


/* Main request brancher */
int
handleJSONRequest (cJSON* req, cJSON* resp, int* reloadAfter)
{
    cJSON* method = cJSON_GetObjectItem (req, "method");
    if (method && method->type == cJSON_String)
    {

        /* Conditionally choose correct code path for method
         * in question */
        if (strcasecmp (method->valuestring, "lsdJsonLibrary") == 0)
            lsdJsonLibrary (req, resp);
        else if (strcasecmp (method->valuestring, "lsdJsonPartitions") == 0)
            lsdJsonPartitions (req, resp);
        else if (strcasecmp (method->valuestring, "lsdJsonPatchSpace") == 0)
            lsdJsonPatchSpace (req, resp);
        else if (strcasecmp (method->valuestring, "lsdAddNode") == 0)
            lsdAddNode (req, resp);
        else if (strcasecmp (method->valuestring, "lsdDeleteNode") == 0)
            lsdDeleteNode (req, resp);
        else if (strcasecmp (method->valuestring, "lsdUpdateNodeName") == 0)
            lsdUpdateNodeName (req, resp);
        else if (strcasecmp (method->valuestring, "lsdSetNodeColour") == 0)
            lsdSetNodeColour (req, resp);
        else if (strcasecmp (method->valuestring, "lsdAddFacade") == 0)
            lsdAddFacade (req, resp);
        else if (strcasecmp (method->valuestring, "lsdDeleteFacade") == 0)
            lsdDeleteFacade (req, resp);
        else if (strcasecmp (method->valuestring, "lsdUpdateFacadeName") == 0)
            lsdUpdateFacadeName (req, resp);
        else if (strcasecmp (method->valuestring, "lsdSetFacadeColour") == 0)
            lsdSetFacadeColour (req, resp);
        else if (strcasecmp (method->valuestring, "lsdGetFacade") == 0)
            lsdGetFacade (req, resp);
        else if (strcasecmp (method->valuestring, "lsdCreateFacadeIn") == 0)
            lsdCreateFacadeIn (req, resp);
        else if (strcasecmp (method->valuestring, "lsdDeleteFacadeIn") == 0)
            lsdDeleteFacadeIn (req, resp);
        else if (strcasecmp (method->valuestring, "lsdUpdateFacadeIn") == 0)
            lsdUpdateFacadeIn (req, resp);
        else if (strcasecmp (method->valuestring, "lsdCreateFacadeOut") == 0)
            lsdCreateFacadeOut (req, resp);
        else if (strcasecmp (method->valuestring, "lsdDeleteFacadeOut") == 0)
            lsdDeleteFacadeOut (req, resp);
        else if (strcasecmp (method->valuestring, "lsdUpdateFacadeOut") == 0)
            lsdUpdateFacadeOut (req, resp);
        else if (strcasecmp (method->valuestring, "lsdWireNodes") == 0)
            lsdWireNodes (req, resp);
        else if (strcasecmp (method->valuestring, "lsdUnwire") == 0)
            lsdUnwire (req, resp);
        else if (strcasecmp (method->valuestring, "lsdPositionNode") == 0)
            lsdPositionNode (req, resp);
        else if (strcasecmp (method->valuestring, "lsdPositionFacade") == 0)
            lsdPositionFacade (req, resp);
        else if (strcasecmp (method->valuestring, "lsdPanPatchSpace") == 0)
            lsdPanPatchSpace (req, resp);
        else if (strcasecmp (method->valuestring, "lsdGetChannelPatch") == 0)
            lsdGetChannelPatch (req, resp);
        else if (strcasecmp (method->valuestring, "lsdCreatePartition") == 0)
        {
            lsdCreatePartition (req, resp);
            *reloadAfter = 1;
        }
        else if (strcasecmp (method->valuestring, "lsdDeletePartition") == 0)
        {
            lsdDeletePartition (req, resp);
            *reloadAfter = 1;
        }
        else if (strcasecmp (method->valuestring, "lsdUpdatePartition") == 0)
            lsdUpdatePartition (req, resp);
        else if (strcasecmp (method->valuestring, "lsdUpdateChannel") == 0)
        {
            lsdUpdateChannel (req, resp);
            *reloadAfter = 1;
        }
        else if (strcasecmp (method->valuestring, "lsdDeleteChannel") == 0)
        {
            lsdDeleteChannel (req, resp);
            *reloadAfter = 1;
        }
        else if (strcasecmp (method->valuestring, "lsdCreateChannel") == 0)
        {
            lsdCreateChannel (req, resp);
            *reloadAfter = 1;
        }
        else if (strcasecmp (method->valuestring, "lsdJsonPlugins") == 0)
            lsdJsonPlugins (req, resp);
        else if (strcasecmp (method->valuestring, "lsdDisablePlugin") == 0)
        {
            lsdDisablePlugin (req, resp);
            *reloadAfter = 1;
        }
        else if (strcasecmp (method->valuestring, "lsdEnablePlugin") == 0)
        {
            lsdEnablePlugin (req, resp);
            *reloadAfter = 1;
        }
        else if (strcasecmp (method->valuestring, "lsdCustomRPC") == 0)
            lsdCustomRPC (req, resp);

        else
            cJSON_AddStringToObject (
                resp,
                "error",
                _("Specified method is not handled by this version of LSD"));

    }
    else
        cJSON_AddStringToObject (resp,
                                 "error",
                                 _("Method key is not present or not a string"));
        /* return -1; */

    return 0;
}


/* Callback for requests made to RPC */
void
rpcReqCB (struct evhttp_request* req, void* arg)
{
    /* Flag to request a reload after request returns */
    int reloadAfter = 0;

    struct evbuffer* inputPostBuf = evhttp_request_get_input_buffer (req);
    /* Ensure input buffer is null terminated (buffer
     * overflows are bad) */
    evbuffer_add_printf (inputPostBuf, "%c", '\0');
    const unsigned char* inputPost = evbuffer_pullup (inputPostBuf, -1);

    /* Setup json objects for parsing/returning */
    cJSON* input = cJSON_Parse ((const char*)inputPost);
    cJSON* returnjson = cJSON_CreateObject ();

    if (input)
    {
        if (handleJSONRequest (input, returnjson, &reloadAfter) < 0)
            doLog (ERROR, LOG_COMP, _("There was a problem while running RPC handler."));
    }
    else
        cJSON_AddStringToObject (returnjson,
                                 "error",
                                 _("Unable to parse any JSON from HTTP POST data"));

    struct evbuffer* repBuf = evbuffer_new ();

    /* Print results into HTTP reply buffer */
    char* returnjsonStr = cJSON_PrintUnformatted (returnjson);
    evbuffer_add_printf (repBuf, "%s", returnjsonStr);
    free (returnjsonStr);

    /* Memory leaks are bad... very bad */
    cJSON_Delete (returnjson);
    cJSON_Delete (input);

    evhttp_add_header (evhttp_request_get_output_headers (req), 
                       "Content-Type", "application/json");
    evhttp_send_reply (req, 200, "OK", repBuf);

    evbuffer_free (repBuf);

    if (reloadAfter)
        handleReload (0, 0, NULL);
}

/* Generates an HTTP error */
void
_serveFileErr (struct evhttp_request* req, const char* msg)
{
    struct evbuffer* errBuf = evbuffer_new ();
    evbuffer_add_printf (errBuf, "<html><body>\n");
    evbuffer_add_printf (errBuf, "<h1>Internal LightShoppe Error</h1>\n");
    evbuffer_add_printf (errBuf, "<p>%s</p>\n", msg);
    evbuffer_add_printf (errBuf, "</body></html>\n");
    evhttp_add_header (evhttp_request_get_output_headers (req),
                       "Content-Type", "text/html");
    evhttp_send_reply (req, 404, "File Not Found", errBuf);
    evbuffer_free (errBuf);
}

/* user-set location prefix */
static const char* urlPrefix;
/* Length of prefix to assist in chopping it from the uri */
static int urlPrefixLen;

struct MIMEEntry
{
    const char* fExt;
    const char* mType;
};

static const struct MIMEEntry mTable[] =
{
    {".html","text/html"},
    {".js","application/javascript"},
    {".css","text/css"},
    {".svg","image/svg+xml"},
    {".png","image/png"},
    {".jpg","image/jpeg"},
    {".ico","image/vnd.microsoft.icon"}
};
static const int mTableLen = 7;

static const char miscMime[] = "application/octet-stream";

void
_serveFile (struct evhttp_request* req, const char* path)
{
    /* No directories allowed */
    int last = strlen (path);
    if (path[last-1] == '/')
    {
        _serveFileErr (req, "Unable to list directories.");
        return;
    }
    
    /* Open file and check its presence */
    FILE* toServe;
    toServe = fopen (path, "rb");
    if (!toServe)
    {
        _serveFileErr (req, "Unable to open file requested.");
        return;
    }
            
    /* Find mime type of opened file */
    const char* fileExt;
    const char* mimeType;
    fileExt = strrchr (path, '.');
    
    if (fileExt)
    {
        int i;
        for (i=0;i<mTableLen;++i)
        {
            const struct MIMEEntry* mEnt = &mTable[i];
            if (strcmp (fileExt, mEnt->fExt) == 0)
            {
                mimeType = mEnt->mType;
                goto mimeDone;
            }
        }
    }
    
    mimeType = miscMime;
    
mimeDone:
        
    /* Attach mime to request */
    evhttp_add_header (evhttp_request_get_output_headers (req),
                       "Content-Type", mimeType);
    
    /* get file length */
    size_t fileLen;
    fseek (toServe, 0, SEEK_END);
    fileLen = ftell (toServe);
    fseek (toServe, 0, SEEK_SET);
    char sfileLen[32];
    snprintf (sfileLen, 32, "%d", (int)fileLen);
    
    /* Attach length to request */
    evhttp_add_header (evhttp_request_get_output_headers (req),
                       "Content-Length", sfileLen);
        
    /* Create and fill buffer with file content */
#define BUFFERCHUNK 2048
    struct evbuffer* fileBuf = evbuffer_new();
    char chunkBuf[BUFFERCHUNK];
    int readSize = BUFFERCHUNK;
    
    while (readSize == BUFFERCHUNK)
    {
        readSize = fread (chunkBuf, 1, BUFFERCHUNK, toServe);
        evbuffer_add (fileBuf, chunkBuf, readSize);
    }
    
    evhttp_send_reply (req, 200, "OK", fileBuf);
    evbuffer_free (fileBuf);
}

void
serveFileCore (struct evhttp_request* req)
{
#ifndef WEB_DIR
    _serveFileErr (req, "WEB_DIR not provided at compile time.")
    return;
#else
    char comppath[256];
    const char* subpath = req->uri + urlPrefixLen;
    snprintf (comppath, 256, "%s%s", WEB_DIR, subpath);
    _serveFile (req, comppath);
#endif
}

void
serveFilePlugin (struct evhttp_request* req)
{
#ifndef WEB_PLUGIN_DIR
    _serveFileErr (req, "WEB_PLUGIN_DIR not provided at compile time.");
    return;
#else
    char comppath[256];
    /* Length of prefix + "/plugins" */
    const char* subpath = req->uri + urlPrefixLen + 8;
    snprintf (comppath, 256, "%s%s", WEB_PLUGIN_DIR, subpath);
    _serveFile (req, comppath);
#endif
}

/* Serve main index.html */
void
srvIndexCB (struct evhttp_request* req, void* arg)
{
    struct evbuffer* repBuf = evbuffer_new ();
    if (lsddb_indexHtmlGen (repBuf) < 0)
        evbuffer_add_printf (repBuf, _("Error while generating index\n"));
    evhttp_add_header (evhttp_request_get_output_headers (
                                                          req), "Content-Type", "text/html");
    evhttp_add_header (evhttp_request_get_output_headers (
                                                          req), "Pragma", "no-cache");
    evhttp_send_reply (req, 200, "OK", repBuf);
    evbuffer_free (repBuf);
}


/* Set location header to prefix/main/ */
void
mainRedirect (struct evhttp_request* req, void* arg)
{
    char compLocation[256];
    snprintf (compLocation, 256, "%s/main/", urlPrefix);
    evhttp_add_header (evhttp_request_get_output_headers (req),
                       "Location", compLocation);
    evhttp_add_header (evhttp_request_get_output_headers (req),
                       "Content-Type", "text/html");
    
    struct evbuffer* redirBuf = evbuffer_new ();
    evbuffer_add_printf (redirBuf, "<html><body>Lightshoppe may be accessed at %s</body></html>",
                         compLocation);
    
    evhttp_send_reply (req, 301, "Moved Permanently", redirBuf);
    evbuffer_free (redirBuf);
}


/* Catchall for requests not made to /lsd/rpc */
void
reqCB (struct evhttp_request* req, void* arg)
{
    
    /* Determine what type of file serve request will be made */
    const char* uriNoPrefix = req->uri + urlPrefixLen;
    
    if (strlen (uriNoPrefix) <= 1)
    {
        /* Root Requested, redirect main */
        mainRedirect (req, NULL);
    }
    else if (strncmp (uriNoPrefix, "/plugins/", 9) == 0)
    {
        /* Get plugin file */
        serveFilePlugin (req);
    }
    else
    {
        /* Normal web file serve */
        serveFileCore (req);
    }
}




/* Libevent stuff below */

static struct event_base* eb;
static struct evhttp* eh;

int
openRPC (struct event_base* ebin, int port, const char* prefix)
{
    urlPrefix = prefix;
    urlPrefixLen = strlen (prefix);
    char compPrefix[256];
    
    eb = ebin;
    eh = evhttp_new (eb);
    if (evhttp_bind_socket (eh, "0.0.0.0", port) < 0)
        return -1;
    
    evhttp_set_gencb (eh, reqCB, NULL);
    
    snprintf (compPrefix, 256, "%s/main/rpc", prefix);
    evhttp_set_cb (eh, compPrefix, rpcReqCB, NULL);
    
    snprintf (compPrefix, 256, "%s/main/", prefix);
    evhttp_set_cb (eh, compPrefix, srvIndexCB, NULL);
    
    snprintf (compPrefix, 256, "%s/main", prefix);
    evhttp_set_cb (eh, compPrefix, mainRedirect, NULL);
    
    evhttp_set_cb (eh, "/", mainRedirect, NULL);
    evhttp_set_cb (eh, "", mainRedirect, NULL);
    

    return 0;
}


int
closeRPC ()
{
    evhttp_free (eh);

    return 0;
}


