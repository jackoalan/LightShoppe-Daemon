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
#include "Logging.h"

/* Gettext stuff */
#ifndef HW_RVL
#include <libintl.h>
#define _(String) gettext (String)
#else
#define _(String) String
#endif

/* Component Log Name */
static const char LOG_COMP[] = "PluginLoader.c";

#if defined(PLUGIN_DIR)
static const char PLUGIN_PATH[] = PLUGIN_DIR;
#endif

#if defined(WEB_PLUGIN_DIR)
static const char WEB_PLUGIN_PATH[] = WEB_PLUGIN_DIR;
#endif


#ifndef HW_RVL
/* compute a hash for the plugin and pass to database system for insertion */
int
checkAddPlugin (const char* pluginFile, const char* pluginName, 
                lt_dlhandle pluginHandle)
{

    /* First get the file's digest */
    char shaCommand[264];
    snprintf (shaCommand, 264, "sha1sum %s", pluginFile);
    FILE* shaStream = popen (shaCommand, "r");

    char digest[42];
    if(!fread (digest, 40, 1, shaStream))
        return -1;
    digest[41] = '\0';

    pclose (shaStream);

    doLog (NOTICE, LOG_COMP, _("%s digest: %.40s."), pluginName, digest);

    /* Now extract the head and load it! */
    ghType getHead = lt_dlsym (pluginHandle, "getPluginHead");
    if (getHead)
    {
        if ( lsddb_pluginHeadLoader (getHead, 0, pluginName, digest,
                                     pluginHandle) < 0 )
        {
            doLog (ERROR, LOG_COMP, _("Unable to validate plugin HEAD of %s."), pluginName);
            lt_dlclose (pluginHandle);
            return -1;
        }
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Error while loading HEAD of %s.\nDetails: %s"), 
               pluginName, lt_dlerror());
        lt_dlclose (pluginHandle);
        return -1;
    }
    
    return 0;

}
#endif

/* Static plugin counterpart of checkAddPlugin 
 * searches for pluginHead accessor within linked plugin */
int
checkAddPlugin_static (const char* pluginName, void* ghPtr)
{
    ghType getHead = (ghType)ghPtr;
    if (getHead)
    {
        if ( lsddb_pluginHeadLoader (getHead, 0, pluginName, "STATIC",
                                     NULL) < 0 )
        {
            doLog (ERROR, LOG_COMP, _("Unable to validate plugin HEAD of %s statically."), pluginName);
            return -1;
        }
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Error while loading HEAD of %s statically."), 
               pluginName);
        return -1;
    }
}


#ifndef HW_RVL
/* Pulls server library out of plugin folder */
int
scrapePlugin (const char *filename, void * data)
{
    lt_dlhandle pluginHandle = lt_dlopenext (filename);
    if (!pluginHandle)
    {
        doLog (ERROR, LOG_COMP, _("Unable to link %s\nDetails: %s."), filename, lt_dlerror());
        return 0;
    }
    
    /* Get Info for loaded Plugin */
    lt_dlinfo const * pluginInfo = lt_dlgetinfo (pluginHandle);
    if (!pluginInfo)
    {
        doLog (ERROR, LOG_COMP, _("Unable to get plugin SO info for %s\nDetails: %s."), 
               filename, lt_dlerror());
        return 0;
    }
    
    /* Continue Loading Plugin */
    doLog (NOTICE, LOG_COMP, _("Found %s."), pluginInfo->name);
    checkAddPlugin (pluginInfo->filename, pluginInfo->name, pluginHandle);
    
    return 0;
}
#endif

#ifndef HW_RVL
/* Opens the plugins directory provided and calls
 * scrapePlugin for the name */
int
loadPluginsDirectory ()
{
    /* Plugin directory is set by autotools using user-defined prefix */
#ifndef PLUGIN_DIR
    doLog (ERROR, LOG_COMP, _("PLUGIN_DIR not provided at compile time."));
    return -1;
#endif
    
    return lt_dlforeachfile (PLUGIN_PATH, scrapePlugin, NULL);
}
#endif

/* Uses dlpreloading to discover plugins that
 * were statically linked at compile time */
int
loadPlugins_static ()
{
    if(LTDL_SET_PRELOADED_SYMBOLS() != 0)
        return -1;
    
    lt_dlsymlist sym;
    int i = 1;
    char* dot;
    int nameLen;
    char pluginName[64];
    char nameComp[128];
    
    while (1) {
        sym = lt_preloaded_symbols[i];
        if (sym.name && !sym.address)
        {
            dot = strrchr (sym.name, '.');
            nameLen = dot - sym.name;
            memset (pluginName, '\0', 64);
            memcpy (pluginName, sym.name, nameLen);
            memset (nameComp, '\0', 128);
            memcpy (nameComp, sym.name, nameLen);
            snprintf (nameComp+nameLen, 128-nameLen, "_LTX_getPluginHead");
        }
        else if (sym.name && sym.address)
        {
            if (strncmp (nameComp, sym.name, 128) == 0)
                checkAddPlugin_static (pluginName, sym.address);
        }
        else
            break;
        ++i;
    }
    
    return 0;
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
        doLog (ERROR, LOG_COMP, _("Invalid use of getPluginWebIncludes()."));
        return -1;
    }
    
#ifndef WEB_PLUGIN_DIR
    doLog (ERROR, LOG_COMP, _("WEB_PLUGIN_DIR not provided at compile time."));
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
            doLog (ERROR, LOG_COMP, _("Unable to allocate memory to accommodate include file for parsing."));
            fclose (includeFile);
            return -1;
        }
        fread (includeFileContent, sizeof( char ) * sz, 1, includeFile);

        cJSON* includeFileParsed = cJSON_Parse (includeFileContent);
        if (!includeFileParsed)
        {
            doLog (ERROR, LOG_COMP, _("Unable to parse JSON from include file."));
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
        doLog (WARNING, LOG_COMP, _("Unable to open %s. No includes added."), pluginIncludePath);
        return -1;
    }

    return fclose (includeFile);

}


