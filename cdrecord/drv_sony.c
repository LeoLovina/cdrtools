/* @(#)drv_sony.c	1.18 98/03/27 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)drv_sony.c	1.18 98/03/27 Copyright 1997 J. Schilling";
#endif
/*
 *	CDR device implementation for
 *	Sony
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

/*#define	SONY_DEBUG*/

#include <mconfig.h>

#include <stdio.h>
#include <standard.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>

#include <utypes.h>
#include <btorder.h>
#include <scgio.h>
#include <scsidefs.h>
#include <scsireg.h>

#include "cdrecord.h"
#include "scsitransp.h"

#ifdef	SONY_DEBUG
#	define		inc_verbose()	verbose++
#	define		dec_verbose()	verbose--
#else
#	define		inc_verbose()
#	define		dec_verbose()
#endif

struct	scg_cmd		scmd;
struct	scsi_inquiry	inq;

extern	int	scsibus;
extern	int	target;
extern	int	lun;

extern	int	silent;
extern	int	verbose;

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct sony_924_mode_page_20 {	/* mastering information */
		MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x06 = 6 Bytes */
	u_char	subcode_header_off;
	Ucbit	res3_0		: 1;
	Ucbit	speudo		: 1;
	Ucbit	res3_2		: 1;
	Ucbit	c2po		: 1;
	Ucbit	subcode_ecc	: 1;
	Ucbit	res3_567	: 3;
	u_char	res_4;
	u_char	cue_sheet_opt;
	u_char	res[2];
};

#else				/* Motorola byteorder */

struct sony_924_mode_page_20 {	/* mastering information */
		MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x06 = 6 Bytes */
	u_char	subcode_header_off;
	Ucbit	res3_567	: 3;
	Ucbit	subcode_ecc	: 1;
	Ucbit	c2po		: 1;
	Ucbit	res3_2		: 1;
	Ucbit	speudo		: 1;
	Ucbit	res3_0		: 1;
	u_char	res_4;
	u_char	cue_sheet_opt;
	u_char	res[2];
};
#endif

struct sony_924_mode_page_22 {	/* disk information */
		MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x1E = 30 Bytes */
	u_char	disk_style;
	u_char	disk_type;
	u_char	first_track;
	u_char	last_track;
	u_char	numsess;
	u_char	res_7;
	u_char	disk_appl_code[4];
	u_char	last_start_time[4];
	u_char	disk_status;
	u_char	num_valid_nra;
	u_char	track_info_track;
	u_char	post_gap;
	u_char	disk_id_code[4];
	u_char	lead_in_start[4];
	u_char	res[4];
};

struct sony_924_mode_page_23 {	/* track information */
		MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x22 = 34 Bytes */
	u_char	res_2;
	u_char	track_num;
	u_char	data_form;
	u_char	write_method;
	u_char	session;
	u_char	track_status;
	u_char	start_lba[4];
	u_char	next_recordable_addr[4];
	u_char	blank_area_cap[4];
	u_char	fixed_packet_size[4];
	u_char	res_24;
	u_char	starting_msf[3];
	u_char	res_28;
	u_char	ending_msf[3];
	u_char	res_32;
	u_char	next_rec_time[3];
};

struct sony_924_mode_page_31 {	/* drive speed */
		MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x02 = 2 Bytes */
	u_char	speed;
	u_char	res;
};

struct cdd_52x_mode_data {
	struct scsi_mode_header	header;
	union cdd_pagex	{
		struct sony_924_mode_page_20	page_s20;
		struct sony_924_mode_page_22	page_s22;
		struct sony_924_mode_page_23	page_s23;
		struct sony_924_mode_page_31	page_s31;
	} pagex;
};

