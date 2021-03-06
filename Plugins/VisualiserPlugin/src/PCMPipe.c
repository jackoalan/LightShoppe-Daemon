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
  * PCMPipe is a small helper program that receives audio
  *format data as its execution arguments and raw PCM data
  *via /tmp/mpd.conf.
  *
  * Samples are processed with a Fast-Fourier Wave Transform
  *and compressed into a small number of bands. These bands
  *are then sent to LightShoppe's music visualiser plugin
  *via shared memory
  */

#include <sys/types.h>
#ifndef HW_RVL
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#else
#include <gctypes.h>
#endif
#include <string.h> /* For strerror(3c) */
#include <errno.h> /* For errno */
#include <unistd.h> /* rand(3c) */
#include <stdio.h>
#include <fftw3.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

#include "../config.h"

#ifdef HAVE_ALSA_ASOUNDLIB_H
#include <alsa/asoundlib.h>
#endif

/* Transformational vars */
#define NUM_SAMPLES 2048
#define FFT_RESULTS ( NUM_SAMPLES / 2 + 1 )
#define NUM_BANDS 3
#define FREQS_PER_BAND ( FFT_RESULTS / NUM_BANDS )

static double* inputBuf = NULL;
static fftw_complex* outputBuf = NULL;
static double outputReals[FFT_RESULTS];
static double outputBands[NUM_BANDS];
static fftw_plan thePlan = NULL;

/* IPC vars */
static int semid;
#ifndef HW_RVL
static struct sembuf sem[2];
#endif
static int shmid;
static void* shmAttach;

/* Log */
/* static FILE* logfile; */

int
shmInit (int prog) // 0 mpd; 1 alsa
{
#ifndef HW_RVL
    /* Set up IPC semaphore */
    key_t semkey, shmkey;
    semkey = ftok ("/tmp/lsdvis.ipc", (prog)?654:321);
    shmkey = ftok ("/tmp/lsdvis.ipc", (prog)?456:123);

    if (semkey == (key_t)-1 || shmkey == (key_t)-1)
        /* fprintf(logfile,"Unable to get IPC key\n"); */
        return -1;

    /* Get semaphore */
    semid = semget (semkey, 1, 0666);

    if (semid < 0)
        /* fprintf(logfile,"Unable to get semaphore:
         * %s\n",strerror(errno)); */
        return -1;

    /* Initialise sem operation buf */
    sem[0].sem_num = 0;
    sem[1].sem_num = 0;
    sem[0].sem_flg = SEM_UNDO;
    sem[1].sem_flg = SEM_UNDO;

    /* Get Shared Memory */
    shmid = shmget (shmkey, sizeof( double ) * NUM_BANDS, 0666);
    if (shmid < 0)
        /* fprintf(logfile,"Unable to get shared memory\n"); */
        return -1;

    shmAttach = (void*)shmat (shmid, NULL, 0);
    if (!shmAttach)
        /* fprintf(logfile,"Unable to attach to shared
         * memory\n"); */
        return -1;
#endif

    return 0;
}


int
fftInit ()
{
    inputBuf = fftw_malloc (sizeof( double ) * NUM_SAMPLES);
    outputBuf = fftw_malloc (sizeof( fftw_complex ) * FFT_RESULTS);
    thePlan = fftw_plan_dft_r2c_1d (NUM_SAMPLES,
                                    inputBuf,
                                    outputBuf,
                                    FFTW_ESTIMATE);
    if (!inputBuf || !outputBuf || !thePlan)
        return -1;
    return 0;
}


int
procSamples ()
{

    int i, j;

    /* TRANSFORM! */
    fftw_execute (thePlan);

    /* Convert back to reals */

    for (i = 0; i < FFT_RESULTS; ++i)
        outputReals[i] = sqrt (
            outputBuf[i][0] * outputBuf[i][0] + outputBuf[i][1] *
            outputBuf[i][1]);

    /* Compress into bands */

    for (i = 0; i < NUM_BANDS; ++i)
    {
        double accumulatedVal = 0;
        for (j = 0; j < FREQS_PER_BAND; ++j)
            accumulatedVal += outputReals[FREQS_PER_BAND * i + j];
         /* Calibration function derived using pink noise */
        double attVal = ( NUM_BANDS / pow (( i + 1 ), 4)) + 2;
        outputBands[i] = accumulatedVal / FREQS_PER_BAND / 60000 / attVal;
    }

    return 0;
}


