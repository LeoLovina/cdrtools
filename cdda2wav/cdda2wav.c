/* @(#)cdda2wav.c	1.17 00/06/24 Copyright 1998,1999,2000 Heiko Eissfeldt */
#ifndef lint
static char     sccsid[] =
"@(#)cdda2wav.c	1.17 00/06/24 Copyright 1998,1999,2000 Heiko Eissfeldt";

#endif
#undef DEBUG_BUFFER_ADDRESSES
#undef GPROF
#undef DEBUG_FORKED
#undef DEBUG_CLEANUP
#undef DEBUG_DYN_OVERLAP
#undef DEBUG_READS
#undef SIM_ILLLEADOUT
#define DEBUG_ILLLEADOUT	0	/* 0 disables, 1 enables */
/*
 * Copyright: GNU Public License 2 applies
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * parts    (C) Peter Widow
 * parts    (C) Thomas Niederreiter
 * parts    (C) RSA Data Security, Inc.
 *
 * last changes:
 *   18.12.93 - first version,	OK
 *   01.01.94 - generalized & clean up HE
 *   10.06.94 - first linux version HE
 *   12.06.94 - wav header alignment problem fixed HE
 *   12.08.94 - open the cdrom device O_RDONLY makes more sense :-)
 *		no more floating point math
 *		change to sector size 2352 which is more common
 *		sub-q-channel information per kernel ioctl requested
 *		doesn't work as well as before
 *		some new options (-max -i)
 *   01.02.95 - async i/o via semaphores and shared memory
 *   03.02.95 - overlapped reading on sectors
 *   03.02.95 - generalized sample rates. all integral divisors are legal
 *   04.02.95 - sun format added
 *              more divisors: all integral halves >= 1 allowed
 *		floating point math needed again
 *   06.02.95 - bugfix for last track and not d0
 *              tested with photo-cd with audio tracks
 *		tested with xa disk 
 *   29.01.96 - new options for bulk transfer
 *   01.06.96 - tested with enhanced cd
 *   01.06.96 - tested with cd-plus
 *   02.06.96 - support pipes
 *   02.06.96 - support raw format
 *   04.02.96 - security hole fixed
 *   22.04.97 - large parts rewritten
 *   28.04.97 - make file names DOS compatible
 *   01.09.97 - add speed control
 *   20.10.97 - add find mono option
 *   Jan/Feb 98 - conversion to use Joerg Schillings SCSI library
 *
 */

#include "config.h"

#include <sys/types.h>
#include <unixstd.h>

#include <stdio.h>
#include <standard.h>
#include <stdxlib.h>
#include <strdefs.h>
#include <schily.h>
#if	defined HAVE_STRINGS_H
#include <strings.h>
#endif
#include <signal.h>
#include <math.h>
#include <fctldefs.h>
#include <time.h>
#if (defined (HAVE_SYS_TIME_H) || (HAVE_SYS_TIME_H != 1)) && defined (TIME_WITH_SYS_TIME)
#include <sys/time.h>
#endif
#if defined (HAVE_LIMITS_H) && (HAVE_LIMITS_H == 1)
#include <limits.h>
#endif
#if defined (HAVE_SYS_IOCTL_H) && (HAVE_SYS_IOCTL_H == 1)
#include <sys/ioctl.h>
#endif
#include <errno.h>
#include <sys/stat.h>

#if defined (HAVE_SYS_WAIT_H) && (HAVE_SYS_WAIT_H == 1)
#include <sys/wait.h>
#endif
#if defined (HAVE_SETPRIORITY) && (HAVE_SETPRIORITY == 1)
#include <sys/resource.h>
#endif
#include <vadefs.h>

#include <standard.h>
#include <scg/scsitransp.h>
#include "mytype.h"
#include "sndconfig.h"

#include "semshm.h"	/* semaphore functions */
#include "sndfile.h"
#include "wav.h"	/* wav file header structures */
#include "sun.h"	/* sun audio file header structures */
#include "raw.h"	/* raw file handling */
#include "aiff.h"	/* aiff file handling */
#include "aifc.h"	/* aifc file handling */
#ifdef	USE_LAME
#include "mp3.h"	/* mp3 file handling */
#endif
#include "interface.h"  /* low level cdrom interfacing */
#include "cdda2wav.h"
#include "resample.h"
#include "toc.h"
#include "setuid.h"
#include "ringbuff.h"
#include "global.h"


static const char * optstring = "D:A:I:O:c:b:r:a:t:i:d:o:n:v:l:E:C:S:p:M:P:K:smweNqxRBVhFGHTJgQ";

extern char *optarg;
extern int optind, opterr, optopt;

int main				__PR((int argc, char **argv));
static void RestrictPlaybackRate	__PR((long newrate));
static void output_indices		__PR((FILE *fp, index_list *p, unsigned trackstart));
static int write_info_file		__PR((char *fname_baseval, unsigned int track, unsigned long SamplesDone, int numbered));
static void CloseAudio			__PR((char *fname_baseval, unsigned int track, int bulkflag, int channels_val, unsigned long nSamples, struct soundfile *audio_out));
static void CloseAll			__PR((void));
static void OpenAudio			__PR((char *fname, double rate, long nBitsPerSample, long channels_val, unsigned long expected_bytes, struct soundfile*audio_out));
static void set_offset			__PR((myringbuff *p, int offset));
static int get_offset			__PR((myringbuff *p));
static void usage			__PR((void));
static void init_globals		__PR((void));
static int is_fifo __PR((char * filename));


#if defined (__GLIBC__) || defined(_GNU_C_SOURCE) || defined(__USE_GNU) || defined(__CYGWIN32__)
#define USE_GETOPT_LONG
#endif

#ifdef USE_GETOPT_LONG
#include <getopt.h>	/* for get_long_opt () */
static struct option options [] = {
	{"device",required_argument,NULL,'D'},
	{"auxdevice",required_argument,NULL,'A'},
	{"interface",required_argument,NULL,'I'},
	{"output-format",required_argument,NULL,'O'},
	{"channels",required_argument,NULL,'c'},
	{"bits-per-sample",required_argument,NULL,'b'},
	{"rate",required_argument,NULL,'r'},
	{"divider",required_argument,NULL,'a'},
	{"track",required_argument,NULL,'t'},
	{"index",required_argument,NULL,'i'},
	{"duration",required_argument,NULL,'d'},
	{"offset",required_argument,NULL,'o'},
	{"sectors-per-request",required_argument,NULL,'n'},
	{"buffers-in-ring",required_argument,NULL,'l'},
	{"output-endianess",required_argument,NULL,'E'},
	{"cdrom-endianess",required_argument,NULL,'C'},
	{"verbose-level",required_argument,NULL,'v'},
	{"sound-device",required_argument,NULL,'K'},
#ifdef MD5_SIGNATURES
	{"md5",required_argument,NULL,'M'},
#endif
	{"stereo",no_argument,NULL,'s'},
	{"mono",no_argument,NULL,'m'},
	{"wait",no_argument,NULL,'w'},
	{"find-extremes",no_argument,NULL,'F'},
	{"find-mono",no_argument,NULL,'G'},
#ifdef	ECHO_TO_SOUNDCARD
	{"echo",no_argument,NULL,'e'},
#endif
	{"info-only",no_argument,NULL,'J'},
	{"no-write",no_argument,NULL,'N'},
	{"no-infofile",no_argument,NULL,'H'},
	{"quiet",no_argument,NULL,'q'},
	{"max",no_argument,NULL,'x'},

	{"set-overlap",required_argument,NULL,'P'},
	{"speed-select",required_argument,NULL,'S'},

	{"dump-rates",no_argument,NULL,'R'},
	{"bulk",no_argument,NULL,'B'},
	{"deemphasize",no_argument,NULL,'T'},
	{"playback-realtime",required_argument,NULL,'p'},
	{"verbose-SCSI",no_argument,NULL,'V'},
	{"help",no_argument,NULL,'h'},
	{"gui",no_argument,NULL,'g'},
	{"silent-SCSI",no_argument,NULL,'Q'},
	{NULL,0,NULL,0}
};
#endif /* USE_GETOPT_LONG */

#if	defined(__CYGWIN32__) ||defined(__EMX__)
#include <io.h>		/* for setmode() prototype */
#endif

/* global variables */
global_t global;

/* static variables */
static unsigned long nSamplesDone = 0;

static	int child_pid = -2;

static unsigned long *nSamplesToDo;
static unsigned int current_track;
static int bulk = 0;

unsigned int get_current_track __PR((void));

unsigned int get_current_track()
{
	return current_track;
}

static void RestrictPlaybackRate( newrate )
	long newrate;
{
       global.playback_rate = newrate;

       if ( global.playback_rate < 25 ) global.playback_rate = 25;   /* filter out insane values */
       if ( global.playback_rate > 250 ) global.playback_rate = 250;

       if ( global.playback_rate < 100 )
               global.nsectors = (global.nsectors*global.playback_rate)/100;
}


long SamplesNeeded( amount, undersampling_val)
	long amount;
	long undersampling_val;
{
  long retval = ((undersampling_val * 2 + Halved)*amount)/2;
  if (Halved && (*nSamplesToDo & 1))
    retval += 2;
  return retval;
}

static int argc2;
static int argc3;
static char **argv2;

static void reset_name_iterator __PR((void));
static void reset_name_iterator ()
{
	argv2 -= argc3 - argc2;
	argc2 = argc3;
}

static char *get_next_name __PR((void));
static char *get_next_name ()
{
	if (argc2 > 0) {
		argc2--;
		return (*argv2++);
	} else {
		return NULL;
	}
}

static char *cut_extension __PR(( char * fname ));

static char
*cut_extension (fname)
	char *fname;
{
	char *pp;

	pp = strrchr(fname, '.');

	if (pp == NULL) {
		pp = fname + strlen(fname);
	}
	*pp = '\0';

	return pp;
}

#ifdef INFOFILES
static void output_indices(fp, p, trackstart)
	FILE *fp;
	index_list *p;
	unsigned trackstart;
{
  int ci;

  fprintf(fp, "Index=\t\t");

  if (p == NULL) {
    fprintf(fp, "0\n");
    return;
  }

  for (ci = 1; p != NULL; ci++, p = p->next) {
    int frameoff = p->frameoffset;

    if (p->next == NULL)
	 fputs("\nIndex0=\t\t", fp);
#if 0
    else if ( ci > 8 && (ci % 8) == 1)
	 fputs("\nIndex =\t\t", fp);
#endif
    if (frameoff != -1)
         fprintf(fp, "%d ", frameoff - trackstart);
    else
         fprintf(fp, "-1 ");
  }
  fputs("\n", fp);
}

