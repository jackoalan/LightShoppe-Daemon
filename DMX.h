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
  * DMX.h
  *
  *  Created on: 19 Dec 2010 Author: Jacko
  */

#ifndef DMX_H_
#define DMX_H_


/**
  * Initialise DMX universe, no parameters
  */
int
initDMX ();


/**
  * Close DMX
  */
void
closeDMX ();


/**
  * Iterate through channels, buffer them into their univs
  */
int
bufferUnivs ();


/**
  * Iterate through univs, send them to OLA
  */
int
writeUnivs ();


#endif /* DMX_H_ */
