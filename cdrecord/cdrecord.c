/* @(#)cdrecord.c	1.53 98/04/15 Copyright 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)cdrecord.c	1.53 98/04/15 Copyright 1995 J. Schilling";
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
#include <stdxlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>	/* for rlimit */
#include <sys/stat.h>
#include <unixstd.h>
#include <sys/mman.h>
#include <string.h>
#include <waitdefs.h>

#include <scgio.h>		/* XXX wegen inquiry */
#include "scsireg.h"		/* XXX wegen SC_NOT_READY */
#include "cdrecord.h"
#include "auheader.h"
#include "scsitransp.h"
#include "scsi_scan.h"

char	cdr_version[] = "1.6";

/*
 * Map toc/track types into names.
 */
char	*toc2name[] = {
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
char	*st2name[] = {
		"Illegal sector type 0",
		"CD-ROM mode 1",
		"CD-ROM mode 2",
		"Illegal sector type 3",
		"CD-DA without preemphasis",
		"CD-DA with preemphasis",
		"Illegal sector type 6",
		"Illegal sector type 7",
};

/*
 * Map data block types into names.
 */
char	*db2name[] = {
		"Raw (audio)",
		"Raw (audio) with P/Q sub channel",
		"Raw (audio) with P/W sub channel",
		"Raw (audio) with P/W raw sub channel",
		"Reserved mode 4",
		"Reserved mode 5",
		"Reserved mode 6",
		"Vendor unique mode 7",
		"CD-ROM mode 1",
		"CD-ROM mode 2",
		"CD-ROM XA mode 1",
		"CD-ROM XA mode 2 form 1",
		"CD-ROM XA mode 2 form 2",
		"CD-ROM XA mode 2 form 1/2/mix",
		"Reserved mode 14",
		"Vendor unique mode 15",
};

int		debug;		/* print debug messages */
int		verbose;	/* SCSI verbose flag */
int		lverbose;	/* local verbose flag */

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

BOOL	isgui;

struct timeval	starttime;
struct timeval	stoptime;
struct timeval	fixtime;

extern	int	silent;

static	long	fs = -1L;	/* fifo (ring buffer) size */

EXPORT	int 	main		__PR((int ac, char **av));
LOCAL	void	usage		__PR((int));
LOCAL	void	blusage		__PR((int));
EXPORT	int	read_buf	__PR((int f, char *bp, int size));
EXPORT	int	get_buf		__PR((int f, char **bpp, int size));
LOCAL	int	write_track_data __PR((cdr_t *, int , track_t *));
EXPORT	int	pad_track	__PR((cdr_t *dp, int track, track_t *trackp,
				     long startsec, long amt,
				     BOOL dolast, long *bytesp));
LOCAL	void	printdata	__PR((int, track_t *));
LOCAL	void	printaudio	__PR((int, track_t *));
LOCAL	void	checkfile	__PR((int, track_t *));
LOCAL	int	checkfiles	__PR((int, track_t *));
LOCAL	void	setpregaps	__PR((int, track_t *));
LOCAL	long	checktsize	__PR((int, track_t *));
LOCAL	void	checksize	__PR((track_t *));
LOCAL	BOOL	checkdsize	__PR((cdr_t *dp, dstat_t *dsp, long tsize));
LOCAL	void	raise_fdlim	__PR((void));
LOCAL	void	gargs		__PR((int, char **, int *, track_t *, char **,
					int *, cdr_t **,
					int *, long *, int *, int *));
LOCAL	void	set_trsizes	__PR((cdr_t *, int, track_t *));
EXPORT	void	load_media	__PR((cdr_t *));
EXPORT	void	unload_media	__PR((cdr_t *, int));
LOCAL	void	check_recovery	__PR((cdr_t *, int));
	void	audioread	__PR((cdr_t *, int));
LOCAL	void	print_msinfo	__PR((cdr_t *));
LOCAL	void	print_toc	__PR((cdr_t *));
LOCAL	void	print_track	__PR((int, long, struct msf *, int, int, int));
LOCAL	void	prtimediff	__PR((const char *fmt,
					struct timeval *start,
					struct timeval *stop));
#if defined(_POSIX_PRIORITY_SCHEDULING)
LOCAL	int	rt_raisepri	__PR((int));
#endif
EXPORT	void	raisepri	__PR((int));
LOCAL	void	checkgui	__PR((void));
LOCAL	long	number		__PR((char* arg, int* retp));
LOCAL	int	getnum		__PR((char* arg, long* valp));
LOCAL	int	getbltype	__PR((char* optstr, long *typep));

EXPORT int 
main(ac, av)
	int	ac;
	char	*av[];
{
	char	*dev = NULL;
	int	timeout = 40;	/* Set default timeout to 40s CW-7502 is slow*/
	int	speed = 1;
	long	flags = 0L;
	int	toctype = -1;
	int	blanktype = 0;
	int	i;
	int	tracks = 0;
	int	trackno;
	long	tsize;
	track_t	track[MAX_TRACK+1];
	cdr_t	*dp = (cdr_t *)0;
	dstat_t	ds;
	long	startsec = 0L;
	int	errs = 0;

	save_args(ac, av);

	raise_fdlim();
	gargs(ac, av, &tracks, track, &dev, &timeout, &dp, &speed, &flags,
							&toctype, &blanktype);
	if (toctype < 0)
		comerrno(EX_BAD, "Internal error: Bad TOC type.\n");
	if ((flags & F_MSINFO) == 0 || lverbose || flags & F_VERSION)
		printf("Cdrecord release %s Copyright (C) 1995-1998 Jörg Schilling\n",
								cdr_version);
	if (flags & F_VERSION)
		exit(0);

	checkgui();

	if (debug || lverbose) {
		printf("TOC Type: %d = %s\n",
			toctype, toc2name[toctype & TOC_MASK]);
	}

	if ((flags & (F_MSINFO|F_TOC|F_FIX|F_VERSION|F_CHECKDRIVE|F_INQUIRY|F_SCANBUS|F_RESET)) == 0) {
		/*
		 * Try to lock us im memory (will only work for root)
		 * but you need access to root anyway to use /dev/scg?
		 */
#if defined(HAVE_MLOCKALL) || defined(_POSIX_MEMLOCK)
		if (mlockall(MCL_CURRENT|MCL_FUTURE) < 0)
			comerr("Cannot do mlockall(2).\n");
#endif

		raisepri(0); /* max priority */
		init_fifo(fs);
	}

	open_scsi(dev, timeout, (flags & F_MSINFO) == 0 || lverbose);

	bufsize = scsi_bufsize(BUF_SIZE);
	if ((buf = scsi_getbuf(bufsize)) == NULL)
		comerr("Cannot get SCSI I/O buffer.\n");

	if ((flags & F_SCANBUS) != 0) {
		select_target();
		exit(0);
	}
	if ((flags & F_RESET) != 0) {
		scsireset();
		exit(0);
	}
	/*
	 * First try to check which type of SCSI device we
	 * have.
	 */
	silent++;
	test_unit_ready();	/* eat up unit attention */
	silent--;
	if (!do_inquiry((flags & F_MSINFO) == 0 || lverbose)) {
		errmsgno(EX_BAD, "Cannot do inquiry for CD-Recorder.\n");
		if (unit_ready())
			errmsgno(EX_BAD, "The unit seems to be hung and needs power cycling.\n");
		exit(EX_BAD);
	}

	if ((flags & F_PRCAP) != 0) {
		print_capabilities();
		exit(0);
	}
	if ((flags & F_INQUIRY) != 0)
		exit(0);

	if (dp == (cdr_t *)NULL) {	/* No driver= option specified */
		dp = get_cdrcmds();
	} else if (!is_unknown_dev() && dp != get_cdrcmds()) {
		errmsgno(EX_BAD, "WARNING: Trying to use other driver on known device.\n");
	}

	if (!is_cddrive())
		comerrno(EX_BAD, "Sorry, no CD-Drive found on this target.\n");
	if (dp == (cdr_t *)0)
		comerrno(EX_BAD, "Sorry, no supported CD-Recorder found on this target.\n");

	if (((flags & (F_MSINFO|F_TOC|F_LOAD|F_EJECT)) == 0 || tracks > 0) &&
					(dp->cdr_flags & CDR_ISREADER) != 0) {
		comerrno(EX_BAD,
		"Sorry, no CD-Recorder or unsupported CD-Recorder found on this target.\n");
	}

	if ((*dp->cdr_attach)() != 0)
		comerrno(EX_BAD, "Cannot attach driver for CD-Recorder.\n");

	if ((flags & F_MSINFO) == 0 || lverbose) {
		printf("Using %s (%s).\n", dp->cdr_drtext, dp->cdr_drname);
		printf("Driver flags   : ");
		if ((dp->cdr_flags & CDR_SWABAUDIO) != 0)
			printf("SWABAUDIO");
		printf("\n");
	}

	if ((flags & F_CHECKDRIVE) != 0)
		exit(0);

	if (tracks == 0 && (flags & F_EJECT) != 0) {
		/*
		 * Do not check if the unit is ready here to allow to open
		 * an empty unit too.
		 */
		unload_media(dp, flags);
		exit(0);
	}

	flush();
	if (tracks > 1)
		sleep(2);	/* Let the user watch the inquiry messages */

	set_trsizes(dp, tracks, track);
	setpregaps(tracks, track);
	checkfiles(tracks, track);
	tsize = checktsize(tracks, track);

	/*
	 * Is this the right place to do this ?
	 */
	check_recovery(dp, flags);

	load_media(dp);

	if ((flags & F_LOAD) != 0) {
		scsi_prevent_removal(0);	/* allow manual open */
		exit(0);
	}
/*audioread(dp, flags);*/
/*unload_media(dp, flags);*/
/*return 0;*/
	fillbytes(&ds, sizeof(ds), '\0');
	if ((*dp->cdr_getdisktype)(dp, &ds) < 0)
		comerrno(EX_BAD, "Cannot get disk type.\n");
	/*
	 * The next actions should depend on the disk type.
	 */

	if (flags & F_MSINFO) {
		print_msinfo(dp);
		scsi_prevent_removal(0);
		exit(0);
	}
	if (flags & F_TOC) {
		print_toc(dp);
		scsi_prevent_removal(0);
		exit(0);
	}

#ifdef	XXX
	if ((*dp->cdr_check_session)() < 0) {
		unload_media(dp, flags);
		exit(EX_BAD);
	}
#endif
	if (tsize == 0) {
		if (tracks > 0) {
			errmsgno(EX_BAD,
			"WARNING: Track size unknown. Data may not fit on disk.\n");
		}
	} else if (!checkdsize(dp, &ds, tsize) && (flags & F_IGNSIZE) == 0) {
		unload_media(dp, flags);
		exit(EX_BAD);
	}
	if (tracks > 0 && fs > 0l) {
		/*
		 * Start the extra process needed for improved buffering.
		 */
		if (!init_faio(tracks, track, bufsize))
			fs = 0L;
	}
	if ((*dp->cdr_set_speed_dummy)(speed, flags & F_DUMMY) < 0) {
		errmsgno(EX_BAD, "Cannot set speed/dummy.\n");
		unload_media(dp, flags);
		exit(EX_BAD);
	}
	/*
	 * Last chance to quit!
	 */
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
	if (tracks > 0 && fs > 0l) {
		/*
		 * Wait for the read-buffer to become full.
		 * This should be take no extra time if the input is a file.
		 * If the input is a pipe (e.g. mkisofs) this can take a
		 * while. If mkisofs dumps core before it starts writing,
		 * we abort before the writing process started.
		 */
		if (!await_faio()) {
			errmsgno(EX_BAD, "Input buffer error, aborting.\n");
			unload_media(dp, flags);
			exit(EX_BAD);
		}
	}
	if (gettimeofday(&starttime, (struct timezone *)0) < 0)
		errmsg("Cannot get start time\n");

	/*
	 * Blank the media if we were requested to do so
	 */
	if (flags & F_BLANK) {
		if ((*dp->cdr_blank)(0L, blanktype) < 0) {
			errmsgno(EX_BAD, "Cannot blank disk, aborting.\n");
			unload_media(dp, flags);
			exit(EX_BAD);
		}
		if (tracks == 0) {
			scsi_prevent_removal(0);
			exit(0);
		}
	}
	/*
	 * Get the number of the next recordable track.
	 */
	silent++;
	if (read_tochdr(dp, NULL, &trackno) < 0) {
		trackno = 0;
	}
	silent--;
	for (i = 1; i <= tracks; i++) {
		track[i].trackno = i + trackno;
	}
	trackno++;
	track[0].trackno = trackno;	/* XXX Hack for TEAC fixate */

	/*
	 * Now we actually start writing to the CD.
	 * XXX Check total size of the tracks and remaining size of disk.
	 */
	if ((*dp->cdr_open_session)(tracks, track, toctype, flags & F_MULTI) < 0) {
		errmsgno(EX_BAD, "Cannot open new session.\n");
		unload_media(dp, flags);
		exit(EX_BAD);
	}

	/*
	 * As long as open_session() will do nothing but
	 * set up parameters, we may leave fix_it here.
	 * I case we have to add an open_session() for a drive
	 * that wants to do something that modifies the disk
	 * We have to think about a new solution.
	 */
	if (flags & F_FIX)
		goto fix_it;

	/*
	 * Need to set trackno to the real value from
	 * the current disk status.
	 */
	for (i = 1; i <= tracks; i++, trackno++) {
		startsec = 0L;

		/*
		 * trackno is the "real" track number while 'i' is a counter
		 * going from 1 to tracks.
		 */
		if ((*dp->cdr_open_track)(dp, trackno, &track[i]) < 0) {
			errs++;
			break;
		}

		if ((*dp->cdr_next_wr_address)(trackno, &track[i], &startsec) < 0) {
			errs++;
			break;
		}
		track[i].trackstart = startsec;
		if (debug || lverbose)
			printf("Starting new track at sector: %ld\n", startsec);

		if (write_track_data(dp, trackno, &track[i]) < 0) {
			errs++;
			sleep(5);
			request_sense();
			(*dp->cdr_close_track)(trackno, &track[i]);
			break;
		}
		if ((*dp->cdr_close_track)(trackno, &track[i]) < 0) {
			/*
			 * Check for "Dummy blocks added" message first.
			 */
			if (scsi_sense_key() != SC_ILLEGAL_REQUEST ||
					scsi_sense_code() != 0xB5) {
				errs++;
				break;
			}
		}
	}
fix_it:
	if (gettimeofday(&stoptime, (struct timezone *)0) < 0)
		errmsg("Cannot get stop time\n");
	if (lverbose)
		prtimediff("Writing  time: ", &starttime, &stoptime);

	if ((flags & F_NOFIX) == 0) {
		if (lverbose) {
			printf("Fixating...\n");
			flush();
		}
		if ((*dp->cdr_fixate)(flags & F_MULTI, flags & F_DUMMY,
				toctype, tracks, track) < 0)
			errs++;

		if (gettimeofday(&fixtime, (struct timezone *)0) < 0)
			errmsg("Cannot get fix time\n");
		if (lverbose)
			prtimediff("Fixating time: ", &stoptime, &fixtime);
	}

	unload_media(dp, flags);

#ifdef	FIFO
	if (tracks > 0 && fs > 0L) {
		kill_faio();
		wait(0);
		if (debug || lverbose)
			fifo_stats();
	}
#endif

	exit(errs?-2:0);
	return (0);
}

LOCAL void 
usage(excode)
	int excode;
{
	errmsgno(EX_BAD, "Usage: %s [options] track1...trackn\n",
		get_progname());

	error("Options:\n");
	error("\t-version	print version information and exit\n");
	error("\t-v		increment general verbose level by one\n");
	error("\t-V		increment SCSI command transport verbose level by one\n");
	error("\t-debug		print additional debug messages\n");
	error("\tdev=target	SCSI target to use as CD-Recorder\n");
	error("\ttimeout=#	set the default SCSI command timeout to #.\n");
	error("\tdriver=name	user supplied driver name, use with extreme care\n");
	error("\t-checkdrive	check if a driver for the drive is present\n");
	error("\t-prcap		print drive capabilities for MMC compliant drives\n");
	error("\t-inq		do an inquiry for the drive end exit\n");
	error("\t-scanbus	scan the SCSI bus end exit\n");
	error("\t-reset		reset the SCSI bus with the cdrecorder (if possible)\n");
	error("\t-ignsize	ignore the known size of a medium (may cause problems)\n");
	error("\tspeed=#		set speed of drive\n");
	error("\tblank=type	blank a CD-RW disc (see blank=help)\n");
#ifdef	FIFO
	error("\tfs=#		Set fifo size to # (0 to disable, default is %ld MB)\n",
							DEFAULT_FIFOSIZE/(1024*1024));
#endif
	error("\t-load		load the disk and exit (works only with tray loader)\n");
	error("\t-eject		eject the disk after doing the work\n");
	error("\t-dummy		do everything with laser turned off\n");
	error("\t-msinfo		retrieve multi-session info for mkisofs >= 1.10\n");
	error("\t-toc		retrieve and print TOC/PMA data\n");
	error("\t-multi		generate a TOC that allows multi session\n");
	error("\t		In this case default track type is CD-ROM XA2\n");
	error("\t-fix		fixate a corrupt or unfixated disk (generate a TOC)\n");
	error("\t-nofix		do not fixate disk after writing tracks\n");
	error("\tbytes=#		Length of valid track data in bytes (obsolete)\n");
	error("\ttsize=#		Length of valid data in next track\n");
	error("\tpadsize=#	Amount of padding for next track\n");
	error("\tpregap=#	Amount of pre-gap sectors before next track\n");
	error("\tdefpregap=#	Amount of pre-gap sectors for all but track #1\n");

	error("\t-audio		Subsequent tracks are CD-DA audio tracks\n");
	error("\t-data		Subsequent tracks are CD-ROM data mode 1 (default)\n");
	error("\t-mode2		Subsequent tracks are CD-ROM data mode 2\n");
	error("\t-xa1		Subsequent tracks are CD-ROM XA mode 1\n");
	error("\t-xa2		Subsequent tracks are CD-ROM XA mode 2\n");
	error("\t-cdi		Subsequent tracks are CDI tracks\n");
	error("\t-isosize	Use iso9660 file system size for next data track\n");
	error("\t-preemp		Audio tracks are mastered with 50/15 µs preemphasis\n");
	error("\t-nopreemp	Audio tracks are mastered with no preemphasis (default)\n");
	error("\t-pad		Pad data tracks with %d zeroed sectors\n", PAD_SECS);
	error("\t		Pad audio tracks to a multiple of %d bytes\n", AUDIO_SEC_SIZE);
	error("\t-nopad		Do not pad data tracks (default)\n");
	error("\t-swab		Audio data source is byte-swapped (little-endian/Intel)\n");
	error("The type of the first track is used for the toc type.\n");
	error("Currently only form 1 tracks are supported.\n");
	exit(excode);
}

LOCAL void
blusage(ret)
	int	ret;
{
	error("Blanking options:\n");
	error("\tall\t\tblank the entire disk\n");
	error("\tdisc\t\tblank the entire disk\n");
	error("\tdisk\t\tblank the entire disk\n");
	error("\tfast\t\tminimally blank the entire disk (PMA, TOC, pregap)\n");
	error("\tminimal\t\tminimally blank the entire disk (PMA, TOC, pregap)\n");
	error("\ttrack\t\tblank a track\n");
	error("\tunreserve\tunreserve a track\n");
	error("\ttrtail\t\tblank a track tail\n");
	error("\tunclose\t\tunclose last session\n");
	error("\tsession\t\tblank last session\n");

	exit(ret);
	/* NOTREACHED */
}

EXPORT int
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

EXPORT int
get_buf(f, bpp, size)
	int	f;
	char	**bpp;
	int	size;
{
	if (fs > 0) {
/*		return (faio_read_buf(f, *bpp, size));*/
		return (faio_get_buf(f, bpp, size));
	} else {
		return (read_buf(f, *bpp, size));
	}
}

LOCAL int
write_track_data(dp, track, trackp)
	cdr_t	*dp;
	int	track;
	track_t	*trackp;
{
	int	f;
	int	isaudio;
	long	startsec;
	long	bytes_read = 0;
	long	bytes	= 0;
	long	savbytes = 0;
	int	count;
	long	tracksize;
	int	secsize;
	int	secspt;
	int	bytespt;
	int	bytes_to_read;
	long	amount;
	int	pad;
	int	bswab;
	BOOL	neednl	= FALSE;
	BOOL	islast	= FALSE;
	char	*bp	= buf;

	if (is_packet(trackp))	/* XXX Ugly hack for now */
		return (write_packet_data(dp, track, trackp));

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
			printf("Track %02d:   0 of %3ld MB written.\r",
			       track, tracksize >> 20);
		else
			printf("Track %02d:   0 MB written.\r", track);
		flush();
		neednl = TRUE;
	}

	do {
		bytes_to_read = bytespt;
		if (tracksize > 0) {
			bytes_to_read = tracksize - bytes_read;
			if (bytes_to_read > bytespt)
				bytes_to_read = bytespt;
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
			bytespt = count;
			secspt = count / secsize;
			if (trackp->padsize == 0 || (bytes_read/secsize) >= 300)
				islast = TRUE;
		}

		amount = (*dp->cdr_write_trackdata)(bp, startsec, bytespt, secspt, islast);
		if (amount < 0) {
			printf("%swrite track data: error after %ld bytes\n",
							neednl?"\n":"", bytes);
			return (-1);
		}
		bytes += amount;
		startsec += amount / secsize;

		if (lverbose && (bytes >= (savbytes + 0x100000))) {
			int	fper;

			printf("Track %02d: %3ld", track, bytes >> 20);
			if (tracksize > 0)
				printf(" of %3ld MB", tracksize >> 20);
			else
				printf(" MB");
			printf(" written");
			fper = fifo_percent(TRUE);
			if (fper >= 0)
				printf(" (fifo %3d%%)", fper);
			printf(".\r");
			savbytes = (bytes >> 20) << 20;
			flush();
			neednl = TRUE;
		}
	} while (tracksize < 0 || bytes_read < tracksize);

	if ((bytes / secsize) < 300) {
		amount = roundup(trackp->padsize, secsize);
		if (((bytes+amount) / secsize) < 300)
			trackp->padsize = 300 * secsize - bytes;
	}
	if (trackp->padsize) {
		if ((trackp->padsize >> 20) > 0) {
			if (neednl)
				printf("\n");
			neednl = TRUE;
		} else if (lverbose) {
			printf("Track %02d: writing %3ld KB of pad data.\n",
						track, trackp->padsize >> 10);
			neednl = FALSE;
		}
		pad_track(dp, track, trackp, startsec, trackp->padsize,
					TRUE, &amount);
		bytes += amount;
		startsec += amount / secsize;
	}
	printf("%sTrack %02d: Total bytes read/written: %ld/%ld (%ld sectors).\n",
	       neednl?"\n":"", track, bytes_read, bytes, bytes/secsize);
	return 0;
}

