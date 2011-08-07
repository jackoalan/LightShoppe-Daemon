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


#ifdef HW_RVL

#include <gccore.h>
#include <network.h>
#include <wiiuse/wpad.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "SceneCore.h"

void
wii_init ()
{
    
    void *xfb = NULL;
    GXRModeObj *rmode = NULL;
    
    // Initialise the video system
	VIDEO_Init();
	
	// This function initialises the attached controllers
	WPAD_Init();
	
	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);
    
	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);
	
	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);
	
	// Make the display visible
	VIDEO_SetBlack(FALSE);
    
	// Flush the video register changes to the hardware
	VIDEO_Flush();
    
	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
    
    
	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[2;0H");
    
    if (net_init () < 0)
    {
        printf ("Something went wibbly with net_init().\n");
    }
	
}

void
wii_deinit ()
{
    net_deinit ();
}

int
main (int argc, const char** argv)
{
    
    /* Parse Command Line */
    int i;
    int verbose = 0;
    int rpcPort = 9196;
    const char* dbpath = NULL;
    const char* pathPrefix = "/lightshoppe";
    if (argc > 0)
        for (i = 0; i < argc; ++i)
        {
            if (strncmp (argv[i], "-h", 2) == 0)
            {
                printf ("Usage: lsd [-hv] [-p port] [-P \"Path Prefix\"] [-d dbfile]\n");
                return 0;
            }
            else if (strncmp (argv[i], "-v", 2) == 0)
                verbose = 1;
            else if (strncmp (argv[i], "-d", 2) == 0)
            {
                if (strlen(argv[i]) > 2)
                    dbpath = argv[i]+2;
                else if (i+1 < argc)
                    dbpath = argv[i+1];
                else
                {
                    printf ("Missing path value for -d.\n");
                    return -1;
                }
            }
            else if (strncmp (argv[i], "-p", 2) == 0)
            {
                const char* portStr;
                if (strlen(argv[i]) > 2)
                    portStr = argv[i]+2;
                else if (i+1 < argc)
                    portStr = argv[i+1];
                else
                {
                    printf ("Missing port value for -p.\n");
                    return -1;
                }
                
                int portNum = atoi (portStr);
                if (!portNum)
                {
                    printf ("Unable to parse port value.\n");
                    return -1;
                }
                
                rpcPort = portNum;
            }
            else if (strncmp (argv[i], "-P", 2) == 0)
            {
                if (strlen(argv[i]) > 2)
                    pathPrefix = argv[i]+2;
                else if (i+1 < argc)
                    pathPrefix = argv[i+1];
                else
                {
                    printf ("Missing string for path prefix.\n");
                    return -1;
                }
                
                if(pathPrefix[0] != '/')
                {
                    printf ("Path must begin with a leading slash.\n");
                    return -1;
                }
            }
            
        }
    
    /* Init Wii */
    wii_init ();
    
    /* Begin Logging */
    initLogging (verbose);
    
    /* Start LSD! */
    int exitCode = lsdSceneEntry (dbpath, rpcPort, pathPrefix);
    
    /* End Logging */
    finishLogging ();
    
    /* Deinit Wii */
    wii_deinit ();
    
    
    return exitCode;
}

#endif