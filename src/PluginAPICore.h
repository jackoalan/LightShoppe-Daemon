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

#ifndef PLUGINAPICORE_H
#define PLUGINAPICORE_H

#include "PluginAPI.h"


/**
  * Api state system allows functions to ensure that they
  *are allowed to operate at the current phase of execution
  *LSD is operating at.
  */
enum API_STATE
{
    STATE_PINIT,
    STATE_PRUN,
    STATE_PCLEAN
};


/**
  * Used by CORE ONLY. I repeat: ONLY INCLUDE THIS HEADER IN
  *LSD CORE
  */
int
lsdapi_setState (enum API_STATE state);


int
lsdapi_initPlugin ();


#endif /* PLUGINAPICORE_H */