EXPORT int
pad_track(dp, track, trackp, startsec, amt, dolast, bytesp)
	cdr_t	*dp;
	int	track;
	track_t	*trackp;
	long	startsec;
	long	amt;
	BOOL	dolast;
	long	*bytesp;
{
	long	bytes	= 0;
	long	savbytes = 0;
	int	secsize;
	int	secspt;
	int	bytespt;
	int	amount;
	BOOL	neednl	= FALSE;
	BOOL	islast	= FALSE;

	secsize = trackp->secsize;
	secspt = trackp->secspt;
	bytespt = secsize * secspt;
	
	fillbytes(buf, bytespt, '\0');

	if ((amt >> 20) > 0) {
		printf("Track %02d:   0 of %3ld MB pad written.\r",
						track, amt >> 20);
		flush();
	}
	do {
		if (amt < bytespt) {
			bytespt = roundup(amt, secsize);
			secspt = bytespt / secsize;	
		}
		if (dolast && (amt - bytespt) <= 0)
			islast = TRUE;

		amount = (*dp->cdr_write_trackdata)(buf, startsec, bytespt, secspt, islast);
		if (amount < 0) {
			printf("%swrite track data: error after %ld bytes\n",
							neednl?"\n":"", bytes);
			if (bytesp)
				*bytesp = bytes;
			return (-1);
		}
		amt -= amount;
		bytes += amount;
		startsec += amount / secsize;

		if (lverbose && (bytes >= (savbytes + 0x100000))) {
			printf("Track %02d: %3ld\r", track, bytes >> 20);
			savbytes = (bytes >> 20) << 20;
			flush();
			neednl = TRUE;
		}
	} while (amt > 0);

	if (bytesp)
		*bytesp = bytes;
	return (bytes);
}

