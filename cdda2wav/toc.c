/* @(#)toc.c	1.3 00/01/11 Copyright 1998,1999 Heiko Eissfeldt */
#ifndef lint
static char     sccsid[] =
"@(#)toc.c	1.3 00/01/11 Copyright 1998,1999 Heiko Eissfeldt";

#endif
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
 * CDDA2WAV (C) Heiko Eissfeldt heiko@colossus.escape.de
 * CDDB routines (C) Ti Kan and Steve Scherf
 */
#include "config.h"
#include <stdio.h>
#include <standard.h>
#include <stdlib.h>
#include <strdefs.h>
#if defined (HAVE_UNISTD_H) && (HAVE_UNISTD_H == 1)
#include <sys/types.h>
#include <unistd.h>		/* sleep */
#endif
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define CD_TEXT
#define CD_EXTRA
#undef DEBUG_XTRA
#undef DEBUG_CDTEXT

#include <scg/scsitransp.h>

#include "mytype.h"
#include "byteorder.h"
#include "interface.h"
#include "cdda2wav.h"
#include "global.h"
#include "sha.h"
#include "base64.h"
#include "toc.h"
#include "ringbuff.h"

int have_CD_text;
int have_CD_extra;

static void UpdateTrackData	__PR((int p_num));
static void UpdateIndexData	__PR((int p_num));
static void UpdateTimeData	__PR((int p_min, int p_sec, int p_frm));
static unsigned int is_cd_extra	__PR((unsigned int no_tracks));
static unsigned int get_end_of_last_audio_track	__PR((unsigned int mult_off, unsigned int no_tracks));
static int cddb_sum		__PR((int n));
static void emit_cddb_form	__PR((char *fname_baseval));
static void emit_cdindex_form	__PR((char *fname_baseval));
static void dump_extra_info	__PR((unsigned int from));
static int GetIndexOfSector	__PR((unsigned int sec, unsigned track));
static int linear_search	__PR((int searchInd, unsigned int Start, unsigned int End, unsigned track));
static int binary_search	__PR((int searchInd, unsigned int Start, unsigned int End, unsigned track));

static unsigned char g_track=0xff, g_index=0xff;	/* current track, index */
static unsigned char g_minute=0xff, g_seconds=0xff;	/* curr. minute, second */
static unsigned char g_frame=0xff;			/* current frame */


/* print the track currently read */
static void UpdateTrackData (p_num)
	int	p_num;
{
  if (global.quiet == 0) { 
    fprintf (stderr, "\ntrack: %.2d, ", p_num); fflush(stderr);
  }
  g_track = (unsigned char) p_num;
}


/* print the index currently read */
static void UpdateIndexData (p_num)
	int p_num;
{
  if (global.quiet == 0) { 
    fprintf (stderr, "index: %.2d\n", p_num); fflush(stderr);
  }
  g_index = (unsigned char) p_num;
}


/* print the time of track currently read */
static void UpdateTimeData (p_min, p_sec, p_frm)
	int p_min;
	int p_sec;
	int p_frm;
{
  if (global.quiet == 0) {
    fprintf (stderr, "time: %.2d:%.2d.%.2d\r", p_min, p_sec, p_frm); 
    fflush(stderr);
  }
  g_minute = (unsigned char) p_min;
  g_seconds = (unsigned char) p_sec;
  g_frame = (unsigned char) p_frm;
}

void AnalyzeQchannel ( frame )
	unsigned frame;
{
    subq_chnl *sub_ch;

    if (trackindex_disp != 0) {
	sub_ch = ReadSubQ(get_scsi_p(), GET_POSITIONDATA,0);
	/* analyze sub Q-channel data */
	if (sub_ch->track != g_track ||
	    sub_ch->index != g_index) {
	    UpdateTrackData (sub_ch->track);
	    UpdateIndexData (sub_ch->index);
	}
    }
    frame += 150;
    UpdateTimeData ((unsigned char) (frame / (60*75)), 
		    (unsigned char) ((frame % (60*75)) / 75), 
		    (unsigned char) (frame % 75));
}

unsigned cdtracks = 0;

long GetStartSector ( p_track )
	unsigned long p_track;
{
  unsigned long i;

  for (i = 0; i < cdtracks; i++) {
    if (g_toc [i].bTrack == p_track) {
      unsigned long dw = g_toc [i].dwStartSector;
      if ((g_toc [i].bFlags & CDROM_DATA_TRACK) != 0)
	return -1;
      return dw;
    }
  }

  return -1;
}


long GetEndSector ( p_track )
	unsigned long p_track;
{
  unsigned long i;

  for ( i = 1; i <= cdtracks; i++ ) {
    if ( g_toc [i-1].bTrack == p_track ) {
      unsigned long dw = g_toc [i].dwStartSector;
      return dw-1;
    }
  }

  return -1;
}

long FirstTrack ( )
{
  return g_toc[0].bTrack;
}

long FirstAudioTrack ( )
{
  unsigned long i;
  
  for ( i = 0; i < cdtracks; i++ ) {
    if ( g_toc [i].bTrack != CDROM_LEADOUT &&
	( g_toc [i].bFlags & CDROM_DATA_TRACK ) == 0 )
      return g_toc [i].bTrack;
  }
  return 0;
}

long LastTrack ( )
{
  return g_toc[cdtracks-1].bTrack;
}

long LastAudioTrack ( )
{
  unsigned i;
  long j = -1;

  for ( i = 0; i < cdtracks; i++ ) {
    if ( g_toc [i].bTrack != CDROM_LEADOUT &&
	( g_toc [i].bFlags & CDROM_DATA_TRACK ) == 0 )
      j = g_toc [i].bTrack;
  }
  return j;
}

long GetLastSectorOnCd( p_track )
	unsigned long p_track;
{
  unsigned long i;
  long LastSec = 0;

  for ( i = 1; i <= cdtracks; i++ ) {
    if ( g_toc [i-1].bTrack < p_track )
      continue;

    /* break if a nonaudio track follows */
    if ( (g_toc [i-1].bFlags & CDROM_DATA_TRACK) != 0) break;
    LastSec = GetEndSector ( g_toc [i-1].bTrack ) + 1;
  }
  return LastSec;
}

int GetTrack( sector )
	unsigned long sector;
{
  unsigned long i;
  for (i = 0; i < cdtracks; i++) {
    if (g_toc[i  ].dwStartSector <= sector &&
	g_toc[i+1].dwStartSector > sector)
      return (g_toc [i].bFlags & CDROM_DATA_TRACK) != 0 ? -1 : (int)(i+1);
  }
  return -1;
}

int CheckTrackrange( from, upto )
	unsigned long from;
	unsigned long upto;
{
  unsigned long i;

  for ( i = 1; i <= cdtracks; i++ ) {
    if ( g_toc [i-1].bTrack < from )
      continue;

    if ( g_toc [i-1].bTrack == upto )
      return 1;

    /* break if a nonaudio track follows */
    if ( (g_toc [i-1].bFlags & CDROM_DATA_TRACK) != 0)
      return 0;
  }
  /* track not found */
  return 0;
}

unsigned
find_an_off_sector __PR((unsigned lSector, unsigned SectorBurstVal));

unsigned find_an_off_sector(lSector, SectorBurstVal)
	unsigned lSector;
	unsigned SectorBurstVal;
{
	long track_of_start = GetTrack(lSector);
	long track_of_end = GetTrack(lSector + SectorBurstVal -1);
	long start = GetStartSector(track_of_start);
	long end = GetEndSector(track_of_end);

	if (lSector - start > end - lSector + SectorBurstVal -1)
		return start;
	else
		return end;
}

#ifdef CD_TEXT
#include "scsi_cmds.h"
#endif


int   handle_cdtext __PR(( void ));

int   handle_cdtext ()
{
#ifdef CD_TEXT
	if (bufferTOC[0] == 0 && bufferTOC[1] == 0) {
		have_CD_text = 0;
		return have_CD_text;
	}

	/* do a quick scan over all pack type indicators */
	{
		int i;
		int count_fails = 0;
		int len = (bufferTOC[0] << 8) | bufferTOC[1];

		len = min(len, 2048);
		for (i = 0; i < len-4; i += 18) {
			if (bufferTOC[4+i] < 0x80 || bufferTOC[4+i] > 0x8f) {
				count_fails++;
			}
		}
		have_CD_text = count_fails < 3;
	}

#else
	have_CD_text = 0;
#endif
	return have_CD_text;
}


#ifdef CD_TEXT
/**************** CD-Text special treatment **********************************/

typedef struct {
	unsigned char headerfield[4];
	unsigned char textdatafield[12];
	unsigned char crcfield[2];
} cdtextpackdata;

static unsigned short crctab[1<<8] = { /* as calculated by initcrctab() */
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0,
    };

#undef SLOWER_CRC
#define SUBSIZE	18*8

static unsigned short updcrc __PR((
	    unsigned int    p_crc,
            register unsigned char   *cp,
            register size_t  cnt));

static unsigned short updcrc(p_crc, cp, cnt)
	unsigned int p_crc;
        register unsigned char   *cp;
        register size_t  cnt;
{
      register unsigned short crc = p_crc;
      while( cnt-- ) {
#ifdef SLOWERCRC
            crc = (crc<<8) ^ crctab[(crc>>(16-8)) ^ ~(*cp++)];
#else
            crc = (crc<<8) ^ crctab[(crc>>(16-8)) ^ (*cp++)];
#endif
      }
      return( crc );
}

static unsigned short calcCRC __PR((unsigned char *buf, unsigned bsize));

static unsigned short calcCRC(buf, bsize)
	unsigned char *buf;
	unsigned bsize;
{
#ifdef SLOWERCRC
      return updcrc( 0x0, (unsigned char *)buf, bsize );
#else
      return updcrc( 0x0, (unsigned char *)buf, bsize ) ^ 0x1d0f;
#endif
}

unsigned char    fliptab[8] = {
        0x01,
        0x02,
        0x04,
        0x08,
        0x10,
        0x20,
        0x40,
        0x80,
};

static int flip_error_corr __PR((unsigned char *b, int crc));

static int flip_error_corr(b, crc)
	unsigned char *b;
	int crc;
{
  if (crc != 0) {
    int i;
    for (i = 0; i < SUBSIZE; i++) {
      char      c;

      c = fliptab[i%8];
      b[i / 8] ^= c;
      if ((crc = calcCRC(b, SUBSIZE/8)) == 0) {
        return crc;
      }
      b[i / 8] ^= c;
    }
  }
  return crc & 0xffff;
}


