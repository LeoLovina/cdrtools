/* @(#)cdrecord.h	1.11 97/09/10 Copyright 1995 J. Schilling */
/*
 *	Definitions for cdrecord
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

/*
 * Defines for option flags
 */
#define	F_DUMMY		0x0001	/* Do dummy writes */
#define	F_MSINFO	0x0002	/* Get multi-session info */
#define	F_TOC		0x0004	/* Get TOC */
#define	F_EJECT		0x0008	/* Eject disk after doing the work */
#define	F_MULTI		0x0010	/* Create linkable TOC (multi-session)*/
#define	F_FIX		0x0020	/* Fixate disk only */
#define	F_NOFIX		0x0040	/* Do not fixate disk */
#define	F_VERSION	0x0080	/* Print version info */
#define	F_CHECKDRIVE	0x0100	/* Check for driver */
#define	F_INQUIRY	0x0200	/* Do inquiry */
#define	F_SCANBUS	0x0400	/* Scan SCSI Bus */

#ifdef	min
#undef	min
#endif
#define min(a, b)	((a)<(b)?(a):(b))

#ifdef	max
#undef	max
#endif
#define max(a, b)	((a)<(b)?(b):(a))


#define DATA_SEC_SIZE	2048
#define AUDIO_SEC_SIZE	2352

#define MAX_TRACK	99	/* Red Book track limit */

#define PAD_SECS	15	/* NOTE: pad must be less than BUF_SIZE */
#define	PAD_SIZE	(PAD_SECS * DATA_SEC_SIZE)

typedef struct index {
	int	dummy;		/* Not yet implemented */
} tindex_t;

