/* @(#)drv_mmc.c	1.1 97/05/20 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)drv_mmc.c	1.1 97/05/20 Copyright 1997 J. Schilling";
#endif
/*
 *	CDR device implementation for
 *	SCSI-3/mmc conforming drives
 *	e.g. Yamaha CDR-400
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

LOCAL	int	speed_select_mmc	__PR((int speed, int dummy));
LOCAL	long	next_wr_addr_mmc	__PR((int track));
LOCAL	int	open_track_mmc		__PR((int track, int secsize, int sectype, int tracktype));
LOCAL	int	close_track_mmc		__PR((int track));


cdr_t	cdr_mmc = {
	0, 0,
	drive_identify,
/*	drive_attach,*/
	attach_unknown,
	drive_getdisktype,
	scsi_load,
	scsi_unload,
	recovery_needed,
	recover,
	speed_select_mmc,
	select_secsize,
	next_wr_addr_mmc,
	reserve_track,
	scsi_cdr_write,
	open_track_mmc,
	close_track_mmc,
	(int(*)__PR((int)))cmd_dummy,
	cmd_dummy,
	fixation,
};

LOCAL int
speed_select_mmc(speed, dummy)
	int	speed;
	int	dummy;
{
	return (0);
}

LOCAL long
next_wr_addr_mmc(track)
	int	track;
{
	return(0);
}

LOCAL int
open_track_mmc(track, secsize, sectype, tracktype)
	int	track;
	int	secsize;
	int	sectype;
	int	tracktype;
{
	return (0);
}

LOCAL int
close_track_mmc(track)
	int	track;
{
	return (0);
}
