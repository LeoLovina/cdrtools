/* @(#)drv_jvc.c	1.1 97/05/20 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)drv_jvc.c	1.1 97/05/20 Copyright 1997 J. Schilling";
#endif
/*
 *	CDR device implementation for
 *	JVC/TEAC
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

LOCAL	int	teac_attach	__PR((void));
LOCAL	long	next_wr_addr_jvc __PR((int track));
LOCAL	int	open_track_jvc	__PR((int track, int secsize, int sectype, int tracktype));

cdr_t	cdr_teac_cdr50 = {
	0, 0,
	drive_identify,
	teac_attach,
	drive_getdisktype,
	scsi_load,
	scsi_unload,
	recovery_needed,
	recover,
	speed_select_philips,
	select_secsize,
	next_wr_addr_jvc,
	reserve_track,
	scsi_cdr_write,
	open_track_jvc,
	close_track_philips,
	(int(*)__PR((int)))cmd_dummy,
	cmd_dummy,
	fixation,
};

LOCAL long
next_wr_addr_jvc(track)
	int	track;
{
	return(0);
}

LOCAL int
open_track_jvc(track, secsize, sectype, tracktype)
	int	track;
	int	secsize;
	int	sectype;
	int	tracktype;
{
	return (0);
}



static const char *sd_teac50_error_str[] = {
	"\100\200diagnostic failure on component parts",	/* 40 80 */
	"\100\201diagnostic failure on memories",		/* 40 81 */
	"\100\202diagnostic failure on cd-rom ecc circuit",	/* 40 82 */
	"\100\203diagnostic failure on gate array",		/* 40 83 */
	"\100\204diagnostic failure on internal SCSI controller",	/* 40 84 */
	"\100\205diagnostic failure on servo processor",	/* 40 85 */
	"\100\206diagnostic failure on program rom",		/* 40 86 */
	"\100\220thermal sensor failure",			/* 40 90 */
	"\220\000pma less disk - not a recordable disk",	/* 90 00 */
	"\224\000toc less disk",				/* 94 00 */
	"\233\000opc initialize error",				/* 9B 00 */
	"\234\000opc execution eror",				/* 9C 00 */
	"\234\001alpc error - opc execution",			/* 9C 01 */
	"\234\002opc execution timeout",			/* 9C 02 */
	"\245\000disk application code does not match host application code",	/* A5 00 */
	"\255\000completed preview write",			/* AD 00 */
	"\257\000pca area full",				/* AF 00 */
	"\264\000full pma area",				/* B4 00 */
	"\265\000read address is atip area - blank",		/* B5 00 */
	"\266\000write address is efm area - aleady written",	/* B6 00 */
	"\272\000no write data - buffer empty",			/* BA 00 */
	"\273\000write emergency occurred",			/* BB 00 */
	"\314\000write request means mixed data mode",		/* CC 00 */
	"\315\000unable to ensure reliable writing with the inserted disk - unsupported disk",	/* CD 00 */
	"\316\000unable to ensure reliable writing as the inserted disk does not support speed",/* CE 00 */
	"\317\000unable to ensure reliable writing as the inserted disk has no char id code",	/* CF 00 */
	NULL
};

LOCAL int
teac_attach()
{
	scsi_setnonstderrs(sd_teac50_error_str);
	return (attach_unknown());
/*	return (0);*/
}
