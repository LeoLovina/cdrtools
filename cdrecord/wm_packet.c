/* @(#)wm_packet.c	1.14 01/02/22 Copyright 1995, 1997, 2001 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)wm_packet.c	1.14 01/02/22 Copyright 1995, 1997, 2001 J. Schilling";
#endif
/*
 *	CDR write method abtraction layer
 *	packet writing intercace routines
 *
 *	Copyright (c) 1995, 1997, 2001 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <mconfig.h>
#include <stdio.h>
#include <standard.h>
#include <stdxlib.h>
#include <unixstd.h>
/*#include <string.h>*/
#include <sys/types.h>
#include <utypes.h>
#include <schily.h>

#include <scg/scsitransp.h>
#include "cdrecord.h"

extern	int	debug;
extern	int	verbose;
extern	int	lverbose;

extern	char	*buf;			/* The transfer buffer */

EXPORT	int	write_packet_data __PR((SCSI *scgp, cdr_t *dp, int track, track_t *trackp));

EXPORT int
write_packet_data(scgp, dp, track, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	int	track;
	track_t	*trackp;
{
	int	f;
	int	isaudio;
	long	startsec;
	Llong	bytes_read = 0;
	Llong	bytes	= 0;
	Llong	savbytes = 0;
	int	count;
	Llong	tracksize;
	int	secsize;
	int	secspt;
	int	bytespt;
	long	amount;
	int	pad;
	int	bswab;
	int	retried;
	long	nextblock; 
	int	bytes_to_read;
	BOOL	neednl	= FALSE;
	BOOL	islast	= FALSE;
	char	*bp	= buf;

	f = trackp->f;
	isaudio = is_audio(trackp);
	tracksize = trackp->tracksize;
	startsec = trackp->trackstart;

	secsize = trackp->secsize;
	secspt = trackp->secspt;
	bytespt = secsize * secspt;
	
	pad = !isaudio && is_pad(trackp);	/* Pad only data tracks */
	bswab = isaudio && is_swab(trackp);	/* Swab only audio tracks */

	if (debug) {
		printf("secsize:%d secspt:%d bytespt:%d audio:%d pad:%d\n",
			secsize, secspt, bytespt, isaudio, pad);
	}

	if (lverbose) {
		if (tracksize > 0)
			printf("\rTrack %02d:   0 of %3lld MB written.",
			       track, tracksize >> 20);
		else
			printf("\rTrack %02d:   0 MB written.", track);
		flush();
		neednl = TRUE;
	}

	do {
		bytes_to_read = bytespt;
		if (tracksize > 0) {
			if ((tracksize - bytes_read) > bytespt)
				bytes_to_read = bytespt;
			else
				bytes_to_read = tracksize - bytes_read;				
		}
		count = get_buf(f, &bp, bytes_to_read);
		if (count < 0)
			comerr("read error on input file\n");
		if (count == 0)
			break;
		bytes_read += count;
		if (tracksize >= 0 && bytes_read >= tracksize) {
			count -= bytes_read - tracksize;
			if (trackp->padsize == 0 || (bytes_read/secsize) >= 300)
				islast = TRUE;
		}

		if (bswab)
			swabbytes(bp, count);

		if (count < bytespt) {
			if (debug) {
				printf("\nNOTICE: reducing block size for last record.\n");
				neednl = FALSE;
			}

			if ((amount = count % secsize) != 0) {
				amount = secsize - amount;
				fillbytes(&bp[count], amount, '\0');
				count += amount;
				printf("\nWARNING: padding up to secsize.\n");
				neednl = FALSE;
			}
			if (is_packet(trackp) && trackp->pktsize > 0) {
				if (count < bytespt) {
					amount = bytespt - count;
					fillbytes(&bp[count], amount, '\0');
					count += amount;
					printf("\nWARNING: padding remainder of packet.\n");
					neednl = FALSE;
				}
			}
			bytespt = count;
			secspt = count / secsize;
			if (trackp->padsize == 0 || (bytes_read/secsize) >= 300)
				islast = TRUE;
		}

		retried = 0;
		retry:
		/* XXX Fixed-packet writes can be very slow*/
		if (is_packet(trackp) && trackp->pktsize > 0)
			scg_settimeout(scgp, 100);
		/* XXX */
		if (is_packet(trackp) && trackp->pktsize == 0) {
			if ((*dp->cdr_next_wr_address)(scgp, track, trackp, &nextblock) == 0) {
				/*
				 * Some drives (e.g. Ricoh MPS6201S) do not
				 * increment the Next Writable Address value to
				 * point to the beginning of a new packet if
				 * their write buffer has underflowed.
				 */
				if (retried && nextblock == startsec) {
					startsec += 7; 
				} else {
					startsec = nextblock;
				}
			}
		}

		amount = (*dp->cdr_write_trackdata)(scgp, bp, startsec, bytespt, secspt, islast);
		if (amount < 0) {
			if (is_packet(trackp) && trackp->pktsize == 0 && !retried) {
				printf("%swrite track data: error after %lld bytes, retry with new packet\n",
					neednl?"\n":"", bytes);
				retried = 1;
				neednl = FALSE;
				goto retry;
			}
			printf("%swrite track data: error after %lld bytes\n",
							neednl?"\n":"", bytes);
			return (-1);
		}
		bytes += amount;
		startsec += amount / secsize;

		if (lverbose && (bytes >= (savbytes + 0x100000))) {
			int	fper;

			printf("\rTrack %02d: %3lld", track, bytes >> 20);
			if (tracksize > 0)
				printf(" of %3lld MB", tracksize >> 20);
			else
				printf(" MB");
			printf(" written");
			fper = fifo_percent(TRUE);
			if (fper >= 0)
				printf(" (fifo %3d%%)", fper);
			printf(".");
			savbytes = (bytes >> 20) << 20;
			flush();
			neednl = TRUE;
		}
	} while (tracksize < 0 || bytes_read < tracksize);

	if ((bytes / secsize) < 300) {
		savbytes = roundup(trackp->padsize, secsize);
		if (((bytes+savbytes) / secsize) < 300)
			trackp->padsize = 300 * secsize - bytes;
	}
	if (trackp->padsize) {
		if (neednl) {
			printf("\n");
			neednl = FALSE;
		}
		if ((trackp->padsize >> 20) > 0) {
			neednl = TRUE;
		} else if (lverbose) {
			printf("Track %02d: writing %3lld KB of pad data.\n",
						track, (Llong)trackp->padsize >> 10);
			neednl = FALSE;
		}
		pad_track(scgp, dp, track, trackp, startsec, trackp->padsize,
					TRUE, &savbytes);
		bytes += savbytes;
		startsec += savbytes / secsize;
	}
	printf("%sTrack %02d: Total bytes read/written: %lld/%lld (%lld sectors).\n",
	       neednl?"\n":"", track, bytes_read, bytes, bytes/secsize);
	return 0;
}

