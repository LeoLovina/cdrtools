/* @(#)drv_mmc.c	1.4 97/08/24 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)drv_mmc.c	1.4 97/08/24 Copyright 1997 J. Schilling";
#endif
/*
 *	CDR device implementation for
 *	SCSI-3/mmc conforming drives
 *	e.g. Yamaha CDR-400, Ricoh MP6200
 *
 *	Currently using close_track_philips() XXX FIXME
 *
 *	Copyright (c) 1997 J. Schilling
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

#include <stdio.h>
#include <standard.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>

#include <btorder.h>
#include <scgio.h>
#include <scsidefs.h>
#include <scsireg.h>

#include "cdrecord.h"
#include "scsitransp.h"

struct	scg_cmd		scmd;
struct	scsi_inquiry	inq;

extern	int	scsibus;
extern	int	target;
extern	int	lun;

extern	int	silent;
extern	int	verbose;
extern	int	lverbose;

LOCAL	int	mmc_load		__PR((void));
LOCAL	int	mmc_unload		__PR((void));
LOCAL	cdr_t	*identify_mmc		__PR((cdr_t *, struct scsi_inquiry *));
LOCAL	int	speed_select_mmc	__PR((int speed, int dummy));
LOCAL	int	next_wr_addr_mmc	__PR((int track, long *ap));
LOCAL	int	open_track_mmc		__PR((int track, track_t *track_info));
LOCAL	int	close_track_mmc		__PR((int track));
LOCAL	int	open_session_mmc	__PR((int toctype, int multi));
LOCAL	int	fixate_mmc		__PR((int onp, int dummy, int toctype));


LOCAL int
mmc_load()
{
	return (scsi_load_unload(1));
}

LOCAL int
mmc_unload()
{
	return (scsi_load_unload(0));
}

cdr_t	cdr_mmc = {
	0,
	CDR_TAO|CDR_DAO|CDR_SWABAUDIO,
	"mmc_cdr",
	"generic SCSI-3/mmc CD-R driver",
	identify_mmc,
	drive_attach,
	drive_getdisktype,
	scsi_load,
/*	mmc_load,*/
	scsi_unload,
	recovery_needed,
	recover,
	speed_select_mmc,
	select_secsize,
	next_wr_addr_mmc,
	reserve_track,
	write_xg1,
	open_track_mmc,
/*	close_track_mmc,*/
	close_track_philips,
	open_session_mmc,
	cmd_dummy,
	read_session_offset,
	fixate_mmc,
};

/*
 * SCSI-3/mmc conformant CD drive
 */
cdr_t	cdr_cd = {
	0,
	CDR_ISREADER|CDR_SWABAUDIO,
	"mmc_cd",
	"generic SCSI-3/mmc CD driver",
	identify_mmc,
	drive_attach,
	drive_getdisktype,
	scsi_load,
	scsi_unload,
	cmd_dummy, 				/* recovery_needed	*/
	(int(*)__PR((int)))cmd_dummy,		/* recover		*/
	speed_select_mmc,
	select_secsize,
	(int(*)__PR((int, long *)))cmd_dummy,	/* next_wr_addr		*/
	reserve_track,
	scsi_cdr_write,
	open_track_mmc,
	close_track_mmc,
	(int(*)__PR((int, int)))cmd_dummy,
	cmd_dummy,
	read_session_offset,
	fixation,
};

/*
 * Old pre SCSI-3/mmc CD drive
 */
cdr_t	cdr_oldcd = {
	0,
	CDR_ISREADER,
	"scsi2_cd",
	"generic SCSI-2 CD driver",
	identify_mmc,
	drive_attach,
	drive_getdisktype,
	scsi_load,
	scsi_unload,
	cmd_dummy, 				/* recovery_needed	*/
	(int(*)__PR((int)))cmd_dummy,		/* recover		*/
	speed_select_mmc,
	select_secsize,
	(int(*)__PR((int, long *)))cmd_dummy,	/* next_wr_addr		*/
	reserve_track,
	scsi_cdr_write,
	open_track_mmc,
	close_track_mmc,
	(int(*)__PR((int, int)))cmd_dummy,
	cmd_dummy,
	read_session_offset_philips,
	fixation,
};

