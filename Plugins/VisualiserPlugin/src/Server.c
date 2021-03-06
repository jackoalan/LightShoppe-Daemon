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
  * From the scope of the plugin's directory, LightShoppe's
  *development headers are accessed this way
  */
#include "../../src/PluginAPI.h"
#include "../../src/cJSON.h"
#include "../../src/NodeInstAPI.h"
#include "../../src/CorePlugin.h"

#include <stdio.h>
#include <string.h>
#ifndef HW_RVL
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#endif
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "PCMPipe.h"

#include "../config.h"

/** SYMBOL ALIAS FOR LIBTOOL **/
#define getPluginHead  VisualiserPlugin_LTX_getPluginHead

/* This should match NUM_BANDS in PCMPipe */
#define NUM_BANDS 3

/* IPC vars */
static int msemid;
#ifndef HW_RVL
static struct sembuf msem[2];
#endif
static int mshmid;
static void* mshmAttach;

#ifdef HAVE_ALSA_ASOUNDLIB_H
static int asemid;
static struct sembuf asem[2];
static int ashmid;
static void* ashmAttach;
#endif

static pid_t mpipeProc;
#ifdef HAVE_ALSA_ASOUNDLIB_H
static pid_t apipeProc;
#endif


/* Process-local storage buffer for band values */
static double mlocalBuffer[NUM_BANDS];
#ifdef HAVE_ALSA_ASOUNDLIB_H
static double alocalBuffer[NUM_BANDS];
#endif


/* Float type */
int floatTypeId;

void
mlocalBufCopy (struct LSD_SceneNodeOutput const* output)
{
#ifndef HW_RVL
    /* Get semaphore */
    msem[0].sem_op = 0;
    msem[1].sem_op = 1;
    if (semop (msemid, msem, 2) < 0)
        return;

    /* Copy buffer */
    double* shm = (double*)mshmAttach;
    int i;
    for (i = 0; i < NUM_BANDS; ++i)
        mlocalBuffer[i] = shm[i];

    /* Release semaphore */
    msem[0].sem_op = -1;
    semop (msemid, msem, 1);
#endif
}

#ifdef HAVE_ALSA_ASOUNDLIB_H
void
alocalBufCopy (struct LSD_SceneNodeOutput const* output)
{
    /* Get semaphore */
    asem[0].sem_op = 0;
    asem[1].sem_op = 1;
    if (semop (asemid, asem, 2) < 0)
        return;
    
    /* Copy buffer */
    double* shm = (double*)ashmAttach;
    int i;
    for (i = 0; i < NUM_BANDS; ++i)
        alocalBuffer[i] = shm[i];
    
    /* Release semaphore */
    asem[0].sem_op = -1;
    semop (asemid, asem, 1);
}
#endif


void*
mgetHigh (struct LSD_SceneNodeOutput const* output)
{
    return &( mlocalBuffer[2] );
}


void*
mgetMid (struct LSD_SceneNodeOutput const* output)
{
    return &( mlocalBuffer[1] );
}


void*
mgetLow (struct LSD_SceneNodeOutput const* output)
{
    return &( mlocalBuffer[0] );
}

#ifdef HAVE_ALSA_ASOUNDLIB_H
void*
agetHigh (struct LSD_SceneNodeOutput const* output)
{
    return &( alocalBuffer[2] );
}


void*
agetMid (struct LSD_SceneNodeOutput const* output)
{
    return &( alocalBuffer[1] );
}


void*
agetLow (struct LSD_SceneNodeOutput const* output)
{
    return &( alocalBuffer[0] );
}
#endif

/* Buffer func tables */
static const bfFunc mbfFuncs[] =
{mlocalBufCopy};

static const bpFunc mbpFuncs[] =
{mgetHigh, mgetMid, mgetLow};

#ifdef HAVE_ALSA_ASOUNDLIB_H
static const bfFunc abfFuncs[] =
{alocalBufCopy};

static const bpFunc abpFuncs[] =
{agetHigh, agetMid, agetLow};
#endif

