/* @(#)cdrecord.c	1.7 97/03/02 Copyright 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)cdrecord.c	1.7 97/03/02 Copyright 1995 J. Schilling";
#endif
/*
 *	Record data on a CD-Recorder
 *
 *	Copyright (c) 1995 J. Schilling
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
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>	/* for rlimit */
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "cdrecord.h"
#include "scsitransp.h"

typedef struct index {
	int	dummy;		/* Not yet implemented */
} tindex_t;

typedef struct track {
	int	f;		/* Open file for this track		*/
	int	tracksize;	/* Size of this track (-1 == until EOF)	*/
	int	secsize;	/* Sector size for this track		*/
	int	secspt;		/* # of sectors to copy for one transfer*/
	char	sectype;	/* Sector type				*/
	int	flags;		/* Flags (see below)			*/
	tindex_t *index;	/* Track index descriptor		*/
} track_t;

/*
 * Defines for flags
 */
#define	TI_AUDIO	0x01	/* File is an audio track		*/
#define	TI_PREEMP	0x02	/* Audio track recorded w/preemphasis	*/
#define	TI_MIX		0x04	/* This is a mixed mode track		*/
#define	TI_RAW		0x08	/* Write this track in raw mode		*/
#define	TI_PAD		0x10	/* Pad data track			*/
#define	TI_SWAB		0x20	/* Swab audio data			*/
#define	TI_ISOSIZE	0x40	/* Use iso size for track		*/

#define	is_audio(tp)	((tp)->flags & TI_AUDIO)
#define	is_preemp(tp)	((tp)->flags & TI_PREEMP)
#define	is_pad(tp)	((tp)->flags & TI_PAD)
#define	is_swab(tp)	((tp)->flags & TI_SWAB)

/*
 * Defines for option flags
 */
#define	F_DUMMY		0x01	/* Do dummy writes */
#define	F_MSINFO	0x02	/* Get multi-session info */
#define	F_TOC		0x04	/* Get TOC */
#define	F_MULTI		0x10	/* Create linkable TOC (multi-session)*/
#define	F_FIX		0x20	/* Fixate disk only */

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

/*
 * Map toc/track types into names.
 */
char	*toctname[] = {
		"CD-DA",
		"CD-ROM",
		"CD-ROM XA mode 1",
		"CD-ROM XA mode 2",
		"CD-I",
		"Illegal toc type 5",
		"Illegal toc type 6",
		"Illegal toc type 7",
};

/*
 * Map sector types into names.
 */
char	*sectname[] = {
		"Illegal sector type 0",
		"CD-ROM mode 1",
		"CD-ROM mode 2",
		"Illegal sector type 3",
		"CD-DA without preemphasis",
		"CD-DA with preemphasis",
		"Illegal sector type 6",
		"Illegal sector type 7",
};

int		debug;		/* print debug messages */
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

char	*buf;			/* The transfer buffer */
long	bufsize;		/* The size of the transfer buffer */
int	data_secs_per_tr;	/* # of data secs per transfer */
int	audio_secs_per_tr;	/* # of audio secs per transfer */

struct timeval	starttime;
struct timeval	stoptime;
struct timeval	fixtime;

extern	int	silent;

EXPORT	int 	main		__PR((int ac, char **av));
LOCAL	void	usage		__PR((int));
	int	scsi_write	__PR((char *, int, int));
LOCAL	int	read_buf	__PR((int f, char *bp, int size));
LOCAL	int	write_track_data __PR((int , track_t *));
LOCAL	void	printdata	__PR((int, track_t *));
LOCAL	void	printaudio	__PR((int, track_t *));
LOCAL	void	checkfile	__PR((int, track_t *));
LOCAL	int	checkfiles	__PR((int, track_t *));
LOCAL	void	checksize	__PR((track_t *));
LOCAL	void	raise_fdlim	__PR((void));
LOCAL	void	gargs		__PR((int , char **, int *, track_t *, char **,
					int *, int *, int *));
LOCAL	void	set_trsizes	__PR((int, track_t *));
LOCAL	void	unload_media	__PR((void));
LOCAL	void	check_recovery	__PR((void));
	void	audioread	__PR((void));
LOCAL	void	print_msinfo	__PR((void));
LOCAL	void	print_toc	__PR((void));
LOCAL	void	print_track	__PR((int, long, struct msf *, int, int, int));
LOCAL	void	prtimediff __PR((const char *fmt, struct timeval *start, struct timeval *stop));
LOCAL	void	raisepri	__PR((void));

