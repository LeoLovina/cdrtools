/* @(#)cdr_drv.c	1.1 97/05/20 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)cdr_drv.c	1.1 97/05/20 Copyright 1997 J. Schilling";
#endif
/*
 *	CDR device abstraction layer
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

#include <standard.h>

#include <scsidefs.h>
#include <scsireg.h>
#include "cdrecord.h"

extern	cdr_t	cdr_philips_cdd521;
extern	cdr_t	cdr_philips_cdd522;
extern	cdr_t	cdr_plasmon_rf4100;
extern	cdr_t	cdr_yamaha_cdr100;
extern	cdr_t	cdr_sony_cdu924;
extern	cdr_t	cdr_ricoh_ro1420;
extern	cdr_t	cdr_teac_cdr50;
extern	cdr_t	cdr_mmc;

EXPORT	BOOL	drive_identify		__PR((struct scsi_inquiry *ip));
EXPORT	int	drive_attach		__PR((void));
EXPORT	int	attach_unknown		__PR((void));
EXPORT	int	drive_getdisktype	__PR((void));
EXPORT	int	cmd_dummy		__PR((void));
EXPORT	cdr_t	*get_cdrcmds		__PR((void));


EXPORT BOOL
drive_identify(ip)
	struct scsi_inquiry *ip;
{
	return (TRUE);
}

EXPORT int
drive_attach()
{
	return (0);
}

EXPORT int
attach_unknown()
{
	errmsgno(EX_BAD, "Unsupported drive type\n");
	return (-1);
}

EXPORT int
drive_getdisktype()
{
	return (0);
}

EXPORT int
cmd_dummy()
{
	return (0);
}

EXPORT cdr_t *
get_cdrcmds()
{
	cdr_t	*dp = (cdr_t *)0;
	extern	struct scsi_inquiry inq;

	switch (dev) {

	case DEV_CDD_521:	dp = &cdr_philips_cdd521;	break;
	case DEV_CDD_522:	dp = &cdr_philips_cdd522;	break;
	case DEV_YAMAHA_CDR_100:dp = &cdr_yamaha_cdr100;	break;
	case DEV_YAMAHA_CDR_400:dp = &cdr_yamaha_cdr100;	break;
	case DEV_PLASMON_RF_4100:dp = &cdr_plasmon_rf4100;	break;
	case DEV_SONY_CDU_924:	dp = &cdr_sony_cdu924;		break;
	case DEV_RICOH_RO_1420C:dp = &cdr_ricoh_ro1420;		break;
	case DEV_TEAC_CD_R50S:	dp = &cdr_teac_cdr50;		break;

	default:		dp = &cdr_mmc;
	}

	if (dp && !dp->cdr_identify(&inq))
		return ((cdr_t *)0);

	return (dp);
}