static int cdtext_crc_ok __PR((cdtextpackdata *c));

static int cdtext_crc_ok (c)
	cdtextpackdata *c;
{
	int crc;

	crc = calcCRC(((unsigned char *)c), 18);
	return 0 == flip_error_corr((unsigned char *)c, crc);
}

static void dump_binary __PR((cdtextpackdata *c));

static void dump_binary(c)
	cdtextpackdata *c;
{
          fprintf(stderr, ": header fields %02x %02x %02x  ",
                          c->headerfield[1], c->headerfield[2], c->headerfield[3]);
          fprintf(stderr,
"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x"
                        , c->textdatafield[0]
                        , c->textdatafield[1]
                        , c->textdatafield[2]
                        , c->textdatafield[3]
                        , c->textdatafield[4]
                        , c->textdatafield[5]
                        , c->textdatafield[6]
                        , c->textdatafield[7]
                        , c->textdatafield[8]
                        , c->textdatafield[9]
                        , c->textdatafield[10]
                        , c->textdatafield[11]
                 );
}


#define DETAILED 0

static int process_header __PR((cdtextpackdata *c, int dbcc, unsigned char *line));

static int process_header(c, dbcc, line)
	cdtextpackdata *c;
	int dbcc;
	unsigned char *line;
{
      switch ((int)c->headerfield[0]) {

        case 0x80: /* Title of album or track */
          if (DETAILED) fprintf (stderr, "Title");
	  if (c->headerfield[1] > 0 && c->headerfield[1] < 100
	    && global.tracktitle[c->headerfield[1]-1] == NULL) {
	    unsigned len;

	    len = strlen((char *)line);

            if (len > 0)
            	global.tracktitle[c->headerfield[1]-1] = (unsigned char *) malloc(len + 1);
            if (global.tracktitle[c->headerfield[1]-1] != NULL) {
               memcpy(global.tracktitle[c->headerfield[1]-1], line, len);
               global.tracktitle[c->headerfield[1]-1][len] = '\0';
            }
          } else 
	  if (c->headerfield[1] == 0
	    && global.disctitle == NULL) {
	    unsigned len;

	    len = strlen((char *)line);

            if (len > 0)
            	global.disctitle = (unsigned char *) malloc(len + 1);
            if (global.disctitle != NULL) {
               memcpy(global.disctitle, line, len);
               global.disctitle[len] = '\0';
            }
	  }
        break;
        case 0x81: /* Name(s) of the performer(s) */
          if (DETAILED) fprintf(stderr, "Performer(s)");
	  if (c->headerfield[1] > 0 && c->headerfield[1] < 100
	    && global.trackcreator[c->headerfield[1]-1] == NULL) {
	    unsigned len;

	    len = strlen((char *)line);

            if (len > 0)
		global.trackcreator[c->headerfield[1]-1] = (unsigned char *) malloc(len + 1);

            if (global.trackcreator[c->headerfield[1]-1] != NULL) {
               memcpy(global.trackcreator[c->headerfield[1]-1], line, len);
               global.trackcreator[c->headerfield[1]-1][len] = '\0';
            }
          } else 
	  if (c->headerfield[1] == 0
	    && global.creator == NULL) {
	    unsigned len;

	    len = strlen((char *)line);

            if (len > 0)
            	global.creator = (unsigned char *) malloc(len + 1);
            if (global.creator != NULL) {
               memcpy(global.creator, line, len);
               global.creator[len] = '\0';
            }
	  }
        break;
        case 0x82: /* Name(s) of the songwriter(s) */
          if (DETAILED) fprintf(stderr, "Songwriter(s)");
        break;
        case 0x83: /* Name(s) of the composer(s) */
          if (DETAILED) fprintf(stderr, "Composer(s)");
        break;
        case 0x84: /* Name(s) of the arranger(s) */
          if (DETAILED) fprintf(stderr, "Arranger(s)");
        break;
        case 0x85: /* Message from content provider and/or artist */
          if (DETAILED) fprintf(stderr, "Message");
        break;
        case 0x86: /* Disc Identification and information */
          if (DETAILED) fprintf(stderr, "Disc identification");
        break;
        case 0x87: /* Genre Identification and information */
          if (DETAILED) fprintf(stderr, "Genre identification");
        break;
        case 0x8e: /* UPC/EAN code or ISRC code */
          if (DETAILED) fprintf(stderr, "UPC or ISRC");
	  if (c->headerfield[1] > 0 && c->headerfield[1] < 100) {
	    memcpy(g_toc[c->headerfield[1]-1].ISRC, line, sizeof(g_toc[0].ISRC));
	  } else
	  if (c->headerfield[1] == 0) {
	    memcpy((char *)MCN, line, 13);
	    MCN[13] = '\0';
	  }
        break;
        case 0x88: /* Table of Content information */
          if (DETAILED) fprintf(stderr, "Table of Content identification");
	  if (DETAILED) dump_binary(c); 
        return 0;
        case 0x89: /* Second Table of Content information */
          if (DETAILED) fprintf(stderr, "Second Table of Content identification");
	  if (DETAILED) dump_binary(c); 
        return 0;
        case 0x8f: /* Size information of the block */
	  if (!(DETAILED)) break;

          switch (c->headerfield[1]) {
	    case 0:
	  fprintf(stderr, "first track is %d, last track is %d\n", 
			c->textdatafield[1],
			c->textdatafield[2]);
	  if (c->textdatafield[3] & 0x80) {
		fprintf(stderr, "Program Area CD Text information available\n");
	        if (c->textdatafield[3] & 0x40) {
		  fprintf(stderr, "Program Area copy protection available\n");
	        }
	  }
	  if (c->textdatafield[3] & 0x07) {
		fprintf(stderr, "message information is %scopyrighted\n", 
			c->textdatafield[3] & 0x04 ? "": "not ");
		fprintf(stderr, "Names of performer/songwriter/composer/arranger(s) are %scopyrighted\n", 
			c->textdatafield[3] & 0x02 ? "": "not ");
		fprintf(stderr, "album and track names are %scopyrighted\n", 
			c->textdatafield[3] & 0x01 ? "": "not ");
	  }
	  fprintf(stderr, "%d packs with album/track names\n", c->textdatafield[4]);
	  fprintf(stderr, "%d packs with performer names\n", c->textdatafield[5]);
	  fprintf(stderr, "%d packs with songwriter names\n", c->textdatafield[6]);
	  fprintf(stderr, "%d packs with composer names\n", c->textdatafield[7]);
	  fprintf(stderr, "%d packs with arranger names\n", c->textdatafield[8]);
	  fprintf(stderr, "%d packs with artist or content provider messages\n", c->textdatafield[9]);
	  fprintf(stderr, "%d packs with disc identification information\n", c->textdatafield[10]);
	  fprintf(stderr, "%d packs with genre identification/information\n", c->textdatafield[11]);
	  break;
	case 1:
	  fprintf(stderr, "%d packs with table of contents information\n", c->textdatafield[0]);
	  fprintf(stderr, "%d packs with second table of contents information\n", c->textdatafield[1]);
	  fprintf(stderr, "%d packs with reserved information\n", c->textdatafield[2]);
	  fprintf(stderr, "%d packs with reserved information\n", c->textdatafield[3]);
	  fprintf(stderr, "%d packs with reserved information\n", c->textdatafield[4]);
	  fprintf(stderr, "%d packs with closed information\n", c->textdatafield[5]);
	  fprintf(stderr, "%d packs with UPC/EAN ISRC information\n", c->textdatafield[6]);
	  fprintf(stderr, "%d packs with size information\n", c->textdatafield[7]);
	  fprintf(stderr, "last sequence numbers for blocks 1-8: %d %d %d %d "
		 ,c->textdatafield[8]
		 ,c->textdatafield[9]
		 ,c->textdatafield[10]
		 ,c->textdatafield[11]
		);
	  break;
	case 2:
	  fprintf(stderr, "%d %d %d %d\n"
		 ,c->textdatafield[0]
		 ,c->textdatafield[1]
		 ,c->textdatafield[2]
		 ,c->textdatafield[3]
		);
	  fprintf(stderr, "Language codes for blocks 1-8: %d %d %d %d %d %d %d %d\n"
		 ,c->textdatafield[4]
		 ,c->textdatafield[5]
		 ,c->textdatafield[6]
		 ,c->textdatafield[7]
		 ,c->textdatafield[8]
		 ,c->textdatafield[9]
		 ,c->textdatafield[10]
		 ,c->textdatafield[11]
		);
	  break;
        }
          if (DETAILED) fprintf(stderr, "Blocksize");
	  if (DETAILED) dump_binary(c); 
        return 0;
#if !defined DEBUG_CDTEXT
        default:
#else
      }
#endif
          if (DETAILED) fprintf(stderr, ": header fields %02x %02x %02x  ",
                          c->headerfield[1], c->headerfield[2], c->headerfield[3]);
#if !defined DEBUG_CDTEXT
      }
      if (c->headerfield[1] == 0) {
            if (DETAILED) fprintf(stderr, " for album   : ->");
      } else {
            if (DETAILED) fprintf(stderr, " for track %2u: ->", c->headerfield[1]);
      }
      if (DETAILED) fputs ((char *) line, stderr);
      if (DETAILED) fputs ("<-", stderr);

      if (dbcc != 0) {
#else
      {
#endif
          if (DETAILED) fprintf(stderr,
"  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x"
                        , c->textdatafield[0]
                        , c->textdatafield[1]
                        , c->textdatafield[2]
                        , c->textdatafield[3]
                        , c->textdatafield[4]
                        , c->textdatafield[5]
                        , c->textdatafield[6]
                        , c->textdatafield[7]
                        , c->textdatafield[8]
                        , c->textdatafield[9]
                        , c->textdatafield[10]
                        , c->textdatafield[11]
                 );
      }
      return 0;
}
#endif


#if defined CDROMMULTISESSION
static int tmp_fd;
#endif

#ifdef CD_EXTRA
/**************** CD-Extra special treatment *********************************/

static unsigned char Extra_buffer[CD_FRAMESIZE_RAW];

