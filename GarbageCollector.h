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



#ifndef GARBAGE_COLLECTOR_H
#define GARBAGE_COLLECTOR_H

int lsdgc_getNewArrayId(int* arrIdBind);

int lsdgc_setArrMark(int arrayId, int arrIdx);

int lsdgc_unsetArrMark(int arrayId, int arrIdx);

int lsdgc_discoverArrMark(int arrayId, int* arrIdxBind);

int lsdgc_removeArrIdMarks(int arrayId);

int lsdgc_prepGCOps();

int lsdgc_finalGCOps();

#endif // GARBAGE_COLLECTOR_H
