/* @(#)drv_sony.c	1.39 00/04/16 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)drv_sony.c	1.39 00/04/16 Copyright 1997 J. Schilling";
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
#include <fctldefs.h>
#include <errno.h>
#include <strdefs.h>

#include <utypes.h>
#include <btorder.h>
#include <intcvt.h>
#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "cdrecord.h"

#ifdef	SONY_DEBUG
#	define		inc_verbose()	scgp->verbose++
#	define		dec_verbose()	scgp->verbose--
#else
#	define		inc_verbose()
#	define		dec_verbose()
#endif

extern	int	lverbose;

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

LOCAL	int	write_continue_sony	__PR((SCSI *scgp, caddr_t bp, long sectaddr, long size, int blocks, BOOL islast));
LOCAL	int	write_track_sony	__PR((SCSI *scgp, long track, int sectype));
LOCAL	int	close_track_sony	__PR((SCSI *scgp, int track, track_t *trackp));
LOCAL	int	finalize_sony		__PR((SCSI *scgp, int onp, int dummy, int type, int tracks, track_t *trackp));
LOCAL	int	recover_sony		__PR((SCSI *scgp, int track));
LOCAL	int	next_wr_addr_sony	__PR((SCSI *scgp, int track, track_t *trackp, long *ap));
LOCAL	int	reserve_track_sony	__PR((SCSI *scgp, unsigned long len));
LOCAL	int	getdisktype_sony	__PR((SCSI *scgp, cdr_t *dp, dstat_t *dsp));
LOCAL	void	di_to_dstat_sony	__PR((struct sony_924_mode_page_22 *dip, dstat_t *dsp));
LOCAL	int	speed_select_sony	__PR((SCSI *scgp, int *speedp, int dummy));
LOCAL	int	next_writable_address_sony __PR((SCSI *scgp, long *ap, int track, int sectype, int tracktype));
LOCAL	int	new_track_sony		__PR((SCSI *scgp, int track, int sectype, int tracktype));
LOCAL	int	open_track_sony		__PR((SCSI *scgp, cdr_t *dp, int track, track_t *track_info));
LOCAL	int	open_session_sony	__PR((SCSI *scgp, cdr_t *dp, int tracks, track_t *trackp, int toctype, int multi));
LOCAL	int	get_page22_sony		__PR((SCSI *scgp, char *mode));
LOCAL	int	sony_attach		__PR((SCSI *scgp, cdr_t *dp));
#ifdef	SONY_DEBUG
LOCAL	void	print_sony_mp22		__PR((struct sony_924_mode_page_22 *xp, int len));
LOCAL	void	print_sony_mp23		__PR((struct sony_924_mode_page_23 *xp, int len));
#endif
LOCAL	int	buf_cap_sony		__PR((SCSI *scgp, long *, long *));

cdr_t	cdr_sony_cdu924 = {
	0,
	CDR_TAO|CDR_DAO|CDR_CADDYLOAD|CDR_SWABAUDIO,
	"sony_cdu924",
	"driver for Sony CDU-924",
	0,
	drive_identify,
	sony_attach,
	getdisktype_sony,
	scsi_load,
	scsi_unload,
	buf_cap_sony,
	(int(*)__PR((SCSI *)))cmd_dummy,	/* recovery_needed	*/
	recover_sony,
	speed_select_sony,
	select_secsize,
	next_wr_addr_sony,
	reserve_track_sony,
	write_continue_sony,
	no_sendcue,
	open_track_sony,
	close_track_sony,
	open_session_sony,
	cmd_dummy,
	read_session_offset_philips,
	finalize_sony,
	blank_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
};

LOCAL int
write_continue_sony(scgp, bp, sectaddr, size, blocks, islast)
	SCSI	*scgp;
	caddr_t	bp;		/* address of buffer */
	long	sectaddr;	/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	blocks;		/* sector count */
	BOOL	islast;		/* last write for track */
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = bp;
	scmd->size = size;
	scmd->flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g1_cdb.cmd = 0xE1;
	scmd->cdb.g1_cdb.lun = scgp->lun;
	g0_cdbaddr(&scmd->cdb.g0_cdb, size); /* Hack, but Sony is silly */
	
	scgp->cmdname = "write_continue";

	if (scsicmd(scgp) < 0)
		return (-1);
	return (size - scsigetresid(scgp));
}

