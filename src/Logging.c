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
#include <time.h>
#include <stdarg.h>

#include "Logging.h"

/* Log File handle */
static FILE* logFile;

/* Print logging to stdout/stderr if set */
static int verbose;


int
initLogging (int verboseMode)
{
    if (verboseMode)
        verbose = 1;
    else
        verbose = 0;
    
    logFile = fopen ("/var/log/lsd.log","a");
    if (!logFile)
        return -1;
    return 0;
}

int
finishLogging ()
{
    if (logFile)
        return fclose (logFile);
    return -1;
}

/* Ran on log file rotation */
int
reloadLogging ()
{
    finishLogging ();
    return initLogging (verbose);
}

/* Various tags to place on log message */
static const char NOTICE_TAG[] = "NOTICE";
static const char WARNING_TAG[] = "WARNING";
static const char ERROR_TAG[] = "ERROR";
static const char DEFAULT_TAG[] = "UNDEFINED";

int 
doLog (enum LogType type, const char* component, const char* msg, ...)
{
    // Get Time
    time_t now = time(NULL);
    struct tm* ts = localtime(&now);
    char tBuf[80];
    strftime(tBuf, sizeof(tBuf), "%Y-%m-%d %H:%M:%S %Z", ts);
    
    // Select correct tag
    const char* tag;
    if (type == NOTICE)
        tag = NOTICE_TAG;
    else if (type == WARNING)
        tag = WARNING_TAG;
    else if (type == ERROR)
        tag = ERROR_TAG;
    else
        tag = DEFAULT_TAG;
    
    // Buffer variable argument list for msg
    char vabuf[256];
    va_list argptr;
    va_start(argptr, msg);
    vsnprintf(vabuf, 256, msg, argptr);
    va_end(argptr);
    
    // Verbose Print
    if (verbose)
        fprintf((type == NOTICE)?stdout:stderr, "[%s] [%s] %s\n", tag, component, vabuf);
    
    // Log File Print
    if (logFile)
        fprintf(logFile, "%s [%s] [%s] %s\n", tBuf, tag, component, vabuf);
    
    return 0;
}


