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



#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <stdlib.h>


struct LSD_SceneNodeOutput;
struct LSD_SceneNodeInst;
struct LSD_SceneNodeClass;

// Function Pointer types used to port around plugin
// functions on existing data. Located here for centrality
typedef void(* const bfFunc)(struct LSD_SceneNodeOutput const *);
typedef void*(* const bpFunc)(struct LSD_SceneNodeOutput const *);


struct LSD_SceneNodeInput {
    int dbId;
    int typeId;
    struct LSD_SceneNodeInst const * parentNode;
    struct LSD_SceneNodeOutput* connection;
};

struct LSD_SceneNodeOutput {
    int dbId;
    int typeId;
    struct LSD_SceneNodeInst const * parentNode;
    uint64_t lastBufferedFrame;
    void* buffer;
    void(*bufferFunc)(struct LSD_SceneNodeOutput const * output);
    void*(*bufferPtr)(struct LSD_SceneNodeOutput const * output);
};

void destruct_SceneNodeOutput(void* nodeOutput);

// Buffer frame counter manipulation
void node_resetFrameCount();
void node_incFrameCount();

// Buffer wrapper func (eliminates redundant bufferings)
void* node_bufferOutput(struct LSD_SceneNodeOutput* output);

#include "PluginAPI.h"

struct LSD_SceneNodeClass {
    int dbId;
	struct LSD_ScenePlugin const * plugin;
    int(*nodeMakeFunc)(struct LSD_SceneNodeInst const *, void* instData);
	int(*nodeRestoreFunc)(struct LSD_SceneNodeInst const *, void* instData);
    void(*nodeCleanFunc)(struct LSD_SceneNodeInst const *, void* instData);
	void(*nodeDeleteFunc)(struct LSD_SceneNodeInst const *, void* instData);
	size_t instDataSize;
	bfFunc* bfFuncTbl;
	bpFunc* bpFuncTbl;
};

struct LSD_SceneNodeInst {
    int dbId;
    struct LSD_SceneNodeClass* nodeClass;
    void* data;
};

void destruct_SceneNodeInst(void* nodeInst);

/**
 * Notes on SceneNodeInstArr:
 * 
 * Insertion and removal of scene nodes is governed by the Plugin
 * API controller and will follow some simple management procedures
 * to prevent fragmentation of this array.
 *
 * First, this array is reallocated when five (5) free entry slots are
 * left. It is reallocated to the next multiple.
 *
 * When entries are freed and their members are deallocated, the index
 * of the array where the removal occured is logged in a mark-sweep
 * table of the database (with set,discover,and unset functions)
 */
struct LSD_SceneNodeInstArr {
    int capacity;
    int instCount;
    struct LSD_SceneNodeInst* insts;
};

/**
 * Discover open space using marksweep table
 * (or use instCount as index) to grant instance a spot in the array
 */
int instarr_addNodeInst(int dbId, int* arrIdxBind);

int instarr_removeNodeInst(int dbId, int* freedIdxBindj);

/**
 * SceneNodeInputArr and SceneNodeOutputArr behave in a similar manner
 * as SceneNodeInstArr
 */
struct LSD_SceneNodeInputArr {
    int capacity;
    int inCount;
    struct LSD_SceneNodeInput* ins;
};

struct LSD_SceneNodeOutputArr {
    int capacity;
    int outCount;
    struct LSD_SceneNodeOutput* outs;
};



#endif // NODE_H