LOCAL int
write_track_sony(scgp, track, sectype)
	SCSI	*scgp;
	long	track;		/* track number 0 == new track */
	int	sectype;	/* no sectype for Sony write track */
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g1_cdb.cmd = 0xF5;
	scmd->cdb.g1_cdb.lun = scgp->lun;
	g1_cdbaddr(&scmd->cdb.g1_cdb, track);
	
	scgp->cmdname = "write_track";

	if (scsicmd(scgp) < 0)
		return (-1);
	return (0);
}

/* XXX NOCH NICHT FERTIG */
LOCAL int
close_track_sony(scgp, track, trackp)
	SCSI	*scgp;
	int	track;
	track_t	*trackp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	track = 0;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g1_cdb.cmd = 0xF0;
	scmd->cdb.g1_cdb.lun = scgp->lun;
	g1_cdbaddr(&scmd->cdb.g1_cdb, track);
/* XXX Padding ??? (bit 0 in addr[0]) */
	
	scgp->cmdname = "close_track";

	if (scsicmd(scgp) < 0)
		return (-1);

	/*
	 * Clear the silly "error situation" from Sony� dummy write end
	 * but notify if real errors occurred. 
	 */
	scgp->silent++;
	if (test_unit_ready(scgp) < 0 && scsi_sense_code(scgp) != 0xD4) {
		scgp->cmdname = "close_track/test_unit_ready";
		scsiprinterr(scgp);
	}
	scgp->silent--;

	return (0);
}

LOCAL int
finalize_sony(scgp, onp, dummy, type, tracks, trackp)
	SCSI	*scgp;
	int	onp;	/* open next program area */
	int	dummy;
	int	type;	/* TOC type 0: CD-DA, 1: CD-ROM, 2: CD-ROM/XA1, 3: CD-ROM/XA2, 4: CDI */
	int	tracks;
	track_t	*trackp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (dummy) {
		printf("Fixating is not possible in dummy write mode.\n");
		return (0);
	}
	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->timeout = 8 * 60;		/* Needs up to 4 minutes */
	scmd->cdb.g1_cdb.cmd = 0xF1;
	scmd->cdb.g1_cdb.lun = scgp->lun;
	scmd->cdb.g1_cdb.count[1] = (onp ? 1 : 0);
/* XXX Padding ??? (bit 0 in addr[0]) */
	
	scgp->cmdname = "finalize";

	if (scsicmd(scgp) < 0)
		return (-1);
	return (0);
}

LOCAL int
recover_sony(scgp, track)
	SCSI	*scgp;
	int	track;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g1_cdb.cmd = 0xF6;
	scmd->cdb.g1_cdb.lun = scgp->lun;
	scmd->cdb.g1_cdb.addr[3] = track;
	
	scgp->cmdname = "recover";

	if (scsicmd(scgp) < 0)
		return (-1);
	return (0);
}

LOCAL int
next_wr_addr_sony(scgp, track, trackp, ap)
	SCSI	*scgp;
	int	track;
	track_t	*trackp;
	long	*ap;
{
	if (next_writable_address_sony(scgp, ap, 0, 0, 0) < 0)
		return (-1);
	return (0);
}

LOCAL int
reserve_track_sony(scgp, len)
	SCSI	*scgp;
	unsigned long len;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g1_cdb.cmd = 0xF3;
	scmd->cdb.g1_cdb.lun = scgp->lun;
	i_to_4_byte(&scmd->cdb.g1_cdb.addr[3], len);
	
	scgp->cmdname = "reserve_track";

	if (scsicmd(scgp) < 0)
		return (-1);
	return (0);
}

#define	IS(what,flag)	printf("  Is %s%s\n", flag?"":"not ",what);