LOCAL	int	write_continue_sony	__PR((caddr_t bp, long sectaddr, long size, int blocks, BOOL islast));
LOCAL	int	write_track_sony	__PR((long track, int sectype));
LOCAL	int	close_track_sony	__PR((int track, track_t *trackp));
LOCAL	int	finalize_sony		__PR((int onp, int dummy, int type, int tracks, track_t *trackp));
LOCAL	int	recover_sony		__PR((int track));
LOCAL	int	next_wr_addr_sony	__PR((int track, track_t *trackp, long *ap));
LOCAL	int	reserve_track_sony	__PR((unsigned long len));
LOCAL	int	reserve_track_sony	__PR((unsigned long len));
LOCAL	int	speed_select_sony	__PR((int speed, int dummy));
LOCAL	int	next_writable_address_sony __PR((long *ap, int track, int sectype, int tracktype));
LOCAL	int	new_track_sony		__PR((int track, int sectype, int tracktype));
LOCAL	int	open_track_sony		__PR((cdr_t *dp, int track, track_t *track_info));
LOCAL	int	open_session_sony	__PR((int tracks, track_t *trackp, int toctype, int multi));
LOCAL	int	sony_attach		__PR((void));
#ifdef	SONY_DEBUG
LOCAL	void	print_sony_mp22		__PR((struct sony_924_mode_page_22 *xp, int len));
LOCAL	void	print_sony_mp23		__PR((struct sony_924_mode_page_23 *xp, int len));
#endif

cdr_t	cdr_sony_cdu924 = {
	0,
	CDR_TAO|CDR_DAO|CDR_CADDYLOAD|CDR_SWABAUDIO,
	"sony_cdu924",
	"driver for Sony CDU-924",
	0,
	drive_identify,
	sony_attach,
	drive_getdisktype,
	scsi_load,
	scsi_unload,
	recovery_needed,
	recover,
	speed_select_sony,
	select_secsize,
	next_wr_addr_sony,
	reserve_track_sony,
	write_continue_sony,
	open_track_sony,
	close_track_sony,
	open_session_sony,
	cmd_dummy,
	read_session_offset_philips,
	finalize_sony,
	blank_dummy,
};

LOCAL int
write_continue_sony(bp, sectaddr, size, blocks, islast)
	caddr_t	bp;		/* address of buffer */
	long	sectaddr;	/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	blocks;		/* sector count */
	BOOL	islast;		/* last write for track */
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = bp;
	scmd.size = size;
	scmd.flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0xE1;
	scmd.cdb.g1_cdb.lun = lun;
	g0_cdbaddr(&scmd.cdb.g0_cdb, size); /* Hack, but Sony is silly */
	
	if (scsicmd("write_continue") < 0)
		return (-1);
	return (size - scmd.resid);
}

LOCAL int
write_track_sony(track, sectype)
	long	track;		/* track number 0 == new track */
	int	sectype;	/* no sectype for Sony write track */
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0xF5;
	scmd.cdb.g1_cdb.lun = lun;
	g1_cdbaddr(&scmd.cdb.g1_cdb, track);
	
	if (scsicmd("write_track") < 0)
		return (-1);
	return (0);
}

/* XXX NOCH NICHT FERTIG */
LOCAL int
close_track_sony(track, trackp)
	int	track;
	track_t	*trackp;
{
	track = 0;

	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0xF0;
	scmd.cdb.g1_cdb.lun = lun;
	g1_cdbaddr(&scmd.cdb.g1_cdb, track);
/* XXX Padding ??? (bit 0 in addr[0]) */
	
	if (scsicmd("close_track") < 0)
		return (-1);

	/*
	 * Clear the silly "error situation" from Sony´ dummy write end
	 * but notify if real errors occurred. 
	 */
	silent++;
	if (test_unit_ready() < 0 && scsi_sense_code() != 0xD4)
		scsiprinterr("close_track/test_unit_ready");
	silent--;

	return (0);
}

