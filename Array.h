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

#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>

struct LSD_ArrayUnit;

enum LSD_ARRAY_DEL_STAT
{
    NO_DEL_ALLOWED,
    DEL_ALLOWED,
    DEL_ID_ASSIGN
};

struct LSD_ArrayHead
{
    int dbId;
    size_t mul;
    enum LSD_ARRAY_DEL_STAT delStat;
    size_t capacity;
    size_t numUnits;
    size_t elemSize;
    size_t numElems;
    size_t maxIdx;
    struct LSD_ArrayUnit* firstUnit;
    struct LSD_ArrayUnit* lastUnit;
    void ( *destructor )(void* elem);
};

struct LSD_ArrayUnit
{
    struct LSD_ArrayHead* parent;
    size_t unitIdx;
    struct LSD_ArrayUnit* nextUnit;
    void* buffer;
};

int makeArray (struct LSD_ArrayHead* target,
               size_t arrMul,
               size_t elemSize,
               short elemDel,
               void ( *destructor )(void* elem));
int
clearArray (struct LSD_ArrayHead* toclear);


int
pickIdx (struct LSD_ArrayHead* array, void** targetPtrBind, size_t idx);


int
delIdx (struct LSD_ArrayHead* array, size_t idx);


int
insertElem (struct LSD_ArrayHead* array,
            size_t* targetIdxBind,
            void** targetPtrBind);



#endif /* ARRAY_H */