static int Read_CD_Extra_Info __PR(( unsigned long sector));
static int Read_CD_Extra_Info(sector)
	unsigned long sector;
{
  unsigned i;
  static int offsets[] = {
     75, 		/* this is observed for 12 cm discs */
     60 };		/* this has been observed for 5 cm disks */

  for (i = 0; i < sizeof(offsets)/sizeof(int); i++) {
#ifdef DEBUG_XTRA
    fprintf(stderr, "debug: Read_CD_Extra_Info at sector %lu\n", sector+offsets[i]);
#endif
    ReadCdRomData(get_scsi_p(), Extra_buffer, sector+offsets[i], 1);

    /* If we are unlucky the drive cannot handle XA sectors by default.
       We try to compensate by ignoring the first eight bytes.
       Of course then we lack the last 8 bytes of the sector...
     */

#ifdef DEBUG_XTRA
    { int ijk;
      for (ijk = 0; ijk < 96; ijk++) {
        fprintf(stderr, "%02x  ", Extra_buffer[ijk]);
        if ((ijk+1) % 16 == 0) fprintf(stderr, "\n");
      }
    } 
#endif

    if (Extra_buffer[0] == 0)
      movebytes(Extra_buffer +8, Extra_buffer, CD_FRAMESIZE - 8);

    /* check for cd extra */
    if (Extra_buffer[0] == 'C' && Extra_buffer[1] == 'D')
	return sector+offsets[i];
  }
  return 0;
}

static void Read_Subinfo __PR(( unsigned pos, unsigned length));
static void Read_Subinfo(pos, length)
	unsigned pos;
	unsigned length;
{
  unsigned num_infos, num;
  unsigned char *Subp, *orgSubp;
  unsigned this_track = 0xff;
#ifdef DEBUG_XTRA
  unsigned char *up;
  unsigned char *sp;
  unsigned u;
  unsigned short s;
#endif

  length += 8;
  length = (length + CD_FRAMESIZE_RAW-1) / CD_FRAMESIZE_RAW;
  length *= CD_FRAMESIZE_RAW;
  orgSubp = Subp = (unsigned char *) malloc(length);

  if (Subp == NULL) {
    fprintf(stderr, "Read_Subinfo alloc error(%d)\n",length);
    goto errorout;
  }

  ReadCdRomData(get_scsi_p(), Subp, pos, 1);

  num_infos = Subp[45]+(Subp[44] << 8);
#ifdef DEBUG_XTRA
  fprintf(stderr, "subinfo version %c%c.%c%c, %d info packets\n",
	  Subp[8],
	  Subp[9],
	  Subp[10],
	  Subp[11],
	  num_infos);
#endif
  length -= 46;
  Subp += 46;
  for (num = 0; num < num_infos && length > 0; num++) {
    static const char *infopacketID[] = { "0", 
			      "track identifier", 
			      "album title",
			      "universal product code", 
			      "international standard book number",
			      "copyright",
			      "track title",
			      "notes",
			      "main interpret",
			      "secondary interpret",
			      "composer",
			      "original composer",
			      "creation date",
			      "release  date",
			      "publisher",
			      "0f",
			      "isrc audio track",
			      "isrc lyrics",
			      "isrc pictures",
			      "isrc MIDI data",
			      "14", "15", "16", "17", "18", "19",
			      "copyright state SUB_INFO",
			      "copyright state intro lyrics",
			      "copyright state lyrics",
			      "copyright state MIDI data",
			      "1e", "1f",
			      "intro lyrics",
			      "pointer to lyrics text file and length", 
			      "22", "23", "24", "25", "26", "27", "28",
			      "29", "2a", "2b", "2c", "2d", "2e", "2f",
			      "still picture descriptor",
			      "31",
			      "32", "33", "34", "35", "36", "37", "38",
			      "39", "3a", "3b", "3c", "3d", "3e", "3f",
			      "MIDI file descriptor",
			      "genre code",
			      "tempo",
			      "key"
			     };
    unsigned id = *Subp;
    unsigned len = *(Subp +1);

    if (id >= sizeof(infopacketID)/sizeof(const char *)) {
      fprintf(stderr, "Off=%4d, ind=%2d/%2d, unknown Id=%2u, len=%2u ",
		Subp - orgSubp, num, num_infos, id, len);
      Subp += 2 + 1;
      length -= 2 + 1;
      break;
    }

    switch (id) {
    case 1:    /* track nummer or 0 */
      this_track = 10 * (*(Subp + 2) - '0') + (*(Subp + 3) - '0');
      break;

    case 0x02: /* album title */
	if (global.disctitle == NULL) {
	    global.disctitle = (unsigned char *) malloc(len + 1);
	    if (global.disctitle != NULL) {
               memcpy(global.disctitle, Subp + 2, len);
	       global.disctitle[len] = '\0';
            }
        }
      break;
    case 0x06: /* track title */
	if (this_track > 0 && this_track < 100
	    && global.tracktitle[this_track-1] == NULL) {
            global.tracktitle[this_track-1] = (unsigned char *) malloc(len + 1);
            if (global.tracktitle[this_track-1] != NULL) {
               memcpy(global.tracktitle[this_track-1], Subp + 2, len);
               global.tracktitle[this_track-1][len] = '\0';
            }
        }
      break;
    case 0x05: /* copyright message */
	if (global.copyright_message == NULL) {
	    global.copyright_message = (unsigned char *) malloc(len + 1);
	    if (global.copyright_message != NULL) {
               memcpy(global.copyright_message, Subp + 2, len);
	       global.copyright_message[len] = '\0';
            }
        }
      break;
    case 0x08: /* creator */
	if (global.creator == NULL) {
	    global.creator = (unsigned char *) malloc(len + 1);
	    if (global.creator != NULL) {
               memcpy(global.creator, Subp + 2, len);
	       global.creator[len] = '\0';
            }
        }
      break;
#if 0
    case 0x03:
    case 0x04:
    case 0x07:
    case 0x09:
    case 0x0a:
    case 0x0b:
    case 0x0c:
    case 0x0d:
    case 0x0e:
    case 0x0f:
      fprintf(stderr, "%s: %*.*s\n",infopacketID[id], (int) len, (int) len, (Subp +2));
      break;
#ifdef DEBUG_XTRA
    case 0x1a:
    case 0x1b:
    case 0x1c:
    case 0x1d:
	fprintf(stderr, "%s %scopyrighted\n", infopacketID[id], *(Subp + 2) == 0 ? "not " : "");
      break;

    case 0x21:
      fprintf(stderr, "lyrics file beginning at sector %u",
	      (unsigned) GET_BE_UINT_FROM_CHARP(Subp + 2));
      if (len == 8)
	fprintf(stderr, ", having length: %u\n", 
                (unsigned) GET_BE_UINT_FROM_CHARP(Subp + 6));
      else
	fputs("\n", stderr);
      break;

    case 0x30:
      sp = Subp + 2;
      while (sp < Subp + 2 + len) {
      /*while (len >= 10) {*/
        s = be16_to_cpu((*(sp)) | (*(sp) << 8));
        fprintf(stderr, "%04x, ", s);
	sp += 2;
        up = sp;
	switch (s) {
	case 0:
	break;
	case 4:
	break;
	case 5:
	break;
	case 6:
	break;
        }
        u = GET_BE_UINT_FROM_CHARP(up);
        fprintf(stderr, "%04lx, ", (long) u);
        up += 4;
        u = GET_BE_UINT_FROM_CHARP(up);
        fprintf(stderr, "%04lx, ", (long) u);
        up += 4;
	sp += 8;
      }
      fputs("\n", stderr);
      break;

    case 0x40:
      fprintf(stderr, "MIDI file beginning at sector %u",
	      (unsigned) GET_BE_UINT_FROM_CHARP(Subp + 2));
      if (len == 8)
	fprintf(stderr, ", having length: %u\n", 
		(unsigned) GET_BE_UINT_FROM_CHARP(Subp + 6));
      else
	fputs("\n", stderr);
      break;

    case 0x42:
      fprintf(stderr, "%s: %d beats per minute\n",infopacketID[id], *(Subp + 2));
      break;
    case 0x41:
      if (len == 8)
        fprintf(stderr, "%s: %x, %x, %x, %x, %x, %x, %x, %x\n",
		infopacketID[id],
		*(Subp + 2),
		*(Subp + 3),
		*(Subp + 4),
		*(Subp + 5),
		*(Subp + 6),
		*(Subp + 7),
		*(Subp + 8),
		*(Subp + 9)
	);
      else
        fprintf(stderr, "%s:\n",infopacketID[id]);
      break;
    case 0x43:
      fprintf(stderr, "%s: %x\n",infopacketID[id], *(Subp + 2));
      break;
    default:
      fprintf(stderr, "%s: %*.*s\n",infopacketID[id], (int) len, (int) len, (Subp +2));
#endif
#endif
    }

    if (len & 1) len++;
    Subp += 2 + len;
    length -= 2 + len;
  }

/* cleanup */

  free(orgSubp);

  return;

errorout:
  exit(2);
}

#endif

static unsigned session_start;
/*
   A Cd-Extra is detected, if it is a multisession CD with
   only audio tracks in the first session and a data track
   in the last session.
 */
static unsigned is_cd_extra(no_tracks)
	unsigned no_tracks;
{
  unsigned mult_off;
#if defined CDROMMULTISESSION
  /*
   * FIXME: we would have to do a ioctl (CDROMMULTISESSION)
   *        for the cdrom device associated with the generic device
   *	    not just AUX_DEVICE
   */
  struct cdrom_multisession ms_str;

  if (interface == GENERIC_SCSI)
    tmp_fd = open (global.aux_name, O_RDONLY);
  else
    tmp_fd = global.cooked_fd;

  if (tmp_fd != -1) {
    int result;

    ms_str.addr_format = CDROM_LBA;
    result = ioctl(tmp_fd, CDROMMULTISESSION, &ms_str);
    if (result == -1) {
      if (global.verbose != 0)
        perror("multi session ioctl not supported: ");
    } else {
#ifdef DEBUG_XTRA
  fprintf(stderr, "current ioctl multisession_offset = %u\n", ms_str.addr.lba);
#endif
	if (interface == GENERIC_SCSI)
		close (tmp_fd);
	if (ms_str.addr.lba > 0)
	  return ms_str.addr.lba;
    }
  }
#endif
  mult_off = (no_tracks > 2 && !IS_AUDIO(no_tracks-2) && IS_AUDIO(no_tracks-3))
                      ? g_toc[no_tracks-2].dwStartSector : 0;
#ifdef DEBUG_XTRA
  fprintf(stderr, "current guessed multisession_offset = %u\n", mult_off);
#endif
  return mult_off;
}

