/* @(#)cdrecord.h	1.5 97/03/02 Copyright 1995 J. Schilling */
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

/*
 * Defines for toc type
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


struct msf {
	char	msf_min;
	char	msf_sec;
	char	msf_frame;
};

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
extern	int	scsi_load_unload __PR((int));
extern	int	scsi_prevent_removal __PR((int));
extern	int	scsi_start_stop_unit __PR((int));
extern	int	qic02		__PR((int));
extern	int	write_xg0	__PR((caddr_t, long, int, int));
extern	int	write_track	__PR((long, int));
extern	int	scsi_flush_cache __PR((void));
extern	int	read_toc	__PR((caddr_t, int, int, int, int));
extern	int	read_header	__PR((caddr_t, long, int, int));
extern	int	read_track_info	__PR((caddr_t, int, int));
extern	int	fixation	__PR((int, int));
extern	int	recover		__PR((void));
extern	long	first_writable_addr __PR((int, int, int, int));
extern	int	reserve_track	__PR((unsigned long));
extern	int	mode_select	__PR((unsigned char *, int, int, int));
extern	int	speed_select	__PR((int, int));
extern	int	write_track_info __PR((int));
extern	int	read_tochdr	__PR((int *, int *));
extern	int	read_trackinfo	__PR((int, long *, struct msf *, int *, int *, int *));
extern	long	read_session_offset __PR((void));
extern	int	select_secsize	__PR((int));
extern	BOOL	is_cddrive	__PR((void));
extern	BOOL	is_cdrecorder	__PR((void));
extern	BOOL	is_yamaha	__PR((void));
extern	BOOL	is_unsupported	__PR((void));
extern	int	read_scsi	__PR((caddr_t, long, int));
extern	int	read_g0		__PR((caddr_t, long, int));
extern	int	read_g1		__PR((caddr_t, long, int));
extern	BOOL	getdev		__PR((BOOL));
extern	void	printdev	__PR((void));
extern	BOOL	do_inquiry	__PR((BOOL));
extern	BOOL	recovery_needed	__PR((void));

/*
 * isosize.c
 */
extern	long	isosize		__PR((int f));
