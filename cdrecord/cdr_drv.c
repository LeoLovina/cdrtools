/* @(#)cdr_drv.c	1.6 98/03/26 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)cdr_drv.c	1.6 98/03/26 Copyright 1997 J. Schilling";
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

#include <stdxlib.h>
#include <standard.h>

#include <scsidefs.h>
#include <scsireg.h>
#include "cdrecord.h"

extern	cdr_t	cdr_oldcd;
extern	cdr_t	cdr_cd;
extern	cdr_t	cdr_mmc;
extern	cdr_t	cdr_philips_cdd521O;
extern	cdr_t	cdr_philips_dumb;
extern	cdr_t	cdr_philips_cdd521;
extern	cdr_t	cdr_philips_cdd522;
extern	cdr_t	cdr_pioneer_dw_s114x;
extern	cdr_t	cdr_plasmon_rf4100;
extern	cdr_t	cdr_yamaha_cdr100;
extern	cdr_t	cdr_sony_cdu924;
extern	cdr_t	cdr_ricoh_ro1420;
extern	cdr_t	cdr_teac_cdr50;

EXPORT	cdr_t 	*drive_identify		__PR((cdr_t *, struct scsi_inquiry *ip));
EXPORT	int	drive_attach		__PR((void));
EXPORT	int	attach_unknown		__PR((void));
EXPORT	int	blank_dummy		__PR((long addr, int blanktype));
EXPORT	int	drive_getdisktype	__PR((cdr_t *dp, dstat_t *dsp));
EXPORT	int	cmd_dummy		__PR((void));
EXPORT	cdr_t	*get_cdrcmds		__PR((void));

/*
 * List of CD-R drivers
 */
cdr_t	*drivers[] = {
	&cdr_mmc,
	&cdr_cd,
	&cdr_oldcd,
	&cdr_philips_cdd521O,
	&cdr_philips_dumb,
	&cdr_philips_cdd521,
	&cdr_philips_cdd522,
	&cdr_pioneer_dw_s114x,
	&cdr_plasmon_rf4100,
	&cdr_yamaha_cdr100,
	&cdr_ricoh_ro1420,
	&cdr_sony_cdu924,
	&cdr_teac_cdr50,
	(cdr_t *)NULL,
};

EXPORT cdr_t *
drive_identify(dp, ip)
	cdr_t			*dp;
	struct scsi_inquiry	*ip;
{
	return (dp);
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
blank_dummy(addr, blanktype)
	long	addr;
	int	blanktype;
{
	printf("This drive or media does not support the BLANK command\n");
	return (-1);
}

EXPORT int
drive_getdisktype(dp, dsp)
	cdr_t	*dp;
	dstat_t	*dsp;
{
	return (0);
}

EXPORT int
cmd_dummy()
{
	return (0);
}

EXPORT void
set_cdrcmds(name, dpp)
	char	*name;
	cdr_t	**dpp;
{
	cdr_t	**d;
	int	n;

	for (d = drivers; *d != (cdr_t *)NULL; d++) {
		if (streql((*d)->cdr_drname, name)) {
			*dpp = *d;
			return;
		}
	}
	if (!streql("help", name))
		error("Illegal driver type '%s'.\n", name);

	error("Driver types:\n");
	for (d = drivers; *d != (cdr_t *)NULL; d++) {
		error("%s%n",
			(*d)->cdr_drname, &n);
		error("%*s%s\n",
			20-n, "",
			(*d)->cdr_drtext);
	}
	if (streql("help", name))
		exit(0);
	exit(EX_BAD);
}

EXPORT cdr_t *
get_cdrcmds()
{
	cdr_t	*dp = (cdr_t *)0;
	extern	struct scsi_inquiry inq;

	/*
	 * First check for SCSI-3/mmc drives.
	 */
	if (is_mmc()) {
		dp = &cdr_mmc;

	} else switch (dev) {

	case DEV_CDROM:		dp = &cdr_oldcd;		break;
	case DEV_MMC_CDROM:	dp = &cdr_cd;			break;
	case DEV_MMC_CDR:	dp = &cdr_mmc;			break;
	case DEV_MMC_CDRW:	dp = &cdr_mmc;			break;

	case DEV_CDD_521_OLD:	dp = &cdr_philips_cdd521O;	break;
	case DEV_CDD_521:	dp = &cdr_philips_cdd521;	break;
	case DEV_CDD_522:
	case DEV_CDD_2000:
	case DEV_CDD_2600:	dp = &cdr_philips_cdd522;	break;
	case DEV_YAMAHA_CDR_100:dp = &cdr_yamaha_cdr100;	break;
	case DEV_YAMAHA_CDR_400:dp = &cdr_mmc;			break;
	case DEV_PLASMON_RF_4100:dp = &cdr_plasmon_rf4100;	break;
	case DEV_SONY_CDU_924:	dp = &cdr_sony_cdu924;		break;
	case DEV_RICOH_RO_1420C:dp = &cdr_ricoh_ro1420;		break;
	case DEV_TEAC_CD_R50S:	dp = &cdr_teac_cdr50;		break;

	case DEV_PIONEER_DW_S114X: dp = &cdr_pioneer_dw_s114x;	break;

	default:		dp = &cdr_mmc;
	}

	if (dp != (cdr_t *)0)
		dp = dp->cdr_identify(dp, &inq);

	return (dp);
}
