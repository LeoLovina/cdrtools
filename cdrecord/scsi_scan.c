/* @(#)scsi_scan.c	1.7 00/01/26 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)scsi_scan.c	1.7 00/01/26 Copyright 1997 J. Schilling";
#endif
/*
 *	Scan SCSI Bus.
 *	Stolen from sformat. Need a more general form to
 *	re-use it in sformat too.
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
#include <btorder.h>
#include <errno.h>
#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "scsi_scan.h"
#include "cdrecord.h"

LOCAL	void	print_product		__PR((struct scsi_inquiry *ip));
EXPORT	void	select_target		__PR((SCSI *scgp));
LOCAL	void	select_unit		__PR((SCSI *scgp));

LOCAL void
print_product(ip)
	struct	scsi_inquiry *ip;
{
	printf("'%.8s' ", ip->info);
	printf("'%.16s' ", ip->ident);
	printf("'%.4s' ", ip->revision);
	if (ip->add_len < 31) {
		printf("NON CCS ");
	}
	scsiprintdev(ip);
}

EXPORT void
select_target(scgp)
	SCSI	*scgp;
{
	int	initiator;
	int	cscsibus = scgp->scsibus;
	int	ctarget =  scgp->target;
	int	clun	=  scgp->lun;
	int	n;
	int	low	= -1;
	int	high	= -1;
	BOOL	have_tgt;

	scgp->silent++;

	for (scgp->scsibus=0; scgp->scsibus < 16; scgp->scsibus++) {
		if (!scsi_havebus(scgp, scgp->scsibus))
			continue;

		initiator = scsi_initiator_id(scgp);
		printf("scsibus%d:\n", scgp->scsibus);

		for (scgp->target=0, scgp->lun=0; scgp->target < 16; scgp->target++) {
			n = scgp->scsibus*100 + scgp->target;
		
			have_tgt = unit_ready(scgp) || scgp->scmd->error != SCG_FATAL;


			if (!have_tgt && scgp->target > 7) {
				if (scgp->scmd->ux_errno == EINVAL)
					break;
/*				if ((high%100) < 8)*/
				continue;
			}

/*			if (print_disknames(scgp->scsibus, scgp->target, -1) < 8)*/
/*				printf("\t");*/
			printf("\t");
			if (printf("%d,%d,%d", scgp->scsibus, scgp->target, scgp->lun) < 8)
				printf("\t");
			else
				printf(" ");
			printf("%3d) ", n);
			if (scgp->target == initiator) {
				printf("HOST ADAPTOR\n");
				continue;
			}
			if (!have_tgt) {
				printf("*\n");
				continue;
			}
			if (low < 0)
				low = n;
			high = n;

			getdev(scgp, FALSE);
			print_product(scgp->inq);
		}
	}
	scgp->silent--;

	if (low < 0)
		comerrno(EX_BAD, "No target found.\n");
	n = -1;
/*	getint("Select target", &n, low, high); */
exit(0);
	scgp->scsibus = n/10;
	scgp->target = n%10;
	select_unit(scgp);

	scgp->scsibus	= cscsibus;
	scgp->target	= ctarget;
	scgp->lun	= clun;
}

LOCAL void
select_unit(scgp)
	SCSI	*scgp;
{
	int	initiator;
	int	clun	= scgp->lun;
	int	low	= -1;
	int	high	= -1;

	scgp->silent++;

	printf("scsibus%d target %d:\n", scgp->scsibus, scgp->target);

	initiator = scsi_initiator_id(scgp);
	for (scgp->lun=0; scgp->lun < 8; scgp->lun++) {

/*		if (print_disknames(scgp->scsibus, scgp->target, scgp->lun) < 8)*/
/*			printf("\t");*/

		printf("\t");
		if (printf("%d,%d,%d", scgp->scsibus, scgp->target, scgp->lun) < 8)
			printf("\t");
		else
			printf(" ");
		printf("%3d) ", scgp->lun);
		if (scgp->target == initiator) {
			printf("HOST ADAPTOR\n");
			continue;
		}
		if (!unit_ready(scgp) && scgp->scmd->error == SCG_FATAL) {
			printf("*\n");
			continue;
		}
		if (unit_ready(scgp)) {
			/* non extended sense illegal lun */
			if (scgp->scmd->sense.code == 0x25) {
				printf("BAD UNIT\n");
				continue;
			}
		}
		if (low < 0)
			low = scgp->lun;
		high = scgp->lun;

		getdev(scgp, FALSE);
		print_product(scgp->inq);
	}
	scgp->silent--;

	if (low < 0)
		comerrno(EX_BAD, "No lun found.\n");
	scgp->lun = -1;
/*	getint("Select lun", &scgp->lun, low, high); */
exit(0);
/*	format_one();*/
	
	scgp->lun	= clun;
}

