/* @(#)drv_mmc.c	1.56 00/07/02 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)drv_mmc.c	1.56 00/07/02 Copyright 1997 J. Schilling";
#endif
/*
 *	CDR device implementation for
 *	SCSI-3/mmc conforming drives
 *	e.g. Yamaha CDR-400, Ricoh MP6200
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

/*#define	DEBUG*/
#define	PRINT_ATIP
#include <mconfig.h>

#include <stdio.h>
#include <standard.h>
#include <fctldefs.h>
#include <errno.h>
#include <strdefs.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <timedefs.h>

#include <utypes.h>
#include <btorder.h>
#include <intcvt.h>
#include <schily.h>

#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include <scsimmc.h>
#include "cdrecord.h"

extern	BOOL	isgui;
extern	char	*driveropts;

extern	int	debug;
extern	int	lverbose;

LOCAL	int	curspeed = 1;

LOCAL	int	mmc_load		__PR((SCSI *scgp));
LOCAL	int	mmc_unload		__PR((SCSI *scgp));
LOCAL	void	mmc_opthelp		__PR((cdr_t *dp, int excode));
LOCAL	cdr_t	*identify_mmc		__PR((SCSI *scgp, cdr_t *, struct scsi_inquiry *));
LOCAL	int	attach_mmc		__PR((SCSI *scgp, cdr_t *));
LOCAL	int	get_diskinfo		__PR((SCSI *scgp, struct disk_info *dip));
LOCAL	void	di_to_dstat		__PR((struct disk_info *dip, dstat_t *dsp));
#ifdef	PRINT_ATIP
LOCAL	int	get_pma			__PR((SCSI *scgp));
#endif
LOCAL	int	getdisktype_mmc		__PR((SCSI *scgp, cdr_t *dp, dstat_t *dsp));
LOCAL	int	speed_select_mmc	__PR((SCSI *scgp, int *speedp, int dummy));
LOCAL	int	next_wr_addr_mmc	__PR((SCSI *scgp, int track, track_t *trackp, long *ap));
LOCAL	int	open_track_mmc		__PR((SCSI *scgp, cdr_t *dp, int track, track_t *trackp));
LOCAL	int	close_track_mmc		__PR((SCSI *scgp, int track, track_t *trackp));
LOCAL	int	open_session_mmc	__PR((SCSI *scgp, cdr_t *dp, int tracks, track_t *trackp, int toctype, int multi));
LOCAL	int	waitfix_mmc		__PR((SCSI *scgp, int secs));
LOCAL	int	fixate_mmc		__PR((SCSI *scgp, int onp, int dummy, int toctype, int tracks, track_t *trackp));
LOCAL	int	blank_mmc		__PR((SCSI *scgp, long addr, int blanktype));
LOCAL	int	send_opc_mmc		__PR((SCSI *scgp, caddr_t, int cnt, int doopc));
LOCAL	int	scsi_sony_write		__PR((SCSI *scgp, caddr_t bp, long sectaddr, long size, int blocks, BOOL islast));

LOCAL	int	send_cue	__PR((SCSI *scgp, int tracks, track_t *trackp));

LOCAL int
mmc_load(scgp)
	SCSI	*scgp;
{
	return (scsi_load_unload(scgp, 1));
}

LOCAL int
mmc_unload(scgp)
	SCSI	*scgp;
{
	return (scsi_load_unload(scgp, 0));
}

cdr_t	cdr_mmc = {
	0,
	CDR_TAO|CDR_DAO|CDR_PACKET|CDR_SWABAUDIO,
	"mmc_cdr",
	"generic SCSI-3/mmc CD-R driver",
	0,
	identify_mmc,
	attach_mmc,
	getdisktype_mmc,
	scsi_load,
/*	mmc_load,*/
	scsi_unload,
	read_buff_cap,
	(int(*)__PR((SCSI *)))cmd_dummy,	/* recovery_needed	*/
	(int(*)__PR((SCSI *, int)))cmd_dummy,	/* recover		*/
	speed_select_mmc,
	select_secsize,
	next_wr_addr_mmc,
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,	/* reserve_track	*/
	scsi_cdr_write,
	send_cue,
	open_track_mmc,
	close_track_mmc,
	open_session_mmc,
	cmd_dummy,
	read_session_offset,
	fixate_mmc,
	blank_mmc,
	send_opc_mmc,
};

cdr_t	cdr_mmc_sony = {
	0,
	CDR_TAO|CDR_DAO|CDR_PACKET|CDR_SWABAUDIO,
	"mmc_cdr_sony",
	"generic SCSI-3/mmc CD-R driver (Sony 928 variant)",
	0,
	identify_mmc,
	attach_mmc,
	getdisktype_mmc,
	scsi_load,
/*	mmc_load,*/
	scsi_unload,
	read_buff_cap,
	(int(*)__PR((SCSI *)))cmd_dummy,	/* recovery_needed	*/
	(int(*)__PR((SCSI *, int)))cmd_dummy,	/* recover		*/
	speed_select_mmc,
	select_secsize,
	next_wr_addr_mmc,
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,	/* reserve_track	*/
	scsi_sony_write,
	send_cue,
	open_track_mmc,
	close_track_mmc,
	open_session_mmc,
	cmd_dummy,
	read_session_offset,
	fixate_mmc,
	blank_mmc,
	send_opc_mmc,
};

/*
 * SCSI-3/mmc conformant CD drive
 */
cdr_t	cdr_cd = {
	0,
	CDR_ISREADER|CDR_SWABAUDIO,
	"mmc_cd",
	"generic SCSI-3/mmc CD driver",
	0,
	identify_mmc,
	attach_mmc,
	drive_getdisktype,
	scsi_load,
	scsi_unload,
	read_buff_cap,
	(int(*)__PR((SCSI *)))cmd_dummy,	/* recovery_needed	*/
	(int(*)__PR((SCSI *, int)))cmd_dummy,	/* recover		*/
	speed_select_mmc,
	select_secsize,
	(int(*)__PR((SCSI *scgp, int, track_t *, long *)))cmd_ill,	/* next_wr_addr		*/
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,	/* reserve_track	*/
	scsi_cdr_write,
	no_sendcue,
	open_track_mmc,
	close_track_mmc,
	(int(*)__PR((SCSI *scgp, cdr_t *, int, track_t *, int, int)))cmd_dummy,
	cmd_dummy,
	read_session_offset,
	(int(*)__PR((SCSI *scgp, int, int, int, int, track_t *)))cmd_dummy,	/* fixation */
	blank_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
};

