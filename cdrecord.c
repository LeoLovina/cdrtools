/* @(#)cdrecord.c	1.1 96/02/04 Copyright 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)cdrecord.c	1.1 96/02/04 Copyright 1995 J. Schilling";
#endif  lint
/*
 *	Record data on a CD-Recorder
 *
 *	Copyright (c) 1995 J. Schilling
 */
/* This program is free software; you can redistribute it and/or modify
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

#include <stdio.h>
#include <standard.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "cdrecord.h"

typedef struct index {
	int	dummy;		/* Not yet implemented */
} tindex_t;

typedef struct track {
	int	f;		/* Open file for this track		*/
	int	tracksize;	/* Size of this track (-1 == until EOF)	*/
	int	secsize;	/* Sector size for this track		*/
	int	secspt;		/* # of sectors to copy for one transfer*/
	int	flags;		/* Flags (see below)			*/
	tindex_t *index;	/* Track index descriptor		*/
} track_t;

/*
 * Defines for flags
 */
#define	TI_AUDIO	0x01	/* File is an audio track		*/
#define	TI_PREEMP	0x02	/* Audio track recorded w/preemphasis	*/
#define	TI_PAD		0x04	/* Pad data track			*/

#define	is_audio(tp)	((tp)->flags & TI_AUDIO)
#define	is_preemp(tp)	((tp)->flags & TI_PREEMP)
#define	is_pad(tp)	((tp)->flags & TI_PAD)

/*
 * Usefull definitions for audio tracks
 */
#define	sample		(44100 * 2)		/* one 16bit audio sample */
#define	ssample		(sample * 2)		/* one stereo sample	*/
#define samples(v)	((v) / ssample)		/* # of stereo samples	*/
#define hsamples(v)	((v) / (ssample/100))	/* 100 * # of stereo samples */

#define	minutes(v)	(samples(v) / 60)
#define	seconds(v)	(samples(v) % 60)
#define	hseconds(v)	(hsamples(v) % 100)

int		ldebug;		/* print debug messages */
int             verbose;	/* SCSI verbose flag */
int             lverbose;	/* local verbose flag */

/*
 * NOTICE:	You should not make BUF_SIZE more than
 *		the buffer size of the CD-Recorder.
 *
 * Do not set BUF_SIZE to be more than 126 KBytes
 * if you are running cdrecord on a sun4c machine.
 *
 * WARNING:	Philips CDD 521 dies if BUF_SIZE is to big.
 */
/*#define	BUF_SIZE	(126*1024)*/
/*#define	BUF_SIZE	(100*1024)*/
#define	BUF_SIZE	(63*1024)
/*#define	BUF_SIZE	(56*1024)*/

char	buf[BUF_SIZE];		/* The transfer buffer (should be allocated) */
int	data_secs_per_tr;	/* # of data secs per transfer */
int	audio_secs_per_tr;	/* # of audio secs per transfer */

extern	int	silent;

void	usage		__PR((int));
int	scsi_write	__PR((char *, int, int));
int	write_track_data __PR((int , track_t *));
void	printdata	__PR((int, track_t *));
void	printaudio	__PR((int, track_t *));
void	checkfile	__PR((int, track_t *));
int	checkfiles	__PR((int, track_t *));
void	checksize	__PR((track_t *));
void	raise_fdlim	__PR((void));
void	gargs		__PR((int , char **, int *, track_t *, char **,
					int *, int *, int *, int *));
void	unload_media	__PR((void));
void	check_recovery	__PR((void));