#define SESSIONSECTORS (152*75)
/*
   The solution is to read the Table of Contents of the first
   session only (if the drive permits that) and directly use
   the start of the leadout. If this is not supported, we subtract
   a constant of SESSIONSECTORS sectors (found heuristically).
 */
static unsigned get_end_of_last_audio_track(mult_off, no_tracks)
	unsigned mult_off;
	unsigned no_tracks;
{
   unsigned retval;

   /* Try to read the first session table of contents.
      This works for Sony and mmc type drives. */
   if (ReadLastAudio && (retval = ReadLastAudio(get_scsi_p(), no_tracks)) != 0) {
     return retval;
   } else {
     return mult_off - SESSIONSECTORS;
   }
}

static void dump_cdtext_info __PR((void));

/* The Table of Contents needs to be corrected if we
   have a CD-Extra. In this case all audio tracks are
   followed by a data track (in the second session).
   Unlike for single session CDs the end of the last audio
   track cannot be set to the start of the following
   track, since the lead-out and lead-in would then
   errenously be part of the audio track. This would
   lead to read errors when trying to read into the
   lead-out area.
   So the length of the last track in case of Cd-Extra
   has to be fixed.
 */
unsigned FixupTOC(no_tracks)
	unsigned no_tracks;
{
    unsigned mult_off;
    unsigned offset = 0;

    /* get the multisession offset in sectors */
    mult_off = is_cd_extra(no_tracks);

    /* if the first track address had been the victim of an underflow,
     * set it to zero.
     */
    if (g_toc[0].dwStartSector > g_toc[no_tracks-1].dwStartSector) {
	fprintf(stderr, "Warning: first sector has negative start sector! Setting to zero.\n");
	g_toc[0].dwStartSector = 0;
    }

#ifdef DEBUG_XTRA
    fprintf(stderr, "current multisession_offset = %u\n", mult_off);
#endif
    dump_cdtext_info();
    if (mult_off > 100) { /* the offset has to have a minimum size */
      int j;
      unsigned real_end;

      /* believe the multisession offset :-) */
      /* adjust end of last audio track to be in the first session */
      real_end = get_end_of_last_audio_track(mult_off, no_tracks);
#ifdef DEBUG_XTRA
    fprintf(stderr, "current end = %u\n", real_end);
#endif
      for (j = no_tracks-2; j > 0; j--) {
	if (!IS_AUDIO(j) && IS_AUDIO(j-1)) {
	  if (g_toc[j].dwStartSector > real_end) {
	    session_start = mult_off;

#ifdef CD_EXTRA
	    offset = Read_CD_Extra_Info(g_toc[j].dwStartSector);
#endif
	have_CD_extra = g_toc[j].dwStartSector;
        dump_extra_info(offset);
#if defined CDINDEX_SUPPORT || defined CDDB_SUPPORT
{
    unsigned long	count_audio_tracks = 0;
    unsigned	i;

    for (i = 0; i < cdtracks; i++)
	if (IS_AUDIO(i)) count_audio_tracks++;
    if (count_audio_tracks > 0 && global.no_cddbfile == 0) {
#if defined CDINDEX_SUPPORT
	emit_cdindex_form(global.fname_base);
#endif
#if defined CDDB_SUPPORT
	emit_cddb_form(global.fname_base);
#endif
    }
}
#endif
            /* set start of track to beginning of lead-out */
	    g_toc[j].dwStartSector = real_end;
#if	defined CD_EXTRA && defined DEBUG_XTRA
	    fprintf(stderr, "setting end of session to %u\n", real_end);
#endif
	  }
	  break;
	}
      }
    } else {
#if defined CDINDEX_SUPPORT || defined CDDB_SUPPORT
if (have_CD_text) {
    unsigned long	count_audio_tracks = 0;
    unsigned	i;

    for (i = 0; i < cdtracks; i++)
	if (IS_AUDIO(i)) count_audio_tracks++;
    if (count_audio_tracks > 0 && global.no_cddbfile == 0) {
#if defined CDINDEX_SUPPORT
	emit_cdindex_form(global.fname_base);
#endif
#if defined CDDB_SUPPORT
	emit_cddb_form(global.fname_base);
#endif
    }
}
#endif
    }
    return offset;
}

static int cddb_sum(n)
	int n;
{
  int ret;

  for (ret = 0; n > 0; n /= 10) {
    ret += (n % 10);
  }

  return ret;
}

void calc_cddb_id()
{
  UINT4 i;
  UINT4 t = 0;
  UINT4 n = 0;

  for (i = 0; i < cdtracks; i++) {
    n += cddb_sum(g_toc[i].dwStartSector/75 + 2);
  }

  t = g_toc[i].dwStartSector/75 - g_toc[0].dwStartSector/75;

  global.cddb_id = (n % 0xff) << 24 | (t << 8) | cdtracks;
}


#undef TESTCDINDEX
#ifdef	TESTCDINDEX
void TestGenerateId()
{
   SHA_INFO       sha;
   unsigned char  digest[20], *base64;
   unsigned long  size;

   sha_init(&sha);
   sha_update(&sha, (unsigned char *)"0123456789", 10);
   sha_final(digest, &sha);

   base64 = rfc822_binary((char *)digest, 20, &size);
   if (strncmp((char*) base64, "h6zsF82dzSCnFsws9nQXtxyKcBY-", size))
   {
       free(base64);

       printf("The SHA-1 hash function failed to properly generate the\n");
       printf("test key.\n");
       exit(0);
   }
   free(base64);
}
#endif

void calc_cdindex_id()
{
	SHA_INFO 	sha;
	unsigned char	digest[20], *base64;
	unsigned long	size;
	unsigned	i;
	char		temp[9];

#ifdef	TESTCDINDEX
	TestGenerateId();
	g_toc[0].bTrack = 1;
	cdtracks = 15;
	g_toc[cdtracks-1].bTrack = 15;
	i = 0;
	g_toc[i++].dwStartSector = 0U;
	g_toc[i++].dwStartSector = 18641U;
	g_toc[i++].dwStartSector = 34667U;
	g_toc[i++].dwStartSector = 56350U;
	g_toc[i++].dwStartSector = 77006U;
	g_toc[i++].dwStartSector = 106094U;
	g_toc[i++].dwStartSector = 125729U;
	g_toc[i++].dwStartSector = 149785U;
	g_toc[i++].dwStartSector = 168885U;
	g_toc[i++].dwStartSector = 185910U;
	g_toc[i++].dwStartSector = 205829U;
	g_toc[i++].dwStartSector = 230142U;
	g_toc[i++].dwStartSector = 246659U;
	g_toc[i++].dwStartSector = 265614U;
	g_toc[i++].dwStartSector = 289479U;
	g_toc[i++].dwStartSector = 325732U;
#endif
	sha_init(&sha);
	sprintf(temp, "%02X", g_toc[0].bTrack);
	sha_update(&sha, (unsigned char *)temp, 2);
	sprintf(temp, "%02X", g_toc[cdtracks-1].bTrack);
	sha_update(&sha, (unsigned char *)temp, 2);

	/* the position of the leadout comes first. */
	sprintf(temp, "%08X", 150+g_toc[cdtracks].dwStartSector);
	sha_update(&sha, (unsigned char *)temp, 8);

	/* now 99 tracks follow with their positions. */
	for (i = 0; i < cdtracks; i++) {
		sprintf(temp, "%08X", 150+g_toc[i].dwStartSector);
		sha_update(&sha, (unsigned char *)temp, 8);
	}
	for (i++  ; i < 100; i++) {
		sha_update(&sha, (unsigned char *)"00000000", 8);
	}
	sha_final(digest, &sha);

	base64 = rfc822_binary((char *)digest, 20, &size);
	global.cdindex_id = base64;
}


#if defined CDDB_SUPPORT

static void emit_cddb_form(fname_baseval)
	char *fname_baseval;
{
  unsigned i;
  unsigned first_audio;
  FILE *cddb_form;
  char fname[200];
  char *pp;

  if (fname_baseval == NULL || fname_baseval[0] == 0)
	return;

  strncpy(fname, fname_baseval, sizeof(fname) -1);
  fname[sizeof(fname) -1] = 0;
  pp = strrchr(fname, '.');
  if (pp == NULL) {
    pp = fname + strlen(fname);
  }
  strncpy(pp, ".cddb", sizeof(fname) - 1 - (pp - fname));

  cddb_form = fopen(fname, "w");
  if (cddb_form == NULL) return;

  first_audio = FirstAudioTrack();
  fprintf( cddb_form, "# xmcd\n#\n");
  fprintf( cddb_form, "# Track frame offsets:\n#\n");
  for (i = first_audio - 1; i < cdtracks; i++) {
    if (!IS_AUDIO(i)) break;
    fprintf( cddb_form, "# %u\n", 150 + g_toc[i].dwStartSector);
  }
  fprintf( cddb_form, "#\n# Disc length: %lu seconds\n#\n", 
           (GetLastSectorOnCd(first_audio) - GetStartSector(first_audio)) / 75);
  fprintf( cddb_form, "# Revision: 0\n" );
  fprintf( cddb_form, "# Submitted via: cdda2wav ");
  fprintf( cddb_form, VERSION);
  fprintf( cddb_form, "\n" );

  fprintf( cddb_form, "DISCID=%8lx\n", global.cddb_id);
  if (global.disctitle == NULL && global.creator == NULL) {
    fprintf( cddb_form, "DTITLE=\n");
  } else {
    if (global.creator == NULL) {
      fprintf( cddb_form, "DTITLE=%s\n", global.disctitle);
    } else if (global.disctitle == NULL) {
      fprintf( cddb_form, "DTITLE=%s\n", global.creator);
    } else {
      fprintf( cddb_form, "DTITLE=%s / %s\n", global.creator, global.disctitle);
    }
  }

  for (i = first_audio - 1; i < cdtracks; i++) {
    if (!IS_AUDIO(i)) break;
    if (global.tracktitle[i] != NULL) {
      fprintf( cddb_form, "TTITLE%d=%s\n", i, global.tracktitle[i]);
    } else {
      fprintf( cddb_form, "TTITLE%d=\n", i);
    }
  }

  if (global.copyright_message == NULL) {
    fprintf( cddb_form, "EXTD=\n");
  } else {
    fprintf( cddb_form, "EXTD=Copyright %s\n", global.copyright_message);
  }

  for (i = first_audio - 1; i < cdtracks; i++) {
    if (!IS_AUDIO(i)) break;
    fprintf( cddb_form, "EXTT%d=\n", i);
  }
  fprintf( cddb_form, "PLAYORDER=\n");
  fclose( cddb_form );
}

