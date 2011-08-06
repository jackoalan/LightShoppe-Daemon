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

#include "NodeInstAPI.h"
#include "Logging.h"

#include <stdlib.h>
#include <stdio.h>

/* Gettext stuff */
#ifndef HW_RVL
#include <libintl.h>
#define _(String) gettext (String)
#else
#define _(String) String
#endif

/* Name of this component for logging */
static const char LOG_COMP[] = "NodeInstAPI.c";

int
plugininst_addInstInput (struct LSD_SceneNodeInst const* inst,
                         int typeId,
                         const char* name,
                         int* inIdBind)
{

    struct LSD_SceneNodeInput* addedIn;
    if (lsddb_addNodeInstInput (inst, typeId, name, &addedIn, inIdBind) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Problem adding inst input."));
        return -1;
    }
    return 0;
}


int
plugininst_addInstOutput (struct LSD_SceneNodeInst const* inst,
                          int typeId,
                          const char* name,
                          int bfFuncIdx,
                          int bpFuncIdx,
                          int* outIdBind)
{

    int outId;
    struct LSD_SceneNodeOutput* addedOut;
    if (lsddb_addNodeInstOutput (inst, typeId, name, &addedOut, &outId,
                                 bfFuncIdx, bpFuncIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Problem adding inst output."));
        return -1;
    }

    if (outIdBind)
        *outIdBind = outId;

    struct LSD_SceneNodeClass* nc = inst->nodeClass;

    if (bfFuncIdx >= 0 && nc->bfFuncTbl[bfFuncIdx])
        addedOut->bufferFunc = nc->bfFuncTbl[bfFuncIdx];
    if (bpFuncIdx >= 0 && nc->bpFuncTbl[bpFuncIdx])
        addedOut->bufferPtr = nc->bpFuncTbl[bpFuncIdx];

    return 0;
}


int
plugininst_removeInstInput (struct LSD_SceneNodeInst const* inst, int inId)
{
    /* Verify ownership of input */
    struct LSD_SceneNodeInst const* verInst;
    if (lsddb_resolveInstFromInId (&verInst, inId) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to get inst from inId %d."), inId);
        return -1;
    }

    if (verInst != inst)
    {
        doLog (ERROR, LOG_COMP, _("Inst failed ownership test in removeInstInput()."));
        return -1;
    }

    if (lsddb_removeNodeInstInput (inId) < 0)
    {
        doLog (ERROR, LOG_COMP, _("There was an error removing inst input %d from node."), inId);
        return -1;
    }

    return 0;
}


int
plugininst_removeInstOutput (struct LSD_SceneNodeInst const* inst, int outId)
{
    /* Verify ownership of output */
    struct LSD_SceneNodeInst const* verInst;
    if (lsddb_resolveInstFromOutId (&verInst, outId) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to get inst from outId %d."), outId);
        return -1;
    }

    if (verInst != inst)
    {
        doLog (ERROR, LOG_COMP, _("Inst failed ownership test in removeInstOutput()."));
        return -1;
    }

    if (lsddb_removeNodeInstOutput (outId) < 0)
    {
        doLog (ERROR, LOG_COMP, _("There was an error removing inst output %d from node."), outId);
        return -1;
    }

    return 0;
}


int
plugininst_getInputStruct (struct LSD_SceneNodeInst const* inst,
                           struct LSD_SceneNodeInput const** inBind, int inId)
{
    if (!inst || !inBind)
        return -1;

    /* Verify ownership of input */
    struct LSD_SceneNodeInput* theIn;
    if (lsddb_resolveInputFromId (&theIn, inId) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to getInputStruct() of inId %d."), inId);
        return -1;
    }

    if (theIn->parentNode != inst)
    {
        doLog (ERROR, LOG_COMP, _("Input %d failed ownership test in getInputStruct()."), inId);
        return -1;
    }

    *inBind = theIn;
    return 0;
}