int
sendBands ()
{
#ifndef HW_RVL
    /* Wait and get the semaphore */
    sem[0].sem_op = 0;
    sem[1].sem_op = 1;
    if (semop (semid, sem, 2) < 0)
        return -1;

    /* Copy the data to LightShoppe */
    double* shm = (double*)shmAttach;
    int i;
    for (i = 0; i < NUM_BANDS; ++i)
        shm[i] = outputBands[i];

    /* Release the semaphore */
    sem[0].sem_op = -1;
    if (semop (semid, sem, 1) < 0)
        return -1;
#endif

    return 0;
}


/*
  * int sendTest(double fade){
  *  // Wait and get the semaphore sem[0].sem_op = 0;
  *  sem[1].sem_op = 1;
  *  if(semop(semid,sem,2) < 0)
  *      return -1;
  *
  *  // Copy the data to LightShoppe double* shm =
  * (double*)shmAttach;
  *  int i;
  *  for(i=0;i<NUM_BANDS;++i) shm[i] = 0.3*i+fade;
  *
  *  // Release the semaphore sem[0].sem_op = -1;
  *  if(semop(semid,sem,1) < 0)
  *      return -1;
  *
  *  return 0;
  * }
  *
  * int testFade(){
  *  struct timeval tv;
  *  tv.tv_sec = 0;
  *
  *  double fade = 0;
  *  while(fade < 1.0){
  *      sendTest(fade);
  *      tv.tv_usec = 50000;
  *      select(0,0,0,0,&tv);
  *      fade += 0.1;
  *  }
  *  return 0;
  * }
  */


int
PCMPipeEntryMpd ()
{
    int run = 1;
    
#ifdef __linux__
    prctl (PR_SET_NAME, (const char*)"lsd_MPDPipe", 0, 0, 0);
#endif
    
    
    int canProc = 1;
    if (fftInit () < 0)
        canProc = 0;
    
    int connected = 1;
    if (shmInit (0) < 0)
        connected = 0;
    
    // MPD Pipe
    static FILE* audioPipe;
    
#ifndef HW_RVL
    audioPipe = fopen ("/tmp/mpd.fifo", "r");
#endif

    
    int16_t samples[NUM_SAMPLES];
    size_t readSize;
    while (run)
    {
#ifndef HW_RVL
        readSize = fread (samples, sizeof( int16_t ), NUM_SAMPLES, audioPipe);
        if (readSize != NUM_SAMPLES){
            break;
        }
#endif
        
        /* Convert samples to floating point for
         * transformation */
        int i;
        for (i = 0; i < NUM_SAMPLES; ++i)
            inputBuf[i] = samples[i];
        if (canProc)
            procSamples ();
        if (connected)
            sendBands ();
    }
    
#ifndef HW_RVL
    fclose (audioPipe);
#endif
    fftw_cleanup ();
#ifndef HW_RVL
    shmdt (shmAttach);
#endif
    
    return 0;
}

#ifdef HAVE_ALSA_ASOUNDLIB_H
int
PCMPipeEntryAlsa ()
{
    int run = 1;

#ifdef __linux__
    prctl (PR_SET_NAME, (const char*)"lsd_ALSAPipe", 0, 0, 0);
#endif


    int canProc = 1;
    if (fftInit () < 0)
        canProc = 0;

    int connected = 1;
    if (shmInit (1) < 0)
        connected = 0;

    
    // ALSA Pipe
    snd_pcm_t *handle;


    if(snd_pcm_open (&handle, "hw:0,0", SND_PCM_STREAM_CAPTURE, 0) < 0)
        _exit(1);
    
    if(snd_pcm_set_params (handle,
                           SND_PCM_FORMAT_S16_LE,
                           SND_PCM_ACCESS_RW_INTERLEAVED,
                           2,
                           44100,
                           1,
                           500000) < 0)
        _exit(1);
    
    if(snd_pcm_start (handle) < 0)
        _exit(1);

    int16_t samples[2][NUM_SAMPLES];
    size_t readSize;
    while (run)
    {
        readSize = snd_pcm_readi(handle, (void**)samples, NUM_SAMPLES);
        if (readSize != NUM_SAMPLES){
            break;
        }

        /* Convert samples to floating point for
         * transformation */
        int i;
        for (i = 0; i < NUM_SAMPLES; ++i)
            inputBuf[i] = (samples[0][i] + samples[1][i]) / 2;
        if (canProc)
            procSamples ();
        if (connected)
            sendBands ();
    }

    fftw_cleanup ();
    shmdt (shmAttach);

    return 0;
}
#endif


/*int main(int argc, char** argv){
    return PCMPipeEntryAlsa();
}*/
