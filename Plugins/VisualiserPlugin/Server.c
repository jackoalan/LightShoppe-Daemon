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
#include "../../PluginAPI.h"
#include "../../cJSON.h"
#include "../../NodeInstAPI.h"
#include "../../CorePlugin.h"

#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "PCMPipe.h"

/* This should match NUM_BANDS in PCMPipe */
#define NUM_BANDS 3

/* IPC vars */
static int semid;
static struct sembuf sem[2];
static int shmid;
static void* shmAttach;

static pid_t pipeProc;

/* Process-local storage buffer for band values */
static double localBuffer[NUM_BANDS];

/* Float type */
int floatTypeId;

void
localBufCopy (struct LSD_SceneNodeOutput const* output)
{
    /* Get semaphore */
    sem[0].sem_op = 0;
    sem[1].sem_op = 1;
    if (semop (semid, sem, 2) < 0)
        return;

    /* Copy buffer */
    double* shm = (double*)shmAttach;
    int i;
    for (i = 0; i < NUM_BANDS; ++i)
        localBuffer[i] = shm[i];

    /* Release semaphore */
    sem[0].sem_op = -1;
    semop (semid, sem, 1);
}


void*
getHigh (struct LSD_SceneNodeOutput const* output)
{
    return &( localBuffer[2] );
}


void*
getMid (struct LSD_SceneNodeOutput const* output)
{
    return &( localBuffer[1] );
}


void*
getLow (struct LSD_SceneNodeOutput const* output)
{
    return &( localBuffer[0] );
}


/* Buffer func tables */
static const bfFunc bfFuncs[] =
{localBufCopy};

static const bpFunc bpFuncs[] =
{getHigh, getMid, getLow};

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
semInit ()
{
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
    semid = semget (semkey, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if (semid < 0)
    {
        fprintf (stderr, "Unable to create semaphore\n");
        return -1;
    }

    if (semctl (semid, 0, SETVAL, 0) < 0)
    {
        fprintf (stderr, "Unable to init semaphore\n");
        return -1;
    }

    /* Initialise sem operation buf */
    sem[0].sem_num = 0;
    sem[1].sem_num = 0;
    sem[0].sem_flg = IPC_NOWAIT;
    sem[1].sem_flg = IPC_NOWAIT;

    /* Get Shared Memory */
    shmid = shmget (shmkey,
                    sizeof( double ) * NUM_BANDS,
                    0666 | IPC_CREAT | IPC_EXCL);
    if (shmid < 0)
    {
        fprintf (stderr, "Unable to open shared memory\n");
        return -1;
    }

    shmAttach = (void*)shmat (shmid, NULL, 0);
    if (!shmAttach)
    {
        fprintf (stderr, "No shared memory pointer produced\n");
        return -1;
    }

    return 0;
}


int
startPipeProcess ()
{
    pipeProc = fork ();
    if (pipeProc == 0)  /* Child */
    {
        PCMPipeEntry ();
        _exit (0);
    }
    else if (pipeProc > 0) /* Success */
        return 0;
    else /* Failure */
        return -1;
    return 0;
}


int
stopPipeProcess ()
{
    kill (pipeProc, SIGKILL);
    return 0;
}


int
visPluginInit (struct LSD_ScenePlugin const* plugin)
{
    floatTypeId = core_getFloatTypeID ();
    localBuffer[0] = 0.0;
    localBuffer[1] = 0.0;
    localBuffer[2] = 0.0;

    semInit ();
    startPipeProcess ();

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
                                  bfFuncs,
                                  bpFuncs);

    return 0;
}


void
visPluginClean (struct LSD_ScenePlugin const* plugin)
{
    struct shmid_ds shmid_struct;

    stopPipeProcess ();

    semctl ( semid, 0, IPC_RMID );
    shmdt (shmAttach);
    shmctl (shmid, IPC_RMID, &shmid_struct);
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