/*
 * write information before the start of the sampling process
 *
 *
 * uglyfied for Joerg Schillings ultra dumb line parser
 */
static int write_info_file(fname_baseval, track, SamplesDone, numbered)
	char *fname_baseval;
	unsigned int track;
	unsigned long int SamplesDone;
	int numbered;
{
  FILE *info_fp;
  char fname[200];
  char datetime[30];
  time_t utc_time;
  struct tm *tmptr;

  /* write info file */
  if (!strcmp(fname_baseval,"standard output")) return 0;

  strncpy(fname, fname_baseval, sizeof(fname) -1);
  fname[sizeof(fname) -1] = 0;
  if (numbered)
    sprintf(cut_extension(fname), "_%02u.inf", track);
  else
    strcpy(cut_extension(fname), ".inf");

  info_fp = fopen (fname, "w");
  if (!info_fp)
    return -1;

#if 0
#ifdef MD5_SIGNATURES
  if (global.md5blocksize)
    MD5Final (global.MD5_result, &global.context);
#endif
#endif

  utc_time = time(NULL);
  tmptr = localtime(&utc_time);
  if (tmptr) {
    strftime(datetime, sizeof(datetime), "%x %X", tmptr);
  } else {
    strncpy(datetime, "unknown", sizeof(datetime));
  }
  fprintf(info_fp, "#created by cdda2wav %s %s\n#\n", VERSION
	  , datetime
	  );
  fprintf(info_fp,
"CDINDEX_DISCID=\t'%s'\n" , global.cdindex_id);
  fprintf(info_fp,
"CDDB_DISCID=\t0x%08lx\nMCN=\t\t%s\nISRC=\t\t%15.15s\n#\nAlbumtitle=\t'%s'\n"
	  , (unsigned long) global.cddb_id
	  , MCN
	  , g_toc[track-1].ISRC
	  , global.disctitle != NULL ? global.disctitle : (const unsigned char *)""
	  );
  fprintf(info_fp,
	  "Tracktitle=\t'%s'\n"
	  , global.tracktitle[track-1] ? global.tracktitle[track-1] : (const unsigned char *)""
	  );
  fprintf(info_fp, "Tracknumber=\t%u\n"
	  , track
	  );
  fprintf(info_fp, 
	  "Trackstart=\t%d\n"
	  , g_toc[track-1].dwStartSector
	  );
  fprintf(info_fp, 
	  "# track length in sectors (1/75 seconds each), rest samples\nTracklength=\t%ld, %d\n"
	  , SamplesDone/588L,(int)(SamplesDone%588));
  fprintf(info_fp, 
	  "Pre-emphasis=\t%s\n"
	  , g_toc[track-1].bFlags & 1 && (global.deemphasize == 0) ? "yes" : "no");
  fprintf(info_fp, 
	  "Channels=\t%d\n"
	  , g_toc[track-1].bFlags & 8 ? 4 : global.channels == 2 ? 2 : 1);
  fprintf(info_fp, 
	  "Copy_permitted=\t%s\n"
	  , g_toc[track-1].bFlags & 2 ? "yes" : "no");
  fprintf(info_fp, 
	  "Endianess=\t%s\n"
	  , global.need_big_endian ? "big" : "little"
	  );
  fprintf(info_fp, "# index list\n");
  output_indices(info_fp, global.trackindexlist[track-1], GetStartSector(track));
#if 0
/* MD5 checksums in info files are currently broken.
 *  for on-the-fly-recording the generation of info files has been shifted
 *  before the recording starts, so there is no checksum at that point.
 */
#ifdef MD5_SIGNATURES
  fprintf(info_fp, 
	  "#(blocksize) checksum\nMD-5=\t\t(%d) %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n"
	  , global.md5blocksize
	  , global.MD5_result[0]
	  , global.MD5_result[1]
	  , global.MD5_result[2]
	  , global.MD5_result[3]
	  , global.MD5_result[4]
	  , global.MD5_result[5]
	  , global.MD5_result[6]
	  , global.MD5_result[7]
	  , global.MD5_result[8]
	  , global.MD5_result[9]
	  , global.MD5_result[10]
	  , global.MD5_result[11]
	  , global.MD5_result[12]
	  , global.MD5_result[13]
	  , global.MD5_result[14]
	  , global.MD5_result[15]);
#endif
#endif
  fclose(info_fp);
  return 0;
}
#endif

static void CloseAudio(fname_baseval, track, bulkflag, channels_val, nSamples,
			audio_out)
	char *fname_baseval;
	unsigned int track;
	int bulkflag;
	int channels_val;
	unsigned long nSamples;
	struct soundfile *audio_out;
{
      /* define length */
      audio_out->ExitSound( global.audio, (nSamples-global.SkippedSamples)*global.OutSampleSize*channels_val );

      close (global.audio);
      global.audio = -1;
}

static unsigned int track = 1;

/* On terminating:
 * define size-related entries in audio file header, update and close file */