#ifdef NOTYET
/* network access for cddb servers */
static int get_list_of_servers()
{
/*
CDDB_SITES_SERVER=cddb.cddb.com
CDDB_SITES_SERVERPORT=8880
*/
return 0;
}

static int scan_servers(cddb_id)
	unsigned cddb_id;
{
return 0;
}

int resolve_id(cddb_id)
	unsigned cddb_idr;)
{
  unsigned servers;

  servers = get_list_of_servers();

  if (servers == 0) return 0;

  return scan_servers(cddb_id);
}
#endif
#endif

#if	defined CDINDEX_SUPPORT

static int IsSingleArtist __PR((void));

/* check, if there are more than one track creators */
static int IsSingleArtist()
{
	unsigned i;

	for (i = 0; i < cdtracks; i++) {
		if (!IS_AUDIO(i)) continue;
		if (global.creator && global.trackcreator[i]
			&& strcmp((char *) global.creator,
				  (char *) global.trackcreator[i]) != 0)
			return 0;
	}
	return 1;
}

static void emit_cdindex_form(fname_baseval)
	char *fname_baseval;
{
  int i;
  FILE *cdindex_form;
  char fname[200];
  char *pp;

  if (fname_baseval == NULL || fname_baseval[0] == 0)
	return;

  strncpy(fname, fname_baseval, sizeof(fname) -1);
  fname[sizeof(fname) -1] = 0;
  pp = strrchr(fname, '.');
  if (pp == NULL) {
    pp = fname + strlen(fname);
  }
  strncpy(pp, ".cdindex", sizeof(fname) - 1 - (pp - fname));

  cdindex_form = fopen(fname, "w");
  if (cdindex_form == NULL) return;

#define	CDINDEX_URL	"http://www.cdindex.org/dtd/CDInfo.dtd"

  /* format XML page according to cdindex DTD (see www.cdindex.org) */
  fprintf( cdindex_form, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n<!DOCTYPE CDInfo SYSTEM \"%s\">\n\n<CDInfo>\n",
		  CDINDEX_URL);
  fprintf( cdindex_form, "   <Title>%s</Title>\n",
	 global.disctitle ? (char *) global.disctitle : "");
  /* 
   * In case of mixed mode and Extra-CD, nonaudio tracks are included!
   */
  fprintf( cdindex_form, "   <NumTracks>%d</NumTracks>\n\n", cdtracks);
  fprintf( cdindex_form, "   <IdInfo>\n      <DiskId>\n         <Id>%s</Id>\n      </DiskId>\n", global.cdindex_id); 

  fprintf( cdindex_form, "   </IdInfo>\n\n");

  if (IsSingleArtist()) {
    fprintf( cdindex_form, "   <SingleArtistCD>\n      <Artist>%s</Artist>\n",
	 global.creator ? (char *) global.creator : "");

    for (i = FirstTrack() - 1; i <= LastTrack() - 1; i++) {
      if (IS_AUDIO(i)) {
        fprintf( cdindex_form,
	 "      <Track Num=\"%d\">\n         <Name>%s</Name>\n      </Track>\n",
		  i+1, global.tracktitle[i] ? (char *) global.tracktitle[i] : "");
      } else {
        fprintf( cdindex_form,
	 "      <Track Num=\"%d\">\n         <Name>data track</Name>\n      </Track>\n",
		  i+1 );
      }
    }
    fprintf( cdindex_form, "   </SingleArtistCD>\n");
  } else {
    fprintf( cdindex_form, "   <MultipleArtistCD>\n");

    for (i = FirstTrack() - 1; i <= LastTrack() - 1; i++) {
      fprintf( cdindex_form, "      <Track Num=\"%d\">\n", i);

      if (IS_AUDIO(i)) {
        fprintf( cdindex_form, "         <Artist>%s</Artist>\n",
		  global.trackcreator[i] ? (char *) global.trackcreator[i] : "");
        fprintf( cdindex_form, "         <Name>%s</Name>\n      </Track>\n",
		  global.tracktitle[i] ? (char *) global.tracktitle[i] : "");
      } else {
        fprintf( cdindex_form,
 "         <Artist>data track</Artist>\n         <Name>data track</Name>\n      </Track>\n");
      }
    }
    fprintf( cdindex_form, "   </MultipleArtistCD>\n");
  }
  fprintf( cdindex_form, "</CDInfo>\n");

  fclose( cdindex_form );
}
#endif

static void dump_cdtext_info()
{
#ifdef CD_TEXT
  /* interpret the contents of CD Text information based on an early draft
     of SCSI-3 mmc version 2 from jan 2, 1998
     CD Text information consists of a text group containing up to
     8 language blocks containing up to
     255 Pack data chunks of
     18 bytes each.
     So we have at most 36720 bytes to cope with.
   */
  {
    short int datalength;
    unsigned char *p = bufferTOC;
    unsigned char lastline[255*12];
    int		lastitem = -1;
    int		itemcount = 0;
    int		inlinecount = 0;
    int		outlinecount = 0;

    lastline[0] = '\0';
    datalength = ((p[0] << 8) + p[1]) - 2;
    datalength = min(datalength, 2048-4);
    p += 4;
    for (;datalength > 0;
	datalength -= sizeof (cdtextpackdata),
	p += sizeof (cdtextpackdata)) {

      /* handle one packet of CD Text Information Descriptor Pack Data */
      /* this is raw R-W subchannel data converted to 8 bit values. */
      cdtextpackdata *c = (cdtextpackdata *)p;
      int extension_flag;
      int sequence_number;
      int dbcc;
      int block_number;
      int character_position;
      int crc_error;

#ifdef DEBUG_CDTEXT
      fprintf(stderr, "datalength =%d\n", datalength);
#endif
      crc_error = !cdtext_crc_ok(c);

      if (lastitem != c->headerfield[0]) {
	itemcount = 0;
	lastitem = c->headerfield[0];
      }
      extension_flag = ((unsigned)(c->headerfield[1] & 0x80)) >> 7;
      c->headerfield[1] &= 0x7f;
      sequence_number = c->headerfield[2];
      dbcc = ((unsigned)(c->headerfield[3] & 0x80)) >> 7; /* double byte character code */
      block_number = ((unsigned)(c->headerfield[3] & 0x30)) >> 4; /* language */
      character_position = c->headerfield[3] & 0x0f;

#if defined DEBUG_CDTEXT
      fprintf(stderr, "CDText: ext_fl=%d, trnr=%d, seq_nr=%d, dbcc=%d, block_nr=%d, char_pos=%d\n",
             extension_flag, (int)c->headerfield[1], sequence_number, dbcc, block_number, character_position);
#endif

      if (dbcc == 0) {
        /* print ASCII information */
	unsigned char *zeroposition;
        memcpy(lastline+inlinecount, c->textdatafield, 12);
	inlinecount += 12;
	zeroposition = (unsigned char *)memchr(lastline+outlinecount, '\0', inlinecount-outlinecount);
	while (zeroposition != NULL) {
	  process_header(c, dbcc, lastline+outlinecount);
	  outlinecount += zeroposition - (lastline+outlinecount) + 1;

#if defined DEBUG_CDTEXT
	  fprintf(stderr, "\tin=%d, out=%d, items=%d, trcknum=%d\n", inlinecount, outlinecount, itemcount, (int) c->headerfield[1]);
{ int q; for (q= outlinecount; q < inlinecount; q++) fprintf (stderr, "%c", lastline[q] ? lastline[q] : 'ß'); fputs("\n", stderr); }
#else
	  if (DETAILED) {
	    if (crc_error) fputs(" ! uncorr. CRC-Error", stderr);
	    fputs("\n", stderr);
	  }
#endif

	  itemcount++;
	  if (itemcount > (int)cdtracks
	      	|| (c->headerfield[0] == 0x8f
		|| (c->headerfield[0] <= 0x8d && c->headerfield[0] >= 0x86))) {
	    outlinecount = inlinecount;
	    break;
	  }
	  c->headerfield[1]++;
	  zeroposition = (unsigned char *)memchr(lastline+outlinecount, '\0', inlinecount-outlinecount);
	}
      }
    }
  }
#endif
}



static void dump_extra_info(from)
	unsigned int from;
{
#ifdef CD_EXTRA
  unsigned char *p;
  unsigned pos, length;

  p = Extra_buffer + 48;
  while (*p != '\0') {
    pos    = GET_BE_UINT_FROM_CHARP(p+2);
    length = GET_BE_UINT_FROM_CHARP(p+6);
    if (pos == (unsigned)-1) {
      pos = from+1;
    } else {
      pos += session_start;
    }

#ifdef DEBUG_XTRA
    if (global.gui == 0 && global.verbose != 0) {
	fprintf(stderr, "Language: %c%c (as defined by ISO 639)", *p, *(p+1));
	fprintf(stderr, " at sector %u, len=%u (sessionstart=%u)", pos, length, session_start);
	fputs("\n", stderr);
    }
#endif
    /* dump this entry */
    Read_Subinfo(pos, length);
    p += 10;

    if (p + 9 > (Extra_buffer + CD_FRAMESIZE))
      break;
  }
#endif
}

static char *quote __PR((unsigned char *string));

static char *quote(string)
	unsigned char * string;
{
  static char result[200];
  unsigned char *p = (unsigned char *)result;

  while (*string) {
    if (*string == '\'' || *string == '\\') {
	*p++ = '\\';
    }
    *p++ = *string++;
  }
  *p = '\0';

  return result;
}


