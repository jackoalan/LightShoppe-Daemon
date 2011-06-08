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


#include "NodeInstAPI.h"

#include <stdlib.h>
#include <stdio.h>

int plugininst_addInstInput(struct LSD_SceneNodeInst const * inst,
                            int typeId,
                            const char* name,
                            void(*connectCB)(struct LSD_SceneNodeInput* input, struct LSD_SceneNodeOutput const * connection),
                            void(*disconnectCB)(struct LSD_SceneNodeInput* input)){
	
	if(lsddb_addNodeInstInput(inst,typeId,name,NULL,NULL)<0){
		fprintf(stderr,"Problem adding inst input\n");
		return -1;
	}
	return 0;
}

int plugininst_addInstOutput(struct LSD_SceneNodeInst const * inst,
                             int typeId,
                             const char* name,
                             int bfFuncIdx,
                             int bpFuncIdx){
	
	struct LSD_SceneNodeOutput* addedOut;
	if(lsddb_addNodeInstOutput(inst,typeId,name,&addedOut,NULL,bfFuncIdx,bpFuncIdx)<0){
		fprintf(stderr,"Problem adding inst output\n");
		return -1;
	}
	

	struct LSD_SceneNodeClass* nc = inst->nodeClass;
	
	if(bfFuncIdx>=0 && nc->bfFuncTbl[bfFuncIdx])
		addedOut->bufferFunc = nc->bfFuncTbl[bfFuncIdx];
	if(bpFuncIdx>=0 && nc->bpFuncTbl[bpFuncIdx])
		addedOut->bufferPtr = nc->bpFuncTbl[bpFuncIdx];
	
	return 0;
}