LOCAL void
printdata(track, trackp)
	int	track;
	track_t	*trackp;
{
	if (trackp->tracksize >= 0) {
		printf("Track %02d: data  %3ld MB        ",
					track, trackp->tracksize >> 20);
	} else {
		printf("Track %02d: data  unknown length",
					track);
	}
	if (trackp->padsize > 0) {
		if ((trackp->padsize >> 20) > 0)
			printf(" padsize: %3ld MB", trackp->padsize >> 20);
		else
			printf(" padsize: %3ld KB", trackp->padsize >> 10);
	}
	if (trackp->pregapsize != 150) {
		printf(" pregapsize: %3ld", trackp->pregapsize);
	}
	printf("\n");
}

LOCAL void
printaudio(track, trackp)
	int	track;
	track_t	*trackp;
{
	if (trackp->tracksize >= 0) {
		printf("Track %02d: audio %3ld MB (%02d:%02d.%02d) %spreemp%s%s",
			track, trackp->tracksize >> 20,
			minutes(trackp->tracksize),
			seconds(trackp->tracksize),
			hseconds(trackp->tracksize),
			is_preemp(trackp) ? "" : "no ",
			is_swab(trackp) ? " swab":"",
			((trackp->tracksize < 300L*trackp->secsize) ||
			(trackp->tracksize % trackp->secsize)) &&
			is_pad(trackp) ? " pad" : "");
	} else {
		printf("Track %02d: audio unknown length    %spreemp%s%s",
			track, is_preemp(trackp) ? "" : "no ",
			is_swab(trackp) ? " swab":"",
			(trackp->tracksize % trackp->secsize) && is_pad(trackp) ? " pad" : "");
	}
	if (trackp->padsize > 0) {
		if ((trackp->padsize >> 20) > 0)
			printf(" padsize: %3ld MB", trackp->padsize >> 20);
		else
			printf(" padsize: %3ld KB", trackp->padsize >> 10);
		printf(" (%02d:%02d.%02d)",
			minutes(trackp->padsize),
			seconds(trackp->padsize),
			hseconds(trackp->padsize));
	}
	if (trackp->pregapsize != 150) {
		printf(" pregapsize: %3ld", trackp->pregapsize);
	}
	printf("\n");
}

