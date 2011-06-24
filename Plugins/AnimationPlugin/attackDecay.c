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

#include <sys/time.h>
#include <math.h>

#include "../../NodeInstAPI.h"

#include "attackDecay.h"
#include "db.h"

static int floatTypeId;

void ad_publishFloatId(int floatId){
    floatTypeId = floatId;
}


void timevalDelta(struct timeval* target, struct timeval* leading, struct timeval* trailing){
    target->tv_sec = leading->tv_sec - trailing->tv_sec;
    int uDiff = leading->tv_usec - trailing->tv_usec;
    if(uDiff < 0){
        --target->tv_sec;
        uDiff += 1000000;
    }
    target->tv_usec = uDiff;
}

int procAttackDecay(double* source, struct AttackDecayState* state){
    if(!source || !state)
        return -1;
    
    struct timeval curTimeval,diffTimeval;
    gettimeofday(&curTimeval,NULL);
    
    // Get time since last value
    timevalDelta(&diffTimeval,&curTimeval,&(state->lastTv));
    
    // Get as fraction of second
    double diffTime = diffTimeval.tv_sec + (diffTimeval.tv_usec / (double)1000000);
    
    if(*source > state->lastVal){
        // Attack
        double attackAmt = diffTime * fabs(state->attackRate);
        double attacked = state->lastVal + attackAmt;

        state->lastVal = (attacked > *source) ? *source : attacked;
    }
    else if(*source < state->lastVal){
        // Decay
        double decayAmt = diffTime * fabs(state->decayRate);
        double decayed = state->lastVal - decayAmt;
        
        state->lastVal = (decayed < *source) ? *source : decayed;
    }
    else{
        // Passthrough
        state->lastVal = *source;
    }
    
    // Update timeval
    state->lastTv.tv_sec = curTimeval.tv_sec;
    state->lastTv.tv_usec = curTimeval.tv_usec;
    
    return 0;
}


int attackDecayNodeMake(struct LSD_SceneNodeInst const * inst, void* instData){
    int inId;
    plugininst_addInstInput(inst,floatTypeId,"Source Float",&inId);
    
    plugininst_addInstOutput(inst,floatTypeId,"Output",0,0,NULL);
    
    return createAttackDecay(inst->dbId, inId);
}

int attackDecayNodeRestore(struct LSD_SceneNodeInst const * inst, void* instData){
    return restoreAttackDecay(inst);
}

void attackDecayNodeClean(struct LSD_SceneNodeInst const * inst, void* instData){
    // Nothing to do
}

void attackDecayNodeDelete(struct LSD_SceneNodeInst const * inst, void* instData){
    deleteAttackDecayDo(inst->dbId);
}