/*
 * Old pre SCSI-3/mmc CD drive
 */
cdr_t	cdr_oldcd = {
	0,
	CDR_ISREADER,
	"scsi2_cd",
	"generic SCSI-2 CD driver",
	0,
	identify_mmc,
	drive_attach,
	drive_getdisktype,
	scsi_load,
	scsi_unload,
	buf_dummy,
	(int(*)__PR((SCSI *)))cmd_dummy,	/* recovery_needed	*/
	(int(*)__PR((SCSI *, int)))cmd_dummy,	/* recover		*/
	speed_select_mmc,
	select_secsize,
	(int(*)__PR((SCSI *scg, int, track_t *, long *)))cmd_ill,	/* next_wr_addr		*/
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,	/* reserve_track	*/
	scsi_cdr_write,
	no_sendcue,
	open_track_mmc,
	close_track_mmc,
	(int(*)__PR((SCSI *scgp, cdr_t *, int, track_t *, int, int)))cmd_dummy,
	cmd_dummy,
	read_session_offset_philips,
	(int(*)__PR((SCSI *scgp, int, int, int, int, track_t *)))cmd_dummy,	/* fixation */
	blank_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
};

LOCAL void
mmc_opthelp(dp, excode)
	cdr_t	*dp;
	int	excode;
{
	error("Driver options:\n");
	if (dp->cdr_cdcap->res_4 != 0) {
		error("burnproof	Prepare writer to use Sanyo BURN-Proof technology\n");
		error("noburnproof	Disable using Sanyo BURN-Proof technology\n");
	} else {
		error("None supported for this drive.\n");
	}
	exit(excode);
}

LOCAL cdr_t *
identify_mmc(scgp, dp, ip)
	SCSI	*scgp;
	cdr_t			*dp;
	struct scsi_inquiry	*ip;
{
	BOOL	cdrr	 = FALSE;	/* Read CD-R	*/
	BOOL	cdwr	 = FALSE;	/* Write CD-R	*/
	BOOL	cdrrw	 = FALSE;	/* Read CD-RW	*/
	BOOL	cdwrw	 = FALSE;	/* Write CD-RW	*/
	Uchar	mode[0x100];
	struct	cd_mode_page_2A *mp;

	if (ip->type != INQ_WORM && ip->type != INQ_ROMD)
		return ((cdr_t *)0);

	allow_atapi(scgp, TRUE);/* Try to switch to 10 byte mode cmds */

	scgp->silent++;
	mp = mmc_cap(scgp, mode);	/* Get MMC capabilities */
	scgp->silent--;
	if (mp == NULL)
		return (&cdr_oldcd);	/* Pre SCSI-3/mmc drive	 	*/

	mmc_getval(mp, &cdrr, &cdwr, &cdrrw, &cdwrw, NULL);

	/*
	 * At this point we know that we have a SCSI-3/mmc compliant drive.
	 * Unfortunately ATAPI drives violate the SCSI spec in returning
	 * a response data format of '1' which from the SCSI spec would
	 * tell us not to use the "PF" bit in mode select. As ATAPI drives
	 * require the "PF" bit to be set, we 'correct' the inquiry data.
	 *
	 * XXX xxx_identify() should not have any side_effects ??
	 */
	if (ip->data_format < 2)
		ip->data_format = 2;

	if (strncmp(ip->vendor_info, "SONY", 4) == 0 &&
	    strncmp(ip->prod_ident, "CD-R   CDU928E", 14) == 0) {
		dp = &cdr_mmc_sony;
	}
	if (!cdwr)			/* SCSI-3/mmc CD drive		*/
		dp = &cdr_cd;

	return (dp);
}

LOCAL int
attach_mmc(scgp, dp)
	SCSI	*scgp;
	cdr_t			*dp;
{
	struct	cd_mode_page_2A *mp;

	allow_atapi(scgp, TRUE);/* Try to switch to 10 byte mode cmds */

	scgp->silent++;
	mp = mmc_cap(scgp, NULL);/* Get MMC capabilities in allocated mp */
	scgp->silent--;
	if (mp == NULL)
		return (-1);	/* Pre SCSI-3/mmc drive	 	*/

	dp->cdr_cdcap = mp;	/* Store MMC cap pointer	*/

	if (mp->loading_type == LT_TRAY)
		dp->cdr_flags |= CDR_TRAYLOAD;
	else if (mp->loading_type == LT_CADDY)
		dp->cdr_flags |= CDR_CADDYLOAD;

	if (driveropts != NULL) {
		if (strcmp(driveropts, "help") == 0) {
			mmc_opthelp(dp, 0);
		}
	}

	return (0);
}

#ifdef	PRINT_ATIP
LOCAL	int	get_atip		__PR((SCSI *scgp, struct atipinfo *atp));
	void	print_di		__PR((struct disk_info *dip));
	void	print_atip		__PR((SCSI *scgp, struct atipinfo *atp));
#endif	/* PRINT_ATIP */

LOCAL int
get_diskinfo(scgp, dip)
	SCSI		*scgp;
	struct disk_info *dip;
{
	int	len;
	int	ret;

	fillbytes((caddr_t)dip, sizeof(*dip), '\0');

	if (read_disk_info(scgp, (caddr_t)dip, 2) < 0)
		return (-1);
	len = a_to_u_2_byte(dip->data_len);
	len += 2;
	ret = read_disk_info(scgp, (caddr_t)dip, len);

#ifdef	DEBUG
	scsiprbytes("Disk info:", (u_char *)dip,
				len-scsigetresid(scgp));
#endif
	return (ret);
}