EXPORT int 
main(ac, av)
	int	ac;
	char	*av[];
{
	char	*dev = NULL;
	int	speed = 1;
	int	flags = 0;
	int	toctype = -1;
	int	i;
	int	tracks = 0;
	track_t	track[MAX_TRACK+1];
	int	isaudio;

	save_args(ac, av);

	raise_fdlim();
	gargs(ac, av, &tracks, track, &dev, &speed, &flags, &toctype);
	if (toctype < 0)
		comerrno(EX_BAD, "Internal error: Bad TOC type.\n");
	if (debug || lverbose) {
		printf("TOC Type: %d = %s\n",
			toctype, toctname[toctype & TOC_MASK]);
	}

	isaudio = checkfiles(tracks, track);

	if ((flags & (F_MSINFO|F_TOC)) == 0) {
		/*
		 * Try to lock us im memory (will only work for root)
		 * but you need access to root anyway to use /dev/scg?
		 */
#ifdef	HAVE_MLOCKALL
		if (mlockall(MCL_CURRENT|MCL_FUTURE) < 0)
			comerr("Cannot do mlockall(2).\n");
#endif

		raisepri();
	}

	open_scsi(dev, -1, (flags & F_MSINFO) == 0 || lverbose);

	bufsize = scsi_bufsize(BUF_SIZE);
	if ((buf = scsi_getbuf(bufsize)) == NULL)
		comerr("Cannot get SCSI I/O buffer.\n");

	set_trsizes(tracks, track);

	/*
	 * First try to check which type of SCSI device we
	 * have.
	 */
	silent++;
	test_unit_ready();	/* eat up unit attention */
	silent--;
	if (!do_inquiry((flags & F_MSINFO) == 0 || lverbose))
		comerrno(EX_BAD, "Cannot do inquiry for CD-Recorder.\n");
	if (is_unsupported())
		comerrno(EX_BAD, "Sorry, unsupported CD-Recorder found on this target.\n");
	if (!is_cdrecorder()) {
		if (flags & (F_MSINFO|F_TOC)) {
			if (!is_cddrive())
				comerrno(EX_BAD,
			"Sorry, no CD-Drive found on this target.\n");
		} else {
			comerrno(EX_BAD,
			"Sorry, no CD-Recorder found on this target.\n");
		}
	}

	check_recovery();
	/*
	 * Do some preparation before...
	 */
	silent++;		/* Be quiet if this fails   */
	if (!is_yamaha())	/* Yamaha cannot load disks */
		scsi_load_unload(1);
	silent--;
	silent++;
	for (i=0; i < 60 && test_unit_ready() < 0; i++)
		sleep(1);
	silent--;
	if (test_unit_ready() < 0)
		comerrno(EX_BAD, "CD-Recorder not ready.\n");

	scsi_prevent_removal(1);
	scsi_start_stop_unit(1);
	silent++;
	while (test_unit_ready() < 0)
		sleep(1);
	silent--;
	rezero_unit();
	test_unit_ready();
	scsi_start_stop_unit(1);
/*audioread();*/
/*unload_media();*/
/*return 0;*/
	if (flags & F_MSINFO) {
		print_msinfo();
		scsi_prevent_removal(0);
		exit(0);
	}
	if (flags & F_TOC) {
		print_toc();
		scsi_prevent_removal(0);
		exit(0);
	}

	/*
	 * Last chance to quit!
	 */
	if (speed_select(speed, flags & F_DUMMY) < 0) {
		errmsg("Cannot set speed/dummy.\n");
		unload_media();
		exit(EX_BAD);
	}
	printf("Starting to write CD at speed %d in %s mode for %s session.\n",
		speed,
		(flags & F_DUMMY) ? "dummy" : "write",
		(flags & F_MULTI) ? "multi" : "single");
	printf("Last chance to quit, starting %s write in 9 seconds.",
		(flags & F_DUMMY)?"dummy":"real");
	flush();
	for (i=9; --i > 0;) {
		sleep(1);
		printf("\b\b\b\b\b\b\b\b\b\b%d seconds.", i);
		flush();
	}
	printf("\n");
	if (gettimeofday(&starttime, (struct timezone *)0) < 0)
		errmsg("Cannot get start time\n");
	if (flags & F_FIX)
		goto fix_it;

	/*
	 * Now we actually start writing to the CD.
	 */
	for (i = 1; i <= tracks; i++) {
		if (select_secsize(track[i].secsize) < 0) {
			scsi_flush_cache();
			break;
		}
/*#define	notneeded*/
#ifdef	notneeded
		if (!is_yamaha()) {
			if (write_track_info(track[i].sectype) < 0) {
				scsi_flush_cache();
				break;
			}
		}
#endif
		if (write_track(0, track[i].sectype) < 0) {
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
	if (gettimeofday(&stoptime, (struct timezone *)0) < 0)
		errmsg("Cannot get stop time\n");
	if (lverbose)
		prtimediff("Writing  time: ", &starttime, &stoptime);

	if (lverbose)
		printf("Fixating...\n");
	fixation(flags & F_MULTI, toctype);

	if (gettimeofday(&fixtime, (struct timezone *)0) < 0)
		errmsg("Cannot get fix time\n");
	if (lverbose)
		prtimediff("Fixating time: ", &stoptime, &fixtime);

	unload_media();
	return 0;
}

LOCAL void 
usage(excode)
	int excode;
{
	errmsgno(EX_BAD, "Usage: %s [options] track1...trackn\n",
		get_progname());

	error("Options:\n");
	error("\t-v		be verbose\n");
	error("\t-V		be verbose for SCSI command transport\n");
	error("\t-debug		print additional debug messages\n");
	error("\tdev=target	SCSI target to use as CD-Recorder\n");
	error("\tspeed=#		set speed of drive\n");
	error("\t-dummy		do everything with laser turned off\n");
	error("\t-msinfo		retrieve multi-session info for mkisofs-1.06b\n");
	error("\t-toc		retrieve and print TOC/PMA data\n");
	error("\t-multi		generate a TOC that allows multi session\n");
	error("\t		In this case default track type is CD-ROM XA2\n");
	error("\t-fix		fixate a corrupt disk (generate a TOC)\n");
	error("\tbytes=#		Length of valid track data in bytes\n");
	error("\t-audio		Subsequent tracks are CD-DA audio tracks\n");
	error("\t-data		Subsequent tracks are CD-ROM data mode 1 tracks (default)\n");
	error("\t-mode2		Subsequent tracks are CD-ROM data mode 2 tracks\n");
	error("\t-xa1		Subsequent tracks are CD-ROM XA mode 1 tracks\n");
	error("\t-xa2		Subsequent tracks are CD-ROM XA mode 2 tracks\n");
	error("\t-cdi		Subsequent tracks are CDI tracks\n");
	error("\t-isosize	Use iso9660 file system size for next data track\n");
	error("\t-preemp		Audio tracks are mastered with 50/15 µs preemphasis\n");
	error("\t-nopreemp	Audio tracks are mastered with no preemphasis (default)\n");
	error("\t-pad		Pad data tracks with %d zeroed sectors\n", PAD_SECS);
	error("\t		Pad audio tracks to a multiple of %d bytes\n", AUDIO_SEC_SIZE);
	error("\t-nopad		Do not pad data tracks (default)\n");
	error("\t-swab		Swab audio data (needed on some writers e.g. Yamaha)\n");
	error("The type of the first track is used for the toc type.\n");
	error("Currently only form 1 tracks are supported.\n");
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

LOCAL int
read_buf(f, bp, size)
	int	f;
	char	*bp;
	int	size;
{
	char	*p = bp;
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

LOCAL int
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
	int		bswab;

	f = trackp->f;
	isaudio = is_audio(trackp);
	tracksize = trackp->tracksize;

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

		if (bswab)
			swabbytes(buf, count);

		if (count < bytespt) {
			if (debug)
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

LOCAL void
printdata(track, trackp)
	int	track;
	track_t	*trackp;
{
	if (trackp->tracksize >= 0) {
		printf("Track %02d: data  %3d Mb         %s %s\n",
					track, trackp->tracksize >> 20,
					is_pad(trackp) ? "pad" : "",
					is_swab(trackp) ? "swab" : "");
	} else {
		printf("Track %02d: data  unknown length %s %s\n",
					track,
					is_pad(trackp) ? "pad" : "",
					is_swab(trackp) ? "swab" : "");
	}
}

LOCAL void
printaudio(track, trackp)
	int	track;
	track_t	*trackp;
{
	if (trackp->tracksize >= 0) {
		printf("Track %02d: audio %3d Mb (%02d:%02d.%02d) %spreemp%s%s\n",
			track, trackp->tracksize >> 20,
			minutes(trackp->tracksize),
			seconds(trackp->tracksize),
			hseconds(trackp->tracksize),
			is_preemp(trackp) ? "" : "no ",
			(trackp->tracksize % trackp->secsize) && is_pad(trackp) ? " pad" : "",
			is_swab(trackp) ? " swab":"");
	} else {
		printf("Track %02d: audio unknown length    %spreemp%s%s\n",
			track, is_preemp(trackp) ? "" : "no ",
			(trackp->tracksize % trackp->secsize) && is_pad(trackp) ? " pad" : "",
			is_swab(trackp) ? " swab":"");
	}
}

LOCAL void
checkfile(track, trackp)
	int	track;
	track_t	*trackp;
{
	if (trackp->tracksize > 0 &&
			is_audio(trackp) &&
			(trackp->tracksize % trackp->secsize) &&
						!is_pad(trackp)) {
		comerrno(EX_BAD, "Bad audio track size %d for track %02d. Must be a multiple of %d Bytes\n",
				trackp->tracksize, track, trackp->secsize);
	}
	
	if (!lverbose)
		return;

	if (is_audio(trackp))
		printaudio(track, trackp);
	else
		printdata(track, trackp);
}

LOCAL int
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

LOCAL void
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
		if ((trackp->flags & TI_ISOSIZE) != 0) {
			trackp->tracksize = isosize(trackp->f);
		} else if (fstat(trackp->f, &st) >= 0 && S_ISREG(st.st_mode))
			trackp->tracksize = st.st_size;
	}
}

LOCAL void
raise_fdlim()
{
	struct rlimit	rlim;

	/*
	 * Set max # of file descriptors to be able to hold all files open
	 */
	getrlimit(RLIMIT_NOFILE, &rlim);
	rlim.rlim_cur = MAX_TRACK + 10;
	if (rlim.rlim_cur > rlim.rlim_max)
		errmsgno(EX_BAD,
			"warning: low file descriptor limit (%ld)\n",
							rlim.rlim_max);
	setrlimit(RLIMIT_NOFILE, &rlim);
}

char	*opts =
"help,dev*,bytes#,speed#,dummy,msinfo,toc,multi,fix,debug,v,V,audio,data,mode2,xa1,xa2,cdi,isosize,nopreemp,preemp,nopad,pad,swab";

LOCAL void
gargs(ac, av, tracksp, trackp, devp, speedp, flagsp, toctypep)
	int	ac;
	char	**av;
	int	*tracksp;
	track_t	*trackp;
	char	**devp;
	int	*speedp;
	int	*flagsp;
	int	*toctypep;
{
	int	cac;
	char	* const*cav;
	int	tracksize;
	int	help = 0;
	int	dummy = 0;
	int	msinfo = 0;
	int	toc = 0;
	int	multi = 0;
	int	fix = 0;
	int	audio;
	int	data;
	int	mode2;
	int	xa1;
	int	xa2;
	int	cdi;
	int	isize;
	int	preemp = 0;
	int	nopreemp;
	int	pad = 0;
	int	bswab = 0;
	int	nopad;
	int	flags;
	int	tracks = *tracksp;
	int	tracktype = TOC_ROM;
	int	sectype = ST_ROM_MODE1;

	cac = --ac;
	cav = ++av;
	for (;; cac--,cav++) {
		tracksize = -1;
		audio = data = mode2 = xa1 = xa2 = cdi = 0;
		isize = 0;
		nopreemp = 0;
		nopad = 0;
		if (getargs(&cac, &cav, opts,
				&help,
				devp,
				&tracksize, speedp, &dummy, &msinfo, &toc,
				&multi, &fix,
				&debug, &lverbose, &verbose,
				&audio, &data, &mode2,
				&xa1, &xa2, &cdi,
				&isize,
				&nopreemp, &preemp,
				&nopad, &pad, &bswab) < 0) {
			errmsgno(EX_BAD, "Bad Option: %s.\n", cav[0]);
			usage(EX_BAD);
		}
		if (help)
			usage(0);
		if (tracks == 0) {
			if (dummy)
				*flagsp |= F_DUMMY;
			if (msinfo)
				*flagsp |= F_MSINFO;
			if (toc)
				*flagsp |= F_TOC;
			if (multi) {
				*flagsp |= F_MULTI;
				tracktype = TOC_XA2;
				sectype = ST_ROM_MODE2;
			}
			if (fix)
				*flagsp |= F_FIX;

			dummy = msinfo = toc = multi = fix = 0;
		} else if ((dummy + msinfo + toc + multi + fix) > 0)
			comerrno(EX_BAD, "Badly placed option. Global options must be before any track.\n");

		if (nopreemp)
			preemp = 0;
		if (nopad)
			pad = 0;

		if ((audio + data + mode2 + xa1 + xa2 + cdi) > 1) {
			errmsgno(EX_BAD, "Too many types for track %d.\n", tracks+1);
			comerrno(EX_BAD, "Only one of -audio, -data, -mode2, -xa1, -xa2, -cdi allowed.\n");
		}
		if (data) {
			tracktype = TOC_ROM;
			sectype = ST_ROM_MODE1;
		}
		if (mode2) {
			tracktype = TOC_ROM;
			sectype = ST_ROM_MODE2;
		}
		if (audio) {
			tracktype = TOC_DA;
			sectype = preemp ? ST_AUDIO_PRE : ST_AUDIO_NOPRE;
		}
		if (xa1) {
			tracktype = TOC_XA1;
			sectype = ST_ROM_MODE1;
		}
		if (xa2) {
			tracktype = TOC_XA2;
			sectype = ST_ROM_MODE2;
		}
		if (cdi) {
			tracktype = TOC_CDI;
			sectype = ST_ROM_MODE2;
		}
		if (tracks == 0)
			*toctypep = tracktype;

		flags = 0;
		if ((sectype & ST_AUDIOMASK) != 0)
			flags |= TI_AUDIO;
		if (isize) {
			flags |= TI_ISOSIZE;
			if ((*flagsp & F_MULTI) != 0)
				comerrno(EX_BAD, "Cannot get isosize for multi session disks.\n");
		}
		if (preemp)
			flags |= TI_PREEMP;
		if (pad)
			flags |= TI_PAD;
		if (bswab)
			flags |= TI_SWAB;

		if (getfiles(&cac, &cav, opts) == 0)
			break;
		tracks++;

		if (tracks > MAX_TRACK)
			comerrno(EX_BAD, "Track limit (%d) exceeded\n",
								MAX_TRACK);

		if (strcmp("-", cav[0]) == 0) {
			trackp[tracks].f = STDIN_FILENO;
		} else {
			if (access(cav[0], R_OK) < 0)
				comerr("No read access for '%s'.\n", cav[0]);

			if ((trackp[tracks].f = open(cav[0], O_RDONLY)) < 0)
				comerr("Cannot open '%s'.\n", cav[0]);
		}
		trackp[tracks].tracksize = tracksize;
		trackp[tracks].secsize = tracktype == TOC_DA ? AUDIO_SEC_SIZE : DATA_SEC_SIZE;
		trackp[tracks].secspt = 0;	/* transfer size is set up in set_trsizes() */
		trackp[tracks].sectype = sectype;
		trackp[tracks].flags = flags;
		checksize(&trackp[tracks]);
	
		if (debug) {
			printf("File: '%s' tracksize: %d secsize: %d tracktype: %d = %s sectype: %X = %s flags %X\n",
				cav[0],	trackp[tracks].tracksize, 
				trackp[tracks].secsize, 
				tracktype, toctname[tracktype & TOC_MASK],
				sectype, sectname[sectype & ST_MASK], flags);
		}
	}
	if (*speedp < 0)
		comerrno(EX_BAD, "Bad speed option.\n");
	if (!*devp) {
		errmsgno(EX_BAD, "No CD-Recorder device specified.\n");
		usage(EX_BAD);
	}
	if (*flagsp & (F_MSINFO|F_TOC|F_FIX)) {
		if (tracks != 0) {
			errmsgno(EX_BAD, "No tracks allowed with this option\n");
			usage(EX_BAD);
		}
		return;
	}
	if (tracks == 0) {
		errmsgno(EX_BAD, "No tracks specified. Need at least one.\n");
		usage(EX_BAD);
	}
	*tracksp = tracks;
}

LOCAL void
set_trsizes(tracks, trackp)
	int	tracks;
	track_t	*trackp;
{
	int	i;

	/*
	 * We are using SCSI Group 0 write
	 * and cannot write more than 255 secs at once.
	 */
	data_secs_per_tr = bufsize/DATA_SEC_SIZE;
	audio_secs_per_tr = bufsize/AUDIO_SEC_SIZE;
	data_secs_per_tr = min(255, data_secs_per_tr);
	audio_secs_per_tr = min(255, audio_secs_per_tr);

	for (i = 1; i <= tracks; i++) {
		trackp[i].secspt =
			is_audio(&trackp[i]) ?
				audio_secs_per_tr :
				data_secs_per_tr;
	}
}

LOCAL void
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

LOCAL void
check_recovery()
{
	if (recovery_needed()) {
		errmsgno(EX_BAD, "Recovery needed.\n");
		exit(EX_BAD);
	}
}

#define	DEBUG
void audioread()
{
#ifdef	DEBUG
	int speed = 1;
	int dummy = 0;

	speed_select(speed, dummy);
	select_secsize(2352);

	read_scsi(buf, 1000, 1);
	printf("XXX:\n");
	write(1, buf, 512);
	unload_media();
	exit(0);
#endif
}

LOCAL void
print_msinfo()
{
	long	off;
	long	fa;

	if ((off = read_session_offset()) < 0)
		comerr("Cannot read session offset\n");
	if (lverbose)
		printf("session offset: %ld\n", off);
/*	if ((fa = first_writable_addr(0, 0, 0, 1)) < 0)*/
	if ((fa = first_writable_addr(0, 0, 0, 0)) < 0)
		comerr("Cannot read first writable address\n");
	printf("%ld,%ld\n", off, fa);
}

LOCAL void
print_toc()
{
	int	first;
	int	last;
	long	lba;
	struct msf msf;
	int	adr;
	int	control;
	int	mode;
	int	i;

	if (read_tochdr(&first, &last) < 0)
		comerr("Cannot read TOC/PMA\n");
	printf("first: %d last %d\n", first, last);
	for (i = first; i <= last; i++) {
		read_trackinfo(i, &lba, &msf, &adr, &control, &mode);
		print_track(i, lba, &msf, adr, control, mode);
	}
	i = 0xAA;
	read_trackinfo(i, &lba, &msf, &adr, &control, &mode);
	print_track(i, lba, &msf, adr, control, mode);
}

LOCAL void
print_track(track, lba, msp, adr, control, mode)
	int	track;
	long	lba;
	struct msf *msp;
	int	adr;
	int	control;
	int	mode;
{
	printf("track: %3d lba: %9ld (%9ld) %02d:%02d:%02d adr: %X control: %X mode: %d\n",
			track, lba, lba/4,
			msp->msf_min,
			msp->msf_sec,
			msp->msf_frame,
			adr, control, mode);
}

LOCAL void
prtimediff(fmt, start, stop)
	const	char	*fmt;
	struct timeval	*start;
	struct timeval	*stop;
{
	struct timeval tv;

	tv.tv_sec = stop->tv_sec - start->tv_sec;
	tv.tv_usec = stop->tv_usec - start->tv_usec;
	while (tv.tv_usec > 1000000) {
		tv.tv_usec -= 1000000;
		tv.tv_sec += 1;
	}
	while (tv.tv_usec < 0) {
		tv.tv_usec += 1000000;
		tv.tv_sec -= 1;
	}
	/*
	 * We need to cast timeval->* to long because
	 * of the broken sys/time.h in Linux.
	 *
	 * XXX GCC gives warning: precision and `0' flag not both allowed
	 */
	printf("%s%4ld.%03.3lds\n", fmt, (long)tv.tv_sec, (long)tv.tv_usec/1000);
}

#ifdef	HAVE_SYS_PRIOCNTL_H

#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>

LOCAL void
raisepri()
{
	int		pid;
	int		classes;
	int		ret;
	pcinfo_t	info;
	pcparms_t	param;
	rtinfo_t	rtinfo;
	rtparms_t	rtparam;

	pid = getpid();

	/* get info */
	strcpy(info.pc_clname, "RT");
	classes = priocntl(P_PID, pid, PC_GETCID, (void *)&info);
	if (classes == -1)
		comerr("Cannot get priority class id priocntl(PC_GETCID)\n");

	movebytes(info.pc_clinfo, &rtinfo, sizeof(rtinfo_t));
 
	/* set priority to max */
	rtparam.rt_pri = rtinfo.rt_maxpri;
	rtparam.rt_tqsecs = 0;
	rtparam.rt_tqnsecs = RT_TQDEF;
	param.pc_cid = info.pc_cid;
	movebytes(&rtparam, param.pc_clparms, sizeof(rtparms_t));
	ret = priocntl(P_PID, pid, PC_SETPARMS, (void *)&param);
	if (ret == -1)
		comerr("Cannot set priority class parameters priocntl(PC_SETPARMS)\n");
}

#else

LOCAL void
raisepri()
{
	if (setpriority(PRIO_PROCESS, getpid(), -20) < 0)
		comerr("Cannot set priority\n");
}

#endif
