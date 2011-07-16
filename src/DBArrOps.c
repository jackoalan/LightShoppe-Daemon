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

#include "DBArrOps.h"

#include <stdio.h>

#include "DBArr.h"
#include "DBOps.h"
#include "Node.h"
#include "PluginAPI.h"
#include "SceneCore.h"
#include "Logging.h"

/* Gettext stuff */
#include <libintl.h>
#define _(String) gettext (String)

/* Name of this component for logging */
static const char LOG_COMP[] = "DBArrOps.c";

static const int STDMUL = 50;

#define MAKE(array, structure, delete, destructor, \
             num) if (makeArray (getArr_ ## array (), \
                                 STDMUL, \
                                 sizeof( struct structure ), delete, \
                                 destructor) < 0) \
    {doLog (ERROR, LOG_COMP, _("Error while establishing array: %d."), num); return -1; }

int
initLsdArrays ()
{
    MAKE (lsdDBStmtArr, LSD_SceneDBStmt, 0, destruct_SceneDBStmt, 1);
    MAKE (lsdNodeInstArr, LSD_SceneNodeInst, 1, destruct_SceneNodeInst, 2);
    MAKE (lsdNodeClassArr, LSD_SceneNodeClass, 0, NULL, 3);
    MAKE (lsdNodeInputArr, LSD_SceneNodeInput, 1, NULL, 4);
    MAKE (lsdNodeOutputArr, LSD_SceneNodeOutput, 1, destruct_SceneNodeOutput, 5);
    MAKE (lsdPluginArr, LSD_ScenePlugin, 0, destruct_ScenePlugin, 7);
    MAKE (lsdPartitionArr, LSD_Partition, 0, NULL, 8);
    MAKE (lsdUnivArr, LSD_Univ, 0, destruct_Univ, 9);
    MAKE (lsdChannelArr, LSD_Channel, 0, NULL, 10);

    return 0;
}


#define CLEAR(array) if (clearArray (getArr_ ## array ()) < 0) \
    {doLog (WARNING, LOG_COMP, _("Error while clearing array.")); problem = -1; }

int
clearLsdArrays ()
{
    int problem = 0;

    CLEAR (lsdDBStmtArr);
    CLEAR (lsdNodeInstArr);
    CLEAR (lsdNodeClassArr);
    CLEAR (lsdNodeInputArr);
    CLEAR (lsdNodeOutputArr);
    CLEAR (lsdPluginArr);
    CLEAR (lsdPartitionArr);
    CLEAR (lsdUnivArr);
    CLEAR (lsdChannelArr);

    return problem;
}