LOCAL int
getdisktype_sony(scgp, dp, dsp)
	SCSI	*scgp;
	cdr_t	*dp;
	dstat_t	*dsp;
{
	long	dummy;
	long	lst;
	msf_t	msf;

	char			mode[256];
	struct scsi_mode_page_header	*mp;
	struct sony_924_mode_page_22	*xp;

	dummy = get_page22_sony(scgp, mode);
	if (dummy >= 0) {
		mp = (struct scsi_mode_page_header *)
			(mode + sizeof(struct scsi_mode_header) +
			((struct scsi_mode_header *)mode)->blockdesc_len);

		xp = (struct sony_924_mode_page_22 *)mp;

		if (xp->disk_appl_code[0] == 0xFF)
			dummy = -1;
	}
	if (dummy < 0)
		return (drive_getdisktype(scgp, dp, dsp));

	if (lverbose && dummy >= 0) {

		printf("ATIP info from disk:\n");
		printf("  Indicated writing power: %d\n",
				(unsigned)(xp->disk_appl_code[1] & 0x70) >> 4);
		IS("unrestricted", xp->disk_appl_code[2] & 0x40);
		printf("  Disk application code: %d\n", xp->disk_appl_code[2] & 0x3F);
		msf.msf_min = xp->lead_in_start[1];
		msf.msf_sec = xp->lead_in_start[2];
		msf.msf_frame = xp->lead_in_start[3];
		lst = msf_to_lba(msf.msf_min, msf.msf_sec, msf.msf_frame);
		if (lst  < -150) {
			/*
			 * The Sony CDU 920 seems to deliver 00:00/00 for
			 * lead-in start time, dont use it.
			 */
			printf("  ATIP start of lead in:  %ld (%02d:%02d/%02d)\n",
				msf_to_lba(msf.msf_min, msf.msf_sec, msf.msf_frame),
				msf.msf_min, msf.msf_sec, msf.msf_frame);
		}
		msf.msf_min = xp->last_start_time[1];
		msf.msf_sec = xp->last_start_time[2];
		msf.msf_frame = xp->last_start_time[3];
		printf("  ATIP start of lead out: %ld (%02d:%02d/%02d)\n",
			msf_to_lba(msf.msf_min, msf.msf_sec, msf.msf_frame),
			msf.msf_min, msf.msf_sec, msf.msf_frame);
		if (lst  < -150) {
			/*
			 * The Sony CDU 920 seems to deliver 00:00/00 for
			 * lead-in start time, dont use it.
			 */
			msf.msf_min = xp->lead_in_start[1];
			msf.msf_sec = xp->lead_in_start[2];
			msf.msf_frame = xp->lead_in_start[3];
			pr_manufacturer(&msf,
					FALSE,	/* Always not erasable */
					(xp->disk_appl_code[2] & 0x40) != 0);
		}
	}
	if (dummy >= 0)
		di_to_dstat_sony(xp, dsp);
	return (drive_getdisktype(scgp, dp, dsp));
}

LOCAL void
di_to_dstat_sony(dip, dsp)
	struct sony_924_mode_page_22	*dip;
	dstat_t	*dsp;
{
	msf_t	msf;

	dsp->ds_diskid = a_to_u_4_byte(dip->disk_id_code);
	if (dsp->ds_diskid != -1)
		dsp->ds_flags |= DSF_DID_V;
	dsp->ds_diskstat = (dip->disk_status >> 6) & 0x03;
#ifdef	XXX
	/*
	 * There seems to be no MMC equivalent...
	 */
	dsp->ds_sessstat = dip->sess_status;
#endif

	dsp->ds_maxblocks = msf_to_lba(dip->last_start_time[1],
					dip->last_start_time[2],
					dip->last_start_time[3]);
	/*
	 * Check for 0xFF:0xFF/0xFF which is an indicator for a complete disk
	 */
	if (dsp->ds_maxblocks == 716730)
		dsp->ds_maxblocks = -1L;

	if (dsp->ds_first_leadin == 0) {
		dsp->ds_first_leadin = msf_to_lba(dip->lead_in_start[1],
						dip->lead_in_start[2],
						dip->lead_in_start[3]);
		/*
		 * Check for illegal values (> 0)
		 * or for empty field (-150) with CDU-920.
		 */
		if (dsp->ds_first_leadin > 0 || dsp->ds_first_leadin == -150)
			dsp->ds_first_leadin = 0;
	}

	if (dsp->ds_last_leadout == 0 && dsp->ds_maxblocks >= 0)
		dsp->ds_last_leadout = dsp->ds_maxblocks;

	msf.msf_min = dip->lead_in_start[1];
	msf.msf_sec = dip->lead_in_start[2];
	msf.msf_frame = dip->lead_in_start[3];
	dsp->ds_maxrblocks = disk_rcap(&msf, dsp->ds_maxblocks,
					FALSE,	/* Always not erasable */
					(dip->disk_appl_code[2] & 0x40) != 0);
}


