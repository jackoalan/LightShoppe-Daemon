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
#include <dlfcn.h>
#include <magic.h>
#include <gcrypt.h>

#include "PluginLoader.h"
#include "DBOps.h"

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

typedef const struct LSD_ScenePluginHEAD* (*ghType)(void);

int checkAddPlugin(const char* pluginDir, const char* pluginFile){

	// First get the file's digest
    char shaCommand[264];
    snprintf(shaCommand,264,"sha1sum %s",pluginFile);
    FILE* shaStream = popen(shaCommand,"r");
    
    char digest[41];
    fread(digest,40,1,shaStream);
    digest[41] = '\0';
    
    pclose(shaStream);
    
    printf("%s digest: %.40s\n",pluginDir,digest);
    
    // Now extract the head and load it!
    const struct LSD_ScenePluginHEAD* ph;
	
	void* librarySO = dlopen(pluginFile, RTLD_LAZY);
	if(!librarySO){
		printf("Error Opening SO: %s\n",dlerror());
		return -1;
	}
	
	ghType getHead = dlsym(librarySO,"getPluginHead");
	if(getHead){
		ph = getHead();
		if( lsddb_pluginHeadLoader(ph,1,pluginDir,digest,librarySO) < 0 ){
			fprintf(stderr,"Error while loading PluginHead\n");
			dlclose(librarySO);
		}
	}
	else{
		printf("Error loading %s:\n",pluginFile);
		printf("%s\n",dlerror());
	}
	
	return 0;
	
}

// Pulls server library out of plugin folder and uses libmagic to verify
int scrapePlugin(const char* pluginsPath, const char* pluginDirName){
	DIR* pluginDir;
	struct dirent* pluginDirItem;
	
	// String to keep a single plugin's path for scraping
	char pluginPath[256];
	char pluginFile[256];
	
	// Magic cookie to do checks on loaded files
	magic_t magicCookie = magic_open(MAGIC_MIME);
	magic_load(magicCookie,NULL);
	
	snprintf(pluginPath,256,"%s/%s",pluginsPath,pluginDirName);
	
	pluginDir = opendir(pluginPath);
	if(!pluginDir){
		fprintf(stderr,"Unable to open plugin directory\n");
		return -1;
	}
	
	int foundLib;
	const char* magicFileDesc;
	pluginDirItem = readdir(pluginDir);
	while(pluginDirItem){
		foundLib = 0;
		// We're only interested in regular files
		if(pluginDirItem->d_type == DT_REG){
			//printf("%s\n",pluginDirItem->d_name);
			
			// Check to see if file could be server library
			if(
#ifdef __APPLE__
			   strncmp(pluginDirItem->d_name,"Server.dylib",12)==0
#else
			   strncmp(pluginDirItem->d_name,"Server.so",9)==0 
#endif
			   
			   ){
				foundLib = 1;
				
				
#ifdef __APPLE__
				snprintf(pluginFile,256,"%s/%s",pluginPath,"Server.dylib");
#else
				snprintf(pluginFile,256,"%s/%s",pluginPath,"Server.so");
#endif

				magicFileDesc = magic_file(magicCookie,pluginFile);
				
				if( strncmp(magicFileDesc, "application/x-sharedlib", 23) == 0 ){
					printf("%s: Loading\n",pluginDirName);
					checkAddPlugin(pluginDirName,pluginFile);
				}
				
				
			}
			
		}
		
		if(!foundLib)
			pluginDirItem = readdir(pluginDir); // Iterate next
		else
			pluginDirItem = NULL; // Finish this plugin
	}
	
	magic_close(magicCookie);
	return closedir(pluginDir);
}

// Opens the plugins directory provided and calls scrapePlugin for the name
int iteratePluginsDirectory(const char* pluginsDirPath){
	DIR* pluginsDir;
	struct dirent* pluginsDirItem;
	
	pluginsDir = opendir(pluginsDirPath);
	if(!pluginsDir){
		fprintf(stderr,"Unable to open plugins directory\n");
		return -1;
	}
	
	pluginsDirItem = readdir(pluginsDir);
	while(pluginsDirItem){
		// We're only interested in directories
		if(pluginsDirItem->d_type == DT_DIR){
			if(pluginsDirItem->d_name[0] != '.'){ // Filter out dot files
				scrapePlugin(pluginsDirPath,pluginsDirItem->d_name);
			}
		}
		
		pluginsDirItem = readdir(pluginsDir);
	}
	
	return closedir(pluginsDir);
}