static void CloseAll ()
{
  int chld_return_status = 0;
  int amichild;

  /* terminate child process first */
  amichild = child_pid == 0;

#if	defined	HAVE_FORK_AND_SHAREDMEM
  if (amichild == 0) {
# ifdef DEBUG_CLEANUP
fprintf(stderr, "Parent (READER) terminating, \n");
# endif

    if (global.iloop > 0) {
      /* set to zero */
      global.iloop = 0;
      kill(child_pid, SIGINT);
    }

#else
  if (1) {
#endif
    /* switch to original mode and close device */
    EnableCdda (get_scsi_p(), 0);
#if	defined	HAVE_FORK_AND_SHAREDMEM
  } else {
#ifdef DEBUG_CLEANUP
fprintf(stderr, "Child (WRITER) terminating, \n");
#endif
#else
# ifdef DEBUG_CLEANUP
fprintf(stderr, "Cdda2wav single process terminating, \n");
# endif
#endif
    /* do general clean up */

    if (global.audio>=0) {
      if (bulk) {
        /* finish sample file for this track */
        CloseAudio(global.fname_base, current_track, bulk, global.channels,
  	  global.nSamplesDoneInTrack, global.audio_out);
      } else {
        /* finish sample file for this track */
        CloseAudio(global.fname_base, track, bulk, global.channels,
  	  (unsigned int) *nSamplesToDo, global.audio_out);
      }
    }

    /* tell minimum and maximum amplitudes, if required */
    if (global.findminmax) {
      fprintf(stderr,
	 "Right channel: minimum amplitude :%d/-32768, maximum amplitude :%d/32767\n", 
	global.minamp[0], global.maxamp[0]);
      fprintf(stderr,
	 "Left  channel: minimum amplitude :%d/-32768, maximum amplitude :%d/32767\n", 
	global.minamp[1], global.maxamp[1]);
    }

    /* tell mono or stereo recording, if required */
    if (global.findmono) {
      fprintf(stderr, "Audio samples are originally %s.\n", global.ismono ? "mono" : "stereo");
    }


#ifdef DEBUG_CLEANUP
fprintf(stderr, "Child (WRITER) terminating, \n");
#endif

    return;
  }

  if (global.have_forked == 1) {
#ifdef DEBUG_CLEANUP
fprintf(stderr, "Parent wait for child death, \n");
#endif

#if defined(HAVE_SYS_WAIT_H) && (HAVE_SYS_WAIT_H == 1)
    /* wait for child to terminate */
    if (0 > wait(&chld_return_status)) {
      perror("");
    } else {
      if (WIFEXITED(chld_return_status)) {
        if (WEXITSTATUS(chld_return_status)) {
          fprintf(stderr, "\nW Child exited with %d\n", WEXITSTATUS(chld_return_status));
        }
      }
      if (WIFSIGNALED(chld_return_status)) {
        fprintf(stderr, "\nW Child exited due to signal %d\n", WTERMSIG(chld_return_status));
      }
      if (WIFSTOPPED(chld_return_status)) {
        fprintf(stderr, "\nW Child is stopped due to signal %d\n", WSTOPSIG(chld_return_status));
      }
    }
#endif

#ifdef DEBUG_CLEANUP
fprintf(stderr, "\nW Parent child death, state:%d\n", chld_return_status);
#endif
  }
#ifdef GPROF
  rename("gmon.out", "gmon.child");
#endif

}


/* report a usage error and exit */
#ifdef  PROTOTYPES
static void usage2 (const char *szMessage, ...)
#else
static void usage2 (szMessage, va_alist)
	const char *szMessage;
	va_dcl
#endif
{
  va_list marker;

#ifdef  PROTOTYPES
  va_start(marker, szMessage);
#else
  va_start(marker);
#endif

  error("%r", szMessage, marker);

  va_end(marker);
  fprintf(stderr, "\nPlease use -h or consult the man page for help.\n");

  exit (1);
}


/* report a fatal error, clean up and exit */
#ifdef  PROTOTYPES
void FatalError (const char *szMessage, ...)
#else
void FatalError (szMessage, va_alist)
	const char *szMessage;
	va_dcl
#endif
{
  va_list marker;

#ifdef  PROTOTYPES
  va_start(marker, szMessage);
#else
  va_start(marker);
#endif

  error("%r", szMessage, marker);

  va_end(marker);

  if (child_pid == -2)
    exit (1);

  if (child_pid == 0) {
    /* kill the parent too */
    kill(getppid(), SIGINT);
  } else {
	kill(child_pid, SIGINT);
  }
  exit (1);
}


/* open the audio output file and prepare the header. 
 * the header will be defined on terminating (when the size
 * is known). So hitting the interrupt key leaves an intact
 * file.
 */
static void OpenAudio (fname, rate, nBitsPerSample, channels_val, expected_bytes, audio_out)
	char *fname;
	double rate;
	long nBitsPerSample;
	long channels_val;
	unsigned long expected_bytes;
	struct soundfile * audio_out;
{
  if (global.audio == -1) {

    global.audio = open (fname, O_CREAT | O_WRONLY | O_TRUNC | O_BINARY
#ifdef SYNCHRONOUS_WRITE
			 | O_SYNC
#endif
		  , 0666);
    if (global.audio == -1) {
      if (errno == EAGAIN && is_fifo(fname)) {
        FatalError ("Could not open fifo %s. Probably no fifo reader present.\n", fname);
      }
      perror("open audio sample file");
      FatalError ("Could not open file %s\n", fname);
    }
  }
  global.SkippedSamples = 0;
  any_signal = 0;
  audio_out->InitSound( global.audio, channels_val, (unsigned long)rate, nBitsPerSample, expected_bytes );

#ifdef MD5_SIGNATURES
  if (global.md5blocksize)
    MD5Init (&global.context);
  global.md5count = global.md5blocksize;
#endif
}

#include "scsi_cmds.h"

static int RealEnd __PR((SCSI *scgp));

static int RealEnd(scgp)
	SCSI	*scgp;
{
	if (myscsierr(scgp) != 0) {
		int c,k,q;

		k = scsi_sense_key(scgp);
		c = scsi_sense_code(scgp);
		q = scsi_sense_qual(scgp);
		if ((k == 0x05 /* ILLEGAL_REQUEST */ &&
		     c == 0x21 /* lba out of range */ &&
		     q == 0x00) ||
		    (k == 0x05 /* ILLEGAL_REQUEST */ &&
		     c == 0x63 /*end of user area encountered on this track*/ &&
		     q == 0x00) ||
		    (k == 0x08 /* BLANK_CHECK */ &&
		     c == 0x64 /* illegal mode for this track */ &&
		     q == 0x00)) {
			return 1;
		}
	}
	return 0;
}

static void set_offset(p, offset)
	myringbuff *p;
	int offset;
{
#ifdef DEBUG_SHM
  fprintf(stderr, "Write offset %d at %p\n", offset, &p->offset);
#endif
  p->offset = offset;
}


static int get_offset(p)
myringbuff *p;
{
#ifdef DEBUG_SHM
  fprintf(stderr, "Read offset %d from %p\n", p->offset, &p->offset);
#endif
  return p->offset;
}


static void usage( )
{
  fprintf( stderr,
"cdda2wav [-c chans] [-s] [-m] [-b bits] [-r rate] [-a divider] [-S speed] [-x]\n");
  fprintf( stderr,
"         [-t track[+endtrack]] [-i index] [-o offset] [-d duration] [-F] [-G]\n");
  fprintf( stderr,
"         [-q] [-w] [-v] [-R] [-P overlap] [-B] [-T] [-C input-endianess]\n");
  fprintf( stderr,
"	  [-e] [-n sectors] [-N] [-J] [-H] [-g] [-l buffers] [-D cd-device]\n");
  fprintf( stderr,
"	  [-I interface] [-K sound-device] [-O audiotype] [-E output-endianess]\n");
  fprintf( stderr,
"         [-A auxdevice] [audiofiles ...]\n");
  fprintf( stderr,
"Version %s\n", VERSION);
  fprintf( stderr,
"cdda2wav copies parts from audio cd's directly to wav or au files.\n");
  fprintf( stderr,
"It requires a supported cdrom drive to work.\n");
  fprintf( stderr,
"options: -D cd-device: set the cdrom or scsi device (as Bus,Id,Lun).\n");
  fprintf( stderr,
"         -A auxdevice: set the aux device (typically /dev/cdrom).\n");
  fprintf( stderr,
"         -K sound-device: set the sound device to use for -e (typically /dev/dsp).\n");
  fprintf( stderr,
"         -I interface: specify the interface for cdrom access.\n");
  fprintf( stderr,
"                     : (generic_scsi or cooked_ioctl).\n");
  fprintf( stderr,
"         -c channels : set 1 for mono, 2 or s for stereo (s: channels swapped).\n");
  fprintf( stderr,
"         -s          : set to stereo recording.\n");
  fprintf( stderr,
"         -m          : set to mono recording.\n");
  fprintf( stderr,
"         -x          : set to maximum quality (stereo/16-bit/44.1 KHz).\n");
  fprintf( stderr,
"         -b bits     : set bits per sample per channel (8, 12 or 16 bits).\n");
  fprintf( stderr,
"         -r rate     : set rate in samples per second. -R gives all rates\n");
  fprintf( stderr,
"         -a divider  : set rate to 44100Hz / divider. -R gives all rates\n");
  fprintf( stderr,
"         -R          : dump a table with all available sample rates\n");
  fprintf( stderr,
"         -S speed    : set the cdrom drive to a given speed during reading\n");
  fprintf( stderr,
"         -P sectors  : set amount of overlap sampling (default is 0)\n");
#ifdef NOTYET
  fprintf( stderr,
"         -X          : switch off extra alignment paranoia; implies -Y -Z\n");
  fprintf( stderr,
"         -Y          : switch off scratch detection; implies -Z\n");
  fprintf( stderr,
"         -Z          : switch off scratch reconstruction\n");
  fprintf( stderr,
"         -y threshold: tune scratch detection filter; low values catch\n");
  fprintf( stderr,
"                     : more scratches but may also catch legitimate values\n");
  fprintf( stderr,
"         -z          : disable 'smart' scratch auto-detect and force \n");
  fprintf( stderr,
"                     : paranoia to treat all data as possibly scratched\n\n");
#endif
  fprintf( stderr,
"         -n sectors  : read 'sectors' sectors per request.\n");
  fprintf( stderr,
"         -l buffers  : use a ring buffer with 'buffers' elements.\n");
  fprintf( stderr,
"         -t track[+end track]\n");
  fprintf( stderr,
"                     : select start track (and optionally end track).\n");
  fprintf( stderr,
"         -i index    : select start index.\n");
  fprintf( stderr,
"         -o offset   : start at 'offset' sectors behind start track/index.\n");
  fprintf( stderr,
"                       one sector equivalents 1/75 second.\n");
  fprintf( stderr,
"         -O audiotype: set wav or au (aka sun) or cdr (aka raw) or aiff or aifc audio format. Default is %s.\n", AUDIOTYPE);
  fprintf( stderr,
"         -C endianess: set little or big input sample endianess.\n");
  fprintf( stderr,
"         -E endianess: set little or big output sample endianess.\n");
  fprintf( stderr,
"         -d duration : set recording time in seconds or 0 for whole track.\n");
  fprintf( stderr,
"         -w          : wait for audio signal, then start recording.\n");
  fprintf( stderr,
"         -F          : find extrem amplitudes in samples.\n");
  fprintf( stderr,
"         -G          : find if input samples are mono.\n");
  fprintf( stderr,
"         -T          : undo pre-emphasis in input samples.\n");
#ifdef	ECHO_TO_SOUNDCARD
  fprintf( stderr,
"         -e          : echo audio data to sound device SOUND_DEV.\n");
#endif
  fprintf( stderr,
"         -v level    : print informations on current cd (level: 0-255).\n");
  fprintf( stderr,
"         -N          : no file operation.\n");
  fprintf( stderr,
"         -J          : give disc information only.\n");
  fprintf( stderr,
"         -H          : no info file generation.\n");
  fprintf( stderr,
"         -g          : generate special output suitable for gui frontends.\n");
  fprintf( stderr,
"         -Q          : do not print status of erreneous scsi-commands.\n");
#ifdef MD5_SIGNATURES
  fprintf( stderr,
"         -M count    : calculate MD-5 checksum for 'count' bytes.\n");
#endif
  fprintf( stderr,
"         -q          : quiet operation, no screen output.\n");
  fprintf( stderr,
"         -p percent  : play (echo) audio at a new pitch rate.\n");
  fprintf( stderr,
"         -V          : increase verbosity for SCSI commands.\n");
  fprintf( stderr,
"         -h          : this help screen.\n"  );
  fprintf( stderr,
"         -B          : record each track into a seperate file.\n");
  fprintf( stderr,
"defaults: %s, %d bit, %d.%02d Hz, track 1, no offset, one track,\n",
	  CHANNELS-1?"stereo":"mono", BITS_P_S,
	 44100 / UNDERSAMPLING,
	 (4410000 / UNDERSAMPLING) % 100);
  fprintf( stderr,
"          type %s '%s', don't wait for signal, not quiet, not verbose,\n",
          AUDIOTYPE, FILENAME);
  fprintf( stderr,
"          use %s, device %s, aux %s",
	  DEF_INTERFACE, CD_DEVICE, AUX_DEVICE);
#ifdef ECHO_TO_SOUNDCARD
  fprintf( stderr,
", sound device %s\n",
	  SOUND_DEV);
#else
  fprintf( stderr, "\n");
#endif
  fprintf( stderr,
"parameters: (optional) a file name or - for standard output.\n");
  exit( 1 );
}

static void init_globals()
{
  strncpy(global.dev_name, CD_DEVICE, sizeof(global.dev_name));	/* device name */
  strncpy(global.aux_name, AUX_DEVICE, sizeof(global.aux_name));/* auxiliary cdrom device */
  strncpy(global.fname_base, FILENAME, sizeof(global.fname_base));/* auxiliary cdrom device */
  global.have_forked = 0;	/* state variable for clean up */
  global.parent_died = 0;	/* state variable for clean up */
  global.audio    = -1;		/* audio file desc */
  global.cooked_fd  = -1;	/* cdrom file desc */
  global.no_file  =  0;		/* flag no_file */
  global.no_infofile  =  0;	/* flag no_infofile */
  global.no_cddbfile  =  0;	/* flag no_cddbfile */
  global.quiet	  =  0;		/* flag quiet */
  global.verbose  =  3 + 32 + 64;	/* verbose level */
  global.scsi_silent = 0;
  global.scsi_verbose = 0;		/* SCSI verbose level */
  global.multiname = 0;		/* multiple file names given */
  global.sh_bits  =  0;		/* sh_bits: sample bit shift */
  global.Remainder=  0;		/* remainder */
  global.iloop    =  0;		/* todo counter */
  global.SkippedSamples =  0;	/* skipped samples */
  global.OutSampleSize  =  0;	/* output sample size */
  global.channels = CHANNELS;	/* output sound channels */
  global.nSamplesDoneInTrack = 0; /* written samples in current track */
  global.buffers = 4;           /* buffers to use */
  global.nsectors = NSECTORS;   /* sectors to read in one request */
  global.overlap = 1;           /* amount of overlapping sectors */
  global.useroverlap = -1;      /* amount of overlapping sectors user override */
  global.need_hostorder = 0;	/* processing needs samples in host endianess */
  global.in_lendian = -1;	/* input endianess from SetupSCSI() */
  global.outputendianess = NONE; /* user specified output endianess */
  global.findminmax  =  0;	/* flag find extrem amplitudes */
#ifdef HAVE_LIMITS_H
  global.maxamp[0] = INT_MIN;	/* maximum amplitude */
  global.maxamp[1] = INT_MIN;	/* maximum amplitude */
  global.minamp[0] = INT_MAX;	/* minimum amplitude */
  global.minamp[1] = INT_MAX;	/* minimum amplitude */
#else
  global.maxamp[0] = -32768;	/* maximum amplitude */
  global.maxamp[1] = -32768;	/* maximum amplitude */
  global.minamp[0] = 32767;	/* minimum amplitude */
  global.minamp[1] = 32767;	/* minimum amplitude */
#endif
  global.speed = DEFAULT_SPEED; /* use default */ 
  global.userspeed = -1;        /* speed user override */
  global.findmono  =  0;	/* flag find if samples are mono */
  global.ismono  =  1;		/* flag if samples are mono */
  global.swapchannels  =  0;	/* flag if channels shall be swapped */
  global.deemphasize  =  0;	/* flag undo pre-emphasis in samples */
  global.playback_rate = 100;   /* new fancy selectable sound output rate */
  global.gui  =  0;		/* flag plain formatting for guis */
  global.cddb_id = 0;           /* disc identifying id for CDDB database */
  global.illleadout_cd = 0;     /* flag if illegal leadout is present */
  global.reads_illleadout = 0;  /* flag if cdrom drive reads cds with illegal leadouts */
  global.disctitle = NULL;
  global.creator = NULL;
  global.copyright_message = NULL;
  memset(global.tracktitle, 0, sizeof(global.tracktitle));
  memset(global.trackindexlist, 0, sizeof(global.trackindexlist));
}

#if !defined (HAVE_STRCASECMP) || (HAVE_STRCASECMP != 1)
#include <ctype.h>
static int strcasecmp __PR(( const char *s1, const char *s2 ));
static int strcasecmp(s1, s2)
	const char *s1;
	const char *s2;
{
  if (s1 && s2) {
    while (*s1 && *s2 && (tolower(*s1) - tolower(*s2) == 0)) {
      s1++;
      s2++;
    }
    if (*s1 == '\0' && *s2 == '\0') return 0;
    if (*s1 == '\0') return -1;
    if (*s2 == '\0') return +1;
    return tolower(*s1) - tolower(*s2);
  }
  return -1;
}
#endif

static int is_fifo(filename)
	char * filename;
{
#if	defined S_ISFIFO
  struct stat statstruct;

  if (stat(filename, &statstruct)) {
    /* maybe the output file does not exist. */
    if (errno == ENOENT)
	return 0;
    else comerr("Error during stat for output file\n");
  } else {
    if (S_ISFIFO(statstruct.st_mode)) {
	return 1;
    }
  }
  return 0;
#else
  return 0;
#endif
}


#if !defined (HAVE_STRTOUL) || (HAVE_STRTOUL != 1)
static unsigned int strtoul __PR(( const char *s1, char **s2, int base ));
static unsigned int strtoul(s1, s2, base)
        const char *s1;
        char **s2;
	int base;
{
	long retval;

	if (base == 10) {
		/* strip zeros in front */
		while (*s1 == '0')
			s1++;
	}
	if (s2 != NULL) {
		*s2 = astol(s1, &retval);
	} else {
		(void) astol(s1, &retval);
	}

	return (unsigned long) retval; 	
}
#endif

static unsigned long SectorBurst;
#if (SENTINEL > CD_FRAMESIZE_RAW)
error block size for overlap check has to be < sector size
#endif


static void
switch_to_realtime_priority __PR((void));

#ifdef  HAVE_SYS_PRIOCNTL_H

#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
static void
switch_to_realtime_priority()
{
        pcinfo_t        info;
        pcparms_t       param;
        rtinfo_t        rtinfo;
        rtparms_t       rtparam;
	int		pid;

	pid = getpid();

        /* get info */
        strcpy(info.pc_clname, "RT");
        if (-1 == priocntl(P_PID, pid, PC_GETCID, (void *)&info)) {
                errmsg("Cannot get priority class id priocntl(PC_GETCID)\n");
		goto prio_done;
	}

        movebytes(info.pc_clinfo, &rtinfo, sizeof(rtinfo_t));

        /* set priority not to the max */
        rtparam.rt_pri = rtinfo.rt_maxpri - 2;
        rtparam.rt_tqsecs = 0;
        rtparam.rt_tqnsecs = RT_TQDEF;
        param.pc_cid = info.pc_cid;
        movebytes(&rtparam, param.pc_clparms, sizeof(rtparms_t));
	needroot(0);
        if (-1 == priocntl(P_PID, pid, PC_SETPARMS, (void *)&param))
                errmsg("Cannot set priority class parameters priocntl(PC_SETPARMS)\n");
prio_done:
	dontneedroot();
}
#else
#if      defined _POSIX_PRIORITY_SCHEDULING
#include <sched.h>

static void
switch_to_realtime_priority()
{
#ifdef  _SC_PRIORITY_SCHEDULING
	if (sysconf(_SC_PRIORITY_SCHEDULING) == -1) {
		errmsg("WARNING: RR-scheduler not available, disabling.\n");
	} else
#endif
	{
	int sched_fifo_min, sched_fifo_max;
	struct sched_param sched_parms;

	sched_fifo_min = sched_get_priority_min(SCHED_FIFO);
	sched_fifo_max = sched_get_priority_max(SCHED_FIFO);
	sched_parms.sched_priority = sched_fifo_max - 1;
	needroot(0);
	if (-1 == sched_setscheduler(getpid(), SCHED_FIFO, &sched_parms)
		&& global.quiet != 1)
		errmsg("cannot set posix realtime scheduling policy\n");
		dontneedroot();
	}
}
#else
#if defined(__CYGWIN32__)

/*
 * NOTE: Base.h has a second typedef for BOOL.
 *	 We define BOOL to make all local code use BOOL
 *	 from Windows.h and use the hidden __SBOOL for
 *	 our global interfaces.
 */
#define	BOOL	WBOOL		/* This is the Win BOOL		*/
#define	format	__format
#include <Windows32/Base.h>
#include <Windows32/Defines.h>
#include <Windows32/Structures.h>
#include <Windows32/Functions.h>
#undef	format

static void
switch_to_realtime_priority()
{
   /* set priority class */
   if (FALSE == SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
     fprintf(stderr, "No realtime priority possible.\n");
     return;
   }

   /* set thread priority */
   if (FALSE == SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
     fprintf(stderr, "Could not set realtime priority.\n");
   }
}
#else
static void
switch_to_realtime_priority()
{
}
#endif
#endif
#endif

/* wrapper for signal handler exit needed for Mac-OS-X */
static void exit_wrapper __PR((int status));

static void exit_wrapper(status)
	int status;
{
	exit(status);
}

/* signal handler for process communication */
static void set_nonforked __PR((int status));

static void set_nonforked(status)
	int status;
{

	PRETEND_TO_USE(status);

	global.parent_died = 1;
#if defined DEBUG_CLEANUP
fprintf( stderr, "SIGPIPE received from %s\n.", child_pid == 0 ? "Child" : "Parent");
#endif
	if (child_pid == 0)
		kill(getppid(), SIGINT);
	else
		kill(child_pid, SIGINT);
	exit(9);
}



static long lSector;
static long lSector_p2;
static double rate = 44100 / UNDERSAMPLING;
static int bits = BITS_P_S;
static char fname[200];
static char audio_type[10];
static long BeginAtSample;
static unsigned long SamplesToWrite; 
static unsigned minover;
static unsigned maxover;

static unsigned long calc_SectorBurst __PR((void));
static unsigned long calc_SectorBurst()
{
	unsigned long SectorBurstVal;

	SectorBurstVal = min(global.nsectors,
		  (global.iloop + CD_FRAMESAMPLES-1) / CD_FRAMESAMPLES);
	if ( lSector+(int)SectorBurst-1 >= lSector_p2 )
		SectorBurstVal = lSector_p2 - lSector;
	return SectorBurstVal;
}

/* if PERCENTAGE_PER_TRACK is defined, the percentage message will reach
 * 100% every time a track end is reached or the time limit is reached.
 *
 * Otherwise if PERCENTAGE_PER_TRACK is not defined, the percentage message
 * will reach 100% once at the very end of the last track.
 */
#define	PERCENTAGE_PER_TRACK

static int do_read __PR((myringbuff *p, unsigned *total_unsuccessful_retries));
static int do_read (p, total_unsuccessful_retries)
	myringbuff	*p;
	unsigned	*total_unsuccessful_retries;
{
      unsigned char *newbuf;
      int offset;
      unsigned int retry_count;
      unsigned int added_size;
      static unsigned oper = 200;

      /* how many sectors should be read */
      SectorBurst =  calc_SectorBurst();

#define MAX_READRETRY 12

      retry_count = 0;
      do {
	SCSI *scgp = get_scsi_p();
	int retval;
#ifdef DEBUG_READS
	fprintf(stderr, "reading from %lu to %lu, overlap %u\n", lSector, lSector + SectorBurst -1, global.overlap);
#endif

#ifdef DEBUG_BUFFER_ADDRESSES
  fprintf(stderr, "%p %l\n", p->data, global.pagesize);
  if (((unsigned)p->data) & (global.pagesize -1) != 0) {
    fprintf(stderr, "Address %p is NOT page aligned!!\n", p->data);
  }
#endif

	if (global.reads_illleadout != 0) scgp->silent++;
        retval = ReadCdRom( scgp, p->data, lSector, SectorBurst );
	if (global.reads_illleadout != 0) {
		scgp->silent--;
		if (retval != SectorBurst) {
			if (RealEnd(scgp)) {
				/* THE END IS NEAR */
				int singles = 0;

				scgp->silent++;
				while (1 == 
				  ReadCdRom( scgp, p->data+singles*CD_FRAMESAMPLES, 
					lSector+singles, 1 )) {
					 singles++;
				}
				scgp->silent--;
				g_toc[cdtracks].dwStartSector = lSector+singles;
				SectorBurst = singles;
if (DEBUG_ILLLEADOUT)
 fprintf(stderr, "iloop=%11lu, nSamplesToDo=%11lu, end=%lu -->\n",
	global.iloop, *nSamplesToDo, lSector+singles);
				*nSamplesToDo -= global.iloop - SectorBurst*CD_FRAMESAMPLES;
				global.iloop = SectorBurst*CD_FRAMESAMPLES;
if (DEBUG_ILLLEADOUT)
 fprintf(stderr, "iloop=%11lu, nSamplesToDo=%11lu\n\n",
	global.iloop, *nSamplesToDo);
				*eorecording = 1;
			} else {
				scsiprinterr(scgp);
			}
		}
	}
        if (NULL ==
                 (newbuf = synchronize( p->data, SectorBurst*CD_FRAMESAMPLES,
                			*nSamplesToDo-global.iloop ))) {
	    /* could not synchronize!
	     * Try to invalidate the cdrom cache.
	     * Increase overlap setting, if possible.
	     */	
      	    /*trash_cache(p->data, lSector, SectorBurst);*/
	    if (global.overlap < global.nsectors - 1) {
	        global.overlap++;
		lSector--;
 		SectorBurst =  calc_SectorBurst();
#ifdef DEBUG_DYN_OVERLAP
		fprintf(stderr, "using increased overlap of %u\n", global.overlap);
#endif
	    } else {
		lSector += global.overlap - 1;
		global.overlap = 1;
		SectorBurst =  calc_SectorBurst();
	    }
	} else
	    break;
      } while (++retry_count < MAX_READRETRY);

      if (retry_count == MAX_READRETRY && newbuf == NULL && global.verbose != 0) {
        (*total_unsuccessful_retries)++;
      }

      if (newbuf) {
        offset = newbuf - ((unsigned char *)p->data);
      } else {
        offset = global.overlap * CD_FRAMESIZE_RAW;
      }
      set_offset(p,offset);

      /* how much has been added? */
      added_size = SectorBurst * CD_FRAMESAMPLES - offset/4;

      if (newbuf && *nSamplesToDo != global.iloop) {
	minover = min(global.overlap, minover);
	maxover = max(global.overlap, maxover);


	/* should we reduce the overlap setting ? */
	if (offset > CD_FRAMESIZE_RAW && global.overlap > 1) {
#ifdef DEBUG_DYN_OVERLAP
          fprintf(stderr, "decreasing overlap from %u to %u (jitter %d)\n", global.overlap, global.overlap-1, offset - (global.overlap)*CD_FRAMESIZE_RAW);
#endif
	  global.overlap--;
	  SectorBurst =  calc_SectorBurst();
	}
      }
      if (global.iloop >= added_size) {
        global.iloop -= added_size;
      } else {
        global.iloop = 0;
      }
      if (global.verbose) {
	unsigned per;

#ifdef	PERCENTAGE_PER_TRACK
        /* Thomas Niederreiter wants percentage per track */
	unsigned start_in_track = max(BeginAtSample,
                  g_toc[current_track-1].dwStartSector*CD_FRAMESAMPLES);

	per = min(BeginAtSample+*nSamplesToDo,
		  g_toc[current_track].dwStartSector*CD_FRAMESAMPLES)
		- start_in_track;

	per = (BeginAtSample+*nSamplesToDo-global.iloop
		- start_in_track
	      )/(per/100);

#else
	per = global.iloop ? (*nSamplesToDo-global.iloop)/(nSamplesToDo/100) : 100;
#endif

	if (global.overlap > 0) {
       	  fprintf(stderr, "\r%2d/%2d/%2d/%7d %3d%%",
       	  	minover, maxover, global.overlap,
       	  	newbuf ? offset - global.overlap*CD_FRAMESIZE_RAW : 9999999,
		per);
		  fflush(stderr);
	} else if (oper != per) {
       	  fprintf(stderr, "\r%3d%%", per);
		  fflush(stderr);
	  oper = per;
	}
      }
      lSector += SectorBurst - global.overlap;

#if	defined	PERCENTAGE_PER_TRACK && defined HAVE_FORK_AND_SHAREDMEM
      while (lSector >= (int)g_toc[current_track].dwStartSector) {
	current_track++;
      }
#endif

      return offset;
}


static unsigned long do_write __PR((myringbuff *p));
static unsigned long do_write (p)
	myringbuff	*p;
{
      int current_offset;
      unsigned int InSamples;
      current_offset = get_offset(p);

      /* how many bytes are available? */
      InSamples = global.nsectors*CD_FRAMESAMPLES - current_offset/4;
      /* how many samples are wanted? */
      InSamples = min((*nSamplesToDo-nSamplesDone),InSamples);

      /* when track end is reached, close current file and start a new one */
      while ((nSamplesDone < *nSamplesToDo) && (InSamples != 0)) {
	long unsigned int how_much = InSamples;

	long int left_in_track;
	left_in_track  = (int)g_toc[current_track].dwStartSector*CD_FRAMESAMPLES
			 - (int)(BeginAtSample+nSamplesDone);

	if (*eorecording != 0 && current_track == cdtracks &&
            (*total_segments_read) == (*total_segments_written)+1)
		left_in_track = InSamples;

if (left_in_track < 0) {
	fprintf(stderr, "internal error: negative left_in_track:%ld\n",left_in_track);
}

	if (bulk)
	  how_much = min(how_much, (unsigned long) left_in_track);


#ifdef MD5_SIGNATURES
	if (global.md5count) {
	  MD5Update (&global.context, ((unsigned char *)p->data) +current_offset, min(global.md5count,how_much));
	  global.md5count -= min(global.md5count,how_much);
	}
#endif
	if ( SaveBuffer ( p->data + current_offset/4,
			 how_much,
			 &nSamplesDone) ) {
	  if (global.have_forked == 1) {
	    /* kill parent */
	    kill(getppid(), SIGINT);
	  }
	  exit(20);
	}
        global.nSamplesDoneInTrack += how_much;
	SamplesToWrite -= how_much;

	/* move residual samples upto buffer start */
	if (how_much < InSamples) 
		movebytes(
		  (char *)(p->data) + current_offset + how_much*4,
		  (char *)(p->data) + current_offset,
		  (InSamples - how_much) * 4);

	if ((unsigned long) left_in_track <= InSamples) {

	  if (bulk) {
	    /* finish sample file for this track */
	    CloseAudio(global.fname_base, current_track, bulk, global.channels,
	  	  global.nSamplesDoneInTrack, global.audio_out);
          } else if (SamplesToWrite == 0) {
	    /* finish sample file for this track */
	    CloseAudio(global.fname_base, track, bulk, global.channels,
	  	  (unsigned int) *nSamplesToDo, global.audio_out);
	  }

	  if (global.verbose) {
	    if (global.tracktitle[current_track -1] != NULL) {
	      fprintf( stderr, "\b\b\b\b100%%  track %2u '%s' successfully recorded", 
			current_track, global.tracktitle[current_track-1]);
            } else {
	      fprintf( stderr, "\b\b\b\b100%%  track %2u successfully recorded", current_track);
            }
	    if (waitforsignal == 1)
		fprintf(stderr, ". %d silent samples omitted", global.SkippedSamples);
	    fputs("\n", stderr);
	    if (global.reads_illleadout && *eorecording == 1) {
		fprintf(stderr, "Real lead out at: %ld sectors\n", 
			(*nSamplesToDo+BeginAtSample)/CD_FRAMESAMPLES);
	    }
          }

          global.nSamplesDoneInTrack = 0;
	  if ( bulk && SamplesToWrite > 0 ) {
	    if ( !global.no_file ) {
	      char *tmp_fname;

	      /* build next filename */
	      tmp_fname = get_next_name();
	      if (tmp_fname != NULL) {
		      strncpy(global.fname_base , tmp_fname, sizeof global.fname_base);
		      global.fname_base[sizeof(global.fname_base)-1]=0;
	      }

	      tmp_fname = cut_extension(global.fname_base);
	      tmp_fname[0] = '\0';

	      if (global.multiname == 0) {
		sprintf(fname, "%s_%02u.%s",global.fname_base,current_track+1,
			audio_type);
	      } else {
		sprintf(fname, "%s.%s",global.fname_base, audio_type);
	      }
	      OpenAudio( fname, rate, bits, global.channels,
			(g_toc[current_track].dwStartSector -
			 g_toc[current_track-1].dwStartSector)*CD_FRAMESIZE_RAW,
			global.audio_out);
	    } /* global.nofile */
	  } /* if ( bulk && SamplesToWrite > 0 ) */
	  current_track++;

	} /* left_in_track <= InSamples */
	InSamples -= how_much;

      }  /* end while */
      return nSamplesDone;
}

#define PRINT_OVERLAP_INIT \
   if (global.verbose) { \
	if (global.overlap > 0) \
		fprintf(stderr, "overlap:min/max/cur, jitter, percent_done:\n??/??/??/???????   0%%"); \
	else \
		fputs("percent_done:\n  0%", stderr); \
   }

#if defined HAVE_FORK_AND_SHAREDMEM
static void forked_read __PR((void));

/* This function does all audio cdrom reads
 * until there is nothing more to do
 */
static void
forked_read()
{
   unsigned total_unsuccessful_retries = 0;

#if !defined(HAVE_SEMGET) || !defined(USE_SEMAPHORES)
   init_child();
#endif

   minover = global.nsectors;

   PRINT_OVERLAP_INIT
   while (global.iloop) { 

      do_read(get_next_buffer(), &total_unsuccessful_retries);

      define_buffer();

   } /* while (global.iloop) */

   if (total_unsuccessful_retries) {
      fprintf(stderr,"%u unsuccessful matches while reading\n",total_unsuccessful_retries);
   }
}

static void forked_write __PR((void));

static void
forked_write()
{

    /* don't need these anymore.  Good security policy says we get rid
       of them ASAP */
    neverneedroot();
    neverneedgroup();

#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
#else
    init_parent();
#endif

    while (1) {
	if (nSamplesDone >= *nSamplesToDo) break;
	if (*eorecording == 1 && (*total_segments_read) == (*total_segments_written)) break;

	/* get oldest buffers */
      
	nSamplesDone = do_write(get_oldest_buffer());

        drop_buffer();

    } /* end while */

}
#else

/* This function implements the read and write calls in one loop (in case
 * there is no fork/thread_create system call).
 * This means reads and writes have to wait for each other to complete.
 */
static void nonforked_loop __PR((void));

static void
nonforked_loop()
{
    unsigned total_unsuccessful_retries = 0;

    minover = global.nsectors;

    PRINT_OVERLAP_INIT
    while (global.iloop) { 

      do_read(get_next_buffer(), &total_unsuccessful_retries);

      do_write(get_oldest_buffer());

    }

    if (total_unsuccessful_retries) {
      fprintf(stderr,"%u unsuccessful matches while reading\n",total_unsuccessful_retries);
    }

}
#endif

/* and finally: the MAIN program */
int main( argc, argv )
	int argc;
	char *argv [];
{
  long lSector_p1;
  long sector_offset = 0;
  unsigned long endtrack = 1;
  double rectime = DURATION;
  int cd_index = -1;
  double int_part;
  int littleendian = -1;
  int just_the_toc = 0;
  char int_name[100];
#ifdef  ECHO_TO_SOUNDCARD
  static char user_sound_device[200] = "";
#endif /* ECHO_TO_SOUNDCARD */
  int c;
#ifdef USE_GETOPT_LONG
  int long_option_index=0;
#endif /* USE_GETOPT_LONG */
  int am_i_cdda2wav;
  char * env_p;
  int tracks_included;

  strcpy(int_name, DEF_INTERFACE);
  strcpy(audio_type, AUDIOTYPE);
  save_args(argc, argv);

  /* init global variables */
  init_globals();

  /* When being invoked as list_audio_tracks, just dump a list of
     audio tracks. */
  am_i_cdda2wav = !(strlen(argv[0]) >= sizeof("list_audio_tracks")-1
	&& !strcmp(argv[0]+strlen(argv[0])+1-sizeof("list_audio_tracks"),"list_audio_tracks"));
  if (!am_i_cdda2wav) global.verbose = SHOW_JUSTAUDIOTRACKS;

  /* Control those set-id privileges... */
  initsecurity();

  env_p = getenv("CDDA_DEVICE");
  if (env_p != NULL) {
    strncpy( global.dev_name, env_p, sizeof(global.dev_name) );
    global.dev_name[sizeof(global.dev_name)-1]=0;
  }

  /* command options parsing */
#ifdef USE_GETOPT_LONG
  while ( (c = getopt_long (argc,argv,optstring,options,&long_option_index)) 
		!= EOF)
#else
  while ( (c = getopt(argc, argv, optstring)) != EOF )
#endif /* USE_GETOPT_LONG */
	{
    switch (c) {
      case 'D':    /* override device */
        strncpy( global.dev_name, optarg, sizeof(global.dev_name) );
        global.dev_name[sizeof(global.dev_name)-1]=0;
	break;
      case 'A':    /* override device */
        strncpy( global.aux_name, optarg, sizeof(global.aux_name) );
        global.aux_name[sizeof(global.aux_name)-1]=0;
	break;
      case 'I':    /* override interface */
	strncpy( int_name, optarg, sizeof(int_name) );
        int_name[sizeof(int_name)-1]=0;
	break;
#ifdef  ECHO_TO_SOUNDCARD
      case 'K':    /* override sound device */
	strncpy( user_sound_device, optarg, sizeof(user_sound_device) );
        user_sound_device[sizeof(user_sound_device)-1]=0;
	break;
#endif /* ECHO_TO_SOUNDCARD */

      /* the following options are relevant to 'cdda2wav' only */
#ifdef MD5_SIGNATURES
      case 'M':
        if (!am_i_cdda2wav) break;
fprintf(stderr, "MD5 signatures are currently broken! Sorry\n");
	/*global.md5blocksize = strtoul( optarg, NULL, 10 ); */
        break;
#endif
      case 'O':    /* override audio type */
        if (!am_i_cdda2wav) break;
	strncpy( audio_type, optarg, 4);
        audio_type[sizeof(audio_type)-1]=0;
	break;
      case 'C':    /* override input endianess */
        if (!am_i_cdda2wav) break;
	if (strcasecmp(optarg, "little") == 0) {
	   littleendian = 1;
	} else 
	if (strcasecmp(optarg, "big") == 0) {
	   littleendian = 0;
	} else 
	if (strcasecmp(optarg, "guess") == 0) {
	   littleendian = -2;
	} else {
	   usage2("wrong parameter '%s' for option -C", optarg);
	}
	break;
      case 'E':    /* override output endianess */
        if (!am_i_cdda2wav) break;
	if (strcasecmp(optarg, "little") == 0) {
	   global.outputendianess = LITTLE;
	} else 
	if (strcasecmp(optarg, "big") == 0) {
	   global.outputendianess = BIG;
	} else {
	   usage2("wrong parameter '%s' for option -E", optarg);
	}
	break;
      case 'c':    /* override channels */
        if (!am_i_cdda2wav) break;
	if (optarg[0] == 's') {
		global.channels = 2;
		global.swapchannels = 1;
	} else {
		global.channels = strtol( optarg, NULL, 10);
	}
	break;
      case 'S':    /* override drive speed */
        if (!am_i_cdda2wav) break;
	global.userspeed = strtoul( optarg, NULL, 10);
	break;
      case 'l':    /* install a ring buffer with 'buffers' elements */
        if (!am_i_cdda2wav) break;
        global.buffers = strtoul( optarg, NULL, 10);
        break;
      case 'b':    /* override bits */
        if (!am_i_cdda2wav) break;
	bits = strtol( optarg, NULL, 10);
	break;
      case 'r':    /* override rate */
        if (!am_i_cdda2wav) break;
	rate = strtol( optarg, NULL, 10);
	break;
      case 'a':    /* override rate */
        if (!am_i_cdda2wav) break;
	if (strtod( optarg, NULL ) != 0.0)
	    rate = 44100.0 / strtod( optarg, NULL );
	else {
	    fprintf(stderr, "-a requires a nonzero, positive divider.\n");
	    exit ( 1 );
	}
	break;
      case 't':    /* override start track */
        if (!am_i_cdda2wav) break;
	{
	char * endptr;
	char * endptr2;
	track = strtoul( optarg, &endptr, 10 );
	endtrack = strtoul( endptr, &endptr2, 10 );
	if (endptr2 == endptr)
		endtrack = track;
	else if (track == endtrack) bulk = -1;
	break;
	}
      case 'i':    /* override start index */
        if (!am_i_cdda2wav) break;
	cd_index = strtoul( optarg, NULL, 10);
	break;
      case 'd':    /* override recording time */
        if (!am_i_cdda2wav) break;
	/* we accept multiple formats now */
	{ char *end_ptr = NULL;
	  rectime = strtod( optarg, &end_ptr );
	  if (*end_ptr == '\0') {
	  } else if (*end_ptr == 'f') {
	    rectime = rectime / 75.0;
	/* TODO: add an absolute end of recording. */
#if	0
	  } else if (*end_ptr == 'F') {
	    rectime = rectime / 75.0;
#endif
	  } else
	    rectime = -1.0;
	}
	break;
      case 'o':    /* override offset */
        if (!am_i_cdda2wav) break;
	sector_offset = strtol( optarg, NULL, 10);
	break;
      case 'n':    /* read sectors per request */
        if (!am_i_cdda2wav) break;
	global.nsectors = strtoul( optarg, NULL, 10);
	break;

       /*-------------- RS 98 -------------*/
      case 'p':    /* specify playback pitch rate */
        global.playback_rate = strtol( optarg, NULL, 10);
        RestrictPlaybackRate( global.playback_rate );
        global.need_hostorder = 1;
        break;
      case 'P':    /* set initial overlap sectors */
        if (!am_i_cdda2wav) break;
	global.useroverlap = strtol( optarg, NULL, 10);
	break;

      case 's':    /* stereo */
        if (!am_i_cdda2wav) break;
	global.channels = 2;
	break;
      case 'm':    /* mono */
        if (!am_i_cdda2wav) break;
	global.channels = 1;
        global.need_hostorder = 1;
	break;
      case 'x':    /* max */
        if (!am_i_cdda2wav) break;
	global.channels = 2; bits = 16; rate = 44100;
	break;
      case 'w':    /* wait for some audio intensity */
        if (!am_i_cdda2wav) break;
	waitforsignal = 1;
	break;
      case 'F':    /* find extreme amplitudes */
        if (!am_i_cdda2wav) break;
	global.findminmax = 1;
        global.need_hostorder = 1;
	break;
      case 'G':    /* find if mono */
        if (!am_i_cdda2wav) break;
	global.findmono = 1;
	break;
      case 'e':	   /* echo to soundcard */
        if (!am_i_cdda2wav) break;
#ifdef	ECHO_TO_SOUNDCARD
	global.echo = 1;
        global.need_hostorder = 1;
#else
	fprintf(stderr, "There is no sound support compiled into %s.\n",argv[0]);
#endif
	break;	
      case 'v':    /* tell us more */
        if (!am_i_cdda2wav) break;
	global.verbose = strtol( optarg, NULL, 10);
	break;
      case 'q':    /* be quiet */
        if (!am_i_cdda2wav) break;
	global.quiet = 1;
	global.verbose = 0;
	break;
      case 'N':    /* don't write to file */
        if (!am_i_cdda2wav) break;
	global.no_file = 1;
	global.no_infofile = 1;
	global.no_cddbfile = 1;
	break;
      case 'J':    /* information only */
        if (!am_i_cdda2wav) break;
	global.verbose = SHOW_MAX;
	just_the_toc = 1;
	bulk = 1;
	break;
      case 'g':	   /* screen output formatted for guis */
        if (!am_i_cdda2wav) break;
#ifdef	Thomas_will_es
	global.no_file = 1;
	global.no_infofile = 1;
	global.verbose = SHOW_MAX;
#endif
	global.no_cddbfile = 1;
	global.gui = 1;
	break;
      case 'Q':   /* silent scsi mode */
	global.scsi_silent = 1;
	break;
      case 'H':    /* don't write extra files */
        if (!am_i_cdda2wav) break;
	global.no_infofile = 1;
	global.no_cddbfile = 1;
	break;
      case 'B':    /* bulk transfer */
        if (!am_i_cdda2wav) break;
	if (bulk != -1) bulk = 1;
	break;
      case 'T':    /* do deemphasis on the samples */
        if (!am_i_cdda2wav) break;
	global.deemphasize = 1;
        global.need_hostorder = 1;
	break;
      case 'R':    /* list available rates */
        if (!am_i_cdda2wav) break;
	{ int ii;
	  fprintf(stderr, "Cdda2wav version %s: available rates are:\nRate   Divider      Rate   Divider      Rate   Divider      Rate   Divider\n", VERSION );
	  for (ii = 1; ii <= 44100 / 880 / 2; ii++) {
	    long i2 = ii;
	    fprintf(stderr, "%-7g  %2ld         %-7g  %2ld.5       ",
                    44100.0/i2,i2,44100/(i2+0.5),i2);
	    i2 += 25;
	    fprintf(stderr, "%-7g  %2ld         %-7g  %2ld.5\n",
                    44100.0/i2,i2,44100/(i2+0.5),i2);
	    i2 -= 25;
	  }
	}
	exit(0);
	break;
      case 'V':
        if (!am_i_cdda2wav) break;
	global.scsi_verbose++;	/* XXX nach open_scsi() scgp->verbose = .. !!!! */
	break;
      case 'h':
	usage();
	break;
      default:
#ifdef USE_GETOPT_LONG
	fputs ("use cdda2wav --help to get more information.\n", stderr);
#else
	fputs ("use cdda2wav -h to get more information.\n", stderr);
#endif
	exit (1);
    }
  }

  /* check all parameters */
  if (global.buffers < 1) {
    usage2("Incorrect buffer setting: %d", global.buffers);
  }

  if (global.nsectors < 1) {
    usage2("Incorrect nsectors setting: %d", global.nsectors);
  }

  if (global.verbose < 0 || global.verbose > SHOW_MAX) {
    usage2("Incorrect verbose level setting: %d",global.verbose);
  }
  if (global.verbose == 0) global.quiet = 1;

  if ( rectime < 0.0 ) {
    usage2("Incorrect recording time setting: %d.%02d",
			(int)rectime, (int)(rectime*100+0.5) % 100);
  }

  if ( global.channels != 1 && global.channels != 2 ) {
    usage2("Incorrect channel setting: %d",global.channels);
  }

  if ( bits != 8 && bits != 12 && bits != 16 ) {
    usage2("Incorrect bits_per_sample setting: %d",bits);
  }

  if ( rate < 827.0 || rate > 44100.0 ) {
    usage2("Incorrect sample rate setting: %d.%02d",
	(int)rate, ((int)rate*100) % 100);
  }

  int_part = (double)(long) (2*44100.0 / rate);
  
  if (2*44100.0 / rate - int_part >= 0.5 ) {
      int_part += 1.0;
      fprintf( stderr, "Nearest available sample rate is %d.%02d Hertz\n",
	      2*44100 / (int)int_part,
	      (2*4410000 / (int)int_part) % 100);
  }
  Halved = ((int) int_part) & 1;
  rate = 2*44100.0 / int_part;
  undersampling = (int) int_part / 2.0;
  samples_to_do = undersampling;

  if (!strcmp((char *)int_name,"generic_scsi"))
      interface = GENERIC_SCSI;
  else if (!strcmp((char *)int_name,"cooked_ioctl"))
      interface = COOKED_IOCTL;
  else  {
    usage2("Incorrect interface setting: %s",int_name);
  }

  /* check * init audio file */
  if (!strncmp(audio_type,"wav",3)) {
    global.audio_out = &wavsound;
  } else if (!strncmp(audio_type, "sun", 3) || !strncmp(audio_type, "au", 2)) {
    /* Enhanced compatibility */
    strcpy(audio_type, "au");
    global.audio_out = &sunsound;
  } else if (!strncmp(audio_type, "cdr", 3) || 
             !strncmp(audio_type, "raw", 3)) {
    global.audio_out = &rawsound;
  } else if (!strncmp(audio_type, "aiff", 4)) {
    global.audio_out = &aiffsound;
  } else if (!strncmp(audio_type, "aifc", 4)) {
    global.audio_out = &aifcsound;
#ifdef USE_LAME
  } else if (!strncmp(audio_type, "mp3", 3)) {
    global.audio_out = &mp3sound;
    if (!global.quiet) {
	unsigned char Lame_version[20];

	fetch_lame_version(Lame_version);
	fprintf(stderr, "Using LAME version %s.\n", Lame_version);
    }
    if (bits < 9) {
	bits = 16;
	fprintf(stderr, "Warning: sample size forced to 16 bit for MP3 format.\n");
    }
#endif /* USE_LAME */
  } else {
    usage2("Incorrect audio type setting: %3s", audio_type);
  }

  if (bulk == -1) bulk = 0;

  global.need_big_endian = global.audio_out->need_big_endian;
  if (global.outputendianess != NONE)
    global.need_big_endian = global.outputendianess == BIG;

  if (global.no_file) global.fname_base[0] = '\0';

  if (!bulk) {
    strcat(global.fname_base, ".");
    strcat(global.fname_base, audio_type);
  }

  /* If we need to calculate with samples or write them to a soundcard,
   * we need a conversion to host byte order.
   */ 
  if (global.channels != 2 
      || bits != 16
      || rate != 44100)
	global.need_hostorder = 1;

  /*
   * all options processed.
   * Now a file name per track may follow 
   */
  argc2 = argc3 = argc - optind;
  argv2 = argv + optind;
  if ( optind < argc ) {
    if (!strcmp(argv[optind],"-")) {
#if     defined(__CYGWIN32__) || defined(__EMX__)
      setmode(fileno(stdout), O_BINARY);
#endif
      global.audio = dup (fileno(stdout));
      strncpy( global.fname_base, "standard output", sizeof(global.fname_base) );
      global.fname_base[sizeof(global.fname_base)-1]=0;
    } else if (is_fifo(argv[optind])) {
    } else {
	/* we do have at least one argument */
	global.multiname = 1;
    }
  }

#define SETSIGHAND(PROC, SIG, SIGNAME) if (signal(SIG, PROC) == SIG_ERR) \
	{ fprintf(stderr, "cannot set signal %s handler\n", SIGNAME); exit(1); }
    SETSIGHAND(exit_wrapper, SIGINT, "SIGINT")
    SETSIGHAND(exit_wrapper, SIGQUIT, "SIGQUIT")
    SETSIGHAND(exit_wrapper, SIGTERM, "SIGTERM")
    SETSIGHAND(exit_wrapper, SIGHUP, "SIGHUP")

    SETSIGHAND(set_nonforked, SIGPIPE, "SIGPIPE")

  /* setup interface and open cdrom device */
  /* request sychronization facilities and shared memory */
  SetupInterface( );

  /* use global.useroverlap to set our overlap */
  if (global.useroverlap != -1)
	global.overlap = global.useroverlap;

  /* check for more valid option combinations */

  if (global.nsectors < 1+global.overlap) {
	fprintf( stderr, "Warning: Setting #nsectors to minimum of %d, due to jitter correction!\n", global.overlap+1);
	  global.nsectors = global.overlap+1;
  }

  if (global.overlap > 0 && global.buffers < 2) {
	fprintf( stderr, "Warning: Setting #buffers to minimum of 2, due to jitter correction!\n");
	  global.buffers = 2;
  }

    /* Value of 'nsectors' must be defined here */

    global.shmsize = HEADER_SIZE + ENTRY_SIZE_PAGE_AL * global.buffers;

#if	defined (HAVE_FORK_AND_SHAREDMEM)
#if	defined(HAVE_SMMAP) || defined(HAVE_USGSHM) || defined(HAVE_DOSALLOCSHAREDMEM)
    he_fill_buffer = request_shm_sem(global.shmsize, (unsigned char **)&he_fill_buffer);
    if (he_fill_buffer == NULL) {
#else /* have shared memory */
    if (1) {
#endif
	fprintf( stderr, "no shared memory available!\n");
	exit(2);
    }
#else /* do not have fork() and shared memory */
    he_fill_buffer = malloc(global.shmsize);
    if (he_fill_buffer == NULL) {
	fprintf( stderr, "no buffer memory available!\n");
	exit(2);
    }
#endif

    if (global.verbose != 0)
   	 fprintf(stderr,
   		 "%u bytes buffer memory requested, %d buffers, %d sectors\n",
   		 global.shmsize, global.buffers, global.nsectors);

    /* initialize pointers into shared memory segment */
    last_buffer = he_fill_buffer + 1;
    total_segments_read = (unsigned long *) (last_buffer + 1);
    total_segments_written = total_segments_read + 1;
    child_waits = (int *) (total_segments_written + 1);
    parent_waits = child_waits + 1;
    in_lendian = parent_waits + 1;
    eorecording = in_lendian + 1;
    nSamplesToDo = (unsigned long *)(eorecording + 1);
    *eorecording = 0;
    *in_lendian = global.in_lendian;

    set_total_buffers(global.buffers, sem_id);


#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
  atexit ( free_sem );
#endif

  /*
   * set input endian default
   */
  if (littleendian != -1)
    *in_lendian = littleendian;

  /* get table of contents */
  cdtracks = ReadToc( get_scsi_p(), g_toc );
#if	defined SIM_ILLLEADOUT
  g_toc[cdtracks].dwStartSector = 20*75;
#endif
  if (cdtracks == 0) {
    fprintf(stderr, "No track in table of contents! Aborting...\n");
    exit(10);
  }
  if (ReadTocText != NULL) ReadTocText(get_scsi_p());

  handle_cdtext();

  calc_cddb_id();
  calc_cdindex_id();
  if ( global.verbose == SHOW_JUSTAUDIOTRACKS ) {
    unsigned int z;

    for (z = 0; z < cdtracks; z++)
	if ((g_toc[z].bFlags & CDROM_DATA_TRACK) == 0)
	  printf("%02d\t%06u\n", g_toc[z].bTrack, g_toc[z].dwStartSector);
    exit(0);
  }

  if ( global.verbose != 0 ) {
    fputs( "#Cdda2wav version ", stderr );
    fputs( VERSION, stderr );
#if defined _POSIX_PRIORITY_SCHEDULING || defined HAVE_SYS_PRIOCNTL_H
    fputs( " real time sched.", stderr );
#endif
#if defined ECHO_TO_SOUNDCARD
    fputs( " soundcard support", stderr );
#endif
    fputs( "\n", stderr );
  }

  FixupTOC(cdtracks + 1);

  /* try to get some extra kicks */
  needroot(0);
#if defined HAVE_SETPRIORITY
  setpriority(PRIO_PROCESS, 0, -20);
#else
# if defined(HAVE_NICE) && (HAVE_NICE == 1)
  nice(-20);
# endif
#endif
  dontneedroot();

  /* switch cdrom to audio mode */
  EnableCdda (get_scsi_p(), 1);

  atexit ( CloseAll );

  DisplayToc ();

  if ( !FirstAudioTrack () )
    FatalError ( "This disk has no audio tracks\n" );

  Read_MCN_ISRC();

  /* check if start track is in range */
  if ( track < 1 || track > cdtracks ) {
    usage2("Incorrect start track setting: %d",track);
  }

  /* check if end track is in range */
  if ( endtrack < track || endtrack > cdtracks ) {
    usage2("Incorrect end track setting: %ld",endtrack);
  }

  do {
    lSector = GetStartSector ( track );
    lSector_p1 = GetEndSector ( track ) + 1;

    if ( lSector < 0 ) {
      if ( bulk == 0 ) {
        FatalError ( "Track %d not found\n", track );
      } else {
        fprintf(stderr, "Skipping data track %d...\n", track);
	if (endtrack == track) endtrack++;
        track++;
      }
    }
  } while (bulk != 0 && track <= cdtracks && lSector < 0);

  if ((global.illleadout_cd == 0 || global.reads_illleadout != 0) && cd_index != -1) {
    if (global.verbose && !global.quiet) {
      global.verbose |= SHOW_INDICES;
    }
    sector_offset += ScanIndices( track, cd_index, bulk );
  } else {
    cd_index = 1;
    if (global.deemphasize || (global.verbose & SHOW_INDICES)) {
      ScanIndices( track, cd_index, bulk );
    }
  }

  lSector += sector_offset;
  /* check against end sector of track */
  if ( lSector >= lSector_p1 ) {
    fprintf(stderr, "W Sector offset %lu exceeds track size (ignored)\n", sector_offset );
    lSector -= sector_offset;
  }

  if ( lSector < 0L ) {
    fputs( "Negative start sector! Set to zero.\n", stderr );
    lSector = 0L;
  }

  lSector_p2 = GetLastSectorOnCd( track );
  if (bulk == 1 && track == endtrack && rectime == 0.0)
     rectime = 99999.0;
  if ( rectime == 0.0 ) {
    /* set time to track time */
    *nSamplesToDo = (lSector_p1 - lSector) * CD_FRAMESAMPLES;
    rectime = (lSector_p1 - lSector) / 75.0;
    if (CheckTrackrange( track, endtrack) == 1) {
      lSector_p2 = GetEndSector ( endtrack ) + 1;

      if (lSector_p2 >= 0) {
        rectime = (lSector_p2 - lSector) / 75.0;
        *nSamplesToDo = (long)(rectime*44100.0 + 0.5);
      } else {
        fputs( "End track is no valid audio track (ignored)\n", stderr );
      }
    } else {
      fputs( "Track range does not consist of audio tracks only (ignored)\n", stderr );
    }
  } else {
    /* Prepare the maximum recording duration.
     * It is defined as the biggest amount of
     * adjacent audio sectors beginning with the
     * specified track/index/offset. */

    if ( rectime > (lSector_p2 - lSector) / 75.0 ) {
      rectime = (lSector_p2 - lSector) / 75.0;
      lSector_p1 = lSector_p2;
    }

    /* calculate # of samples to read */
    *nSamplesToDo = (long)(rectime*44100.0 + 0.5);
  }

  global.OutSampleSize = (1+bits/12);
  if (*nSamplesToDo/undersampling == 0L) {
      usage2("Time interval is too short. Choose a duration greater than %d.%02d secs!", 
	       undersampling/44100, (int)(undersampling/44100) % 100);
  }
  if ( optind < argc ) {
    if (!strcmp(argv[optind],"-") || is_fifo(argv[optind])) {
      /*
       * pipe mode
       */
      if (bulk == 1) {
        fprintf(stderr, "W Bulk mode is disabled while outputting to a %spipe\n",
		is_fifo(argv[optind]) ? "named " : "");
        bulk = 0;
      }
      global.no_cddbfile = 1;
    }
  }
  if (global.no_infofile == 0) {
    global.no_infofile = 1;
    if (global.channels == 1 || bits != 16 || rate != 44100) {
      fprintf(stderr, "W Sample conversions disable generation of info files!\n");
    } else if (waitforsignal == 1) {
      fprintf(stderr, "W Option -w 'wait for signal' disables generation of info files!\n");
    } else if (sector_offset != 0) {
      fprintf(stderr, "W Using an start offset (option -o) disables generation of info files!\n");
    } else if (!bulk &&
      !(lSector == GetStartSector(track) &&
        (lSector + rectime*75.0) == GetEndSector(track) + 1)) {
      fprintf(stderr, "W Duration is not set for complete tracks (option -d), this disables generation\n  of info files!\n");
    } else {
      global.no_infofile = 0;
    }
  }

  SamplesToWrite = *nSamplesToDo*2/(int)int_part;

  tracks_included = GetTrack(
	      (unsigned) (lSector + *nSamplesToDo/CD_FRAMESAMPLES -1))
				     - max(track,FirstAudioTrack()) + 1;

  if (global.multiname != 0 && optind + tracks_included > argc) {
	global.multiname = 0;
  }

  if ( !waitforsignal ) {

#ifdef INFOFILES
      if (!global.no_infofile) {
	int i;

	for (i = track; i < track + tracks_included; i++) {
		unsigned minsec, maxsec;
	        char *tmp_fname;

	        /* build next filename */

	        tmp_fname = get_next_name();
	        if (tmp_fname != NULL)
		  strncpy( global.fname_base, tmp_fname, sizeof(global.fname_base)-8 );
 		global.fname_base[sizeof(global.fname_base)-1]=0;
		minsec = max(lSector, GetStartSector(i));
		maxsec = min(lSector + rectime*75.0, 1+GetEndSector(i));
		if (minsec == GetStartSector(i) &&
		    maxsec == 1+GetEndSector(i)) {
			write_info_file(global.fname_base,i,(maxsec-minsec)*CD_FRAMESAMPLES, bulk && global.multiname == 0);
		} else {
			fprintf(stderr,
				 "Partial length copy for track %d, no info file will be generated for this track!\n", i);
		}
		if (!bulk) break;
	}
	reset_name_iterator();
      }
#endif

  }

  if (just_the_toc) exit(0);

#ifdef  ECHO_TO_SOUNDCARD
  if (user_sound_device[0] != '\0') {
      set_snd_device(user_sound_device);
  }
  init_soundcard(rate, bits);
#endif /* ECHO_TO_SOUNDCARD */

  if (global.userspeed > -1)
     global.speed = global.userspeed;

  if (global.speed != 0 && SelectSpeed != NULL) {
     SelectSpeed(get_scsi_p(), global.speed);
  }

  current_track = track;

  if ( !global.no_file ) {
    {
      char *myfname;

      myfname = get_next_name();

      if (myfname != NULL) {
        strncpy( global.fname_base, myfname, sizeof(global.fname_base)-8 );
        global.fname_base[sizeof(global.fname_base)-1]=0;
      }
    }

    /* strip audio_type extension */
    {
      char *cp = global.fname_base;

      cp = strrchr(cp, '.');
      if (cp == NULL) {
	cp = global.fname_base + strlen(global.fname_base);
      }
      *cp = '\0';
    }
    if (bulk && global.multiname == 0) {
      sprintf(fname, "%s_%02u.%s",global.fname_base,current_track,audio_type);
    } else {
      sprintf(fname, "%s.%s",global.fname_base,audio_type);
    }

    OpenAudio( fname, rate, bits, global.channels, 
	      (unsigned)(SamplesToWrite*global.OutSampleSize*global.channels),
		global.audio_out);
  }

  global.Remainder = (75 % global.nsectors)+1;

  global.sh_bits = 16 - bits;		/* shift counter */

  global.iloop = *nSamplesToDo;
  if (Halved && (global.iloop&1))
      global.iloop += 2;

  BeginAtSample = lSector * CD_FRAMESAMPLES;

  if ( 1 ) {
      if ( (global.verbose & SHOW_SUMMARY) && !just_the_toc && (global.reads_illleadout == 0 || lSector+*nSamplesToDo/CD_FRAMESAMPLES <= g_toc[cdtracks-1].dwStartSector)) {
	fprintf(stderr, "samplefile size will be %lu bytes.\n",
           global.audio_out->GetHdrSize() +
	   global.audio_out->InSizeToOutSize(SamplesToWrite*global.OutSampleSize*global.channels)  ); 
        fprintf (stderr, "recording %d.%05d seconds %s with %d bits @ %5d.%01d Hz"
	      ,(int)rectime , (int)(rectime * 10000) % 10000,
	      global.channels == 1 ? "mono":"stereo", bits, (int)rate, (int)(rate*10)%10);
        if (!global.no_file && *global.fname_base)
		fprintf(stderr, " ->'%s'...", global.fname_base );
        fputs("\n", stderr);
      }
  }
#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
#else
  init_pipes();
#endif

#if defined(HAVE_FORK_AND_SHAREDMEM)

  /* Everything is set up. Now fork and let one process read cdda sectors
     and let the other one store them in a wav file */

  /* forking */
  child_pid = fork();
  if (child_pid > 0 && global.gui > 0 && global.verbose > 0) fprintf( stderr, "child pid is %d\n", child_pid);

  /*********************** fork **************************************/
  if (child_pid == 0) {
    /* child WRITER section */

#ifdef	__EMX__
    if (DosGetSharedMem(he_fill_buffer, 3)) {
      comerr("DosGetSharedMem() failed.\n");
    }
#endif
    global.have_forked = 1;
    forked_write();
#ifdef	__EMX__
    DosFreeMem(he_fill_buffer);
    _exit(0);
#endif
    exit(0);
  } else if (child_pid > 0) {
    /* parent READER section */
    
    global.have_forked = 1;
    switch_to_realtime_priority();

    forked_read();
#ifdef	__EMX__
    DosFreeMem(he_fill_buffer);
#endif
    exit(0);
  } else
#else
  /* version without fork */
  {
    global.have_forked = 0;
    switch_to_realtime_priority();

    fprintf(stderr, "a nonforking version is running...\n");
    nonforked_loop();
    exit(0);
  }
#endif

  return 0;
}
