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


#ifndef DBARR_H
#define DBARR_H

#include "Array.h"

// Global ArrayHead Variables
// ONLY INCLUDE THIS FILE IN DBOps.c!

#define ARRAYH(array) struct LSD_ArrayHead* getArr_##array()

ARRAYH(lsdDBStmtArr);
ARRAYH(lsdNodeInstArr);
ARRAYH(lsdNodeClassArr);
ARRAYH(lsdNodeInputArr);
ARRAYH(lsdNodeOutputArr);
ARRAYH(lsdPluginArr);
ARRAYH(lsdPartitionArr);
ARRAYH(lsdUnivArr);
ARRAYH(lsdChannelArr);


// Note: LSD_Addr objects are composited into triples within LSD_Channel objects.



#endif // DBARR_H