int 
main(ac, av)
	int	ac;
	char	*av[];
{
	char	*dev = NULL;
	int	speed = 1;
	int	dummy = 0;
	int	multi = 0;
	int	fix = 0;
	int	timeout = 4 * 60; /* def=20s but need 4 minutes for fixation*/
	int	i;
	int	tracks = 0;
	track_t	track[MAX_TRACK+1];
	int	isaudio;

	save_args(ac, av);

	/*
	 * We are using SCSI Group 0 write
	 * and cannot write more than 255 secs at once.
	 */
	data_secs_per_tr = BUF_SIZE/DATA_SEC_SIZE;
	audio_secs_per_tr = BUF_SIZE/AUDIO_SEC_SIZE;
	data_secs_per_tr = min(255, data_secs_per_tr);
	audio_secs_per_tr = min(255, audio_secs_per_tr);

	raise_fdlim();
	gargs(ac, av, &tracks, track, &dev, &speed, &dummy, &multi, &fix);

	isaudio = checkfiles(tracks, track);

	/*
	 * Try to lock us im memory (will only work for root)
	 * but you need access to root anyway to use /dev/scg?
	 */
	if (mlockall(MCL_CURRENT|MCL_FUTURE) < 0)
		comerr("Cannot do mlockall(2).\n");

	open_scsi(dev, timeout);

	/*
	 * First try to check which type of SCSI device we
	 * have.
	 */
	silent++;
	test_unit_ready();	/* eat up unit attention */
	silent--;
	if (!do_inquiry(1))
		comerrno(BAD, "Cannot do inquiry for CD-Recorder.\n");
	if (!is_cdrecorder())
		comerrno(BAD, "Sorry, no CD-Recorder found on this target.\n");

	check_recovery();
	/*
	 * Do some preparation before...
	 */
	if (!is_yamaha())	/* Yamaha cannot load disks */
		scsi_load_unload(1);
	silent++;
	for (i=0; i < 60 && test_unit_ready() < 0; i++)
		sleep(1);
	silent--;
	if (test_unit_ready() < 0)
		comerrno(BAD, "CD-Recorder not ready.\n");

	scsi_prevent_removal(1);
	scsi_start_stop_unit(1);
	silent++;
	while (test_unit_ready() < 0)
		sleep(1);
	silent--;
	rezero_unit();
	test_unit_ready();
	scsi_start_stop_unit(1);

	/*
	 * Last chance to quit!
	 */
	if (speed_select(speed, dummy) < 0) {
		errmsg("Cannot set speed/dummy.\n");
		unload_media();
		exit(BAD);
	}
	printf("Starting to write CD at speed %d in %s mode for %s session.\n",
		speed, dummy ? "dummy" : "write", multi ? "multi" : "single");
	sleep(5);
	if (fix)
		goto fix_it;

	/*
	 * Now we actually start writing to the CD.
	 */
	for (i = 1; i <= tracks; i++) {
		if (is_yamaha()) {
			if (select_secsize(is_audio(&track[i]) ?
					AUDIO_SEC_SIZE : DATA_SEC_SIZE) < 0) {
				scsi_flush_cache();
				break;
			}
		} else {
			if (write_track_info(is_audio(&track[i]),
					is_preemp(&track[i])) < 0) {
				scsi_flush_cache();
				break;
			}
		}
		if (write_track(0, is_audio(&track[i]),
						is_preemp(&track[i])) < 0) {
			scsi_flush_cache();
			break;
		}

		if (write_track_data(i, &track[i]) < 0 ) {
			sleep(5);
			request_sense();
			scsi_flush_cache();
			break;
		}
		scsi_flush_cache();
	}
fix_it:
	if (lverbose)
		printf("Fixating...\n");
	fixation(multi, isaudio ? 0 : 1);

	unload_media();
	return 0;
}

void 
usage(excode)
	int excode;
{
	errmsgno(BAD, "Usage: %s [options] track1...trackn\n",
		get_progname());

	error("Options:\n");
	error("\t-v		be verbose\n");
	error("\t-V		be verbose for SCSI command transport\n");
	error("\tdev=target	SCSI target to use as CD-Recorder\n");
	error("\tspeed=#		set speed of drive\n");
	error("\t-dummy		do everything with laser turned off\n");
	error("\t-multi		generate a TOC that allows multi session\n");
	error("\t-fix		fixate a corrupt disk (generate a TOC)\n");
	error("\tbytes=#		Length of valid track data in bytes\n");
	error("\t-audio		Subsequent tracks are CD-DA audio\n");
	error("\t-data		Subsequent tracks are CD-ROM data (default)\n");
	error("\t-preemp		Audio tracks are mastered with 50/15 µs preemphasis\n");
	error("\t-nopreemp	Audio tracks are mastered with no preemphasis (default)\n");
	error("\t-pad		Pad data tracks with %d zeroed sectors\n", PAD_SECS);
	error("\t-nopad		Do not pad data tracks (default)\n");
	exit(excode);
}

/*#define TEST*/
#ifdef	TEST
int
scsi_write(bufp, size, blocks)
	char	*bufp;
	int	size;
	int	blocks;
{
/*	printf("scsi_write(%d, %d)\n", size, blocks);*/
	return(size);
}
#else
#	define	scsi_write(bp,size,blocks)	write_xg0(bp, 0, size, blocks)
#endif

int
read_buf(f, buf, size)
	int	f;
	char	*buf;
	int	size;
{
	char	*p = buf;
	int	amount = 0;
	int	n;

	do {
		do {
			n = read(f, p, size-amount);
		} while (n < 0 && (errno == EAGAIN || errno == EINTR));
		if (n < 0)
			return (n);
		amount += n;
		p += n;

	} while (amount < size && n > 0);
	return (amount);
}

