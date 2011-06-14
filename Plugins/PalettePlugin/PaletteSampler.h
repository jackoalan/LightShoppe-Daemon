#ifndef PALETTE_SAMPLER_H
#define PALETTE_SAMPLER_H

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

#include "../../CorePlugin.h"

struct PaletteSamplerOutputData {
	int outId;
	int outIdx;
	double samplePos;
	struct RGB_TYPE outVal;
};

enum SourceModeEnum {
	MANUAL = 0, // Takes data from user
	PARAM = 1 // Takes data from node input
};

enum RepeatMode { // Determines how gradient extrapolations behave
	CLAMP = 0, // Static colour (default)
	AUTO_LOOP = 1, // Seamless loop around (start's stop added to end)
	AUTO_BLACKOUT = 2 // Black stops added to ends
};

struct PaletteSamplerInstData {
	enum SourceModeEnum selMode;
	int curPaletteId;
	int swatchCount;
	struct RGB_TYPE* swatchArr;
	int numOuts;
	enum SourceModeEnum sampleMode;
	enum RepeatMode paramSampleRepeatMode;
	struct LSD_SceneNodeInput const * selIn;
	struct LSD_SceneNodeInput const ** sampleStopInArr;
	struct PaletteSamplerOutputData* outDataArr;
};

#endif // PALETTE_SAMPLER_H