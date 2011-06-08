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


#ifndef NODEINSTAPI_H
#define NODEINSTAPI_H

#include "DBOps.h"
#include "Node.h"


int plugininst_addInstInput(struct LSD_SceneNodeInst const * inst,
                            int typeId,
                            const char* name,
                            void(*connectCB)(struct LSD_SceneNodeInput* input, struct LSD_SceneNodeOutput const * connection),
                            void(*disconnectCB)(struct LSD_SceneNodeInput* input));

int plugininst_addInstOutput(struct LSD_SceneNodeInst const * inst,
                             int typeId,
                             const char* name,
                             int bfFuncIdx,
                             int bpFuncIdx);

#endif // NODEINSTAPI_H