int
write_track_data(track, trackp)
	int	track;
	track_t	*trackp;
{
	int             f ;
	int             isaudio;
	int             bytes_read = 0;
	int             bytes = 0;
	int             savbytes = 0;
	int             count;
	int             tracksize;
	int		secsize;
	int		secspt;
	int		bytespt;
	int		amount;
	int             pad;

	f = trackp->f;
	isaudio = is_audio(trackp);
	tracksize = trackp->tracksize;

	secsize = trackp->secsize;
	secspt = trackp->secspt;
	bytespt = secsize * secspt;
	
	pad = !isaudio && is_pad(trackp);	/* Pad only data tracks */

	if (ldebug) {
		printf("secsize:%d secspt:%d bytespt:%d audio:%d pad:%d\n",
			secsize, secspt, bytespt, isaudio, pad);
	}

	if (lverbose) {
		if (tracksize > 0)
			printf("Track %02d:   0 of %d Mb written.\r",
			       track, tracksize >> 20);
		else
			printf("Track %02d:   0 Mb written.\r", track);
		fflush(stdout);
	}

	do {
		count = read_buf(f, buf, bytespt);
		if (count < 0)
			comerr("read error on input file\n");
		if (count == 0)
			break;
		bytes_read += count;

		if (count < bytespt) {
			if (ldebug)
				printf("\nNOTICE: reducing block size for last record.\n");

			if ((amount = count % secsize) != 0) {
				amount = secsize - amount;
				fillbytes(&buf[count], amount, '\0');
				count += amount;
				printf("WARNING: padding up to secsize.\n");
			}
			bytespt = count;
			secspt = count / secsize;
		}

		amount = scsi_write(buf, bytespt, secspt);
		if (amount < 0) {
			printf("scsi_write error after %d bytes\n", bytes);
			return (-1);
		}
		bytes += amount;

		if (lverbose && (bytes > (savbytes + 0x100000))) {
			printf("Track %02d: %3d\r", track, bytes >> 20);
			savbytes = bytes;
			fflush(stdout);
		}
	} while (tracksize < 0 || bytes_read < tracksize);

	if (pad) {
		bytespt = PAD_SIZE;
		fillbytes(buf, PAD_SIZE, '\0');

		secspt = bytespt / secsize;

		amount = scsi_write(buf, bytespt, secspt);
		if (amount < 0) {
			printf("scsi_write error after %d bytes\n", bytes);
			return (-1);
		}
		bytes += amount;
	}
	printf("Track %02d: Total bytes read/written: %d/%d (%d sectors).\n",
	       track, bytes_read, bytes, bytes/secsize);
	return 0;
}

void
printdata(track, trackp)
	int	track;
	track_t	*trackp;
{
	if (trackp->tracksize >= 0) {
		printf("Track %02d: data  %3d Mb         %s\n",
					track, trackp->tracksize >> 20,
					is_pad(trackp) ? "pad" : "");
	} else {
		printf("Track %02d: data  unknown length %s\n",
					track, is_pad(trackp) ? "pad" : "");
	}
}

void
printaudio(track, trackp)
	int	track;
	track_t	*trackp;
{
	if (trackp->tracksize >= 0) {
		printf("Track %02d: audio %3d Mb (%02d:%02d.%02d) %spreemp\n",
					track, trackp->tracksize >> 20,
					minutes(trackp->tracksize),
					seconds(trackp->tracksize),
					hseconds(trackp->tracksize),
					is_preemp(trackp) ? "" : "no ");
	} else {
		printf("Track %02d: audio unknown length    %spreemp\n",
					track, is_preemp(trackp) ? "" : "no ");
	}
}

void
checkfile(track, trackp)
	int	track;
	track_t	*trackp;
{
	if (trackp->tracksize > 0 &&
			is_audio(trackp) &&
			(trackp->tracksize % trackp->secsize))
		comerrno(BAD, "Bad audio track size %d for track %02d. Must ba a multiple of %d\n",
				trackp->tracksize, track, trackp->secsize);
	
	if (!lverbose)
		return;

	if (is_audio(trackp))
		printaudio(track, trackp);
	else
		printdata(track, trackp);
}

int
checkfiles(tracks, trackp)
	int	tracks;
	track_t	*trackp;
{
	int	i;
	int	isaudio = 1;

	for (i = 1; i <= tracks; i++) {
		if (!is_audio(&trackp[i]))
			isaudio = 0;
		checkfile(i, &trackp[i]);
	}
	return (isaudio);
}

