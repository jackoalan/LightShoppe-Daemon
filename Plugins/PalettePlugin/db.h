#ifndef PALETTE_SAMPLE_DB_H
#define PALETTE_SAMPLE_DB_H

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

#include "../../cJSON.h"
#include "../../Node.h"
#include "PaletteSampler.h"

// RPC methods

int paletteDBGetSampler(cJSON* target, int nodeId);

int paletteDBSetSelMode(int nodeId, enum SourceModeEnum mode);

int paletteDBInsertPalette(const char* name);

int paletteDBUpdatePalette(int paletteId, const char* name);

int paletteDBDeletePalette(int paletteId);

int paletteDBInsertSwatch(int paletteId, cJSON* colour);

int paletteDBUpdateSwatch(int swatchId, cJSON* colour);

int paletteDBDeleteSwatch(int swatchId);

int paletteDBReorderSwatches(int paletteId, cJSON* swatchIdArr);

int paletteDBActivatePalette(int nodeId, int paletteId);

int paletteDBSetNumOut(int nodeId, int numOut);

int paletteDBSampleMode(int nodeId, enum SourceModeEnum mode);

int paletteDBSamplePos(int nodeId, double pos);

int paletteDBRepeatMode(int nodeId, enum RepeatMode mode);

int restorePaletteInst(struct LSD_SceneNodeInst const * inst);

int deletePaletteInst(int nodeId);


// Gradient helpers


// Init method

int paletteDBInit(struct LSD_ScenePlugin const * plugin);

#endif // PALETTE_SAMPLE_DB_H