int
makeNode (struct LSD_SceneNodeInst const* inst, void* instData)
{
    plugininst_addInstOutput (inst, floatTypeId, "High Band", 0, 0, NULL);
    plugininst_addInstOutput (inst, floatTypeId, "Middle Band", 0, 1, NULL);
    plugininst_addInstOutput (inst, floatTypeId, "Low Band", 0, 2, NULL);
    return 0;
}


int
restoreNode (struct LSD_SceneNodeInst const* inst, void* instData)
{
    return 0;
}


void
cleanNode (struct LSD_SceneNodeInst const* inst, void* instData)
{
}


void
deleteNode (struct LSD_SceneNodeInst const* inst, void* instData)
{
}


int
msemInit ()
{
#ifndef HW_RVL
    int ofile = open ("/tmp/lsdvis.ipc", O_CREAT | O_WRONLY, 0666);
    close (ofile);

    /* Set up IPC semaphore */
    key_t semkey, shmkey;
    semkey = ftok ("/tmp/lsdvis.ipc", 321);
    shmkey = ftok ("/tmp/lsdvis.ipc", 123);

    if (semkey == (key_t)-1 || shmkey == (key_t)-1)
    {
        fprintf (stderr, "Unable to get IPC key: %s\n", strerror (errno));
        return -1;
    }

    /* Get semaphore */
    msemid = semget (semkey, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if (msemid < 0)
    {
        fprintf (stderr, "Unable to create semaphore\n");
        return -1;
    }

    if (semctl (msemid, 0, SETVAL, 0) < 0)
    {
        fprintf (stderr, "Unable to init semaphore\n");
        return -1;
    }

    /* Initialise sem operation buf */
    msem[0].sem_num = 0;
    msem[1].sem_num = 0;
    msem[0].sem_flg = IPC_NOWAIT;
    msem[1].sem_flg = IPC_NOWAIT;

    /* Get Shared Memory */
    mshmid = shmget (shmkey,
                    sizeof( double ) * NUM_BANDS,
                    0666 | IPC_CREAT | IPC_EXCL);
    if (mshmid < 0)
    {
        fprintf (stderr, "Unable to open shared memory\n");
        return -1;
    }

    mshmAttach = (void*)shmat (mshmid, NULL, 0);
    if (!mshmAttach)
    {
        fprintf (stderr, "No shared memory pointer produced\n");
        return -1;
    }
#endif
    return 0;
}

#ifdef HAVE_ALSA_ASOUNDLIB_H
int
asemInit ()
{
    int ofile = open ("/tmp/lsdvis.ipc", O_CREAT | O_WRONLY, 0666);
    close (ofile);
    
    /* Set up IPC semaphore */
    key_t semkey, shmkey;
    semkey = ftok ("/tmp/lsdvis.ipc", 654);
    shmkey = ftok ("/tmp/lsdvis.ipc", 456);
    
    if (semkey == (key_t)-1 || shmkey == (key_t)-1)
    {
        fprintf (stderr, "Unable to get IPC key: %s\n", strerror (errno));
        return -1;
    }
    
    /* Get semaphore */
    asemid = semget (semkey, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if (asemid < 0)
    {
        fprintf (stderr, "Unable to create semaphore\n");
        return -1;
    }
    
    if (semctl (asemid, 0, SETVAL, 0) < 0)
    {
        fprintf (stderr, "Unable to init semaphore\n");
        return -1;
    }
    
    /* Initialise sem operation buf */
    asem[0].sem_num = 0;
    asem[1].sem_num = 0;
    asem[0].sem_flg = IPC_NOWAIT;
    asem[1].sem_flg = IPC_NOWAIT;
    
    /* Get Shared Memory */
    ashmid = shmget (shmkey,
                     sizeof( double ) * NUM_BANDS,
                     0666 | IPC_CREAT | IPC_EXCL);
    if (ashmid < 0)
    {
        fprintf (stderr, "Unable to open shared memory\n");
        return -1;
    }
    
    ashmAttach = (void*)shmat (ashmid, NULL, 0);
    if (!ashmAttach)
    {
        fprintf (stderr, "No shared memory pointer produced\n");
        return -1;
    }
    
    return 0;
}
#endif

int
mstartPipeProcess ()
{
#ifndef HW_RVL
    mpipeProc = fork ();
    if (mpipeProc == 0)  /* Child */
    {
        PCMPipeEntryMpd ();
        _exit (0);
    }
    else if (mpipeProc > 0) /* Success */
        return 0;
    else /* Failure */
        return -1;
    return 0;
#endif
}


int
mstopPipeProcess ()
{
#ifndef HW_RVL
    kill (mpipeProc, SIGKILL);
    return 0;
#endif
}

#ifdef HAVE_ALSA_ASOUNDLIB_H
int
astartPipeProcess ()
{
    apipeProc = fork ();
    if (apipeProc == 0)  /* Child */
    {
        PCMPipeEntryAlsa ();
        _exit (0);
    }
    else if (apipeProc > 0) /* Success */
        return 0;
    else /* Failure */
        return -1;
    return 0;
}
#endif


#ifdef HAVE_ALSA_ASOUNDLIB_H
int
astopPipeProcess ()
{
    kill (apipeProc, SIGKILL);
    return 0;
}
#endif

int
visPluginMPDInit (struct LSD_ScenePlugin const* plugin)
{
    mlocalBuffer[0] = 0.0;
    mlocalBuffer[1] = 0.0;
    mlocalBuffer[2] = 0.0;
    
    msemInit ();
    mstartPipeProcess ();

    plugininit_registerNodeClass (plugin,
                                  NULL,
                                  makeNode,
                                  restoreNode,
                                  cleanNode,
                                  deleteNode,
                                  0,
                                  "MPD Visualiser",
                                  "Desc",
                                  0,
                                  mbfFuncs,
                                  mbpFuncs);
    
    return 0;
}

#ifdef HAVE_ALSA_ASOUNDLIB_H
int
visPluginALSAInit (struct LSD_ScenePlugin const* plugin)
{
    alocalBuffer[0] = 0.0;
    alocalBuffer[1] = 0.0;
    alocalBuffer[2] = 0.0;
    
    asemInit ();
    astartPipeProcess ();
    
    
    
    plugininit_registerNodeClass (plugin,
                                  NULL,
                                  makeNode,
                                  restoreNode,
                                  cleanNode,
                                  deleteNode,
                                  0,
                                  "ALSA Visualiser",
                                  "Desc",
                                  1,
                                  abfFuncs,
                                  abpFuncs);
    
    return 0;
}
#endif

int
visPluginInit (struct LSD_ScenePlugin const* plugin)
{
    
    floatTypeId = core_getFloatTypeID ();
    
    visPluginMPDInit (plugin);
#ifdef HAVE_ALSA_ASOUNDLIB_H
    visPluginALSAInit (plugin);
#endif
    
    return 0;
}


void
visPluginClean (struct LSD_ScenePlugin const* plugin)
{
#ifndef HW_RVL
    struct shmid_ds shmid_struct;

    mstopPipeProcess ();
    semctl ( msemid, 0, IPC_RMID );
    shmdt (mshmAttach);
    shmctl (mshmid, IPC_RMID, &shmid_struct);
#endif
    
#ifdef HAVE_ALSA_ASOUNDLIB_H
    astopPipeProcess ();
    semctl ( asemid, 0, IPC_RMID );
    shmdt (ashmAttach);
    shmctl (ashmid, IPC_RMID, &shmid_struct);
#endif
}


void
visPluginRPC (cJSON* in, cJSON* out)
{
}


static const struct LSD_ScenePluginHEAD pluginHead = {
    visPluginInit,
    visPluginClean,
    visPluginRPC
};


/*
  * In order for LightShoppe to properly traverse scopes and
  *connect to the Plugin HEAD, an accessor function named
  *`getPluginHead`
  * must be included by the plugin author that returns a
  *reference to the Plugin HEAD
  */

extern const struct LSD_ScenePluginHEAD*
getPluginHead ()
{
    return &pluginHead;
}