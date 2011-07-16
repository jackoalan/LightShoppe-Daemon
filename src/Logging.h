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

#ifndef LOGGING_H
#define LOGGING_H

enum LogType {
    NOTICE, // Sent via stdout and tagged with [NOTICE]
    WARNING, // Sent via stderr and tagged with [WARNING]
    ERROR // Sent via stderr and tagged with [ERROR]
};

int 
initLogging (int verboseMode);

int
finishLogging ();

int
reloadLogging ();

int 
doLog (enum LogType type, const char* component, const char* msg, ...);

#endif // LOGGING_H