int	sony_speeds[] = {
		-1,		/* Speed null is not allowed */
		0,		/* Single speed */
		1,		/* Double speed */
		-1,		/* Three times */
		3,		/* Quad speed */
};

LOCAL int
speed_select_sony(scgp, speedp, dummy)
	SCSI	*scgp;
	int	*speedp;
	int	dummy;
{
	struct cdd_52x_mode_data md;
	int	count;
	int	err;
	int	speed = 1;

	if (speedp) {
		speed = *speedp;
		if (speed < 1 || speed > 4 || sony_speeds[speed] < 0)
			return (-1);
	}

	fillbytes((caddr_t)&md, sizeof(md), '\0');

	count  = sizeof(struct scsi_mode_header) +
		sizeof(struct sony_924_mode_page_20);

	md.pagex.page_s20.p_code = 0x20;
	md.pagex.page_s20.p_len =  0x06;
	md.pagex.page_s20.speudo = dummy?1:0;
	
	err = mode_select(scgp, (u_char *)&md, count, 0, 1);
	if (err < 0)
		return (err);

	if (speedp == 0)
		return (0);

	fillbytes((caddr_t)&md, sizeof(md), '\0');

	count  = sizeof(struct scsi_mode_header) +
		sizeof(struct sony_924_mode_page_31);

	md.pagex.page_s31.p_code = 0x31;
	md.pagex.page_s31.p_len =  0x02;
	md.pagex.page_s31.speed = sony_speeds[speed];
	
	return (mode_select(scgp, (u_char *)&md, count, 0, 1));
}