void DisplayToc ( )
{
  unsigned i;
  unsigned long dw;
  unsigned mins;
  unsigned secnds;
  unsigned centi_secnds;	/* hundreds of a second */
  unsigned frames;
  int count_audio_trks;


  /* get total time */
  dw = (unsigned long) g_toc[cdtracks].dwStartSector + 150;
  mins	       =       dw / ( 60*75 );
  secnds       =     ( dw % ( 60*75 ) ) / 75;
  frames	   =     ( dw %      75   );
  centi_secnds = (4* frames +1 ) /3; /* convert from 1/75 to 1/100 */

  if ( global.gui == 0 && (global.verbose & SHOW_SUMMARY) != 0 ) {
    unsigned ii;

    /* summary */
    count_audio_trks = 0;
    i = 0;
    while ( i < cdtracks ) {
      int from;

      from = g_toc [i].bTrack;
      while ( i < cdtracks && g_toc [i].bFlags == g_toc [i+1].bFlags ) i++;
      if (i >= cdtracks) i--;
      
  /* g_toc [i].bFlags contains two fields:
       bits 7-4 (ADR) : 0 no sub-q-channel information
                      : 1 sub-q-channel contains current position
		      : 2 sub-q-channel contains media catalog number
		      : 3 sub-q-channel contains International Standard
		                                 Recording Code ISRC
		      : other values reserved
       bits 3-0 (Control) :
       bit 3 : when set indicates there are 4 audio channels else 2 channels
       bit 2 : when set indicates this is a data track else an audio track
       bit 1 : when set indicates digital copy is permitted else prohibited
       bit 0 : when set indicates pre-emphasis is present else not present
  */
      if (g_toc[i].bFlags & 4) {
        fputs( " DATAtrack recorded      copy-permitted tracktype\n" , stderr);
      	fprintf(stderr, "     %2d-%2d %13.13s %14.14s      data\n",from,g_toc [i].bTrack,
			g_toc [i].bFlags & 1 ? "incremental" : "uninterrupted", /* how recorded */
			g_toc [i].bFlags & 2 ? "yes" : "no" /* copy-perm */
                       );
      } else { 
        fputs( "AUDIOtrack pre-emphasis  copy-permitted tracktype channels\n" , stderr);
	fprintf(stderr, "     %2d-%2d %12.12s  %14.14s     audio    %1c\n",from,g_toc [i].bTrack,
			g_toc [i].bFlags & 1 ? "yes" : "no", /* pre-emph */
			g_toc [i].bFlags & 2 ? "yes" : "no", /* copy-perm */
			g_toc [i].bFlags & 8 ? '4' : '2'
                       );
	count_audio_trks++;
      }
      i++;
    }
    fprintf ( stderr, 
	     "Table of Contents: total tracks:%u, (total time %u:%02u.%02u)\n",
	     cdtracks, mins, secnds, frames );

    for ( i = 0, ii = 0; i < cdtracks; i++ ) {
      if ( g_toc [i].bTrack <= MAXTRK ) {
	dw = (unsigned long) (g_toc[i+1].dwStartSector - g_toc[i].dwStartSector /* + 150 - 150 */);
	mins         =       dw / ( 60*75 );
	secnds       =     ( dw % ( 60*75 )) / 75;
    frames	     =     ( dw %      75   );
    centi_secnds = (4* frames +1 ) /3; /* convert from 1/75 to 1/100 */
	if ( (g_toc [i].bFlags & CDROM_DATA_TRACK) != 0 ) {
		fprintf ( stderr, " %2u.[%2u:%02u.%02u]",
			g_toc [i].bTrack,mins,secnds,frames );
	} else {
	  if (have_CD_extra && i == cdtracks - 2) {
		fprintf ( stderr, " %2u.|%2u:%02u.%02u|",
			g_toc [i].bTrack,mins,secnds,frames );
	  } else {
		fprintf ( stderr, " %2u.(%2u:%02u.%02u)",
			g_toc [i].bTrack,mins,secnds,frames );
	  }
	}
        ii++;
      }
      if ( ii % 5 == 0 )
	fputs( "\n", stderr );
      else
	fputc ( ',', stderr );
    }
    if ( (ii+1) % 5 != 0 )
      fputs( "\n", stderr );

    if ((global.verbose & SHOW_STARTPOSITIONS) != 0) {
      fputs ("\nTable of Contents: starting sectors\n", stderr);
      for ( i = 0; i < cdtracks; i++ ) {
	if (have_CD_extra && i == cdtracks - 1) {
	  fprintf ( stderr, " %2u.(%8u)", g_toc [i].bTrack, have_CD_extra
#ifdef DEBUG_CDDB
	+150
#endif
);
	} else {
	  fprintf ( stderr, " %2u.(%8u)", g_toc [i].bTrack, g_toc[i].dwStartSector
#ifdef DEBUG_CDDB
	+150
#endif
);
	}

	if ( (i+1) % 5 == 0 )
	  fputs( "\n", stderr );
	else
	  fputc ( ',', stderr );
      }
      fprintf ( stderr, " lead-out(%8u)", g_toc[i].dwStartSector);
      fputs ("\n", stderr);
    }
    if (global.quiet == 0) {
      fprintf(stderr, "CDINDEX discid: %s\n", global.cdindex_id);
      fprintf(stderr, "CDDB discid: 0x%08lx\n", (unsigned long) global.cddb_id);
      if (have_CD_text != 0) {
	fprintf(stderr, "CD-Text: detected\n");
      } else {
	fprintf(stderr, "CD-Text: not detected\n");
      }
      if (have_CD_extra != 0) {
	fprintf(stderr, "CD-Extra: detected\n");
      } else {
	fprintf(stderr, "CD-Extra: not detected\n");
      }
    }
    if ((global.verbose & SHOW_TITLES) != 0) {
      int maxlen = 0;

      if ( global.disctitle != NULL ) {
        fprintf( stderr, "Album title: '%s'", global.disctitle);
	if ( global.creator != NULL ) {
	  fprintf( stderr, "\t[from %s]", global.creator);
	}
	fputs("\n", stderr);
      }

      for ( i = 0; i < cdtracks; i++ ) {
	if ( global.tracktitle[i] != NULL ) {
	  int len = strlen((char *)global.tracktitle[i]);
          maxlen = max(maxlen, len);
        }
      }
      maxlen = (maxlen + 12 + 8 + 7)/8;

      for ( i = 0; i < cdtracks; i++ ) {
	if ( global.tracktitle[i] != NULL ) {
	  fprintf( stderr, "Track %2u: '%s'", i+1, global.tracktitle[i]);
	  if ( global.trackcreator[i] != NULL
            && global.trackcreator[i][0] != '\0'
#if 1
	    && (global.creator == NULL
	        || 0 != strcmp((char *)global.creator,(char *)global.trackcreator[i]))
#endif
          ) {
	    int j;
	    for ( j = 0;
	          j < (maxlen - ((int)strlen((char *)global.tracktitle[i]) + 12)/8);
		  j++)
		 fprintf(stderr, "\t");
	    fprintf( stderr, "[from %s]", global.trackcreator[i]);
	  }
	  fputs("\n", stderr);
        }
      }
    }
  } else if (global.gui == 1) { /* line formatting when in gui mode */

    count_audio_trks = 0;
    i = 0;
    fprintf( stderr, "Tracks:%u %u:%02u.%02u\n", cdtracks, mins, secnds, frames );
    fprintf( stderr, "CDINDEX discid: %s\n", global.cdindex_id);
    fprintf( stderr, "CDDB discid: 0x%08lx\n", (unsigned long) global.cddb_id);

    if (have_CD_text != 0) {
	fprintf(stderr, "CD-Text: detected\n");
        dump_cdtext_info();
    } else {
	fprintf(stderr, "CD-Text: not detected\n");
    }
    if (have_CD_extra != 0) {
	fprintf(stderr, "CD-Extra: detected\n");
        dump_extra_info(have_CD_extra);
    } else {
	fprintf(stderr, "CD-Extra: not detected\n");
    }
 
    fprintf( stderr, "Album title: '%s'", (void *)global.disctitle != NULL ?
			 quote(global.disctitle) : "");
	fprintf( stderr, " from '%s'\n", (void *)global.creator != NULL ? quote(global.creator) : "");

    while ( i <= cdtracks ) {
      int from;

      from = g_toc [i].bTrack;

      if (i == cdtracks) {
      	  fprintf(stderr, "Leadout: %7u\n", g_toc[i].dwStartSector);
      } else {
      
	  if (g_toc[i].bFlags & 4) {
		unsigned int real_start = have_CD_extra ? have_CD_extra : g_toc[i].dwStartSector;

	      dw = (unsigned long) (g_toc[i+1].dwStartSector - real_start);
	      mins         =         dw / ( 60*75 );
	      secnds       =       ( dw % ( 60*75 )) / 75;
          frames	     =     ( dw %      75   );
          centi_secnds = (4* frames +1 ) /3; /* convert from 1/75 to 1/100 */
      	     fprintf(stderr, "T%02d: %7u %2u:%02u.%02u data %s %s N/A\n",from,
	        	real_start,
			mins,secnds,frames,
			g_toc [i].bFlags & 1 ? "incremental" : "uninterrupted", /* how recorded */
			g_toc [i].bFlags & 2 ? "copyallowed" : "copydenied" /* copy-perm */
                       );
          } else { 
	      dw = (unsigned long) (g_toc[i+1].dwStartSector - g_toc[i].dwStartSector /* + 150 - 150 */);
	      mins         =         dw / ( 60*75 );
	      secnds       =       ( dw % ( 60*75 )) / 75;
          frames	     =     ( dw %      75   );
          centi_secnds = (4* frames +1 ) /3; /* convert from 1/75 to 1/100 */
	         fprintf(stderr, "T%02d: %7u %2u:%02u.%02u audio %s %s %s title '%s' from '%s'\n",from,
	        g_toc[i].dwStartSector,
			mins,secnds,frames,
			g_toc [i].bFlags & 1 ? "pre-emphasized" : "linear", /* pre-emph */
			g_toc [i].bFlags & 2 ? "copyallowed" : "copydenied", /* copy-perm */
			g_toc [i].bFlags & 8 ? "quadro" : "stereo",
            (void *) global.tracktitle[i] != NULL ? quote(global.tracktitle[i]) : "", 
			(void *) global.trackcreator[i] != NULL ? quote(global.trackcreator[i]) : ""
			);
	        count_audio_trks++;
          }
      }
      i++;
    }
  }
}