struct cd__mode_data {
	struct scsi_mode_header	header;
	union cd_pagex	{
		struct cd_mode_page_05	page05;
		struct cd_mode_page_2A	page2A;
	} pagex;
};

LOCAL cdr_t *
identify_mmc(dp, ip)
	cdr_t			*dp;
	struct scsi_inquiry	*ip;
{
	BOOL	cdrr	 = FALSE;	/* Read CD-R	*/
	BOOL	cdwr	 = FALSE;	/* Write CD-R	*/
	BOOL	cdrrw	 = FALSE;	/* Read CD-RW	*/
	BOOL	cdwrw	 = FALSE;	/* Write CD-RW	*/

	if (ip->type != INQ_WORM && ip->type != INQ_ROMD)
		return ((cdr_t *)0);

	if (!mmc_check(&cdrr, &cdwr, &cdrrw, &cdwrw))
		return (&cdr_oldcd);	/* Pre SCSI-3/mmc drive	 	*/

	if (!cdwr)			/* SCSI-3/mmc CD drive		*/
		return (&cdr_cd);
	return (dp);			/* Generic SCSI-3/mmc CD-R	*/
}

LOCAL int
speed_select_mmc(speed, dummy)
	int	speed;
	int	dummy;
{
	u_char	mode[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	if (!get_mode_params(0x05, "CD write parameter",
			mode, (u_char *)0, (u_char *)0, (u_char *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);


	mp->test_write = dummy != 0;

	if (!set_mode_params("CD write parameter", mode, len, 0, -1))
		return (-1);

/*XXX max speed aus page 0x2A lesen !! */

/*	if (scsi_set_speed(-1, speed*176) < 0)*/

	/*
	 * 44100 * 2 * 2 =  176400 bytes/s
	 *
	 * The right formula would be:
	 * tmp = (((long)speed) * 1764) / 10;
	 *
	 * But the standard is rounding the wrong way.
	 * Furtunately rounding down is guaranteed.
	 */
	if (scsi_set_speed(-1, speed*177) < 0)
		return (-1);
	return (0);
}

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct track_info {
	u_char	data_len[2];		/* Data len without this info	*/
	u_char	track_number;		/* Track number for this info	*/
	u_char	session_number;		/* Session number for this info	*/
	u_char	res4;			/* Reserved			*/
	u_char	track_mode	:4;	/* Track mode (Q-sub control)	*/
	u_char	copy		:1;	/* This track is a higher copy	*/
	u_char	damage		:1;	/* if 1 & nwa_valid 0: inc track*/
	u_char	res5_67		:2;	/* Reserved			*/
	u_char	data_mode	:4;	/* Data mode of this track	*/
	u_char	fp		:1;	/* This is a fixed packet track	*/
	u_char	packet		:1;	/* This track is in packet mode	*/
	u_char	blank		:1;	/* This is an invisible track	*/
	u_char	rt		:1;	/* This is a reserved track	*/
	u_char	nwa_valid	:1;	/* Next writable addr valid	*/
	u_char	res7_17		:7;	/* Reserved			*/
	u_char	track_start[4];		/* Track start address		*/
	u_char	next_writable_addr[4];	/* Next writable address	*/
	u_char	free_blocks[4];		/* Free usr blocks in this track*/
	u_char	packet_size[4];		/* Packet size if in fixed mode	*/
	u_char	track_size[4];		/* # of user data blocks in trk	*/
};

#else				/* Motorola byteorder */

struct track_info {
	u_char	data_len[2];		/* Data len without this info	*/
	u_char	track_number;		/* Track number for this info	*/
	u_char	session_number;		/* Session number for this info	*/
	u_char	res4;			/* Reserved			*/
	u_char	res5_67		:2;	/* Reserved			*/
	u_char	damage		:1;	/* if 1 & nwa_valid 0: inc track*/
	u_char	copy		:1;	/* This track is a higher copy	*/
	u_char	track_mode	:4;	/* Track mode (Q-sub control)	*/
	u_char	rt		:1;	/* This is a reserved track	*/
	u_char	blank		:1;	/* This is an invisible track	*/
	u_char	packet		:1;	/* This track is in packet mode	*/
	u_char	fp		:1;	/* This is a fixed packet track	*/
	u_char	data_mode	:4;	/* Data mode of this track	*/
	u_char	res7_17		:7;	/* Reserved			*/
	u_char	nwa_valid	:1;	/* Next writable addr valid	*/
	u_char	track_start[4];		/* Track start address		*/
	u_char	next_writable_addr[4];	/* Next writable address	*/
	u_char	free_blocks[4];		/* Free usr blocks in this track*/
	u_char	packet_size[4];		/* Packet size if in fixed mode	*/
	u_char	track_size[4];		/* # of user data blocks in trk	*/
};

#endif

LOCAL int
next_wr_addr_mmc(track, ap)
	int	track;
	long	*ap;
{
	struct	track_info	track_info;
	long	next_addr;

	if (read_track_info((caddr_t)&track_info, 0xFF, sizeof(track_info)) < 0)
/*	if (read_track_info((caddr_t)&track_info, 0x00, sizeof(track_info)) < 0)*/
		return (-1);
	if (lverbose)
		scsiprbytes("track info:", (u_char *)&track_info,
				sizeof(track_info)-scsigetresid());
	next_addr = a_to_u_long(track_info.next_writable_addr);
	if (ap)
		*ap = next_addr;
	return (0);
}

int	st2mode[] = {
	0,		/* 0			*/
	TM_DATA,	/* 1 ST_ROM_MODE1	*/
	TM_DATA,	/* 2 ST_ROM_MODE2	*/
	0,		/* 3			*/
	0,		/* 4 ST_AUDIO_NOPRE	*/
	TM_PREEM,	/* 5 ST_AUDIO_PRE	*/
	0,		/* 6			*/
	0,		/* 7			*/
};

LOCAL int
open_track_mmc(track, track_info)
	int	track;
	track_t *track_info;
{
	u_char	mode[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	if (!get_mode_params(0x05, "CD write parameter",
			mode, (u_char *)0, (u_char *)0, (u_char *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);


/*	mp->track_mode = ???;*/
	mp->track_mode = st2mode[track_info->sectype & ST_MASK];
/*	mp->copy = ???;*/
	mp->dbtype = track_info->dbtype;

/*i_to_short(mp->audio_pause_len, 300);*/
/*i_to_short(mp->audio_pause_len, 150);*/
/*i_to_short(mp->audio_pause_len, 0);*/

	if (!set_mode_params("CD write parameter", mode, len, 0, track_info->secsize))
		return (-1);

	return (0);
}

LOCAL int
close_track_mmc(track)
	int	track;
{
	return (0);
}

int	toc2sess[] = {
	SES_DA_ROM,	/* CD-DA		 */
	SES_DA_ROM,	/* CD-ROM		 */
	SES_XA,		/* CD-ROM XA mode 1	 */
	SES_XA,		/* CD-ROM XA MODE 2	 */
	SES_CDI,	/* CDI			 */
	SES_DA_ROM,	/* Invalid - use default */
	SES_DA_ROM,	/* Invalid - use default */
};

LOCAL int
open_session_mmc(toctype, multi)
	int	toctype;
	int	multi;
{
	u_char	mode[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	if (!get_mode_params(0x05, "CD write parameter",
			mode, (u_char *)0, (u_char *)0, (u_char *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);


	mp->write_type = WT_TAO;	/* Currently only Track at once */

	mp->multi_session = (multi != 0) ? MS_MULTI : MS_NONE;
	mp->session_format = toc2sess[toctype & TOC_MASK];

	if (!set_mode_params("CD write parameter", mode, len, 0, -1))
		return (-1);

	return (0);
}

LOCAL int
fixate_mmc(onp, dummy, toctype)
	int	onp;
	int	dummy;
	int	toctype;
{
	return (scsi_close_session(0));
}
