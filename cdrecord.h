/* @(#)cdrecord.h	1.4 96/10/03 Copyright 1995 J. Schilling */
/*
 *	Definitions for cdrecord
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

#define	BAD	(-1)

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

extern	int	open_scsi	__PR((char *, int));
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
extern	int	write_track	__PR((long, int, int));
extern	int	scsi_flush_cache __PR((void));
extern	int	fixation	__PR((int, int));
extern	int	recover		__PR((void));
extern	int	reserve_track	__PR((unsigned long));
extern	int	mode_select	__PR((unsigned char *, int, int, int));
extern	int	speed_select	__PR((int, int));
extern	int	write_track_info __PR((int, int));
extern	int	select_secsize	__PR((int));
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