typedef struct track {
	int	f;		/* Open file for this track		*/
	int	tracksize;	/* Size of this track (-1 == until EOF)	*/
	int	secsize;	/* Sector size for this track		*/
	int	secspt;		/* # of sectors to copy for one transfer*/
	char	sectype;	/* Sector type				*/
	char	tracktype;	/* Track type (toc type)		*/
	char	dbtype;		/* Data block type for this track	*/
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
 * Defines for toc type / track type
 */
#define	TOC_DA		0	/* CD-DA				*/
#define	TOC_ROM		1	/* CD-ROM				*/
#define	TOC_XA1		2	/* CD_ROM XA with first track in mode 1 */
#define	TOC_XA2		3	/* CD_ROM XA with first track in mode 2 */
#define	TOC_CDI		4	/* CDI					*/

#define	TOC_MASK	7	/* Mask needed for toctname[]		*/

extern	char	*toctname[];

/*
 * Defines for sector type
 *
 * Mode is 2 bits
 * Aud  is 1 bit
 *
 * Sector is: aud << 2 | mode 
 */
#define	ST_ROM_MODE1	1	/* CD-ROM in mode 1 (vanilla cdrom)	*/
#define	ST_ROM_MODE2	2	/* CD-ROM in mode 2			*/
#define	ST_AUDIO_NOPRE	4	/* CD-DA stereo without preemphasis	*/
#define	ST_AUDIO_PRE	5	/* CD-DA stereo with preemphasis	*/

#define	ST_AUDIOMASK	0x04	/* Mask for audio bit			*/
#define	ST_MODEMASK	0x03	/* Mask for mode bits in sector type	*/
#define	ST_MASK		0x07	/* Mask needed for sectname[]		*/

extern	char	*secctname[];

/*
 * Control nibble bits:
 *
 * 0	with preemphasis (audio) / incremental (data)
 * 1	digital copy permitted
 * 2	data (not audio) track
 * 3	4 channels (not 2)
 */
#define	TM_PREEM	0x1	/* Audio track with preemphasis	*/ 
#define	TM_INCREMENTAL	0x1	/* Incremental data track	*/ 
#define	TM_ALLOW_COPY	0x2	/* Digital copy permitted	*/
#define	TM_DATA		0x4	/* This is a data track		*/
#define	TM_QUADRO	0x8	/* Four channel audio		*/

/*
 * Defines for write type (from SCSI-3/mmc)
 */
#define	WT_PACKET	0x0	/* Packet writing	*/
#define	WT_TAO		0x1	/* Track at once	*/
#define	WT_SAO		0x2	/* Session at once	*/
#define	WT_RAW		0x3	/* Raw			*/
#define	WT_RES_4	0x4	/* Reserved		*/
#define	WT_RES_5	0x5	/* Reserved		*/
#define	WT_RES_6	0x6	/* Reserved		*/
#define	WT_RES_7	0x7	/* Reserved		*/
#define	WT_RES_8	0x8	/* Reserved		*/
#define	WT_RES_9	0x9	/* Reserved		*/
#define	WT_RES_A	0xA	/* Reserved		*/
#define	WT_RES_B	0xB	/* Reserved		*/
#define	WT_RES_C	0xC	/* Reserved		*/
#define	WT_RES_D	0xD	/* Reserved		*/
#define	WT_RES_E	0xE	/* Reserved		*/
#define	WT_RES_F	0xF	/* Reserved		*/

/*
 * Defines for data block type (from SCSI-3/mmc)
 */
#define	DB_RAW		0	/* 2352 bytes of raw data		  */
#define	DB_RAW_PQ	1	/* 2368 bytes (raw data + P/Q Subchannel) */
#define	DB_RAW_PW	2	/* 2448 bytes (raw data + P-W Subchannel) */
#define	DB_RAW_PW_R	3	/* 2448 bytes (raw data + P-W raw Subchannel)*/
#define	DB_RES_4	4	/* -	Reserved			  */
#define	DB_RES_5	5	/* -	Reserved			  */
#define	DB_RES_6	6	/* -	Reserved			  */
#define	DB_VU_7		7	/* -	Vendor specific			  */
#define	DB_ROM_MODE1	8	/* 2048 bytes Mode 1 (ISO/IEC 10149)	  */
#define	DB_ROM_MODE2	9	/* 2336 bytes Mode 2 (ISO/IEC 10149)	  */
#define	DB_XA_MODE1	10	/* 2048 bytes Mode 1? (CD-ROM XA form 1)  */
#define	DB_XA_MODE2	10	/* 2048 bytes Mode 2? (CD-ROM XA form 1)  */
#define	DB_XA_MODE2_F1	11	/* 2056 bytes Mode 2 (CD-ROM XA form 1)	  */
#define	DB_XA_MODE2_F2	12	/* 2324 bytes Mode 2 (CD-ROM XA form 2)	  */
#define	DB_XA_MODE2_MIX	13	/* 2332 bytes Mode 2 (CD-ROM XA 1/2+subhdr) */
#define	DB_RES_14	14	/* -	Reserved			  */
#define	DB_VU_15	15	/* -	Vendor specific			  */

/*
 * Defines for multi session type (from SCSI-3/mmc)
 */
#define	MS_NONE		0	/* No B0 pointer. Next session not allowed*/
#define	MS_FINAL	1	/* B0 = FF:FF:FF. Next session not allowed*/
#define	MS_RES		2	/* Reserved				  */
#define	MS_MULTI	3	/* B0 = Next PA.  Next session allowed	  */

/*
 * Defines for session format (from SCSI-3/mmc)
 */
#define	SES_DA_ROM	0x00	/* CD-DA or CD-ROM disk			  */
#define	SES_CDI		0x10	/* CD-I disk				  */
#define	SES_XA		0x20	/* CD-ROM XA disk			  */

/*
 * Useful definitions for audio tracks
 */
#define	sample		(44100 * 2)		/* one 16bit audio sample */
#define	ssample		(sample * 2)		/* one stereo sample	*/
#define samples(v)	((v) / ssample)		/* # of stereo samples	*/
#define hsamples(v)	((v) / (ssample/100))	/* 100 * # of stereo samples */

#define	minutes(v)	(samples(v) / 60)
#define	seconds(v)	(samples(v) % 60)
#define	hseconds(v)	(hsamples(v) % 100)

struct msf {
	char	msf_min;
	char	msf_sec;
	char	msf_frame;
};

#define	FMT_TOC		0
#define	FMT_SINFO	1
#define	FMT_FULLTOC	2
#define	FMT_PMA		3
#define	FMT_ATIP	4

typedef	struct disk_status	dstat_t;

struct disk_status {
	int	ds_disktype;		/* disk type		*/
	int	ds_diskstat;		/* disk status		*/
	int	ds_trfirst;		/* first track #	*/
	int	ds_trlast;		/* last track #		*/
	int	ds_maxblock;		/* last block # on disk	*/
	long	ds_fwa;			/* first writable addr	*/
};

/*
 * First (alpha) approach of a CDR device abstraction layer.
 * This interface will change as long as I did not find the
 * optimum that fits for all devices.
 */
/*--------------------------------------------------------------------------*/
typedef	struct cdr_cmd	cdr_t;

struct cdr_cmd {
	int	cdr_dev;
	int	cdr_flags;
	char	*cdr_drname;					/* Driver ID string */
	char	*cdr_drtext;					/* Driver ID text */
	cdr_t	*(*cdr_identify)	__PR((cdr_t *, struct scsi_inquiry *));	/* identify drive */
	int	(*cdr_attach)		__PR((void));		/* init error decoding etc*/
	int	(*cdr_getdisktype)	__PR((cdr_t *, dstat_t *));	/* get disk type */
	int	(*cdr_load)		__PR((void));		/* load disk */
	int	(*cdr_unload)		__PR((void));		/* unload disk */
	int	(*cdr_check_recovery)	__PR((void));		/* check if recover is needed */
	int	(*cdr_recover)		__PR((int track));	/* do recover */
	int	(*cdr_set_speed_dummy)	__PR((int speed, int dummy));	/* set recording speed & dummy write */
	int	(*cdr_set_secsize)	__PR((int secsize));	/* set sector size */
	int	(*cdr_next_wr_address)	__PR((int track, long *ap));	/* get next writable addr. */
	int	(*cdr_reserve_track)	__PR((unsigned long len));	/* reserve a track fro future use */
	int	(*cdr_write_trackdata)	__PR((caddr_t buf, long daddr, long bytecnt, int seccnt));

	int	(*cdr_open_track)	__PR((int track, track_t *track_info));	/* open new track */
	int	(*cdr_close_track)	__PR((int track));		/* close written track */
	int	(*cdr_open_session)	__PR((int toctype, int multi));		/* open new session */
	int	(*cdr_close_session)	__PR((void));		/* really needed ??? */
	int	(*cdr_session_offset)	__PR((long *soff));		/* read session offset*/
	int	(*cdr_fixate)		__PR((int onp, int dummy, int toctype));	/* write toc on disk */
};

/*
 * Definitions for cdr_flags
 */
#define	CDR_TAO		0x01		/* Drive supports Track at once	*/
#define	CDR_DAO		0x02		/* Drive supports Disk at once	*/
#define	CDR_PACKET	0x04		/* Drive supports packet writing*/
#define	CDR_SWABAUDIO	0x08		/* Drive swabs audio data	*/
#define	CDR_ISREADER	0x10		/* Drive is s CD-ROM reader	*/
#define	CDR_TRAYLOAD	0x20		/* Drive loads CD with tray	*/
#define	CDR_CADDYLOAD	0x40		/* Drive loads CD with caddy	*/


/*
 * cdrecord.c
 */
extern	void	load_media	__PR((cdr_t *));
extern	void	unload_media	__PR((cdr_t *, int));

/*
 * modes.c
 */
extern	BOOL	get_mode_params	__PR((int page, char *pagename,
					u_char *modep, u_char *cmodep,
					u_char *dmodep, u_char *smodep,
					int *lenp));
extern	BOOL	set_mode_params	__PR((char *pagename, u_char *modep,
					int len, int save, int secsize));

/*
 * scsi_cdr.c
 */
extern	int	open_scsi	__PR((char *, int, int));
extern	void	scsi_settimeout	__PR((int));
extern	BOOL	unit_ready	__PR((void));
extern	int	test_unit_ready	__PR((void));
extern	int	rezero_unit	__PR((void));
extern	int	request_sense	__PR((void));
extern	int	inquiry		__PR((caddr_t, int));
extern	int	read_capacity	__PR((void));
extern	int	scsi_load_unload __PR((int));
extern	int	load_unload_philips __PR((int));
extern	int	scsi_prevent_removal __PR((int));
extern	int	scsi_start_stop_unit __PR((int, int));
extern	int	scsi_set_speed	__PR((int readspeed, int writespeed));
extern	int	qic02		__PR((int));
extern	int	write_xg0	__PR((caddr_t, long, long, int));
extern	int	write_xg1	__PR((caddr_t, long, long, int));
extern	int	write_track	__PR((long, int));
extern	int	scsi_flush_cache __PR((void));
extern	int	read_toc	__PR((caddr_t, int, int, int, int));
extern	int	read_toc_philips __PR((caddr_t, int, int, int, int));
extern	int	read_header	__PR((caddr_t, long, int, int));
extern	int	read_track_info	__PR((caddr_t, int, int));
extern	int	close_track_philips __PR((int track));
extern	int	fixation	__PR((int, int, int));
extern	int	scsi_close_session __PR((int type));
extern	int	recover		__PR((int));
extern	int	first_writable_addr __PR((long *, int, int, int, int));
extern	int	reserve_track	__PR((unsigned long));
extern	int	mode_select	__PR((unsigned char *, int, int, int));
extern	int	mode_sense	__PR((u_char *dp, int cnt, int page, int pcf));
extern	int	speed_select_yamaha	__PR((int speed, int dummy));
extern	int	speed_select_philips	__PR((int speed, int dummy));
extern	int	write_track_info __PR((int));
extern	int	read_tochdr	__PR((cdr_t *, int *, int *));
extern	int	read_trackinfo	__PR((int, long *, struct msf *, int *, int *, int *));
extern	int	read_session_offset __PR((long *));
extern	int	read_session_offset_philips __PR((long *));
extern	int	select_secsize	__PR((int));
extern	BOOL	is_cddrive	__PR((void));
extern	BOOL	is_unknown_dev	__PR((void));
extern	int	read_scsi	__PR((caddr_t, long, int));
extern	int	read_g0		__PR((caddr_t, long, int));
extern	int	read_g1		__PR((caddr_t, long, int));
extern	BOOL	getdev		__PR((BOOL));
extern	void	printdev	__PR((void));
extern	BOOL	do_inquiry	__PR((BOOL));
extern	BOOL	recovery_needed	__PR((void));
extern	int	scsi_load	__PR((void));
extern	int	scsi_unload	__PR((void));
extern	int	scsi_cdr_write	__PR((char *bufp, long sectaddr, long size, int blocks));
extern	BOOL	is_mmc		__PR((void));
extern	BOOL	mmc_check	__PR((BOOL *cdrrp, BOOL *cdwrp,
					BOOL *cdrrwp, BOOL *cdwrwp));

/*
 * cdr_drv.c
 */
extern	cdr_t	*drive_identify		__PR((cdr_t *, struct scsi_inquiry *ip));
extern	int	drive_attach		__PR((void));
extern	int	attach_unknown		__PR((void));
extern	int	drive_getdisktype	__PR((cdr_t *dp, dstat_t *dsp));
extern	int	cmd_dummy		__PR((void));
extern	void	set_cdrcmds		__PR((char *name, cdr_t **dpp));
extern	cdr_t	*get_cdrcmds		__PR((void));


/*
 * isosize.c
 */
extern	long	isosize		__PR((int f));