LOCAL int
finalize_sony(onp, dummy, type, tracks, trackp)
	int	onp;	/* open next program area */
	int	dummy;
	int	type;	/* TOC type 0: CD-DA, 1: CD-ROM, 2: CD-ROM/XA1, 3: CD-ROM/XA2, 4: CDI */
	int	tracks;
	track_t	*trackp;
{
	if (dummy) {
		printf("Fixating is not possible in dummy write mode.\n");
		return (0);
	}
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.timeout = 8 * 60;		/* Needs up to 4 minutes */
	scmd.cdb.g1_cdb.cmd = 0xF1;
	scmd.cdb.g1_cdb.lun = lun;
	scmd.cdb.g1_cdb.count[1] = (onp ? 1 : 0);
/* XXX Padding ??? (bit 0 in addr[0]) */
	
	if (scsicmd("finalize") < 0)
		return (-1);
	return (0);
}

LOCAL int
recover_sony(track)
	int	track;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0xF6;
	scmd.cdb.g1_cdb.lun = lun;
	scmd.cdb.g1_cdb.addr[3] = track;
	
	if (scsicmd("recover") < 0)
		return (-1);
	return (0);
}

LOCAL int
next_wr_addr_sony(track, trackp, ap)
	int	track;
	track_t	*trackp;
	long	*ap;
{
	if (next_writable_address_sony(ap, 0, 0, 0) < 0)
		return (-1);
	return (0);
}

LOCAL int
reserve_track_sony(len)
	unsigned long len;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0xF3;
	scmd.cdb.g1_cdb.lun = lun;
	i_to_long(&scmd.cdb.g1_cdb.addr[3], len);
	
	if (scsicmd("reserve_track") < 0)
		return (-1);
	return (0);
}

int	sony_speeds[] = {
		-1,		/* Speed null is not allowed */
		0,		/* Single speed */
		1,		/* Double speed */
		-1,		/* Three times */
		3,		/* Quad speed */
};

LOCAL int
speed_select_sony(speed, dummy)
	int	speed;
	int	dummy;
{
	struct cdd_52x_mode_data md;
	int	count;
	int	err;

	if (speed < 1 || speed > 4 || sony_speeds[speed] < 0)
		return (-1);

	fillbytes((caddr_t)&md, sizeof(md), '\0');

	count  = sizeof(struct scsi_mode_header) +
		sizeof(struct sony_924_mode_page_20);

	md.pagex.page_s20.p_code = 0x20;
	md.pagex.page_s20.p_len =  0x06;
	md.pagex.page_s20.speudo = dummy?1:0;
	
	err = mode_select((u_char *)&md, count, 0, 1);
	if (err < 0)
		return (err);

	fillbytes((caddr_t)&md, sizeof(md), '\0');

	count  = sizeof(struct scsi_mode_header) +
		sizeof(struct sony_924_mode_page_31);

	md.pagex.page_s31.p_code = 0x31;
	md.pagex.page_s31.p_len =  0x02;
	md.pagex.page_s31.speed = sony_speeds[speed];
	
	return (mode_select((u_char *)&md, count, 0, 1));
}

LOCAL int
next_writable_address_sony(ap, track, sectype, tracktype)
	long	*ap;
	int	track;
	int	sectype;
	int	tracktype;
{
	struct	scsi_mode_page_header *mp;
	char			mode[256];
	int			len = 0x30;
	int			page = 0x23;
	struct sony_924_mode_page_23	*xp;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	inc_verbose();
	if (!get_mode_params(page, "CD track information",
			(u_char *)mode, (u_char *)0, (u_char *)0, (u_char *)0, &len)) {
		dec_verbose();
		return (-1);
	}
	dec_verbose();
	if (len == 0)
		return (-1);

	mp = (struct scsi_mode_page_header *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);


	xp = (struct sony_924_mode_page_23 *)mp;

#ifdef	SONY_DEBUG
	print_sony_mp23(xp, len);
#endif
	if (ap)
		*ap = a_to_u_long(xp->next_recordable_addr);
	return (0);
}