void
checksize(trackp)
	track_t	*trackp;
{
	struct stat     st;

	/*
	 * If the current input file is a regular file and
	 * '-bytes' has not been specified,
	 * use fstat() to get the size of the file.
	 */
	if (trackp->tracksize < 0) {
		if (!fstat(trackp->f, &st) && S_ISREG(st.st_mode))
			trackp->tracksize = st.st_size;
	}
}

void
raise_fdlim()
{
	struct rlimit	rlim;

	/*
	 * Set max # of file descriptors to be able to hold all files open
	 */
	getrlimit(RLIMIT_NOFILE, &rlim);
	rlim.rlim_cur = MAX_TRACK + 10;
	if (rlim.rlim_cur > rlim.rlim_max)
		errmsgno(BAD,
			"warning: low file descriptor limit (%ld)\n",
							rlim.rlim_max);
	setrlimit(RLIMIT_NOFILE, &rlim);
}

char	*opts = "help,dev*,bytes#,speed#,dummy,multi,fix,debug,v,V,audio,data,nopreemp,preemp,nopad,pad";

void
gargs(ac, av, tracksp, trackp, devp, speedp, dummyp, multip, fixp)
	int	ac;
	char	**av;
	int	*tracksp;
	track_t	*trackp;
	char	**devp;
	int	*speedp;
	int	*dummyp;
	int	*multip;
	int	*fixp;
{
	int	cac;
	char	* const*cav;
	int	tracksize;
	int	help = 0;
	int	audio;
	int	data = 1;
	int	preemp = 0;
	int	nopreemp;
	int	pad = 0;
	int	nopad;
	int	flags;
	int	tracks = *tracksp;

	cac = --ac;
	cav = ++av;
	for (;; cac--,cav++) {
		tracksize = -1;
		audio = 0;
		nopreemp = 0;
		nopad = 0;
		if (getargs(&cac, &cav, opts,
				&help,
				devp,
				&tracksize, speedp, dummyp, multip,
				fixp,
				&ldebug, &lverbose, &verbose,
				&audio, &data,
				&nopreemp, &preemp,
				&nopad, &pad) < 0) {
			errmsgno(BAD, "Bad Option: %s.\n", cav[0]);
			usage(BAD);
		}
		if (help)
			usage(0);
		if (getfiles(&cac, &cav, opts) == 0)
			break;
		tracks++;

		if (tracks > MAX_TRACK)
			comerrno(BAD, "Track limit (%d) exceeded\n",
								MAX_TRACK);

		if (audio)
			data = 0;
		if (nopreemp)
			preemp = 0;
		if (nopad)
			pad = 0;

		flags = 0;
		if (!data)
			flags |= TI_AUDIO;
		if (preemp)
			flags |= TI_PREEMP;
		if (pad)
			flags |= TI_PAD;

		if (strcmp("-", cav[0]) == 0)
			trackp[tracks].f = STDIN_FILENO;
		else
			if ((trackp[tracks].f = open(cav[0], O_RDONLY)) < 0)
				comerr("Cannot open '%s'.\n", cav[0]);
		trackp[tracks].tracksize = tracksize;
		trackp[tracks].secsize = data ? DATA_SEC_SIZE : AUDIO_SEC_SIZE;
		trackp[tracks].secspt = data ? data_secs_per_tr : audio_secs_per_tr;
		trackp[tracks].flags = flags;
		checksize(&trackp[tracks]);
	
		if (ldebug)
			printf("File: '%s' tracksize: %d flags %X\n",
				cav[0],	trackp[tracks].tracksize, flags);
	}
	if (*speedp < 0)
		comerrno(BAD, "Bad speed option.\n");
	if (!*devp) {
		errmsgno(BAD, "No CD-Recorder device specified.\n");
		usage(BAD);
	}
	if (!*fixp && tracks == 0) {
		errmsgno(BAD, "No tracks specified. Need at least one.\n");
		usage(BAD);
	}
	*tracksp = tracks;
}

void
unload_media()
{
	scsi_start_stop_unit(0);
	silent++;
	while (test_unit_ready() < 0)
		sleep(1);
	silent--;
	scsi_prevent_removal(0);
	scsi_load_unload(0);
}

void
check_recovery()
{
	if (recovery_needed()) {
		errmsgno(BAD, "Recovery needed.\n");
		exit(BAD);
	}
}

/*#define	DEBUG*/
void audioread()
{
#ifdef	DEBUG
	int speed = 1;
	int dummy = 0;

	speed_select(speed, dummy);
	select_secsize(2352);

	read_scsi(buf, 0, 1);
	printf("XXX:\n");
	write(1, buf, 512);
	exit(0);
#endif
}
