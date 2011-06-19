/*
**    This file is part of LightShoppe.
**    Copyright 2011 Jack Andersen
**
**    LightShoppe is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    LightShoppe is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with LightShoppe.  If not, see <http://www.gnu.org/licenses/>.
**
**    @author Jack Andersen <jackoalan@gmail.com>
*/


#ifndef SCENECORE_H
#define SCENECORE_H

/**
 * Core Library for LSD's Scene Node Architecture
 */

/**
 * This file provides bindings for initialising
 * and maintaining all necessary arrays and in-memory database to
 * maintain LSD's state. All arrays are allocated once
 * at startup and requiring deallocation and reallocation (restart)
 * to introduce additional entries. An expection to this rule is the
 * SceneNodeInstArr. This array is a dynamic one which always holds a
 * default multiple of 50 instance entries. This multiple can be
 * adjusted by the user in LSD's configuration (argument or otherwise).
 */


/**
 * Late Break:
 * This file also contains (following this message)
 * Low Level object structures for Partitions, Channels,
 * Universes (for OLA), Addresses and the like.
 */

#include "Node.h"
#include <event.h>

struct LSD_Univ{
    int olaUnivId;
    int maxIdx;
    uint8_t* buffer;
};

void destruct_Univ(void* univ);

struct LSD_Addr {
    int dbId;
    struct LSD_Univ* univ;
    int addr;
    int b16;
};

struct LSD_Channel {
    int dbId;
    int single;
    struct LSD_Addr rAddr;
    struct LSD_Addr gAddr;
    struct LSD_Addr bAddr;
    struct LSD_SceneNodeOutput* output;
};


struct LSD_Partition{
    int dbId;
    struct LSD_SceneNodeInst* facade;
    size_t numChans;
    struct LSD_Channel* chans;
};


#include <sqlite3.h>

// Primary entry point for scene node architecture
// Establishes managerial support for database operations
// and the like
int lsdSceneEntry();

int lsdSceneRevert(const char* dbpath, sqlite3* revertTarget);

// Request call for state reload
void handleReload(int ont,short int two, void* three);

struct event_base* getMainEB();

#endif // SCENECORE_H