LOCAL void
di_to_dstat(dip, dsp)
	struct disk_info	*dip;
	dstat_t	*dsp;
{
	dsp->ds_diskid = a_to_u_4_byte(dip->disk_id);
	if (dip->did_v)
		dsp->ds_flags |= DSF_DID_V;
	dsp->ds_diskstat = dip->disk_status;
	dsp->ds_sessstat = dip->sess_status;

	dsp->ds_maxblocks = msf_to_lba(dip->last_lead_out[1],
					dip->last_lead_out[2],
					dip->last_lead_out[3], TRUE);
	/*
	 * Check for 0xFF:0xFF/0xFF which is an indicator for a complete disk
	 */
	if (dsp->ds_maxblocks == 716730)
		dsp->ds_maxblocks = -1L;

	if (dsp->ds_first_leadin == 0) {
		dsp->ds_first_leadin = msf_to_lba(dip->last_lead_in[1],
						dip->last_lead_in[2],
						dip->last_lead_in[3], FALSE);
		if (dsp->ds_first_leadin > 0)
			dsp->ds_first_leadin = 0;
	}

	if (dsp->ds_last_leadout == 0 && dsp->ds_maxblocks >= 0)
		dsp->ds_last_leadout = dsp->ds_maxblocks;
}

#ifdef	PRINT_ATIP

LOCAL int
get_atip(scgp, atp)
	SCSI		*scgp;
	struct atipinfo *atp;
{
	int	len;
	int	ret;

	fillbytes((caddr_t)atp, sizeof(*atp), '\0');

	if (read_toc(scgp, (caddr_t)atp, 0, 2, 0, FMT_ATIP) < 0)
		return (-1);
	len = a_to_u_2_byte(atp->hd.len);
	len += 2;
	ret = read_toc(scgp, (caddr_t)atp, 0, len, 0, FMT_ATIP);

#ifdef	DEBUG
	scsiprbytes("ATIP info:", (u_char *)atp,
				len-scsigetresid(scgp));
#endif
	/*
	 * Yamaha sometimes returns zeroed ATIP info for disks without ATIP
	 */
	if (atp->desc.lead_in[1] == 0 &&
			atp->desc.lead_in[2] == 0 &&
			atp->desc.lead_in[3] == 0 &&
			atp->desc.lead_out[1] == 0 &&
			atp->desc.lead_out[2] == 0 &&
			atp->desc.lead_out[3] == 0)
		return (-1);

	if (atp->desc.lead_in[1] >= 0x90 && debug) {
		/*
		 * Only makes sense with buggy Ricoh firmware.
		 */
		errmsgno(EX_BAD, "Converting ATIP from BCD\n");
		atp->desc.lead_in[1] = from_bcd(atp->desc.lead_in[1]);
		atp->desc.lead_in[2] = from_bcd(atp->desc.lead_in[2]);
		atp->desc.lead_in[3] = from_bcd(atp->desc.lead_in[3]);

		atp->desc.lead_out[1] = from_bcd(atp->desc.lead_out[1]);
		atp->desc.lead_out[2] = from_bcd(atp->desc.lead_out[2]);
		atp->desc.lead_out[3] = from_bcd(atp->desc.lead_out[3]);
	}

	return (ret);
}

LOCAL int
/*get_pma(atp)*/
get_pma(scgp)
	SCSI	*scgp;
/*	struct atipinfo *atp;*/
{
	int	len;
	int	ret;
char	atp[1024];

	fillbytes((caddr_t)atp, sizeof(*atp), '\0');

/*	if (read_toc(scgp, (caddr_t)atp, 0, 2, 1, FMT_PMA) < 0)*/
	if (read_toc(scgp, (caddr_t)atp, 0, 2, 0, FMT_PMA) < 0)
		return (-1);
/*	len = a_to_u_2_byte(atp->hd.len);*/
	len = a_to_u_2_byte(atp);
	len += 2;
/*	ret = read_toc(scgp, (caddr_t)atp, 0, len, 1, FMT_PMA);*/
	ret = read_toc(scgp, (caddr_t)atp, 0, len, 0, FMT_PMA);

#ifdef	DEBUG
	scsiprbytes("PMA:", (u_char *)atp,
				len-scsigetresid(scgp));
#endif
	ret = read_toc(scgp, (caddr_t)atp, 0, len, 1, FMT_PMA);

#ifdef	DEBUG
	scsiprbytes("PMA:", (u_char *)atp,
				len-scsigetresid(scgp));
#endif
	return (ret);
}

#endif	/* PRINT_ATIP */