void Read_MCN_ISRC()
{
  unsigned i;

  if ((global.verbose & SHOW_MCN) != 0) {
    subq_chnl *sub_ch;
    subq_catalog *subq_cat = NULL;

    if (MCN[0] == '\0') {
      /* get and display Media Catalog Number ( one per disc ) */
      fprintf(stderr, "scanning for MCN...");
    
      sub_ch = ReadSubQ(get_scsi_p(), GET_CATALOGNUMBER,0);

#define EXPLICIT_READ_MCN_ISRC 1
#if EXPLICIT_READ_MCN_ISRC == 1 /* TOSHIBA HACK */
      if (Toshiba3401() != 0 && global.quiet == 0 && 
	(sub_ch != 0 || (((subq_catalog *)sub_ch->data)->mc_valid & 0x80))) {
        /* no valid MCN yet. do more searching */
        unsigned long h = g_toc[0].dwStartSector;
    
        while (h <= g_toc[0].dwStartSector + 100) {
	  if (Toshiba3401())
	    ReadCdRom(get_scsi_p(), RB_BASE->data, h, global.nsectors);
	  sub_ch = ReadSubQ(get_scsi_p(), GET_CATALOGNUMBER,0);
	  if (sub_ch != NULL) {
	    subq_cat = (subq_catalog *) sub_ch->data;
	    if ((subq_cat->mc_valid & 0x80) != 0) {
	      break;
	    }
	  }
	  h += global.nsectors;
        }
      }
#endif

      if (sub_ch != NULL)
        subq_cat = (subq_catalog *)sub_ch->data;
  
      if (sub_ch != NULL && (subq_cat->mc_valid & 0x80) != 0 && global.quiet == 0) {

        /* unified format guesser:
         * format MCN all digits in bcd
         *     1                                  13
         * A: ab cd ef gh ij kl m0  0  0  0  0  0  0  Plextor 6x Rel. 1.02
         * B: 0a 0b 0c 0d 0e 0f 0g 0h 0i 0j 0k 0l 0m  Toshiba 3401
         * C: AS AS AS AS AS AS AS AS AS AS AS AS AS  ASCII SCSI-2 Plextor 4.5x and 6x Rel. 1.06
         */
        unsigned char *cp = subq_cat->media_catalog_number;
        if (!(cp[8] | cp[9] | cp[10] | cp[11] | cp[12]) &&
	  ((cp[0] & 0xf0) | (cp[1] & 0xf0) | (cp[2] & 0xf0) | 
	   (cp[3] & 0xf0) | (cp[4] & 0xf0) | (cp[5] & 0xf0) | 
	   (cp[6] & 0xf0))) {
	  /* reformat A: to B: */
	  cp[12] = cp[6] >> 4;
	  cp[11] = cp[5] & 0xf;
	  cp[10] = cp[5] >> 4;
	  cp[ 9] = cp[4] & 0xf;
	  cp[ 8] = cp[4] >> 4;
	  cp[ 7] = cp[3] & 0xf;
	  cp[ 6] = cp[3] >> 4;
	  cp[ 5] = cp[2] & 0xf;
	  cp[ 4] = cp[2] >> 4;
	  cp[ 3] = cp[1] & 0xf;
	  cp[ 2] = cp[1] >> 4;
	  cp[ 1] = cp[0] & 0xf;
	  cp[ 0] = cp[0] >> 4;
        }
        if (!isdigit(cp[0])) {
	  if (memcmp(subq_cat->media_catalog_number,"\0\0\0\0\0\0\0\0\0\0\0\0\0",13) != 0)
	    sprintf((char *) subq_cat->media_catalog_number, 
		  "%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X", 
		  subq_cat->media_catalog_number [0],
		  subq_cat->media_catalog_number [1],
		  subq_cat->media_catalog_number [2],
		  subq_cat->media_catalog_number [3],
		  subq_cat->media_catalog_number [4],
		  subq_cat->media_catalog_number [5],
		  subq_cat->media_catalog_number [6],
		  subq_cat->media_catalog_number [7],
		  subq_cat->media_catalog_number [8],
		  subq_cat->media_catalog_number [9],
		  subq_cat->media_catalog_number [10],
		  subq_cat->media_catalog_number [11],
		  subq_cat->media_catalog_number [12]
		);
        }
        if (memcmp(subq_cat->media_catalog_number,"0000000000000",13) != 0) {
	  memcpy((char *)MCN, (char *)subq_cat->media_catalog_number, 13);
          MCN[13] = 0;
        }
      }
    }
    if (MCN[0] != '\0') fprintf(stderr, "\rMedia catalog number: %13.13s\n", MCN);
  }

  if ((global.verbose & SHOW_ISRC) != 0) {
    subq_chnl *sub_ch;

    /* get and display Track International Standard Recording Codes 
       (for each track) */
    for ( i = 0; i < cdtracks; i++ ) {
      subq_track_isrc * subq_tr;
      unsigned j;

      if (g_toc[i].ISRC[0] == '\0') {
        fprintf(stderr, "\rscanning for ISRCs: %d ...", i+1);

        subq_tr = NULL;
        sub_ch = ReadSubQ(get_scsi_p(), GET_TRACK_ISRC,i+1);

#if EXPLICIT_READ_MCN_ISRC == 1 /* TOSHIBA HACK */
        if (Toshiba3401() != 0) {
	  j = (g_toc[i].dwStartSector/100 + 1) * 100;
	  do {
	    ReadCdRom(get_scsi_p(),  RB_BASE->data, j, global.nsectors);
	    sub_ch = ReadSubQ(get_scsi_p(), GET_TRACK_ISRC, g_toc[i].bTrack);
	    if (sub_ch != NULL) {
	      subq_tr = (subq_track_isrc *) sub_ch->data;
	      if (subq_tr != NULL && (subq_tr->tc_valid & 0x80) != 0)
	        break;
	    }
	    j += global.nsectors;
	  } while (j < (g_toc[i].dwStartSector/100 + 1) * 100 + 100);
        }
#endif
    
        if (sub_ch != NULL)
	  subq_tr = (subq_track_isrc *)sub_ch->data;

        if (sub_ch != NULL && (subq_tr->tc_valid & 0x80) && global.quiet == 0) {
	  unsigned char p_start[16];
	  unsigned char *p = p_start;
	  unsigned char *cp = subq_tr->track_isrc;

#if 0
	  int ijk;
	  for (ijk = 0; ijk < 15; ijk++) {
		fprintf(stderr, "%02x  ", cp[ijk]);
          }
	  fputs("", stderr);
#endif
	  /* unified format guesser:
	   * there are 60 bits and 15 bytes available.
	   * 5 * 6bit-items + two zero fill bits + 7 * 4bit-items
	   *
	   * A: ab cd ef gh ij kl mn o0 0  0  0  0  0  0  0  Plextor 6x Rel. 1.02
	   * B: 0a 0b 0c 0d 0e 0f 0g 0h 0i 0j 0k 0l 0m 0n 0o Toshiba 3401
	   * C: AS AS AS AS AS AS AS AS AS AS AS AS AS AS AS ASCII SCSI-2
	   * eg 'G''B''-''A''0''7''-''6''8''-''0''0''2''7''0' makes most sense
	   * D: 'G''B''A''0''7''6''8''0''0''2''7''0'0  0  0  Plextor 6x Rel. 1.06 and 4.5x R. 1.01 and 1.04
	   */
	  if (!(cp[8] | cp[9] | cp[10] | cp[11] | cp[12] | cp[13] | cp[14]) &&
	    ((cp[0] & 0xf0) | (cp[1] & 0xf0) | (cp[2] & 0xf0) | 
	     (cp[3] & 0xf0) | (cp[4] & 0xf0) | (cp[5] & 0xf0) | 
	     (cp[6] & 0xf0) | (cp[7] & 0xf0))) {
	    /* reformat A: to B: */
	    cp[14] = cp[7] >> 4;
	    cp[13] = cp[6] & 0xf;
	    cp[12] = cp[6] >> 4;
	    cp[11] = cp[5] & 0xf;
	    cp[10] = cp[5] >> 4;
	    cp[ 9] = cp[4] & 0xf;
	    cp[ 8] = cp[4] >> 4;
	    cp[ 7] = cp[3] & 0xf;
	    cp[ 6] = cp[3] >> 4;
	    cp[ 5] = cp[2] & 0xf;
	    cp[ 4] = cp[2] >> 4;
	    cp[ 3] = cp[1] & 0xf;
	    cp[ 2] = cp[1] >> 4;
	    cp[ 1] = cp[0] & 0xf;
	    cp[ 0] = cp[0] >> 4;
	  }
      
	  /* If not yet in ASCII format, do the conversion */
	  if (cp[0] != '0' && cp[1] != '0' &&
	      (!isupper(cp[0]) || !isupper(cp[1]))) {
	    /* coding table for International Standard Recording Code */
	    static char bin2ISRC[] = 
	      {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	         0,0,0,0,0,0,'@',
	       'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
	       'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
	       'W', 'X', 'Y', 'Z'
	      };
	
	    /* build 6-bit vector of coded values */
	    unsigned ind;
	    int bits;
	
	    ind = (cp[0] << 26) +
	          (cp[1] << 22) +
	          (cp[2] << 18) + 
	          (cp[3] << 14) +
	          (cp[4] << 10) +
	          (cp[5] << 6) +
	          (cp[6] << 2) +
	          (cp[7] >> 2);
	  
	    /* decode ISRC due to IEC 908 */
	    for (bits = 0; bits < 30; bits +=6) {
	      int binval = (ind & ((unsigned long) 0x3fL << (24L-bits))) >> (24L-bits);
	      *p++ = bin2ISRC[binval];
	  
	      /* insert a dash after two country characters for legibility */
	      if (bits == 6)
	        *p++ = '-';
	    }
	  
	    /* format year and serial number */
	    sprintf ((char *)p, "-%.1X%.1X-%.1X%.1X%.1X%.1X%.1X",
		   subq_tr->track_isrc [8],
		   subq_tr->track_isrc [9],
		   subq_tr->track_isrc [10],
		   subq_tr->track_isrc [11],
		   subq_tr->track_isrc [12],
		   subq_tr->track_isrc [13],
		   subq_tr->track_isrc [14]
		   ); 
	  } else {
	    /* It is really in ASCII, surprise */
	    int ii;
	    for (ii = 0; ii < 12; ii++) {
	      if ((ii == 2 || ii == 5 || ii == 7) && cp[ii] != ' ')
	        *p++ = '-';
	      *p++ = cp[ii];
	    }
	    if (p - p_start >= 16)
	      *(p_start + 15) = '\0';
	    else
	      *p = '\0';
	  }
          if (memcmp(p_start,"00-000-00-00000",15) != 0) {
	    memcpy((char *)g_toc[i].ISRC, (const char *)p_start, 15);
	  }
        }
      }
      if (g_toc[i].ISRC[0] != '\0') {
        fprintf (stderr, "\rT: %2d ISRC: %15.15s\n", i+1, g_toc[i].ISRC);
        fflush(stderr); 
      }
    }
  fputs("\n", stderr);
  }
}


