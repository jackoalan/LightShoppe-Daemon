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

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include "PluginLoader.h"

/*
Plugin Directory Structure:
-Plugins (Directory)
    -<PluginName> (Directory)
        -Server.so 
        -Client.js
            -<PluginName_clienthead> object containing an array of members
             for each class provided by plugin
        -Client images and other resources

This file contains:

-function to iterate through directories within a given directory and
 pass the directory's name to a loader function

-loader function to receive a directory name, open plugin, verify validity
    -if valid, it will be added to DB with the directory name and SHA of binary
     with load net to no according to process below
        -if name already exists in DB, check to see if SHA matches
            -if so, check load flag
                -if so, set loaded and load
            -if not, unset load flag and update SHA
            
-function to generate PluginBindings.js, which contains an indexed array
 where the index corresponds to the plugin ID of the plugin.
 Each entry contains a reference to an array with function references to the configuration
 function of each class provided by the plugin, indexed privately by the plugin
 and sent to the client along with each class. This allows the client to 
 resolve a connection to the appropirate JavaScript functions related to the
 class.
*/