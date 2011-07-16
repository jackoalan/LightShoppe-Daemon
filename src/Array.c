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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "GarbageCollector.h"
#include "DBArrOps.h"
#include "Logging.h"

#include "Array.h"

#include "DBArr.h"

/* Gettext stuff */
#include <libintl.h>
#define _(String) gettext (String)

/* Name of this component for logging */
static const char LOG_COMP[] = "Array.c";

/* Private function to run destructor on entire unit using
 * head data */
void
destructAll (struct LSD_ArrayUnit* unit)
{

    if (unit)
    {
        if (!unit->parent->destructor)
            return;

        int i;
        for (i = 0; i < unit->parent->mul; ++i)
        {

            if (unit->unitIdx >= unit->parent->numUnits - 1)
                if (i > unit->parent->maxIdx % unit->parent->mul)
                    break;


            unit->parent->destructor (unit->buffer +
                                      ( unit->parent->elemSize * i ));
        }
    }
}


int
recursiveClear (struct LSD_ArrayUnit* unit)
{
    int result;

    if (!unit)
    {
        doLog (ERROR, LOG_COMP, _("Unit is NULL in recursiveClear()."));
        return -1;
    }

    if (unit->nextUnit)
    {
        result = recursiveClear (unit->nextUnit);
        destructAll (unit);
        free (unit->nextUnit);
        unit->nextUnit = NULL;
        free (unit->buffer);
        unit->buffer = NULL;
        return result;
    }
    else
    {
        destructAll (unit);
        free (unit->buffer);
        unit->buffer = NULL;
        return 0;
    }
}


int
recursiveResolve (struct LSD_ArrayUnit* curUnit, size_t targetUnitNum,
                  struct LSD_ArrayUnit** targetPtrBind)
{
    if (!curUnit)
    {
        doLog (ERROR, LOG_COMP, _("curUnit is NULL in recursiveResolve()."));
        return -1;
    }

    if (curUnit->unitIdx == targetUnitNum)
    {
        if (targetPtrBind)
            *targetPtrBind = curUnit;
        return 0;
    }
    else if (curUnit->nextUnit)
        return recursiveResolve (curUnit->nextUnit,
                                 targetUnitNum,
                                 targetPtrBind);
    else
    {
        doLog (ERROR, LOG_COMP, _("Error while linking to ArrayUnit: Unit index %d not found."),
                 (int)targetUnitNum);
        return -1;
    }
}


int
makeArray (struct LSD_ArrayHead* target,
           size_t arrMul,
           size_t elemSize,
           short elemDel,
           void ( *destructor )(void* elem))
{
    if (!target)
    {
        doLog (ERROR, LOG_COMP, _("No target pointer provided in makeArray()."));
        return -1;
    }

    if (elemSize < 1)
    {
        doLog (ERROR, LOG_COMP, _("Invalid element size declared in makeArray()."));
        return -1;
    }

    if (arrMul < 1)
    {
        doLog (ERROR, LOG_COMP, _("Invalid array multiple declared in makeArray()."));
        return -1;
    }

    if (destructor)
        target->destructor = destructor;

    if (elemDel)
        target->delStat = DEL_ALLOWED;
    else
        target->delStat = NO_DEL_ALLOWED;

    target->mul = arrMul;
    target->capacity = arrMul;

    target->elemSize = elemSize;

    target->numElems = 0;

    target->maxIdx = -1;

    target->numUnits = 1;

    target->firstUnit = malloc (sizeof( struct LSD_ArrayUnit ));

    target->lastUnit = target->firstUnit;

    if (!target->firstUnit)
    {
        doLog (ERROR, LOG_COMP, _("Error occurred while allocating first ArrayUnit for array."));
        return -1;
    }

    target->firstUnit->parent = target;
    target->firstUnit->unitIdx = 0;
    target->firstUnit->nextUnit = NULL;
    size_t arrSize = elemSize * arrMul;
    target->firstUnit->buffer = malloc (arrSize);
    if (!target->firstUnit->buffer)
    {
        doLog (ERROR, LOG_COMP, _("Error occurred while allocating array block for first ArrayUnit."));
        return -1;
    }
    /* Set Memory to zero to ensure proper behaviour (NULL
     * is a skip condition for destructors) */
    memset (target->firstUnit->buffer, 0, arrSize);

    return 0;
}


int
clearArray (struct LSD_ArrayHead* toclear)
{
    if (!toclear)
    {
        doLog (ERROR, LOG_COMP, _("No arrayHead provided for clearArray()."));
        return -1;
    }

    int errcode = 0;

    if (toclear->firstUnit)
    {
        errcode = recursiveClear (toclear->firstUnit);
        free (toclear->firstUnit);
        toclear->firstUnit = NULL;
        toclear->lastUnit = NULL;
    }

    if (toclear->delStat == DEL_ID_ASSIGN)
    {
        lsdgc_removeArrIdMarks (toclear->dbId);
        toclear->delStat = NO_DEL_ALLOWED;
    }

    toclear->capacity = 0;
    toclear->numUnits = 0;
    toclear->numElems = 0;
    toclear->maxIdx = 0;

    return errcode;
}


