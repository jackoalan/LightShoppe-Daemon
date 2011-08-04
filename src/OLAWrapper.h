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


/*
  * OLAWrapper.h
  *
  *  Created on: 2 Jan 2011 Author: Jacko
  */

#ifndef OLAWRAPPER_H_
#define OLAWRAPPER_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
  * Creates new OLA Client to use updateDMX
  */
int
initOlaClient ();


/**
  * Sends DMX buffer to universe <univId>
  */
int
olaUpdateDMX (uint8_t* univ, size_t len, int univId);


/**
  * Stops client
  */
void
stopOlaClient ();


#ifdef __cplusplus
}
#endif

#endif /* OLAWRAPPER_H_ */
