/* @(#)scsi_scan.c	1.1 97/08/24 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)scsi_scan.c	1.1 97/08/24 Copyright 1997 J. Schilling";
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
#include <scgio.h>
#include <scsidefs.h>
#include <scsireg.h>

#include "cdrecord.h"
#include "scsi_scan.h"
#include "scsitransp.h"

extern	struct	scg_cmd		scmd;
extern	struct	scsi_inquiry	inq;
extern	struct	scsi_capacity	cap;

extern	int	scsibus;
extern	int	target;
extern	int	lun;

extern	int	silent;
extern	int	verbose;
extern	int	lverbose;

LOCAL	void	print_product		__PR((struct scsi_inquiry *ip));
EXPORT	void	select_target		__PR((void));
LOCAL	void	select_unit		__PR((void));

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
select_target()
{
	int	initiator;
	int	cscsibus = scsibus;
	int	ctarget = target;
	int	clun	= lun;
	int	n;
	int	low	= -1;
	int	high	= -1;

	silent++;

	for (scsibus=0; scsibus < 8; scsibus++) {
		initiator = -1;
/*		initiator = scg_initiator_id(scsibus);*/
		if (!scsi_havebus(scsibus))
			continue;
		printf("scsibus%d:\n", scsibus);

		for (target=0, lun=0; target < 8; target++) {
			n = scsibus*100 + target;
		
/*			if (print_disknames(scsibus, target, -1) < 8)*/
				printf("\t");
			printf("\t%3d) ", n);
			if (target == initiator) {
				printf("HOST ADAPTOR\n");
				continue;
			}
			if (!unit_ready() && scmd.error == SCG_FATAL) {
				printf("*\n");
				continue;
			}
			if (low < 0)
				low = n;
			high = n;

			getdev(FALSE);
			print_product(&inq);
		}
	}
	silent--;

	if (low < 0)
		comerrno(EX_BAD, "No target found.\n");
	n = -1;
/*	getint("Select target", &n, low, high); */
exit(0);
	scsibus = n/10;
	target = n%10;
	select_unit();

	scsibus	= cscsibus;
	target	= ctarget;
	lun	= clun;
}

LOCAL void
select_unit()
{
	int	initiator;
	int	clun	= lun;
	int	low	= -1;
	int	high	= -1;

	silent++;

	printf("scsibus%d target %d:\n", scsibus, target);

	initiator = -1;
/*	initiator = scg_initiator_id(scsibus);*/
	for (lun=0; lun < 8; lun++) {

/*		if (print_disknames(scsibus, target, lun) < 8)*/
			printf("\t");
		printf("\t%3d) ", lun);
		if (target == initiator) {
			printf("HOST ADAPTOR\n");
			continue;
		}
		if (!unit_ready() && scmd.error == SCG_FATAL) {
			printf("*\n");
			continue;
		}
		if (unit_ready()) {
			/* non extended sense illegal lun */
			if (scmd.sense.code == 0x25) {
				printf("BAD UNIT\n");
				continue;
			}
		}
		if (low < 0)
			low = lun;
		high = lun;

		getdev(FALSE);
		print_product(&inq);
	}
	silent--;

	if (low < 0)
		comerrno(EX_BAD, "No lun found.\n");
	lun = -1;
/*	getint("Select lun", &lun, low, high); */
exit(0);
/*	format_one();*/
	
	lun	= clun;
}