LOCAL int
new_track_sony(track, sectype, tracktype)
	int	track;
	int	sectype;
	int	tracktype;
{
	struct	scsi_mode_page_header *mp;
	char			mode[256];
	int			len = 0x30;
	int			page = 0x23;
	struct sony_924_mode_page_23	*xp;
	int	i;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	inc_verbose();
	if (!get_mode_params(page, "CD track information",
			(u_char *)mode, (u_char *)0, (u_char *)0, (u_char *)0, &len)) {
		dec_verbose();
		return (-1);
	}
	dec_verbose();
	if (len == 0)
		return (-1);

	mp = (struct scsi_mode_page_header *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);


	xp = (struct sony_924_mode_page_23 *)mp;

#ifdef	SONY_DEBUG
	print_sony_mp23(xp, len);
#endif

	xp->write_method = 0;	/* Track at one recording */

	if (sectype & ST_AUDIOMASK) {
		xp->data_form = sectype == ST_AUDIO_PRE ? 0x02 : 0x00;
	} else {
		if (tracktype == TOC_ROM) {
			xp->data_form = sectype == ST_ROM_MODE1 ? 0x10 : 0x11;
		} else if (tracktype == TOC_XA1) {
			xp->data_form = 0x12;
		} else if (tracktype == TOC_XA2) {
			xp->data_form = 0x12;
		} else if (tracktype == TOC_CDI) {
			xp->data_form = 0x12;
		}
	}

	((struct scsi_modesel_header *)mode)->sense_data_len	= 0;
	((struct scsi_modesel_header *)mode)->res2		= 0;

	i = ((struct scsi_mode_header *)mode)->blockdesc_len;
	if (i > 0) {
		i_to_3_byte(
			((struct scsi_mode_data *)mode)->blockdesc.nlblock,
								0);
	}

	if (mode_select((u_char *)mode, len, 0, inq.data_format >= 2) < 0) {
		return (-1);
	}

	return (0);
}

LOCAL int
open_track_sony(dp, track, track_info)
	cdr_t	*dp;
	int	track;
	track_t *track_info;
{
	if (select_secsize(track_info->secsize) < 0)
		return (-1);

	if (new_track_sony(track, track_info->sectype, track_info->tracktype) < 0)
		return (-1);

	if (write_track_sony(0L, track_info->sectype) < 0)
		return (-1);

	return (0);
}