LOCAL int
getdisktype_mmc(scgp, dp, dsp)
	SCSI	*scgp;
	cdr_t	*dp;
	dstat_t	*dsp;
{
extern	char	*buf;
	struct disk_info *dip;
	u_char	mode[0x100];
	char	ans[2];
	msf_t	msf;
	BOOL	did_atip = FALSE;
	BOOL	did_dummy = FALSE;

	msf.msf_min = msf.msf_sec = msf.msf_frame = 0;
#ifdef	PRINT_ATIP
	/*
	 * It seems that there are drives that do not support to
	 * read ATIP (e.g. HP 7100)
	 * Also if a NON CD-R media is inserted, this will never work.
	 * For this reason, make a failure non-fatal.
	 */
	scgp->silent++;
	if (get_atip(scgp, (struct atipinfo *)mode) >= 0) {
		msf.msf_min =		mode[8];
		msf.msf_sec =		mode[9];
		msf.msf_frame =		mode[10];
		did_atip = TRUE;
		if (lverbose) {
			print_atip(scgp, (struct atipinfo *)mode);
			pr_manufacturer(&msf,
				((struct atipinfo *)mode)->desc.erasable,
				((struct atipinfo *)mode)->desc.uru);
		}
	}
	scgp->silent--;
#endif
again:
	dip = (struct disk_info *)buf;
	if (get_diskinfo(scgp, dip) < 0)
		return (-1);

	/*
	 * Check for non writable disk first.
	 */
	if (dip->disk_status == DS_COMPLETE && dsp->ds_cdrflags & RF_WRITE) {
		if (!did_dummy) {
			int	xspeed = 0xFFFF;

			/*
			 * Try to clear the dummy bit to reset the virtual
			 * drive status. Not all drives support it even though
			 * it is mentioned in the MMC standard.
			 */
			if (lverbose)
				printf("Trying to clear drive status.\n");
			speed_select_mmc(scgp, &xspeed, FALSE);
			did_dummy = TRUE;
			goto again;
		}
		errmsgno(EX_BAD, "Drive needs to reload the media to return to proper status.\n");
		unload_media(scgp, dp, F_EJECT);

		if ((dp->cdr_flags & CDR_TRAYLOAD) != 0) {
			scgp->silent++;
			load_media(scgp, dp, FALSE);
			scgp->silent--;
		}
		scgp->silent++;
		if (((dp->cdr_flags & CDR_TRAYLOAD) == 0) ||
				!wait_unit_ready(scgp, 5)) {
			scgp->silent--;

			printf("Re-load disk and hit <CR>");
			if (isgui)
				printf("\n");
			flush();
			getline(ans, 1);
		} else {
			scgp->silent--;
		}
		load_media(scgp, dp, TRUE);
	}
	if (get_diskinfo(scgp, dip) < 0)
		return (-1);
	di_to_dstat(dip, dsp);
	if (!did_atip && dsp->ds_first_leadin < 0)
		lba_to_msf(dsp->ds_first_leadin, &msf);

	if (lverbose && !did_atip) {
		print_min_atip(dsp->ds_first_leadin, dsp->ds_last_leadout);
		if (dsp->ds_first_leadin < 0)
	                pr_manufacturer(&msf,
				dip->erasable,
				dip->uru);
	}
	dsp->ds_maxrblocks = disk_rcap(&msf, dsp->ds_maxblocks,
				dip->erasable,
				dip->uru);


#ifdef	PRINT_ATIP
#ifdef	DEBUG
	if (get_atip(scgp, (struct atipinfo *)mode) < 0)
		return (-1);
	/*
	 * Get pma gibt Ärger mit CW-7502
	 * Wenn Die Disk leer ist, dann stuerzt alles ab.
	 * Firmware 4.02 kann nicht get_pma
	 */
	if (dip->disk_status != DS_EMPTY) {
/*		get_pma();*/
	}
	printf("ATIP lead in:  %ld (%02d:%02d/%02d)\n",
		msf_to_lba(mode[8], mode[9], mode[10], FALSE),
		mode[8], mode[9], mode[10]);
	printf("ATIP lead out: %ld (%02d:%02d/%02d)\n",
		msf_to_lba(mode[12], mode[13], mode[14], TRUE),
		mode[12], mode[13], mode[14]);
	print_di(dip);
	print_atip(scgp, (struct atipinfo *)mode);
#endif
#endif	/* PRINT_ATIP */
	return (drive_getdisktype(scgp, dp, dsp));
}

#ifdef	PRINT_ATIP

#define	DOES(what,flag)	printf("  Does %s%s\n", flag?"":"not ",what);
#define	IS(what,flag)	printf("  Is %s%s\n", flag?"":"not ",what);
#define	VAL(what,val)	printf("  %s: %d\n", what, val[0]*256 + val[1]);
#define	SVAL(what,val)	printf("  %s: %s\n", what, val);

void
print_di(dip)
	struct disk_info	*dip;
{
static	char *ds_name[] = { "empty", "incomplete/appendable", "complete", "illegal" };
static	char *ss_name[] = { "empty", "incomplete/appendable", "illegal", "complete", };

	IS("erasable", dip->erasable);
	printf("disk status: %s\n", ds_name[dip->disk_status]);
	printf("session status: %s\n", ss_name[dip->sess_status]);
	printf("first track: %d number of sessions: %d first track in last sess: %d last track in last sess: %d\n",
		dip->first_track,
		dip->numsess,
		dip->first_track_ls,
		dip->last_track_ls);
	IS("unrestricted", dip->uru);
	printf("Disk type: ");
	switch (dip->disk_type) {

	case SES_DA_ROM:	printf("CD-DA or CD-ROM");	break;
	case SES_CDI:		printf("CDI");			break;
	case SES_XA:		printf("CD-ROM XA");		break;
	case SES_UNDEF:		printf("undefined");		break;
	default:		printf("reserved");		break;
	}
	printf("\n");
	if (dip->did_v)
		printf("Disk id: 0x%lX\n", a_to_u_4_byte(dip->disk_id));

	printf("last start of lead in: %ld\n",
		msf_to_lba(dip->last_lead_in[1],
		dip->last_lead_in[2],
		dip->last_lead_in[3], FALSE));
	printf("last start of lead out: %ld\n",
		msf_to_lba(dip->last_lead_out[1],
		dip->last_lead_out[2],
		dip->last_lead_out[3], TRUE));

	if (dip->dbc_v)
		printf("Disk bar code: 0x%lX%lX\n",
			a_to_u_4_byte(dip->disk_barcode),
			a_to_u_4_byte(&dip->disk_barcode[4]));

	if (dip->num_opc_entries > 0) {
		printf("OPC table:\n");
	}
}

LOCAL	char	clv_to_speed[8] = {
		0, 2, 4, 6, 8, 0, 0, 0
};

char	*cdr_subtypes[] = {
	"Normal Rewritable (CLV) media",
	"High speed Rewritable (CAV) media",
	"Medium Type A, low Beta category (A-)",
	"Medium Type A, high Beta category (A+)",
	"Medium Type B, low Beta category (B-)",
	"Medium Type B, high Beta category (B+)",
	"Medium Type C, low Beta category (C-)",
	"Medium Type C, high Beta category (C+)",
};

void
print_atip(scgp, atp)
	SCSI		*scgp;
	struct atipinfo *atp;
{
	char	*sub_type;

	if (scgp->verbose)
		scsiprbytes("ATIP info: ", (Uchar *)atp, sizeof (*atp));