LOCAL int
next_writable_address_sony(scgp, ap, track, sectype, tracktype)
	SCSI	*scgp;
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
	if (!get_mode_params(scgp, page, "CD track information",
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
		*ap = a_to_4_byte(xp->next_recordable_addr);
	return (0);
}


LOCAL int
new_track_sony(scgp, track, sectype, tracktype)
	SCSI	*scgp;
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
	get_page22_sony(scgp, mode);

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	inc_verbose();
	if (!get_mode_params(scgp, page, "CD track information",
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

	if (mode_select(scgp, (u_char *)mode, len, 0, scgp->inq->data_format >= 2) < 0) {
		return (-1);
	}

	return (0);
}

LOCAL int
open_track_sony(scgp, dp, track, track_info)
	SCSI	*scgp;
	cdr_t	*dp;
	int	track;
	track_t *track_info;
{
	if (select_secsize(scgp, track_info->secsize) < 0)
		return (-1);

	if (new_track_sony(scgp, track, track_info->sectype, track_info->tracktype) < 0)
		return (-1);

	if (write_track_sony(scgp, 0L, track_info->sectype) < 0)
		return (-1);

	return (0);
}

LOCAL int
open_session_sony(scgp, dp, tracks, trackp, toctype, multi)
	SCSI	*scgp;
	cdr_t	*dp;
	int	tracks;
	track_t	*trackp;
	int	toctype;
	int	multi;
{
	struct	scsi_mode_page_header *mp;
	char			mode[256];
	int	i;
	int	len = 0x30;
	struct sony_924_mode_page_22	*xp;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	if ((len = get_page22_sony(scgp, mode)) < 0)
		return (-1);

	mp = (struct scsi_mode_page_header *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	xp = (struct sony_924_mode_page_22 *)mp;

	xp->disk_type = toc2sess[toctype & TOC_MASK];

	((struct scsi_modesel_header *)mode)->sense_data_len	= 0;
	((struct scsi_modesel_header *)mode)->res2		= 0;

	i = ((struct scsi_mode_header *)mode)->blockdesc_len;
	if (i > 0) {
		i_to_3_byte(
			((struct scsi_mode_data *)mode)->blockdesc.nlblock,
								0);
	}

	if (mode_select(scgp, (u_char *)mode, len, 0, scgp->inq->data_format >= 2) < 0) {
		return (-1);
	}
	return (0);
}

LOCAL int
get_page22_sony(scgp, mode)
	SCSI	*scgp;
	char	*mode;
{
	struct	scsi_mode_page_header *mp;
	int	len = 0x30;
	int	page = 0x22;
	struct sony_924_mode_page_22	*xp;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	inc_verbose();
	if (!get_mode_params(scgp, page, "CD disk information",
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
	return (len);
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
	"\313\001write error track recovered",			/* CB 01 */
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
sony_attach(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	scsi_setnonstderrs(scgp, sd_cdu_924_error_str);
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
	printf("disk appl code: %lX\n", a_to_u_4_byte(xp->disk_appl_code));
	printf("last start time: %lX\n", a_to_u_4_byte(xp->last_start_time));
	printf("disk status: %X\n", xp->disk_status);
	printf("num valid nra: %X\n", xp->num_valid_nra);
	printf("track info track: %X\n", xp->track_info_track);
	printf("post gap: %X\n", xp->post_gap);
	printf("disk id code: %lX\n", a_to_u_4_byte(xp->disk_id_code));
	printf("lead in start: %lX\n", a_to_u_4_byte(xp->lead_in_start));
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

/*
 * XXX Check for signed/unsigned a_to_*() conversion.
 */
	printf("start lba: %lX\n", a_to_4_byte(xp->start_lba));
	printf("next recordable addr: %lX\n", a_to_4_byte(xp->next_recordable_addr));
	printf("blank area cap: %lX\n", a_to_u_4_byte(xp->blank_area_cap));
	printf("fixed packet size: %lX\n", a_to_u_4_byte(xp->fixed_packet_size));
	printf("starting msf: %lX\n", a_to_u_4_byte(xp->starting_msf));
	printf("ending msf: %lX\n", a_to_u_4_byte(xp->ending_msf));
	printf("next rec time: %lX\n", a_to_u_4_byte(xp->next_rec_time));
}
#endif

LOCAL int
buf_cap_sony(scgp, sp, fp)
	SCSI	*scgp;
	long	*sp;	/* Size pointer */
	long	*fp;	/* Free pointer */
{
	char	resp[8];
	Ulong	freespace;
	Ulong	bufsize;
	int	per;
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = (caddr_t)resp;
	scmd->size = sizeof(resp);
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g1_cdb.cmd = 0xEC;		/* Read buffer cap */
	scmd->cdb.g1_cdb.lun = scgp->lun;
	
	scgp->cmdname = "read buffer cap sony";

	if (scsicmd(scgp) < 0)
		return (-1);

	bufsize   = a_to_u_3_byte(&resp[1]);
	freespace = a_to_u_3_byte(&resp[5]);
	if (sp)
		*sp = bufsize;
	if (fp)
		*fp = freespace;
	
	if (scgp->verbose || (sp == 0 && fp == 0))
		printf("BFree: %ld K BSize: %ld K\n", freespace >> 10, bufsize >> 10);

	if (bufsize == 0)
		return (0);
	per = (100 * (bufsize - freespace)) / bufsize;
	if (per < 0)
		return (0);
	if (per > 100)
		return (100);
	return (per);
}
