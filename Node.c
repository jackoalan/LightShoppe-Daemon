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



#include "Node.h"


void destruct_SceneNodeOutput(void* nodeOutput){
    if(nodeOutput){
        struct LSD_SceneNodeOutput* castOut = nodeOutput;
        if(castOut->buffer){
            free(castOut->buffer);
            castOut->buffer = NULL;
        }
    }
}

void destruct_SceneNodeInst(void* nodeInst){
    if(nodeInst){
        struct LSD_SceneNodeInst* castInst = nodeInst;
        if(castInst->nodeClass && castInst->nodeClass->nodeCleanFunc){
            castInst->nodeClass->nodeCleanFunc(castInst,castInst->data);
        }
		free(castInst->data);
		castInst->data = NULL;
    }
}

// Current frame count (for buffering purposes)
static uint64_t curFrame;

void node_resetFrameCount(){
    curFrame = 0;
}

void node_incFrameCount(){
    ++curFrame;
}

// Buffer wrapper func (eliminates redundant bufferings)
void* node_bufferOutput(struct LSD_SceneNodeOutput* output){
    if(output->lastBufferedFrame != curFrame){
        output->bufferFunc(output);
        output->lastBufferedFrame = curFrame;
    }
    return output->bufferPtr(output);
}
