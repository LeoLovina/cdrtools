/* @(#)scsi-hpux.c	1.4 97/09/01 Copyright 1997 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-hpux.c	1.4 97/09/01 Copyright 1997 J. Schilling";
#endif
/*
 *	Interface for the HP-UX generic SCSI implementation.
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

#undef	sense
#include <sys/scsi.h>

#define	MAX_SCG		4	/* Max # of SCSI controllers */
#define	MAX_TGT		8
#define	MAX_LUN		8

LOCAL	short	scgfiles[MAX_SCG][MAX_TGT][MAX_LUN];

#ifdef	SCSI_MAXPHYS
#	define	MAX_DMA_HP	SCSI_MAXPHYS
#else
#	define	MAX_DMA_HP	(63*1024)	/* Check if this is not too big */
#endif


LOCAL	int	scsi_send	__PR((int f, struct scg_cmd *sp));

extern int scsibus;
extern int target;
extern int lun;

EXPORT
int scsi_open()
{
	register int	f;
	register int	b;
	register int	t;
	register int	l;
	register int	nopen = 0;
	char		devname[64];

	for (b=0; b < MAX_SCG; b++) {
		for (t=0; t < MAX_TGT; t++) {
			for (l=0; l < MAX_LUN ; l++)
				scgfiles[b][t][l] = (short)-1;
		}
	}

	if (scsibus >= 0 && target >= 0 && lun >= 0) {	
		if (scsibus >= MAX_SCG || target >= MAX_TGT || lun >= MAX_LUN)
			return (-1);

		sprintf(devname, "/dev/rscsi/c%dt%dl%d", scsibus, target, lun);
		f = open(devname, O_RDWR);
		if (f < 0)
			return(-1);
		scgfiles[scsibus][target][lun] = f;
		return(1);
	} else {
		for (b=0; b < MAX_SCG; b++) {
			for (t=0; t < MAX_TGT; t++) {
/*				for (l=0; l < MAX_LUN ; l++) {*/
				for (l=0; l < 1 ; l++) {
					sprintf(devname, "/dev/rscsi/c%dt%dl%d", b, t, l);
/*error("name: '%s'\n", devname);*/
					f = open(devname, O_RDWR);
					if (f >= 0) {
						scgfiles[b][t][l] = (short)f;
						nopen++;
					} else if (debug) {
						errmsg("open '%s'\n", devname);
					}
				}
			}
		}
	}
	return (nopen);
}

LOCAL long
scsi_maxdma()
{
	return	(MAX_DMA_HP);
}

EXPORT void *
scsi_getbuf(amt)
	long	amt;
{
	void	*ret;

	if (scg_maxdma == 0)
		scg_maxdma = scsi_maxdma();

	if (amt <= 0 || amt > scg_maxdma)
		return ((void *)0);
	if (debug)
		printf("scsi_getbuf: %ld bytes\n", amt);
	ret = valloc((size_t)(amt));
	return (ret);
}

EXPORT
BOOL scsi_havebus(busno)
	int	busno;
{
	register int	t;
	register int	l;

	if (busno < 0 || busno >= MAX_SCG)
		return (FALSE);

	for (t=0; t < MAX_TGT; t++) {
		for (l=0; l < MAX_LUN ; l++)
			if (scgfiles[busno][t][l] >= 0)
				return (TRUE);
	}
	return (FALSE);
}

EXPORT
int scsi_fileno(busno, tgt, tlun)
	int	busno;
	int	tgt;
	int	tlun;
{
	if (busno < 0 || busno >= MAX_SCG ||
	    tgt < 0 || tgt >= MAX_TGT ||
	    tlun < 0 || tlun >= MAX_LUN)
		return (-1);

	return ((int)scgfiles[busno][tgt][tlun]);
}

EXPORT
int scsireset()
{
	int	f = scsi_fileno(scsibus, target, lun);

	return (ioctl(f, SIOC_RESET_BUS, 0));
}

LOCAL int
scsi_send(f, sp)
	int		f;
	struct scg_cmd	*sp;
{
	int	ret;
	int	flags;
	struct sctl_io	sctl_io;

	if ((f < 0) || (sp->cdb_len > sizeof(sctl_io.cdb))) {
		sp->error = SCG_FATAL;
		return (0);
	}

	fillbytes((caddr_t)&sctl_io, sizeof(sctl_io), '\0');

	flags = 0;
/*	flags = SCTL_INIT_WDTR|SCTL_INIT_SDTR;*/
	if (sp->flags & SCG_RECV_DATA)
		flags |= SCTL_READ;
	if ((sp->flags & SCG_DISRE_ENA) == 0)
		flags |= SCTL_NO_ATN;

	sctl_io.flags		= flags;

	movebytes(&sp->cdb, sctl_io.cdb, sp->cdb_len);
	sctl_io.cdb_length	= sp->cdb_len;

	sctl_io.data_length	= sp->size;
	sctl_io.data		= sp->addr;

	if (sp->timeout == 0)
		sctl_io.max_msecs = 0;
	else
		sctl_io.max_msecs = (sp->timeout * 1000) + 500;

	errno		= 0;
	sp->error	= SCG_NO_ERROR;
	sp->sense_count	= 0;
	sp->u_scb.cmd_scb[0] = 0;
	sp->resid	= 0;

	ret = ioctl(f, SIOC_IO, &sctl_io);
	if (ret < 0) {
		sp->error = SCG_FATAL;
		sp->ux_errno = errno;
		return (ret);
	}
if (debug)
error("cdb_status: %X, size: %d xfer: %d\n", sctl_io.cdb_status, sctl_io.data_length, sctl_io.data_xfer);

	if (sctl_io.cdb_status == 0 || sctl_io.cdb_status == 0x02)
		sp->resid = sp->size - sctl_io.data_xfer;

	if (sctl_io.cdb_status & SCTL_SELECT_TIMEOUT ||
			sctl_io.cdb_status & SCTL_INVALID_REQUEST) {
		sp->error = SCG_FATAL;
	} else if (sctl_io.cdb_status & SCTL_INCOMPLETE) {
		sp->error = SCG_TIMEOUT;
	} else if (sctl_io.cdb_status > 0xFF) {
		errmsgno(EX_BAD, "SCSI problems: cdb_status: %X\n", sctl_io.cdb_status);

	} else if ((sctl_io.cdb_status & 0xFF) != 0) {
		sp->error = SCG_RETRYABLE;
		sp->ux_errno = EIO;

		sp->u_scb.cmd_scb[0] = sctl_io.cdb_status & 0xFF;

		sp->sense_count = sctl_io.sense_xfer;
		if (sp->sense_count > SCG_MAX_SENSE)
			sp->sense_count = SCG_MAX_SENSE;

		if (sctl_io.sense_status != S_GOOD) {
			sp->sense_count = 0;
		} else {
			movebytes(sctl_io.sense, sp->u_sense.cmd_sense, sp->sense_count);
		}

	}
	return (ret);
}
#define	sense	u_sense.Sense
