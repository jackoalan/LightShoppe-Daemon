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

int
checkAddPlugin (const char* pluginDir, const char* pluginFile)
{

    /* First get the file's digest */
    char shaCommand[264];
    snprintf (shaCommand, 264, "sha1sum %s", pluginFile);
    FILE* shaStream = popen (shaCommand, "r");

    char digest[42];
    fread (digest, 40, 1, shaStream);
    digest[41] = '\0';

    pclose (shaStream);

    printf ("%s digest: %.40s\n", pluginDir, digest);

    /* Now extract the head and load it! */

    lt_dlhandle pluginSo;
    pluginSo = lt_dlopen (pluginFile);
    
    if (!pluginSo)
    {
        fprintf (stderr, "Error while loading %s: %s\n", pluginDir, lt_dlerror());
        return -1;
    }
    
    ghType getHead = lt_dlsym (pluginSo, "getPluginHead");
    if (getHead)
    {
        if ( lsddb_pluginHeadLoader (getHead, 0, pluginDir, digest,
                                     pluginSo) < 0 )
        {
            fprintf (stderr, "Unable to validate plugin HEAD\n");
            lt_dlclose (pluginSo);
            return -1;
        }
    }
    else
    {
        printf ("Error while loading HEAD of %s:\n", pluginFile);
        printf ("%s\n", lt_dlerror());
        lt_dlclose (pluginSo);
        return -1;
    }
    
    return 0;

}


/* Pulls server library out of plugin folder and uses
 * libmagic to verify */
int
scrapePlugin (const char* pluginsPath, const char* pluginDirName)
{
    
    DIR* pluginDir;
    struct dirent* pluginDirItem;

    /* String to keep a single plugin's path for scraping */
    char pluginFile[256];

    snprintf (pluginFile, 256, "%s%c%s%c%s.la", pluginsPath, LSD_DIRSEP_CHAR, 
              pluginDirName, LSD_DIRSEP_CHAR, pluginDirName);
    
    printf ("%s: Loading\n", pluginDirName);
    return checkAddPlugin (pluginDirName, pluginFile);
    
}


/* Opens the plugins directory provided and calls
 * scrapePlugin for the name */
int
iteratePluginsDirectory (const char* pluginsDirPath)
{
    DIR* pluginsDir;
    struct dirent* pluginsDirItem;

    pluginsDir = opendir (pluginsDirPath);
    if (!pluginsDir)
    {
        fprintf (stderr, "Unable to open plugins directory\n");
        return -1;
    }

    pluginsDirItem = readdir (pluginsDir);
    while (pluginsDirItem)
    {
        /* We're only interested in directories */
        if (pluginsDirItem->d_type == DT_DIR)
            if (pluginsDirItem->d_name[0] != '.') /* Filter
                                                   * out dot
                                                   * files */
                scrapePlugin (pluginsDirPath, pluginsDirItem->d_name);

        pluginsDirItem = readdir (pluginsDir);
    }

    return closedir (pluginsDir);
}


/*
  * This function provides a way of opening and parsing out
  *an include file named "Client.json" in each plugin
  *directory for the benefit of the webbrowser
  */
int
getPluginWebIncludes (struct evbuffer* target,
                      const char* pluginsDirPath,
                      const char* pluginDirName)
{
    if (!target || !pluginsDirPath || !pluginDirName)
    {
        fprintf (stderr, "Invalid use of getPluginWebIncludes()\n");
        return -1;
    }

    char pluginIncludePath[256];
    snprintf (pluginIncludePath,
              256,
              "%s/%s/Client.json",
              pluginsDirPath,
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


