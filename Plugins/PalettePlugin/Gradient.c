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

#include <math.h>

#include "PaletteSampler.h"
#include "db.h"
#include "../../Node.h"



int paletteGradientClamp(struct RGB_TYPE* target, int swatchCount, 
						 struct RGB_TYPE* swatchArr, double samplePos){
	// No swatches will output black
	if(swatchCount <= 0){
		target->r = 0.0;
		target->g = 0.0;
		target->b = 0.0;
		return 0;
	}
	
	// If only one swatch present, output its colour
	if(swatchCount == 1){
		target->r = swatchArr[0].r;
		target->g = swatchArr[0].g;
		target->b = swatchArr[0].b;
		return 0;
	}
	
	// If position is <= 0 output first swatch
	if(samplePos <= 0.0){
		target->r = swatchArr[0].r;
		target->g = swatchArr[0].g;
		target->b = swatchArr[0].b;
		return 0;
	}
	
	// If position is >= 1 output last swatch
	if(samplePos >= 1.0){
		target->r = swatchArr[swatchCount-1].r;
		target->g = swatchArr[swatchCount-1].g;
		target->b = swatchArr[swatchCount-1].b;
		return 0;
	}
	
	// Determine which two swatches the position is in between
	double interval = 1.0 / (double)(swatchCount - 1);
	
	double swaA = 0.0;
	double swaB = 0.0;
	double innerPos = 0.0;
	
	int i;
	for(i=1;i<swatchCount;++i){
		swaA = swaB;
		swaB = interval*i;
		if(swaB >= samplePos){
			// i is the latter swatch, scale coordinates to focus on area between swatches
			innerPos = (samplePos - swaA) / interval;
			
			// Crossfade between swatch i-1 and i using innerPos as blend factor
			target->r = (swatchArr[i-1].r * (1.0 - innerPos)) + (swatchArr[i].r * innerPos);
			target->g = (swatchArr[i-1].g * (1.0 - innerPos)) + (swatchArr[i].g * innerPos);
			target->b = (swatchArr[i-1].b * (1.0 - innerPos)) + (swatchArr[i].b * innerPos);
			
			return 0;
		}
	}
	
	return 0;
}

int paletteGradientBlackout(struct RGB_TYPE* target, int swatchCount, 
							struct RGB_TYPE* swatchArr, double samplePos){
	
	// No swatches; black
	if(swatchCount <= 0){
		target->r = 0.0;
		target->g = 0.0;
		target->b = 0.0;
		return 0;
	}
	
	// Determine size of overhang
	double interval = 1.0 / (double)(swatchCount - 1);
	
	// Account for low limit
	if(samplePos <= -interval){
		target->r = 0.0;
		target->g = 0.0;
		target->b = 0.0;
		return 0;
	}
	
	// Account for high limit
	if(samplePos >= (1.0 + interval)){
		target->r = 0.0;
		target->g = 0.0;
		target->b = 0.0;
		return 0;
	}
	
	// Account for normal range
	if(samplePos >= 0.0 && samplePos <= 1.0){
		paletteGradientClamp(target,swatchCount,swatchArr,samplePos);
		return 0;
	}
	
	// Account for lower black interval
	if(samplePos < 0.0){
		double level = (samplePos / interval) + 1.0;
		// Take first swatch multiplied by level
		target->r = swatchArr[0].r * level;
		target->g = swatchArr[0].g * level;
		target->b = swatchArr[0].b * level;
		return 0;
	}
	
	// Account for upper black interval
	if(samplePos > 1.0){
		double level = ((samplePos - 1.0) / interval);
		// Take last swatch multiplied by level
		target->r = swatchArr[swatchCount-1].r * level;
		target->g = swatchArr[swatchCount-1].g * level;
		target->b = swatchArr[swatchCount-1].b * level;
		return 0;
	}
	
	return 0;
	
}

int paletteGradientLoop(struct RGB_TYPE* target, int swatchCount, 
						struct RGB_TYPE* swatchArr, double samplePos){
	
	// No swatches; black
	if(swatchCount <= 0){
		target->r = 0.0;
		target->g = 0.0;
		target->b = 0.0;
		return 0;
	}
	
	// Determine size of overhang
	double interval = 1.0 / (double)(swatchCount - 1);
	
	// Determine size of loop cycle
	double loopCycle = 1.0 + interval;
	
	// Determine position within loop cycle
	double cyclePos = (samplePos / loopCycle);
	cyclePos -= floor(cyclePos);
	cyclePos *= loopCycle;
	
	if(cyclePos > 0.0){
		double xFac = (cyclePos - 1.0) / interval;
		// X-fade between last and first swatch using xFac
		target->r = (swatchArr[swatchCount-1].r * (1.0 - xFac)) + (swatchArr[0].r * xFac);
		target->g = (swatchArr[swatchCount-1].g * (1.0 - xFac)) + (swatchArr[0].g * xFac);
		target->b = (swatchArr[swatchCount-1].b * (1.0 - xFac)) + (swatchArr[0].b * xFac);
		return 0;
	}
	else{
		paletteGradientClamp(target,swatchCount,swatchArr,cyclePos);
		return 0;
	}
	
	return 0;
}

int paletteGradientOut(struct LSD_SceneNodeInst const * inst, int outId){
	int i;
	
	struct PaletteSamplerInstData* instData = (struct PaletteSamplerInstData*)inst->data;
	
	// Get palette from input if parametric palette selection is active
	if(instData->selMode == PARAM && instData->selIn){ 
		// Check to see if curPaletteId is current
		int* selInInt = (int*)node_bufferOutput(instData->selIn->connection);
		if(selInInt){
			if(*selInInt != instData->curPaletteId){
				// Instruct DB to load another palette
				paletteDBLoadSwatchData(inst, *selInInt);
			}
		}
	}
	
	// Get sampler positions from inputs if parametric sampling is active
	if(instData->sampleMode == PARAM){
		for(i=0;i<instData->numOuts;++i){
			double* samplePos = (double*)node_bufferOutput(instData->sampleStopInArr[i]->connection);
			if(samplePos)
				instData->outDataArr[i].samplePos = *samplePos;
			else
				instData->outDataArr[i].samplePos = 0.0;
		}
	}
	
	// Find result colour by clamping each out
	for(i=0;i<instData->numOuts;++i){
		if(instData->sampleMode == MANUAL || instData->paramSampleRepeatMode == CLAMP){
			paletteGradientClamp(&(instData->outDataArr[i].outVal),instData->swatchCount,
							 instData->swatchArr,instData->outDataArr[i].samplePos);
		}
		else if(instData->paramSampleRepeatMode == AUTO_LOOP){
			paletteGradientLoop(&(instData->outDataArr[i].outVal),instData->swatchCount,
								instData->swatchArr,instData->outDataArr[i].samplePos);
		}
		else if(instData->paramSampleRepeatMode == AUTO_BLACKOUT){
			paletteGradientBlackout(&(instData->outDataArr[i].outVal),instData->swatchCount,
									instData->swatchArr,instData->outDataArr[i].samplePos);
		}
	}
	
	
	
	return 0;
}