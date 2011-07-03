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

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <ltdl.h>
#include <evhttp.h>
#include "cJSON.h"

#include "PluginLoader.h"
#include "DBOps.h"

#if defined(LT_DIRSEP_CHAR)
#define LSD_DIRSEP_CHAR LT_DIRSEP_CHAR
#else
#define LSD_DIRSEP_CHAR '/'
#endif

#if defined(PLUGIN_DIR)
static const char PLUGIN_PATH[] = PLUGIN_DIR;
#endif

#if defined(WEB_PLUGIN_DIR)
static const char WEB_PLUGIN_PATH[] = WEB_PLUGIN_DIR;
#endif


int
checkAddPlugin (const char* pluginFile, const char* pluginName, 
                lt_dlhandle pluginHandle)
{

    /* First get the file's digest */
    char shaCommand[264];
    snprintf (shaCommand, 264, "sha1sum %s", pluginFile);
    FILE* shaStream = popen (shaCommand, "r");

    char digest[42];
    fread (digest, 40, 1, shaStream);
    digest[41] = '\0';

    pclose (shaStream);

    printf ("%s digest: %.40s\n", pluginName, digest);

    /* Now extract the head and load it! */
    ghType getHead = lt_dlsym (pluginHandle, "getPluginHead");
    if (getHead)
    {
        if ( lsddb_pluginHeadLoader (getHead, 0, pluginName, digest,
                                     pluginHandle) < 0 )
        {
            fprintf (stderr, "Unable to validate plugin HEAD\n");
            lt_dlclose (pluginHandle);
            return -1;
        }
    }
    else
    {
        fprintf (stderr, "Error while loading HEAD of %s:\n", pluginName);
        fprintf (stderr, "%s\n", lt_dlerror());
        lt_dlclose (pluginHandle);
        return -1;
    }
    
    return 0;

}


/* Pulls server library out of plugin folder */
int
scrapePlugin (const char *filename, void * data)
{
    lt_dlhandle pluginHandle = lt_dlopenext (filename);
    if (!pluginHandle)
    {
        fprintf (stderr, "Unable to link %s\nDetails: %s\n", filename, lt_dlerror());
        return 0;
    }
    
    /* Get Info for loaded Plugin */
    lt_dlinfo const * pluginInfo = lt_dlgetinfo (pluginHandle);
    if (!pluginInfo)
    {
        fprintf (stderr, "Unable to get plugin SO info for %s\nDetails: %s\n", filename, lt_dlerror());
        return 0;
    }
    
    /* Continue Loading Plugin */
    printf ("Found %s\n", pluginInfo->name);
    checkAddPlugin (pluginInfo->filename, pluginInfo->name, pluginHandle);
    
    return 0;
}


/* Opens the plugins directory provided and calls
 * scrapePlugin for the name */
int
loadPluginsDirectory ()
{
    /* Plugin directory is set by autotools using user-defined prefix */
#ifndef PLUGIN_DIR
    fprintf (stderr, "Plugin Directory not provided at compile time\n");
    return -1;
#endif
    

    return lt_dlforeachfile (PLUGIN_PATH, scrapePlugin, NULL);
    
}


/*
  * This function provides a way of opening and parsing out
  *an include file named "Client.json" in each plugin
  *directory for the benefit of the webbrowser
  */
int
getPluginWebIncludes (struct evbuffer* target,
                      const char* pluginDirName)
{
    if (!target || !pluginDirName)
    {
        fprintf (stderr, "Invalid use of getPluginWebIncludes()\n");
        return -1;
    }
    
#ifndef WEB_PLUGIN_DIR
    fprintf (stderr, "Web Plugin Directory not provided at compile time\n");
    return -1;
#endif

    char pluginIncludePath[256];
    snprintf (pluginIncludePath,
              256,
              "%s/%s/Client.json",
              WEB_PLUGIN_PATH,
              pluginDirName);

    FILE* includeFile = fopen (pluginIncludePath, "rb");
    if (includeFile)
    {

        /* Get file size first */
        fseek (includeFile, 0L, SEEK_END);
        size_t sz = ftell (includeFile);
        fseek (includeFile, 0L, SEEK_SET);

        char* includeFileContent = malloc (sizeof( char ) * sz);
        if (!includeFileContent)
        {
            fprintf (
                stderr,
                "Unable to allocate memory to accomodate include file for parsing\n");
            fclose (includeFile);
            return -1;
        }
        fread (includeFileContent, sizeof( char ) * sz, 1, includeFile);

        cJSON* includeFileParsed = cJSON_Parse (includeFileContent);
        if (!includeFileParsed)
        {
            fprintf (stderr, "Unable to parse JSON from include file\n");
            fclose (includeFile);
            free (includeFileContent);
            return -1;
        }

        /* Done with string after parsing */
        free (includeFileContent);

        /* Make comment in HTML */
        evbuffer_add_printf (target, "\t\t<!-- Includes for %.50s -->\n",
                             pluginDirName);

        /* PARSE! */
        int i;

        cJSON* jsArr = cJSON_GetObjectItem (includeFileParsed, "js");
        if (jsArr && jsArr->type == cJSON_Array)
            for (i = 0; i < cJSON_GetArraySize (jsArr); ++i)
            {
                cJSON* jsItem = cJSON_GetArrayItem (jsArr, i);
                if (jsItem && jsItem->type == cJSON_String)
                    evbuffer_add_printf (
                        target,
                        "\t\t<script type=\"text/javascript\" src=\"../plugins/%.50s/%.50s\"></script>\n",
                        pluginDirName,
                        jsItem->valuestring);
            }

        cJSON* cssArr = cJSON_GetObjectItem (includeFileParsed, "css");
        if (cssArr && cssArr->type == cJSON_Array)
            for (i = 0; i < cJSON_GetArraySize (cssArr); ++i)
            {
                cJSON* cssItem = cJSON_GetArrayItem (cssArr, i);
                if (cssItem && cssItem->type == cJSON_String)
                    evbuffer_add_printf (
                        target,
                        "\t\t<link rel=\"stylesheet\" type=\"text/css\" href=\"../plugins/%.50s/%.50s\" />\n",
                        pluginDirName,
                        cssItem->valuestring);
            }

        /* Line break */
        evbuffer_add_printf (target, "\n");

        /* Free JSON */
        cJSON_Delete (includeFileParsed);

    }
    else
    {
        fprintf (stderr, "Unable to open %s\n", pluginIncludePath);
        return -1;
    }

    return fclose (includeFile);

}