	printf("ATIP info from disk:\n");
	printf("  Indicated writing power: %d\n", atp->desc.ind_wr_power);
	if (atp->desc.erasable || atp->desc.ref_speed)
		printf("  Reference speed: %d\n", clv_to_speed[atp->desc.ref_speed]);
	IS("unrestricted", atp->desc.uru);
/*	printf("  Disk application code: %d\n", atp->desc.res5_05);*/
	IS("erasable", atp->desc.erasable);
	sub_type = cdr_subtypes[atp->desc.sub_type];
	if (atp->desc.sub_type)
		printf("  Disk sub type: %s (%d)\n", sub_type, atp->desc.sub_type);
	printf("  ATIP start of lead in:  %ld (%02d:%02d/%02d)\n",
		msf_to_lba(atp->desc.lead_in[1],
		atp->desc.lead_in[2],
		atp->desc.lead_in[3], FALSE),
		atp->desc.lead_in[1],
		atp->desc.lead_in[2],
		atp->desc.lead_in[3]);
	printf("  ATIP start of lead out: %ld (%02d:%02d/%02d)\n",
		msf_to_lba(atp->desc.lead_out[1],
		atp->desc.lead_out[2],
		atp->desc.lead_out[3], TRUE),
		atp->desc.lead_out[1],
		atp->desc.lead_out[2],
		atp->desc.lead_out[3]);
	if (atp->desc.a1_v) {
		if (atp->desc.clv_low != 0 || atp->desc.clv_high != 0)
			printf("  speed low: %d speed high: %d\n",
				clv_to_speed[atp->desc.clv_low],
				clv_to_speed[atp->desc.clv_high]);
		printf("  power mult factor: %d %d\n", atp->desc.power_mult, atp->desc.tgt_y_pow);
		if (atp->desc.erasable)
			printf("  recommended erase/write power: %d\n", atp->desc.rerase_pwr_ratio);
	}
	if (atp->desc.a2_v) {
		printf("  A2 values: %02X %02X %02X\n",
				atp->desc.a2[0],
				atp->desc.a2[1],
				atp->desc.a2[2]);
	}
	if (atp->desc.a3_v) {
		printf("  A3 values: %02X %02X %02X\n",
				atp->desc.a3[0],
				atp->desc.a3[1],
				atp->desc.a3[2]);
	}
}
#endif	/* PRINT_ATIP */