LOCAL int
open_session_sony(tracks, trackp, toctype, multi)
	int	tracks;
	track_t	*trackp;
	int	toctype;
	int	multi;
{
	struct	scsi_mode_page_header *mp;
	char			mode[256];
	int	i;
	int	len = 0x30;
	int	page = 0x22;
	struct sony_924_mode_page_22	*xp;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	inc_verbose();
	if (!get_mode_params(page, "CD disk information",
			(u_char *)mode, (u_char *)0, (u_char *)0, (u_char *)0, &len)) {
		dec_verbose();
		return (-1);
	}
	dec_verbose();
	if (len == 0)
		return (-1);

	mp = (struct scsi_mode_page_header *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	xp = (struct sony_924_mode_page_22 *)mp;

#ifdef	SONY_DEBUG
	print_sony_mp22(xp, len);
#endif

	xp->disk_type = toc2sess[toctype & TOC_MASK];

	((struct scsi_modesel_header *)mode)->sense_data_len	= 0;
	((struct scsi_modesel_header *)mode)->res2		= 0;

	i = ((struct scsi_mode_header *)mode)->blockdesc_len;
	if (i > 0) {
		i_to_3_byte(
			((struct scsi_mode_data *)mode)->blockdesc.nlblock,
								0);
	}

	if (mode_select((u_char *)mode, len, 0, inq.data_format >= 2) < 0) {
		return (-1);
	}
	return (0);
}

static const char *sd_cdu_924_error_str[] = {

	"\200\000write complete",				/* 80 00 */
	"\201\000logical unit is reserved",			/* 81 00 */
	"\205\000audio address not valid",			/* 85 00 */
	"\210\000illegal cue sheet",				/* 88 00 */
	"\211\000inappropriate command",			/* 89 00 */

	"\266\000media load mechanism failed",			/* B6 00 */
	"\271\000audio play operation aborted",			/* B9 00 */
	"\277\000buffer overflow for read all subcodes command",/* BF 00 */
	"\300\000unrecordable disk",				/* C0 00 */
	"\301\000illegal track status",				/* C1 00 */
	"\302\000reserved track present",			/* C2 00 */
	"\303\000buffer data size error",			/* C3 00 */
	"\304\001illegal data form for reserve track command",	/* C4 01 */
	"\304\002unable to reserve track, because track mode has been changed",	/* C4 02 */
	"\305\000buffer error during at once recording",	/* C5 00 */
	"\306\001unwritten area encountered",			/* C6 01 */
	"\306\002link blocks encountered",			/* C6 02 */
	"\306\003nonexistent block encountered",		/* C6 03 */
	"\307\000disk style mismatch",				/* C7 00 */
	"\310\000no table of contents",				/* C8 00 */
	"\311\000illegal block length for write command",	/* C9 00 */
	"\312\000power calibration error",			/* CA 00 */
	"\313\000write error",					/* CB 00 */
	"\313\001write rrror track recovered",			/* CB 01 */
	"\314\000not enough space",				/* CC 00 */
	"\315\000no track present to finalize",			/* CD 00 */
	"\316\000unrecoverable track descriptor encountered",	/* CE 00 */
	"\317\000damaged track present",			/* CF 00 */
	"\320\000pma area full",				/* D0 00 */
	"\321\000pca area full",				/* D1 00 */
	"\322\000unrecoverable damaged track cause too small writing area",	/* D2 00 */
	"\323\000no bar code",					/* D3 00 */
	"\323\001not enough bar code margin",			/* D3 01 */
	"\323\002no bar code start pattern",			/* D3 02 */
	"\323\003illegal bar code length",			/* D3 03 */
	"\323\004illegal bar code format",			/* D3 04 */
	"\324\000exit from pseudo track at once recording",	/* D4 00 */
	NULL
};

LOCAL int
sony_attach()
{
	scsi_setnonstderrs(sd_cdu_924_error_str);
	return (0);
}

#ifdef	SONY_DEBUG
LOCAL void
print_sony_mp22(xp, len)
	struct sony_924_mode_page_22	*xp;
	int				len;
{
	printf("disk style: %X\n", xp->disk_style);
	printf("disk type: %X\n", xp->disk_type);
	printf("first track: %X\n", xp->first_track);
	printf("last track: %X\n", xp->last_track);
	printf("numsess:    %X\n", xp->numsess);
	printf("disk appl code: %X\n", a_to_u_long(xp->disk_appl_code));
	printf("last start time: %X\n", a_to_u_long(xp->last_start_time));
	printf("disk status: %X\n", xp->disk_status);
	printf("num valid nra: %X\n", xp->num_valid_nra);
	printf("track info track: %X\n", xp->track_info_track);
	printf("post gap: %X\n", xp->post_gap);
	printf("disk id code: %X\n", a_to_u_long(xp->disk_id_code));
	printf("lead in start: %X\n", a_to_u_long(xp->lead_in_start));
}

LOCAL void
print_sony_mp23(xp, len)
	struct sony_924_mode_page_23	*xp;
	int				len;
{
	printf("len: %d\n", len);

	printf("track num: %X\n", xp->track_num);
	printf("data form: %X\n", xp->data_form);
	printf("write method: %X\n", xp->write_method);
	printf("session: %X\n", xp->session);
	printf("track status: %X\n", xp->track_status);

	printf("start lba: %X\n", a_to_u_long(xp->start_lba));
	printf("next recordable addr: %X\n", a_to_u_long(xp->next_recordable_addr));
	printf("blank area cap: %X\n", a_to_u_long(xp->blank_area_cap));
	printf("fixed packet size: %X\n", a_to_u_long(xp->fixed_packet_size));
	printf("starting msf: %X\n", a_to_u_long(xp->starting_msf));
	printf("ending msf: %X\n", a_to_u_long(xp->ending_msf));
	printf("next rec time: %X\n", a_to_u_long(xp->next_rec_time));
}
#endif