int
pickIdx (struct LSD_ArrayHead* array, void** targetPtrBind, size_t idx)
{

    if (!array)
    {
        doLog (ERROR, LOG_COMP, _("Array is NULL in pickIdx()."));
        return -1;
    }

    if (idx < 0)
    {
        doLog (ERROR, LOG_COMP, _("Invalid value for idx in pickIdx()."));
        return -1;
    }

    if (idx > array->maxIdx)
    {
        doLog (ERROR, LOG_COMP, _("Index %d out of bounds in pickIdx()."), idx);
        return -1;
    }

    int unitnum;
    int elemidx;
    unitnum = idx / array->mul;
    elemidx = idx % array->mul;

    struct LSD_ArrayUnit* targetUnit;
    if (recursiveResolve (array->firstUnit, unitnum, &targetUnit) == 0)
    {
        if (targetPtrBind)
            *targetPtrBind = targetUnit->buffer + array->elemSize * elemidx;
        return 0;
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Cannot resolve array idx %d."), (int)idx);
        return -1;
    }
}


int
zeroIdx (struct LSD_ArrayHead* array, size_t idx)
{
    int unitnum;
    int elemidx;
    unitnum = idx / array->mul;
    elemidx = idx % array->mul;

    struct LSD_ArrayUnit* targetUnit;
    if (recursiveResolve (array->firstUnit, unitnum, &targetUnit) == 0)
    {
        memset (targetUnit->buffer + array->elemSize * elemidx,
                0,
                array->elemSize);
        return 0;
    }
    doLog (ERROR, LOG_COMP, _("Unable to zero memory for delIdx()."));
    return -1;
}


int
delIdx (struct LSD_ArrayHead* array, size_t idx)
{
    if (!array)
    {
        doLog (ERROR, LOG_COMP, _("array is NULL for delIdx()."));
        return -1;
    }

    if (idx < 0)
    {
        doLog (ERROR, LOG_COMP, _("Invalid value for idx in pickIdx()."));
        return -1;
    }

    if (idx > array->maxIdx)
    {
        doLog (ERROR, LOG_COMP, _("Error in delIdx(): index %d out of bounds."), idx);
        return -2;
    }

    if (array->delStat == NO_DEL_ALLOWED)
    {
        doLog (ERROR, LOG_COMP, _("Delete is explicitly not allowed on this array."));
        return -1;
    }

    /* Run destructor for index */
    void* destructTarget;
    if (pickIdx (array, &destructTarget, idx) == 0)
        if (array->destructor)
            array->destructor (destructTarget);

    /* Zero out this memory region */
    zeroIdx (array, idx);

    if (array->delStat == DEL_ALLOWED)
    {
        int arrId;
        lsdgc_getNewArrayId (&arrId);
        if (lsdgc_setArrMark (arrId, idx) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Error setting initial array mark in DB."));
            return -1;
        }
        --array->numElems;
        array->dbId = arrId;
        array->delStat = DEL_ID_ASSIGN;
    }
    else if (array->delStat == DEL_ID_ASSIGN)
    {
        if (lsdgc_setArrMark (array->dbId, idx) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Error setting array mark in DB."));
            return -1;
        }
        --array->numElems;
    }

    return 0;
}


int
insertElem (struct LSD_ArrayHead* array,
            size_t* targetIdxBind,
            void** targetPtrBind)
{
    if (!array)
    {
        doLog (ERROR, LOG_COMP, _("array is NULL for insertElem()."));
        return -1;
    }

    /* Attempt to use marksweep first */
    void* targetPtr;
    if (array->delStat == DEL_ID_ASSIGN)
    {
        int idxBind;
        if (lsdgc_discoverArrMark (array->dbId, &idxBind) == 0)
        {
            if (targetIdxBind)
                *targetIdxBind = idxBind;
            if (lsdgc_unsetArrMark (array->dbId, idxBind) < 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to unset array mark after discovery."));
                return -1;
            }

            if (pickIdx (array, &targetPtr, idxBind) < 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to resolve pointer after insertion."));
                return -1;
            }
            if (targetPtrBind)
                *targetPtrBind = targetPtr;
            ++array->numElems;
            return 0;
        }
    }

    /* Must trailblaze instead */

    int newIdx;
    newIdx = ++array->maxIdx;
    if (newIdx >= array->capacity)  /* Make new ArrayUnit */
    {
        struct LSD_ArrayUnit* au = malloc (sizeof( struct LSD_ArrayUnit ));
        if (!au)
        {
            doLog (ERROR, LOG_COMP, _("Cannot allocate memory for new ArrayUnit."));
            return -1;
        }
        size_t arraySize = array->elemSize * array->mul;
        au->buffer = malloc (arraySize);
        if (!au->buffer)
        {
            doLog (ERROR, LOG_COMP, _("Unable to allocate memory for new ArrayUnit's buffer."));
            return -1;
        }
        memset (au->buffer, 0, arraySize);

        if (!array->lastUnit)
            doLog (ERROR, LOG_COMP, _("No lastUnit in arrayHead %d."), (int)array);
        array->lastUnit->nextUnit = au;
        array->lastUnit = au;
        au->unitIdx = array->numUnits;
        au->parent = array;
        au->nextUnit = NULL;
        ++array->numUnits;
        array->capacity = array->numUnits * array->mul;
    }

    if (targetIdxBind)
        *targetIdxBind = newIdx;

    if (pickIdx (array, &targetPtr, newIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to resolve pointer using maxIdx."));
        return -1;
    }

    if (targetPtrBind)
        *targetPtrBind = targetPtr;
    ++array->numElems;
    return 0;
}