LOCAL void
checkfile(track, trackp)
	int	track;
	track_t	*trackp;
{
	if (trackp->tracksize > 0 &&
			is_audio(trackp) &&
			((trackp->tracksize < 300L*trackp->secsize) ||
			(trackp->tracksize % trackp->secsize)) &&
						!is_pad(trackp)) {
		errmsgno(EX_BAD, "Bad audio track size %ld for track %02d.\n",
				trackp->tracksize, track);
		comerrno(EX_BAD, "Audio tracks must be at least %ld bytes and a multiple of %d.\n",
				300L*trackp->secsize, trackp->secsize);
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
setpregaps(tracks, trackp)
	int	tracks;
	track_t	*trackp;
{
	int	i;
	int	sectype;
	track_t	*tp;

	sectype = trackp[1].sectype;

	for (i = 1; i <= tracks; i++) {
		tp = &trackp[i];
		if (tp->pregapsize == -1L) {
			tp->pregapsize = 150;		/* Default Pre GAP */
			if (sectype != tp->sectype) {
				tp->pregapsize = 255;	/* Pre GAP is 255 */
			}
		}
		sectype = tp->sectype;			/* Save old sectype */
	}
}

LOCAL long
checktsize(tracks, trackp)
	int	tracks;
	track_t	*trackp;
{
	int	i;
	long	curr;
	long	total = 0;
	long	btotal;
	track_t	*tp;

	for (i = 1; i <= tracks; i++) {
		tp = &trackp[i];
		if (tp->tracksize >=0) {
			curr = (tp->tracksize + (tp->secsize-1)) / tp->secsize;
			curr += (tp->padsize + (tp->secsize-1)) / tp->secsize;

			if (curr < 300)		/* Minimum track size is 4s */
				curr = 300;
			if (!is_audio(tp))
				curr += 2;
			if (i > 1)
				curr += tp->pregapsize;
			total += curr;
		}
	}
	if (!lverbose)
		return (total);

	btotal = total * 2352;
/* XXX Sector Size ??? */
	if (tracks > 0) {
		printf("Total size:     %3ld MB (%02d:%02d.%02d) = %ld sectors\n",
			btotal >> 20,
			minutes(btotal),
			seconds(btotal),
			hseconds(btotal), total);
		btotal += 150 * 2352;
		printf("Lout start:     %3ld MB (%02d:%02d/%02d) = %ld sectors\n",
			btotal >> 20,
			minutes(btotal),
			seconds(btotal),
			frames(btotal), total);
	}
	return (total);
}

LOCAL void
checksize(trackp)
	track_t	*trackp;
{
	struct stat	st;

	/*
	 * If the current input file is a regular file and
	 * 'padsize=' has not been specified,
	 * use fstat() or file parser to get the size of the file.
	 */
	if (trackp->tracksize < 0 && (trackp->flags & TI_ISOSIZE) != 0) {
		trackp->tracksize = isosize(trackp->f);
	}
	if (trackp->tracksize < 0 && (trackp->flags & TI_NOAUHDR) == 0) {
		trackp->tracksize = ausize(trackp->f);
	}
	if (trackp->tracksize < 0 && (trackp->flags & TI_NOAUHDR) == 0) {
		trackp->tracksize = wavsize(trackp->f);
		if (trackp->tracksize > 0)	/* Force little endian input */
			trackp->flags |= TI_SWAB;
	}
	if (trackp->tracksize == AU_BAD_CODING) {
		comerrno(EX_BAD, "Inappropriate audio coding in '%s'.\n",
							trackp->filename);
	}
	if (trackp->tracksize < 0 &&
			fstat(trackp->f, &st) >= 0 && S_ISREG(st.st_mode)) {
		trackp->tracksize = st.st_size;
	}
}

LOCAL BOOL
checkdsize(dp, dsp, tsize)
	cdr_t	*dp;
	dstat_t	*dsp;
	long	tsize;
{
	long	startsec = 0L;

	silent++;
	(*dp->cdr_next_wr_address)(/*i*/ 0, (track_t *)0, &startsec);
	silent--;

	if (dsp->ds_maxblocks > 0) {
		if (lverbose)
			printf("Blocks total: %ld Blocks remaining: %ld\n",
					dsp->ds_maxblocks,
					dsp->ds_maxblocks - startsec);

		if (startsec + tsize > dsp->ds_maxblocks) {
			errmsgno(EX_BAD,
			"WARNING: Data may not fit on current disk.\n");

			/* XXX Check for flags & CDR_NO_LOLIMIT */
/*			return (FALSE);*/
		}
		if (lverbose && dsp->ds_maxrblocks > 0)
			printf("RBlocks total: %ld RBlocks remaining: %ld\n",
					dsp->ds_maxrblocks,
					dsp->ds_maxrblocks - startsec);
		if (dsp->ds_maxrblocks > 0 && startsec + tsize > dsp->ds_maxrblocks) {
			errmsgno(EX_BAD,
			"Data does not fit on current disk.\n");
			return (FALSE);
		}
	} else {
		if (startsec + tsize >= (405000-301)) {		/*<90 min disk*/
			errmsgno(EX_BAD,
				"Data will not fit on any disk.\n");
			return (FALSE);
		} else if (startsec + tsize >= (333000-150)) {	/* 74 min disk*/
			errmsgno(EX_BAD,
			"WARNING: Data may not fit on standard 74min disk.\n");
		}
	}
	return (TRUE);
}

LOCAL void
raise_fdlim()
{
#ifdef	RLIMIT_NOFILE

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

#endif	/* RLIMIT_NOFILE */
}

char	*opts =
"help,version,checkdrive,prcap,inq,scanbus,reset,ignsize,dev*,timeout#,driver*,bytes#,tsize&,padsize&,pregap&,defpregap&,speed#,load,eject,dummy,msinfo,toc,multi,fix,nofix,debug,v+,V+,audio,data,mode2,xa1,xa2,cdi,isosize,nopreemp,preemp,nopad,pad,swab,fs&,blank&,pktsize#,packet,noclose";

LOCAL void
gargs(ac, av, tracksp, trackp, devp, timeoutp, dpp, speedp, flagsp, toctypep, blankp)
	int	ac;
	char	**av;
	int	*tracksp;
	track_t	*trackp;
	cdr_t	**dpp;
	char	**devp;
	int	*timeoutp;
	int	*speedp;
	long	*flagsp;
	int	*toctypep;
	int	*blankp;
{
	int	cac;
	char	* const*cav;
	char	*driver = NULL;
	long	bltype = -1;
	long	tracksize;
	long	padsize;
	long	pregapsize;
	long	defpregap = -1L;
	long	secsize;
	int	pktsize;
	int	speed = -1;
	int	help = 0;
	int	version = 0;
	int	checkdrive = 0;
	int	prcap = 0;
	int	inq = 0;
	int	scanbus = 0;
	int	reset = 0;
	int	ignsize = 0;
	int	load = 0;
	int	eject = 0;
	int	dummy = 0;
	int	msinfo = 0;
	int	toc = 0;
	int	multi = 0;
	int	fix = 0;
	int	nofix = 0;
	int	audio;
	int	data;
	int	mode2;
	int	xa1;
	int	xa2;
	int	cdi;
	int	isize;
	int	ispacket = 0;
	int	noclose = 0;
	int	preemp = 0;
	int	nopreemp;
	int	pad = 0;
	int	bswab = 0;
	int	nopad;
	int	flags;
	int	tracks = *tracksp;
	int	tracktype = TOC_ROM;
	int	sectype = ST_ROM_MODE1;
	int	dbtype = DB_ROM_MODE1;

	cac = --ac;
	cav = ++av;
	for (;; cac--,cav++) {
		tracksize = -1L;
		padsize = 0L;
		pregapsize = defpregap;
		audio = data = mode2 = xa1 = xa2 = cdi = 0;
		isize = nopreemp = nopad = 0;
		pktsize = 0;
		if (getargs(&cac, &cav, opts,
				&help, &version, &checkdrive, &prcap,
				&inq, &scanbus, &reset, &ignsize,
				devp, timeoutp, &driver,
				&tracksize,
				getnum, &tracksize,
				getnum, &padsize,
				getnum, &pregapsize,
				getnum, &defpregap,
				&speed,
				&load, &eject, &dummy, &msinfo, &toc,
				&multi, &fix, &nofix,
				&debug, &lverbose, &verbose,
				&audio, &data, &mode2,
				&xa1, &xa2, &cdi,
				&isize,
				&nopreemp, &preemp,
				&nopad, &pad, &bswab, getnum, &fs,
				getbltype, &bltype, &pktsize,
				&ispacket, &noclose) < 0) {
			errmsgno(EX_BAD, "Bad Option: %s.\n", cav[0]);
			usage(EX_BAD);
		}
		if (help)
			usage(0);
		if (tracks == 0) {
			if (driver)
				set_cdrcmds(driver, dpp);
			if (version)
				*flagsp |= F_VERSION;
			if (checkdrive)
				*flagsp |= F_CHECKDRIVE;
			if (prcap)
				*flagsp |= F_PRCAP;
			if (inq)
				*flagsp |= F_INQUIRY;
			if (scanbus)
				*flagsp |= F_SCANBUS;
			if (reset)
				*flagsp |= F_RESET;
			if (ignsize)
				*flagsp |= F_IGNSIZE;
			if (load)
				*flagsp |= F_LOAD;
			if (eject)
				*flagsp |= F_EJECT;
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
				dbtype = DB_XA_MODE2;
			}
			if (fix)
				*flagsp |= F_FIX;
			if (nofix)
				*flagsp |= F_NOFIX;
			if (bltype >= 0) {
				*flagsp |= F_BLANK;
				*blankp = bltype;
			}
			version = checkdrive = prcap = inq = scanbus = reset = ignsize =
			load = eject = dummy = msinfo = toc = multi = fix = nofix = 0;
		} else if ((version + checkdrive + prcap + inq + scanbus + reset + ignsize +
			    load + eject + dummy + msinfo + toc + multi + fix + nofix) > 0)
			comerrno(EX_BAD, "Badly placed option. Global options must be before any track.\n");

		if (nopreemp)
			preemp = 0;
		if (nopad)
			pad = 0;

		if ((audio + data + mode2 + xa1 + xa2 + cdi) > 1) {
			errmsgno(EX_BAD, "Too many types for track %d.\n", tracks+1);
			comerrno(EX_BAD, "Only one of -audio, -data, -mode2, -xa1, -xa2, -cdi allowed.\n");
		}
		if (ispacket && audio) {
			comerrno(EX_BAD, "Audio data cannot be written in packet mode.\n");
		}
		if (data) {
			tracktype = TOC_ROM;
			sectype = ST_ROM_MODE1;
			dbtype = DB_ROM_MODE1;
		}
		if (mode2) {
			tracktype = TOC_ROM;
			sectype = ST_ROM_MODE2;
			dbtype = DB_ROM_MODE2;
		}
		if (audio) {
			tracktype = TOC_DA;
			sectype = preemp ? ST_AUDIO_PRE : ST_AUDIO_NOPRE;
			dbtype = DB_RAW;
		}
		if (xa1) {
			tracktype = TOC_XA1;
			sectype = ST_ROM_MODE1;
			dbtype = DB_XA_MODE1;
		}
		if (xa2) {
			tracktype = TOC_XA2;
			sectype = ST_ROM_MODE2;
			dbtype = DB_XA_MODE2_F1;
		}
		if (cdi) {
			tracktype = TOC_CDI;
			sectype = ST_ROM_MODE2;
			dbtype = DB_XA_MODE2_F1;
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

		if ((flags & TI_AUDIO) == 0 && padsize > 0L)
			pad = TRUE;
		if (pad) {
			flags |= TI_PAD;
			if ((flags & TI_AUDIO) == 0 && padsize == 0L)
				padsize = PAD_SIZE;
		}
		if (bswab)
			flags |= TI_SWAB;
		if (ispacket) 
			flags |= TI_PACKET;
		if (noclose) 
			flags |= TI_NOCLOSE;

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
		if (!is_auname(cav[0]) && !is_wavname(cav[0]))
			flags |= TI_NOAUHDR;
		if (tracks == 1 && (pregapsize != -1L && pregapsize != 150))
			pregapsize = -1L;
		secsize = tracktype == TOC_DA ? AUDIO_SEC_SIZE : DATA_SEC_SIZE;
		trackp[tracks].filename = cav[0];;
		trackp[tracks].trackstart = 0L;
		trackp[tracks].tracksize = tracksize;
		trackp[tracks].pregapsize = pregapsize;
		trackp[tracks].padsize = padsize;
		trackp[tracks].secsize = secsize;
		trackp[tracks].secspt = 0;	/* transfer size is set up in set_trsizes() */
		trackp[tracks].pktsize = pktsize;
		trackp[tracks].trackno = tracks;
		trackp[tracks].sectype = sectype;
		trackp[tracks].tracktype = tracktype;
		trackp[tracks].dbtype = dbtype;
		trackp[tracks].flags = flags;
		checksize(&trackp[tracks]);
		tracksize = trackp[tracks].tracksize;
		if (tracksize > 0 && (tracksize / secsize) < 300) {
			tracksize = roundup(tracksize, secsize);
			padsize = tracksize + roundup(padsize, secsize);
			if ((padsize / secsize) < 300) {
				trackp[tracks].padsize =
					300 * secsize - tracksize;
			}
		}
		if (debug) {
			printf("File: '%s' tracksize: %ld secsize: %d tracktype: %d = %s sectype: %X = %s dbtype: %s flags %X\n",
				cav[0],	trackp[tracks].tracksize, 
				trackp[tracks].secsize, 
				tracktype, toc2name[tracktype & TOC_MASK],
				sectype, st2name[sectype & ST_MASK], db2name[dbtype], flags);
		}
	}
	if (fs < 0L && fs != -1L)
		comerrno(EX_BAD, "Bad fifo size option.\n");
	if (fs < 0L) {
		char	*p = getenv("CDR_FIFOSIZE");

		if (p) {
			if (getnum(p, &fs) != 1)
				comerrno(EX_BAD, "Bad fifo size environment.\n");
		}
	}
	if (fs < 0L)
		fs = DEFAULT_FIFOSIZE;
	if (speed < 0 && speed != -1)
		comerrno(EX_BAD, "Bad speed option.\n");
	if (speed < 0) {
		char	*p = getenv("CDR_SPEED");

		if (p) {
			speed = atoi(p);
			if (speed < 0)
				comerrno(EX_BAD, "Bad speed environment.\n");
		}
	}
	if (speed >= 0)
		*speedp = speed;
	if (!*devp)
		*devp = getenv("CDR_DEVICE");
	if (!*devp && (*flagsp & (F_VERSION|F_SCANBUS)) == 0) {
		errmsgno(EX_BAD, "No CD-Recorder device specified.\n");
		usage(EX_BAD);
	}
	if (*flagsp & (F_LOAD|F_MSINFO|F_TOC|F_FIX|F_VERSION|F_CHECKDRIVE|F_PRCAP|F_INQUIRY|F_SCANBUS|F_RESET)) {
		if (tracks != 0) {
			errmsgno(EX_BAD, "No tracks allowed with this option\n");
			usage(EX_BAD);
		}
		return;
	}
	if (tracks == 0 && (*flagsp & (F_LOAD|F_EJECT|F_BLANK)) == 0) {
		errmsgno(EX_BAD, "No tracks specified. Need at least one.\n");
		usage(EX_BAD);
	}
	*tracksp = tracks;
}

LOCAL void
set_trsizes(dp, tracks, trackp)
	cdr_t	*dp;
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

	trackp[1].flags		|= TI_FIRST;
	trackp[tracks].flags	|= TI_LAST;
	
	for (i = 1; i <= tracks; i++) {
		trackp[i].secspt =
			is_audio(&trackp[i]) ?
				audio_secs_per_tr :
				data_secs_per_tr;
		if (is_packet(&trackp[i]) && trackp[i].pktsize > 0) {
			if (trackp[i].secspt >= trackp[i].pktsize) {
				trackp[i].secspt = trackp[i].pktsize;
			} else {
				comerrno(EX_BAD,
					"Track %d packet size %d exceeds buffer limit of %d sectors",
					i, trackp[i].pktsize, trackp[i].secspt);
			}
		}
		if ((dp->cdr_flags & CDR_SWABAUDIO) != 0 &&
					is_audio(&trackp[i])) {
			trackp[i].flags ^= TI_SWAB;
		}
	}
}

EXPORT void
load_media(dp)
	cdr_t	*dp;
{
	register int	i;

	/*
	 * Do some preparation before...
	 */
	silent++;			/* Be quiet if this fails	*/
	(*dp->cdr_load)();		/* First try to load media and	*/
	scsi_start_stop_unit(1, 0);	/* start unit in silent mode	*/
	silent--;

	if (!wait_unit_ready(60)) {
		i = scsi_sense_code();
		if (scsi_sense_key() == SC_NOT_READY && (i == 0x3A || i == 0x30))
			comerrno(EX_BAD, "No disk / Wrong disk!\n");
		comerrno(EX_BAD, "CD-Recorder not ready.\n");
	}

	scsi_prevent_removal(1);
	scsi_start_stop_unit(1, 0);
	wait_unit_ready(120);
	silent++;
	rezero_unit();	/* Is this needed? Not supported by some drvives */
	silent--;
	test_unit_ready();
	scsi_start_stop_unit(1, 0);
}

EXPORT void
unload_media(dp, flags)
	cdr_t	*dp;
	int	flags;
{
	scsi_prevent_removal(0);
	if ((flags & F_EJECT) != 0)
		(*dp->cdr_unload)();
}

LOCAL void
check_recovery(dp, flags)
	cdr_t	*dp;
	int	flags;
{
	if ((*dp->cdr_check_recovery)()) {
		errmsgno(EX_BAD, "Recovery needed.\n");
		unload_media(dp, flags);
		exit(EX_BAD);
	}
}

#define	DEBUG
void audioread(dp, flags)
	cdr_t	*dp;
	int	flags;
{
#ifdef	DEBUG
	int speed = 1;
	int dummy = 0;

	if ((*dp->cdr_set_speed_dummy)(speed, dummy) < 0)
		exit(-1);
	if ((*dp->cdr_set_secsize)(2352) < 0)
		exit(-1);

	read_scsi(buf, 1000, 1);
	printf("XXX:\n");
	write(1, buf, 512);
	unload_media(dp, flags);
	exit(0);
#endif
}

LOCAL void
print_msinfo(dp)
	cdr_t	*dp;
{
	long	off;
	long	fa;

	if ((*dp->cdr_session_offset)(&off) < 0) {
		errmsgno(EX_BAD, "Cannot read session offset\n");
		return;
	}
	if (lverbose)
		printf("session offset: %ld\n", off);

	if (dp->cdr_next_wr_address(0, (track_t *)0, &fa) < 0) {
		errmsgno(EX_BAD, "Cannot read first writable address\n");
		return;
	}
	printf("%ld,%ld\n", off, fa);
}

LOCAL void
print_toc(dp)
	cdr_t	*dp;
{
	int	first;
	int	last;
	long	lba;
	struct msf msf;
	int	adr;
	int	control;
	int	mode;
	int	i;
extern	struct scsi_capacity	cap;

	silent++;
	if (read_capacity() < 0) {
		silent--;
		errmsgno(EX_BAD, "Cannot read capacity\n");
		return;
	}
	silent--;
	if (read_tochdr(dp, &first, &last) < 0) {
		errmsgno(EX_BAD, "Cannot read TOC/PMA\n");
		return;
	}
	printf("first: %d last %d\n", first, last);
	for (i = first; i <= last; i++) {
		read_trackinfo(i, &lba, &msf, &adr, &control, &mode);
		if (cap.c_bsize == 512)
			lba /= 4;
		print_track(i, lba, &msf, adr, control, mode);
	}
	i = 0xAA;
	read_trackinfo(i, &lba, &msf, &adr, &control, &mode);
	if (cap.c_bsize == 512)
		lba /= 4;
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
	long	lba_512 = lba*4;

	if (track == 0xAA)
		printf("track:lout ");
	else
		printf("track: %3d ", track);

	printf("lba: %9ld (%9ld) %02d:%02d:%02d adr: %X control: %X mode: %d\n",
			lba, lba_512,
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
	 */
	printf("%s%4ld.%03lds\n", fmt, (long)tv.tv_sec, (long)tv.tv_usec/1000);
}

#ifdef	HAVE_SYS_PRIOCNTL_H

#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>

EXPORT	void
raisepri(pri)
	int pri;
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
	rtparam.rt_pri = rtinfo.rt_maxpri - pri;
	rtparam.rt_tqsecs = 0;
	rtparam.rt_tqnsecs = RT_TQDEF;
	param.pc_cid = info.pc_cid;
	movebytes(&rtparam, param.pc_clparms, sizeof(rtparms_t));
	ret = priocntl(P_PID, pid, PC_SETPARMS, (void *)&param);
	if (ret == -1)
		comerr("Cannot set priority class parameters priocntl(PC_SETPARMS)\n");
}

#else	/* HAVE_SYS_PRIOCNTL_H */

#if defined(_POSIX_PRIORITY_SCHEDULING)
/*
 * XXX Ugly but needed because of a typo in /usr/iclude/sched.h on Linux.
 * XXX This should be removed as soon as we are sure that Linux-2.0.29 is gone.
 */
#ifdef	__linux
#define	_P	__P
#endif

#include <sched.h>

#ifdef	__linux
#undef	_P
#endif

LOCAL	int
rt_raisepri(pri)
	int pri;
{
	struct sched_param scp;

	fillbytes(&scp, sizeof(scp), '\0');
	scp.sched_priority = sched_get_priority_max(SCHED_RR) - pri;
	if (sched_setscheduler(0, SCHED_RR, &scp) < 0) {
		errmsg("WARNING: Cannot set RR-scheduler\n");
		return (-1);
	}
	return (0);
}

#else

LOCAL	int
rt_raisepri(pri)
	int pri;
{
	return (-1);
}

#endif

EXPORT	void
raisepri(pri)
	int pri;
{
	if (rt_raisepri(pri) >= 0)
		return;
	if (setpriority(PRIO_PROCESS, getpid(), -20 + pri) < 0)
		comerr("Cannot set priority\n");
}

#endif	/* HAVE_SYS_PRIOCNTL_H */

LOCAL void
checkgui()
{
	struct stat st;

	if (fstat(STDERR_FILENO, &st) >= 0 && !S_ISCHR(st.st_mode)) {
		isgui = TRUE;
		if (lverbose > 1)
			printf("Using remote (pipe) mode for interactive i/o.\n");
	}
}

LOCAL long
number(arg, retp)
	register char	*arg;
		int	*retp;
{
	long	val	= 0;

	if (*retp != 1)
		return (val);
	if (*arg == '\0')
		*retp = -1;
	else if (*(arg = astol(arg, &val))) {
		if (*arg == 'm' || *arg == 'M') {
			val *= (1024*1024);
			arg++;
		}
		else if (*arg == 'f' || *arg == 'F') {
			val *= 2352;
			arg++;
		}
		else if (*arg == 's' || *arg == 'S') {
			val *= 2048;
			arg++;
		}
		else if (*arg == 'k' || *arg == 'K') {
			val *= 1024;
			arg++;
		}
		else if (*arg == 'b' || *arg == 'B') {
			val *= 512;
			arg++;
		}
		else if (*arg == 'w' || *arg == 'W') {
			val *= 2;
			arg++;
		}
		if (*arg == '*' || *arg == 'x')
			val *= number(++arg, retp);
		else if (*arg != '\0')
			*retp = -1;
	}
	return (val);
}

LOCAL int
getnum(arg, valp)
	char	*arg;
	long	*valp;
{
	int	ret = 1;

	*valp = number(arg, &ret);
	return (ret);
}

LOCAL int
getbltype(optstr, typep)
	char	*optstr;
	long	*typep;
{
	if (streql(optstr, "all")) {
		*typep = BLANK_DISC;
	} else if (streql(optstr, "disc")) {
		*typep = BLANK_DISC;
	} else if (streql(optstr, "disk")) {
		*typep = BLANK_DISC;
	} else if (streql(optstr, "fast")) {
		*typep = BLANK_MINIMAL;
	} else if (streql(optstr, "minimal")) {
		*typep = BLANK_MINIMAL;
	} else if (streql(optstr, "track")) {
		*typep = BLANK_TRACK;
	} else if (streql(optstr, "unreserve")) {
		*typep = BLANK_UNRESERVE;
	} else if (streql(optstr, "trtail")) {
		*typep = BLANK_TAIL;
	} else if (streql(optstr, "unclose")) {
		*typep = BLANK_UNCLOSE;
	} else if (streql(optstr, "session")) {
		*typep = BLANK_SESSION;
	} else if (streql(optstr, "help")) {
		blusage(0);
	} else {
		error("Illegal blanking type '%s'.\n", optstr);
		blusage(EX_BAD);
		return (-1);
	}
	return (TRUE);
}
