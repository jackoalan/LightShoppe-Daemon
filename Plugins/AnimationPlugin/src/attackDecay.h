/*
 **    This file is part of LightShoppe. Copyright 2011 Jack Andersen
 **
 **    LightShoppe is free software: you can redistribute it
 **    and/or modify it under the terms of the GNU General
 **    Public License as published by the Free Software
 **    Foundation, either version 3 of the License, or (at your
 **    option) any later version.
 **
 **    LightShoppe is distributed in the hope that it will
 **    be useful, but WITHOUT ANY WARRANTY; without even the
 **    implied warranty of MERCHANTABILITY or FITNESS FOR A
 **    PARTICULAR PURPOSE.  See the GNU General Public License
 **    for more details.
 **
 **    You should have received a copy of the GNU General
 **    Public License along with LightShoppe.  If not, see
 **    <http://www.gnu.org/licenses/>.
 **
 **    @author Jack Andersen <jackoalan@gmail.com>
 */

#ifndef ATTACK_DECAY_H
#define ATTACK_DECAY_H

#include <sys/time.h>

struct AttackDecayState
{
    double attackRate;
    double decayRate;
    double lastVal;
    struct timeval lastTv;
    struct LSD_SceneNodeInput const* srcIn;
};

void
ad_publishFloatId (int floatId);


int
procAttackDecay (double* source, struct AttackDecayState* state);


int
attackDecayNodeMake (struct LSD_SceneNodeInst const* inst, void* instData);


int
attackDecayNodeRestore (struct LSD_SceneNodeInst const* inst, void* instData);


void
attackDecayNodeClean (struct LSD_SceneNodeInst const* inst, void* instData);


void
attackDecayNodeDelete (struct LSD_SceneNodeInst const* inst, void* instData);


#endif /* ATTACK_DECAY_H */