/*
 * We rely on audio sectors here!!!
 * The only method that worked even with my antique Toshiba 3401,
 * is playing the sector and then request the subchannel afterwards.
 *
 * TODO:
 * For modern drives implement a second method. If the drive supports
 * reading of subchannel data, do direct reads.
 */
static int GetIndexOfSector( sec, track )
	unsigned sec;
	unsigned track;
{
    subq_chnl *sub_ch;

    if (-1 == Play_at(get_scsi_p(), sec, 1)) {
      if ((long)sec == GetEndSector(track)) {
        fprintf(stderr, "Firmware bug detected! Drive cannot play the very last sector!\n");
      }
      return -1;
    }

    sub_ch = ReadSubQ(get_scsi_p(), GET_POSITIONDATA,0);

    /* compare control field with the one from the TOC */
    if ((g_toc[track-1].bFlags & 0x0f) != (sub_ch->control_adr & 0x0f)) {
	fprintf(stderr, "\ncontrol field mismatch TOC: %1x, in-track subchannel: %1x\tcorrecting TOC\n",
		g_toc[track-1].bFlags & 0x0f, sub_ch->control_adr & 0x0f);
	g_toc[track-1].bFlags &= 0xF0;
	g_toc[track-1].bFlags |= sub_ch->control_adr & 0x0f;
    }

    return sub_ch ? sub_ch->index == 244 ? 1 : sub_ch->index : -1;
}

static int
ScanBackwardFrom __PR((unsigned sec, unsigned limit, int *where, unsigned track));

static int ScanBackwardFrom(sec, limit, where, track)
	unsigned sec;
	unsigned limit;
	int *where;
	unsigned track;
{
	unsigned lastindex = 0;
	unsigned mysec = sec;

	/* try to find the transition of index n to index 0,
	 * if the track ends with an index 0.
	 */
	while ((lastindex = GetIndexOfSector(mysec, track)) == 0) {
		if (mysec < limit+75) {
			break;
		}
		mysec -= 75;
	}
	if (mysec == sec) {
		/* there is no pre-gap in this track */
		if (where != NULL) *where = -1;
	} else {
		/* we have a pre-gap in this track */

		if (lastindex == 0) {
			/* we did not cross the transition yet -> search backward */
			do {
				if (mysec < limit+1) {
					break;
				}
				mysec --;
			} while ((lastindex = GetIndexOfSector(mysec,track)) == 0);
			if (lastindex != 0) {
				/* successful */
				mysec ++;
				/* register mysec as transition */
				if (where != NULL) *where = (int) mysec;
			} else {
				/* could not find transition */
				if (!global.quiet)
					fprintf(stderr,
					 "Could not find index transition for pre-gap.\n");
				if (where != NULL) *where = -1;
			}
		} else {
			int myindex = -1;
			/* we have crossed the transition -> search forward */
			do {
				if (mysec >= sec) {
					break;
				}
				mysec ++;
			} while ((myindex = GetIndexOfSector(mysec,track)) != 0);
			if (myindex == 0) {
				/* successful */
				/* register mysec as transition */
				if (where != NULL) *where = (int) mysec;
			} else {
				/* could not find transition */
				if (!global.quiet)
					fprintf(stderr,
					 "Could not find index transition for pre-gap.\n");
				if (where != NULL) *where = -1;
			}
		}
	}
	return lastindex;
}

static int linear_search(searchInd, Start, End, track)
	int searchInd;
	unsigned Start;
	unsigned End;
	unsigned track;
{
      int l = Start;
      int r = End;

      for (; l <= r; l++ ) {
          int ind;

	  ind = GetIndexOfSector(l, track);
	  if ( searchInd == ind ) {
	      break;
	  }
      }
      if ( l <= r ) {
        /* Index found. */
	return l;
      }

      return -1;
}

#undef DEBUG_BINSEARCH
static int binary_search(searchInd, Start, End, track)
	int searchInd;
	unsigned Start;
	unsigned End;
	unsigned track;
{
      int l = Start;
      int r = End;
      int x = 0;
      int ind;

      while ( l <= r ) {
	  x = ( l + r ) / 2;
	  /* try to avoid seeking */
	  ind = GetIndexOfSector(x, track);
	  if ( searchInd == ind ) {
	      break;
	  } else {
	      if ( searchInd < ind ) r = x - 1;
	      else	     	     l = x + 1;
	  }
      }
#ifdef DEBUG_BINSEARCH
fprintf(stderr, "(%d,%d,%d > ",l,x,r);
#endif
      if ( l <= r ) {
        /* Index found. Now find the first position of this index */
	/* l=LastPos	x=found		r=NextPos */
        r = x;
	while ( l < r-1 ) {
	  x = ( l + r ) / 2;
	  /* try to avoid seeking */
	  ind = GetIndexOfSector(x, track);
	  if ( searchInd == ind ) {
	      r = x;
	  } else {
	      l = x;
	  }
#ifdef DEBUG_BINSEARCH
fprintf(stderr, "%d -> ",x);
#endif
        }
#ifdef DEBUG_BINSEARCH
fprintf(stderr, "%d,%d)\n",l,r);
#endif
	if (searchInd == GetIndexOfSector(l, track))
	  return l;
	else
	  return r;
      }

      return -1;
}


static void
register_index_position __PR((int IndexOffset, index_list **last_index_entry));

static void
register_index_position(IndexOffset, last_index_entry)
	int IndexOffset;
	index_list **last_index_entry;
{
      index_list *indexentry;

      /* register higher index entries */
      if (*last_index_entry != NULL) {
        indexentry = (index_list *) malloc( sizeof(index_list) );
      } else {
        indexentry = NULL;
      }
      if (indexentry != NULL) {
        indexentry->next = NULL;
        (*last_index_entry)->next = indexentry;
        *last_index_entry = indexentry;
        indexentry->frameoffset = IndexOffset;
      } else {
#if defined INFOFILES
        fprintf( stderr, "No memory for index lists. Index positions\nwill not be written in info file!\n");
#endif
      }
}

#undef DEBUG_INDLIST
/* experimental code */
/* search for indices (audio mode required) */
unsigned ScanIndices( track, cd_index, bulk )
	unsigned track;
	unsigned cd_index;
	int bulk;
{
  /* scan for indices. */
  /* look at last sector of track. */
  /* when the index is not equal 1 scan by bipartition 
   * for offsets of all indices */

  unsigned i; 
  unsigned starttrack, endtrack;
  unsigned startindex, endindex;

  unsigned j;
  int LastIndex=0;
  int n_0_transition;
  unsigned StartSector;
  unsigned retval = 0;

  index_list *baseindex_pool;
  index_list *last_index_entry;

  if (!global.quiet && !(global.verbose & SHOW_INDICES))
    fprintf(stderr, "seeking index start ...");
  if (bulk != 1) {
    starttrack = track; endtrack = track;
  } else {
    starttrack = 1; endtrack = cdtracks;
  }
  baseindex_pool = (index_list *) malloc( sizeof(index_list) * (endtrack - starttrack + 1));
#ifdef DEBUG_INDLIST
  fprintf(stderr, "index0-mem-pool %p\n", baseindex_pool);
#endif

  for (i = starttrack; i <= endtrack; i++) {
    if ( global.verbose & SHOW_INDICES ) { 
	fprintf( stderr, "index scan: %d...\r", i ); 
	fflush (stderr);
    }
    if ( g_toc [i-1].bFlags & CDROM_DATA_TRACK )
	continue;/* skip nonaudio tracks */
    StartSector = GetStartSector(i);
    LastIndex = ScanBackwardFrom(GetEndSector(i), StartSector, &n_0_transition, i);
    /* register first index entry for this track */
    if (baseindex_pool != NULL) {
#ifdef DEBUG_INDLIST
  fprintf(stderr, "audio track %d, arrindex %d\n", i, i - starttrack);
#endif
      baseindex_pool[i - starttrack].next = NULL;
      baseindex_pool[i - starttrack].frameoffset = StartSector;
      global.trackindexlist[i-1] = &baseindex_pool[i - starttrack];
#ifdef DEBUG_INDLIST
  fprintf(stderr, "indexbasep %p\n", &baseindex_pool[i - starttrack]);
#endif
    } else {
      global.trackindexlist[i-1] = NULL;
    }
    last_index_entry = global.trackindexlist[i-1];
    if (LastIndex < 2) {
      register_index_position(n_0_transition, &last_index_entry);
      continue;
    }

    if ((global.verbose & SHOW_INDICES) && LastIndex > 1)
	fprintf(stderr, "track %2d has %d indices, index table (pairs of 'index: frame offset')\n", i, LastIndex);

    if (0 && cd_index != 1) {
	startindex = cd_index; endindex = cd_index;
    } else {
	startindex = 0; endindex = LastIndex;
    }
    for (j = startindex; j <= endindex; j++) {
      int IndexOffset;

      /* this track has indices */

      if (1 || endindex - startindex < 3) {
        /* do a binary search */
        IndexOffset = binary_search(j, StartSector, GetEndSector(i), i);
      } else {
        /* do a linear search */
        IndexOffset = linear_search(j, StartSector, GetEndSector(i), i);
      }

      if (IndexOffset != -1) {
		StartSector = IndexOffset;
      }
      register_index_position(IndexOffset, &last_index_entry);

      if ( IndexOffset == -1 ) {
	  if (global.verbose & SHOW_INDICES) {
	    if (global.gui == 0) {
	      fprintf(stderr, "%2u: N/A   ",j);
	    } else {
	      fprintf(stderr, "T%02d I%02u N/A\n",i,j);
	    }
	  }
      } else {
	  if (global.verbose & SHOW_INDICES) {
	    if (global.gui == 0) {
	      fprintf(stderr, 
		    "%2u:%6lu ",
		    j,
		    IndexOffset-GetStartSector(i));
	    } else {
	      fprintf(stderr, "T%02d I%02u %06lu\n",i,j,IndexOffset-GetStartSector(i));
	    }
	  }
	  if (track == i && cd_index == j) {
	     retval = IndexOffset-GetStartSector(i);
	  }
      }
    }
    register_index_position(n_0_transition, &last_index_entry);
    if (global.gui == 0 && global.verbose & SHOW_INDICES)
      fputs("\n", stderr);
  }
  StopPlay(get_scsi_p());
  return retval;
}