LOCAL int
speed_select_mmc(scgp, speedp, dummy)
	SCSI	*scgp;
	int	*speedp;
	int	dummy;
{
	u_char	mode[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;
	int	val;

	if (speedp)
		curspeed = *speedp;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	if (!get_mode_params(scgp, 0x05, "CD write parameter",
			mode, (u_char *)0, (u_char *)0, (u_char *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
#ifdef	DEBUG
	scsiprbytes("CD write parameter:", (u_char *)mode, len);
#endif


	mp->test_write = dummy != 0;
	/*
	 * Set default values:
	 * Write type = 01 (track at once)
	 * Track mode = 04 (CD-ROM)
	 * Data block type = 08 (CD-ROM)
	 * Session format = 00 (CD-ROM)
	 */
	mp->write_type = WT_TAO;
	mp->track_mode = TM_DATA; 
	mp->dbtype = DB_ROM_MODE1;
	mp->session_format = SES_DA_ROM;/* Matsushita has illegal def. value */


#ifdef	DEBUG
	scsiprbytes("CD write parameter:", (u_char *)mode, len);
#endif
	if (!set_mode_params(scgp, "CD write parameter", mode, len, 0, -1))
		return (-1);

	if (speedp == 0)
		return (0);

/*	if (scsi_set_speed(-1, curspeed*176) < 0)*/

	/*
	 * 44100 * 2 * 2 =  176400 bytes/s
	 *
	 * The right formula would be:
	 * tmp = (((long)curspeed) * 1764) / 10;
	 *
	 * But the standard is rounding the wrong way.
	 * Furtunately rounding down is guaranteed.
	 */
	if (scsi_set_speed(scgp, -1, curspeed*177) < 0)
		return (-1);

	if (scsi_get_speed(scgp, 0, &val) >= 0) {
		curspeed = val / 176;
		*speedp = curspeed;
	}
	return (0);
}

LOCAL int
next_wr_addr_mmc(scgp, track, trackp, ap)
	SCSI	*scgp;
	int	track;
	track_t	*trackp;
	long	*ap;
{
	struct	track_info	track_info;
	long	next_addr;
	int	result = -1;


	/*
	 * Reading info for current track may require doing the read_track_info
	 * with either the track number (if the track is currently being written)
	 * or with 0xFF (if the track hasn't been started yet and is invisible
	 */

	if (track > 0 && is_packet(trackp)) {
		scgp->silent++;
		result = read_track_info(scgp, (caddr_t)&track_info, track,
							sizeof(track_info));
		scgp->silent--;
	}

	if (result < 0) {
		if (read_track_info(scgp, (caddr_t)&track_info, 0xFF,
						sizeof(track_info)) < 0)
		return (-1);
	}
	if (scgp->verbose)
		scsiprbytes("track info:", (u_char *)&track_info,
				sizeof(track_info)-scsigetresid(scgp));
	next_addr = a_to_4_byte(track_info.next_writable_addr);
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
open_track_mmc(scgp, dp, track, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	int	track;
	track_t *trackp;
{
	u_char	mode[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;

	if (!is_tao(trackp)) {
		if (trackp->pregapsize > 0 && (trackp->flags & TI_PREGAP) == 0) {
			if (lverbose) {
				printf("Writing pregap for track %d at %ld\n",
					track,
					trackp->trackstart-trackp->pregapsize);
			}
			pad_track(scgp, dp, track, trackp,
				trackp->trackstart-trackp->pregapsize,
				trackp->pregapsize*trackp->secsize,
					FALSE, 0);
		}
		return (0);
	}

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	if (!get_mode_params(scgp, 0x05, "CD write parameter",
			mode, (u_char *)0, (u_char *)0, (u_char *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);


/*	mp->track_mode = ???;*/
	mp->track_mode = st2mode[trackp->sectype & ST_MASK];
/*	mp->copy = ???;*/
	mp->dbtype = trackp->dbtype;

/*i_to_short(mp->audio_pause_len, 300);*/
/*i_to_short(mp->audio_pause_len, 150);*/
/*i_to_short(mp->audio_pause_len, 0);*/

	if (is_packet(trackp)) {
		mp->write_type = WT_PACKET;
		mp->track_mode |= TM_INCREMENTAL;
		mp->fp = (trackp->pktsize > 0) ? 1 : 0;
		i_to_4_byte(mp->packet_size, trackp->pktsize);
	} else {
		mp->write_type = WT_TAO;
		mp->fp = 0;
		i_to_4_byte(mp->packet_size, 0);
	}
	if (trackp->isrc) {
		mp->ISRC[0] = 0x80;	/* Set ISRC valid */
		strncpy((char *)&mp->ISRC[1], trackp->isrc, 12);

	} else {
		fillbytes(&mp->ISRC[0], sizeof(mp->ISRC), '\0');
	}
 
#ifdef	DEBUG
	scsiprbytes("CD write parameter:", (u_char *)mode, len);
#endif
	if (!set_mode_params(scgp, "CD write parameter", mode, len, 0, trackp->secsize))
		return (-1);

	return (0);
}

LOCAL int
close_track_mmc(scgp, track, trackp)
	SCSI	*scgp;
	int	track;
	track_t	*trackp;
{
	int	ret;

	if (!is_tao(trackp))
		return (0);

	if (scsi_flush_cache(scgp) < 0) {
		printf("Trouble flushing the cache\n");
		return -1;
	}
	wait_unit_ready(scgp, 300);		/* XXX Wait for ATAPI */
	if (is_packet(trackp) && !is_noclose(trackp)) {
			/* close the incomplete track */
		ret = scsi_close_tr_session(scgp, 1, 0xFF, FALSE);
		wait_unit_ready(scgp, 300);	/* XXX Wait for ATAPI */
		return (ret);
	}
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
open_session_mmc(scgp, dp, tracks, trackp, toctype, multi)
	SCSI	*scgp;
	cdr_t	*dp;
	int	tracks;
	track_t	*trackp;
	int	toctype;
	int	multi;
{
	u_char	mode[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	if (!get_mode_params(scgp, 0x05, "CD write parameter",
			mode, (u_char *)0, (u_char *)0, (u_char *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	mp->write_type = WT_TAO; /* fix to allow DAO later */
	if (!is_tao(trackp)) {
		mp->write_type = WT_SAO;
		mp->track_mode = 0;
		mp->dbtype = DB_RAW;
	}

	if (driveropts != NULL) {
		if (strcmp(driveropts, "burnproof") == 0 && dp->cdr_cdcap->res_4 != 0) {
			errmsgno(EX_BAD, "Turning BURN-Proof on\n");
			mp->res_2 |= 2;
		} else if (strcmp(driveropts, "noburnproof") == 0) {
			errmsgno(EX_BAD, "Turning BURN-Proof off\n");
			mp->res_2 &= ~2;
		} else if (strcmp(driveropts, "help") == 0) {
			mmc_opthelp(dp, 0);
		} else {
			errmsgno(EX_BAD, "Bad driver opts '%s'.\n", driveropts);
			mmc_opthelp(dp, EX_BAD);
		}
	}

	mp->multi_session = (multi != 0) ? MS_MULTI : MS_NONE;
	mp->session_format = toc2sess[toctype & TOC_MASK];

	if (trackp->isrc) {
		mp->media_cat_number[0] = 0x80;	/* Set MCN valid */
		strncpy((char *)&mp->media_cat_number[1], trackp->isrc, 13);

	} else {
		fillbytes(&mp->media_cat_number[0], sizeof(mp->media_cat_number), '\0');
	}
#ifdef	DEBUG
	scsiprbytes("CD write parameter:", (u_char *)mode, len);
#endif
	if (!set_mode_params(scgp, "CD write parameter", mode, len, 0, -1))
		return (-1);

	return (0);
}

LOCAL int
waitfix_mmc(scgp, secs)
	SCSI	*scgp;
	int	secs;
{
	char	dibuf[16];
	int	i;
	int	key;
#define	W_SLEEP	2

	scgp->silent++;
	for (i = 0; i < secs/W_SLEEP; i++) {
		if (read_disk_info(scgp, dibuf, sizeof(dibuf)) >= 0) {
			scgp->silent--;
			return (0);
		}
		key = scsi_sense_key(scgp);
		if (key != SC_UNIT_ATTENTION && key != SC_NOT_READY)
			break;
		sleep(W_SLEEP);
	}
	scgp->silent--;
	return (-1);
#undef	W_SLEEP
}

LOCAL int
fixate_mmc(scgp, onp, dummy, toctype, tracks, trackp)
	SCSI	*scgp;
	int	onp;
	int	dummy;
	int	toctype;
	int	tracks;
	track_t	*trackp;
{
	int	ret = 0;
	int	key = 0;
	int	code = 0;
	struct timeval starttime;
	struct timeval stoptime;

	starttime.tv_sec = 0;
	starttime.tv_usec = 0;
	stoptime = starttime;
	gettimeofday(&starttime, (struct timezone *)0);

	if (dummy && lverbose)
		printf("WARNING: Some drives don't like fixation in dummy mode.\n");

	scgp->silent++;
	if (is_tao(trackp)) {
		ret = scsi_close_tr_session(scgp, 2, 0, FALSE);
	} else {
		if (scsi_flush_cache(scgp) < 0) {
			if (!scsi_in_progress(scgp))
				printf("Trouble flushing the cache\n");
		}
	}
	scgp->silent--;
	key = scsi_sense_key(scgp);
	code = scsi_sense_code(scgp);

	scgp->silent++;
	if (debug && !unit_ready(scgp)) {
		error("Early return from fixating. Ret: %d Key: %d, Code: %d\n", ret, key, code);
	}
	scgp->silent--;

	if (ret >= 0) {
		wait_unit_ready(scgp, 420/curspeed);	/* XXX Wait for ATAPI */
		waitfix_mmc(scgp, 420/curspeed);	/* XXX Wait for ATAPI */
		return (ret);
	}

	if ((dummy != 0 && (key != SC_ILLEGAL_REQUEST)) ||
		/*
		 * Try to suppress messages from drives that don't like fixation
		 * in -dummy mode.
		 */
	    ((dummy == 0) &&
	     ( ((key != SC_UNIT_ATTENTION) && (key != SC_NOT_READY)) ||
				((code != 0x2E) && (code != 0x04)) ))) {
		/*
		 * UNIT ATTENTION/2E seems to be a magic for old Mitsumi ATAPI drives
		 * NOT READY/ code 4 qual 7 (logical unit not ready, operation in progress)
		 * seems to be a magic for newer Mitsumi ATAPI drives
		 * NOT READY/ code 4 qual 8 (logical unit not ready, long write in progress)
		 * seems to be a magic for SONY drives
		 * when returning early from fixating.
		 * Try to supress the error message in this case to make
		 * simple minded users less confused.
		 */
		scsiprinterr(scgp);
		scsiprintresult(scgp);	/* XXX restore key/code in future */
	}

	if (debug && !unit_ready(scgp)) {
		error("Early return from fixating. Ret: %d Key: %d, Code: %d\n", ret, key, code);
	}
	scgp->silent--;

	wait_unit_ready(scgp, 420);	/* XXX Wait for ATAPI */
	waitfix_mmc(scgp, 420/curspeed);/* XXX Wait for ATAPI */

	if (!dummy &&
		(ret >= 0 || (key == SC_UNIT_ATTENTION && code == 0x2E))) {
		/*
		 * Some ATAPI drives (e.g. Mitsumi) imply the
		 * IMMED bit in the SCSI cdb. As there seems to be no
		 * way to properly check for the real end of the
		 * fixating process we wait for the expected time.
		 */
		gettimeofday(&stoptime, (struct timezone *)0);
		timevaldiff(&starttime, &stoptime);
		if (stoptime.tv_sec < (220 / curspeed)) {
			unsigned secs;

			if (lverbose) {
				printf("Actual fixating time: %ld seconds\n",
							(long)stoptime.tv_sec);
			}
			secs = (280 / curspeed) - stoptime.tv_sec;
			if (lverbose) {
				printf("ATAPI early return: sleeping %d seconds.\n",
								secs);
			}
			sleep(secs);
		}
	}
	return (ret);
}

char	*blank_types[] = {
	"entire disk",
	"PMA, TOC, pregap",
	"incomplete track",
	"reserved track",
	"tail of track",
	"closing of last session",
	"last session",
	"reserved blanking type",
};

LOCAL int
blank_mmc(scgp, addr, blanktype)
	SCSI	*scgp;
	long	addr;
	int	blanktype;
{
	BOOL	cdrr	 = FALSE;	/* Read CD-R	*/
	BOOL	cdwr	 = FALSE;	/* Write CD-R	*/
	BOOL	cdrrw	 = FALSE;	/* Read CD-RW	*/
	BOOL	cdwrw	 = FALSE;	/* Write CD-RW	*/

	mmc_check(scgp, &cdrr, &cdwr, &cdrrw, &cdwrw, NULL);
	if (!cdwrw)
		return (blank_dummy(scgp, addr, blanktype));

	if (lverbose) {
		printf("Blanking %s\n", blank_types[blanktype & 0x07]);
		flush();
	}

	return (scsi_blank(scgp, addr, blanktype, FALSE));
}

LOCAL int
send_opc_mmc(scgp, bp, cnt, doopc)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
	int	doopc;
{
	int	ret;

	scgp->silent++;
	ret = send_opc(scgp, bp, cnt, doopc);
	scgp->silent--;

	if (ret >= 0)
		return (ret);

	/*
	 * Send OPC is optional.
	 */
	if (scsi_sense_key(scgp) != SC_ILLEGAL_REQUEST) {
		if (scgp->silent <= 0)
			scsiprinterr(scgp);
		return (ret);
	}
	return (0);
}

LOCAL int
scsi_sony_write(scgp, bp, sectaddr, size, blocks, islast)
	SCSI	*scgp;
	caddr_t	bp;		/* address of buffer */
	long	sectaddr;	/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	blocks;		/* sector count */
	BOOL	islast;		/* last write for track */
{
	return (write_xg5(scgp, bp, sectaddr, size, blocks));
}

Uchar	db2df[] = {
	0x00,			/*  0 2352 bytes of raw data			*/
	0xFF,			/*  1 2368 bytes (raw data + P/Q Subchannel)	*/
	0xFF,			/*  2 2448 bytes (raw data + P-W Subchannel)	*/
	0xFF,			/*  3 2448 bytes (raw data + P-W raw Subchannel)*/
	0xFF,			/*  4 -    Reserved				*/
	0xFF,			/*  5 -    Reserved				*/
	0xFF,			/*  6 -    Reserved				*/
	0xFF,			/*  7 -    Vendor specific			*/
	0x10,			/*  8 2048 bytes Mode 1 (ISO/IEC 10149)		*/
	0xFF,			/*  9 2336 bytes Mode 2 (ISO/IEC 10149)		*/
	0xFF,			/* 10 2048 bytes Mode 2! (CD-ROM XA form 1)	*/
	0xFF,			/* 11 2056 bytes Mode 2 (CD-ROM XA form 1)	*/
	0xFF,			/* 12 2324 bytes Mode 2 (CD-ROM XA form 2)	*/
	0xFF,			/* 13 2332 bytes Mode 2 (CD-ROM XA 1/2+subhdr)	*/
	0xFF,			/* 14 -    Reserved				*/
	0xFF,			/* 15 -    Vendor specific			*/
};

LOCAL	void	fillcue		__PR((struct mmc_cue *cp, int ca, int tno, int idx, int dataform, int scms, msf_t *mp));
EXPORT	int	do_cue		__PR((int tracks, track_t *trackp, struct mmc_cue **cuep));
EXPORT	int	_do_cue		__PR((int tracks, track_t *trackp, struct mmc_cue **cuep, BOOL needgap));
LOCAL	int	send_cue	__PR((SCSI *scgp, int tracks, track_t *trackp));

EXPORT int
do_cue(tracks, trackp, cuep)
	int	tracks;
	track_t	*trackp;
	struct mmc_cue	**cuep;
{
	return (_do_cue(tracks, trackp, cuep, FALSE));
}

EXPORT int
_do_cue(tracks, trackp, cuep, needgap)
	int	tracks;
	track_t	*trackp;
	struct mmc_cue	**cuep;
	BOOL	needgap;
{
	int	i;
	struct mmc_cue	*cue;
	struct mmc_cue	*cp;
	int	ncue = 0;
	int	icue = 0;
	int	pgsize;
	msf_t	m;
	int	ctl;
	int	df;
	int	scms;

	cue = (struct mmc_cue *)malloc(1);

	for (i = 0; i <= tracks; i++) {
		ctl = (st2mode[trackp[i].sectype & ST_MASK]) << 4;
		if (is_copy(&trackp[i]))
			ctl |= TM_ALLOW_COPY << 4;
		df = db2df[trackp[i].dbtype & 0x0F];

		if (trackp[i].isrc) {	/* MCN or ISRC */
			ncue += 2;
			cue = (struct mmc_cue *)realloc(cue, ncue * sizeof(*cue));
			cp = &cue[icue++];
			if (i == 0) {
				cp->cs_ctladr = 0x02;
				movebytes(&trackp[i].isrc[0], &cp->cs_tno, 7);
				cp = &cue[icue++];
				cp->cs_ctladr = 0x02;
				movebytes(&trackp[i].isrc[7], &cp->cs_tno, 7);
			} else {
				cp->cs_ctladr = 0x03;
				cp->cs_tno = i;
				movebytes(&trackp[i].isrc[0], &cp->cs_index, 6);
				cp = &cue[icue++];
				cp->cs_ctladr = 0x03;
				cp->cs_tno = i;
				movebytes(&trackp[i].isrc[6], &cp->cs_index, 6);
			}
		}
		if (i == 0) {	/* Lead in */
			if (df < 0x10)
				df |= 1;
			else
				df |= 4;
			lba_to_msf(-150, &m);
			cue = (struct mmc_cue *)realloc(cue, ++ncue * sizeof(*cue));
			cp = &cue[icue++];
			fillcue(cp, ctl|0x01, i, 0, df, 0, &m);
		} else {
			scms = 0;

			if (is_scms(&trackp[i]))
				scms = 0x80;
			pgsize = trackp[i].pregapsize;
			if (pgsize == 0 && needgap)
				pgsize++;
			lba_to_msf(trackp[i].trackstart-pgsize, &m);
			cue = (struct mmc_cue *)realloc(cue, ++ncue * sizeof(*cue));
			cp = &cue[icue++];
			fillcue(cp, ctl|0x01, i, 0, df, scms, &m);

			if (trackp[i].nindex == 1) {
				lba_to_msf(trackp[i].trackstart, &m);
				cue = (struct mmc_cue *)realloc(cue, ++ncue * sizeof(*cue));
				cp = &cue[icue++];
				fillcue(cp, ctl|0x01, i, 1, df, scms, &m);
			} else {
				int	idx;
				long	*idxlist;

				ncue += trackp[i].nindex;
				idxlist = trackp[i].tindex;
				cue = (struct mmc_cue *)realloc(cue, ncue * sizeof(*cue));

				for(idx = 1; idx <=trackp[i].nindex; idx++) {
					lba_to_msf(trackp[i].trackstart + idxlist[idx], &m);
					cp = &cue[icue++];
					fillcue(cp, ctl|0x01, i, idx, df, scms, &m);
				}
			}
		}
	}
	/* Lead out */
	ctl = (st2mode[trackp[tracks+1].sectype & ST_MASK]) << 4;
	df = db2df[trackp[tracks+1].dbtype & 0x0F];
	if (df < 0x10)
		df |= 1;
	else
		df |= 4;
	lba_to_msf(trackp[tracks+1].trackstart, &m);
	cue = (struct mmc_cue *)realloc(cue, ++ncue * sizeof(*cue));
	cp = &cue[icue++];
	fillcue(cp, ctl|0x01, 0xAA, 1, df, 0, &m);

	if (lverbose > 1) for (i = 0; i < ncue; i++) {
		scsiprbytes("", (Uchar *)&cue[i], 8);
	}
	if (cuep)
		*cuep = cue;
	else
		free (cue);
	return (ncue);
}

LOCAL void
fillcue(cp, ca, tno, idx, dataform, scms, mp)
	struct mmc_cue	*cp;	/* The target cue entry		*/
	int	ca;		/* Control/adr for this entry	*/
	int	tno;		/* Track number for this entry	*/
	int	idx;		/* Index for this entry		*/
	int	dataform;	/* Data format for this entry	*/
	int	scms;		/* Serial copy management	*/
	msf_t	*mp;		/* MSF value for this entry	*/
{
	cp->cs_ctladr = ca;	/* XXX wie lead in */
	cp->cs_tno = tno;
	cp->cs_index = idx;
	cp->cs_dataform = dataform;/* XXX wie lead in */
	cp->cs_scms = scms;
	cp->cs_min = mp->msf_min;
	cp->cs_sec = mp->msf_sec;
	cp->cs_frame = mp->msf_frame;
}

LOCAL int
send_cue(scgp, tracks, trackp)
	SCSI	*scgp;
	int	tracks;
	track_t	*trackp;
{
	struct mmc_cue	*cp;
	int		ncue;
	int		ret;
	int		i;

	ncue = do_cue(tracks, trackp, &cp);
	for (i = 1; i <= tracks; i++) {
		if (trackp[i].tracksize < 0) {
			errmsgno(EX_BAD, "Track %d has unknown length.\n", i);
			return (-1);
		}
	}
	scgp->silent++;
	ret = send_cue_sheet(scgp, (caddr_t)cp, ncue*8);
	scgp->silent--;
	free(cp);
	if (ret < 0) {
		errmsgno(EX_BAD, "CUE sheet not accepted. Retrying with minimum pregapsize = 1.\n");
		ncue = _do_cue(tracks, trackp, &cp, TRUE);
		ret = send_cue_sheet(scgp, (caddr_t)cp, ncue*8);
		free(cp);
	}
	return (ret);
}
