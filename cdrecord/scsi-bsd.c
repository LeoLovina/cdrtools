/* @(#)scsi-bsd.c	1.5 98/03/14 Copyright 1997 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-bsd.c	1.5 98/03/14 Copyright 1997 J. Schilling";
#endif
/*
 *	Interface for the NetBSD/FreeBSD/OpenBSD generic SCSI implementation.
 *
 *	This is a hack, that tries to emulate the functionality
 *	of the scg driver.
 *	The SCSI tranport of the generic *BSD implementation is very
 *	similar to the SCSI command transport of the 
 *	6 years older scg driver.
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
#include <sys/scsiio.h>

#define	MAX_SCG		4	/* Max # of SCSI controllers */
#define	MAX_TGT		8
#define	MAX_LUN		8

LOCAL	short	scgfiles[MAX_SCG][MAX_TGT][MAX_LUN];

/*#define	MAX_DMA_BSD	(32*1024)*/
#define	MAX_DMA_BSD	(60*1024)	/* More seems to make problems */


LOCAL	int	scsi_send	__PR((int f, struct scg_cmd *sp));
LOCAL	int	scsi_setup	__PR((int f));

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

		sprintf(devname, "/dev/su%d-%d-%d", scsibus, target, lun);
		f = open(devname, O_RDWR);
		if (f < 0) {
/*			return(-1);*/
			goto looser;
		}
		scgfiles[scsibus][target][lun] = f;
		return(1);

	} else for (b=0; b < MAX_SCG; b++) {
		for (t=0; t < MAX_TGT; t++) {
			for (l=0; l < MAX_LUN ; l++) {
				sprintf(devname, "/dev/su%d-%d-%d", b, t, l);
				f = open(devname, 2);
/*				error("open (%s) = %d\n", devname, f);*/

				if (f < 0) {
					if (errno != ENOENT &&
					    errno != ENXIO &&
					    errno != ENODEV)
						comerr("Cannot open '%s'.\n", devname);
				} else {
					if (scsi_setup(f) < 0)
						close(f);
					else
						nopen++;
				}
			}
		}
	}
	/*
	 * Hack for the loosers!
	 * Create a link /dev/scgx to your device.
	 */
looser:
	if (nopen == 0) {
		f = open("/dev/scgx", 2);
		if (f < 0)
			return (0);
		if (scsi_setup(f) < 0)
			close(f);
		else
			nopen++;
	}
	return (nopen);
}

LOCAL int
scsi_setup(f)
	int	f;
{
	struct scsi_addr saddr;
	int	Bus;
	int	Target;
	int	Lun;

	if (ioctl(f, SCIOCIDENTIFY, &saddr) < 0) {
		errmsg("Cannot get SCSI addr.\n");
		return (-1);
	}
	if (debug)
		printf("Bus: %d Target: %d Lun: %d\n",
			saddr.scbus, saddr.target, saddr.lun);

	Bus	= saddr.scbus;
	Target	= saddr.target;
	Lun	= saddr.lun;

	if (Bus >= MAX_SCG || Target >= 8 || Lun >= 8)
		return (-1);
 
	scgfiles[Bus][Target][Lun] = (short)f;
	return (0);
}

LOCAL long
scsi_maxdma()
{
	long maxdma = MAX_DMA_BSD;

	return (maxdma);
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

	return (ioctl(f, SCIOCRESET, 0));
}

LOCAL int
scsi_send(int f, struct scg_cmd *sp)
{
	scsireq_t	req;
	register long	*lp1;
	register long	*lp2;
	int		ret = 0;

/*	printf("f: %d\n", f);*/
	if (f < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}
	req.flags = SCCMD_ESCAPE;	/* We set the SCSI cmd len */
	if (sp->flags & SCG_RECV_DATA)
		req.flags |= SCCMD_READ;
	else if (sp->size > 0)
		req.flags |= SCCMD_WRITE;

	req.timeout = sp->timeout * 1000;
	lp1 = (long *)sp->cdb.cmd_cdb;
	lp2 = (long *)req.cmd;
	*lp2++ = *lp1++;
	*lp2++ = *lp1++;
	*lp2++ = *lp1++;
	*lp2++ = *lp1++;
	req.cmdlen = sp->cdb_len;
	req.databuf = sp->addr;
	req.datalen = sp->size;
	req.datalen_used = 0;
	fillbytes(req.sense, sizeof(req.sense), '\0');
	if (sp->sense_len > sizeof(req.sense))
		req.senselen = sizeof(req.sense);
	else if (sp->sense_len < 0)
		req.senselen = 0;
	else
		req.senselen = sp->sense_len;
	req.senselen_used = 0;
	req.status = 0;
	req.retsts = 0;
	req.error = 0;

	if (ioctl(f, SCIOCCOMMAND, (void *)&req) < 0) {
		ret  = -1;
		sp->ux_errno = geterrno();
		if (sp->ux_errno != ENOTTY)
			ret = 0;
	} else {
		sp->ux_errno = 0;
	}
	fillbytes(&sp->scb, sizeof(sp->scb), '\0');
	fillbytes(&sp->u_sense.cmd_sense, sizeof(sp->u_sense.cmd_sense), '\0');
	sp->resid = req.datalen - req.datalen_used;
	sp->sense_count = req.senselen_used;
	if (sp->sense_count > SCG_MAX_SENSE)
		sp->sense_count = SCG_MAX_SENSE;
	movebytes(req.sense, sp->u_sense.cmd_sense, sp->sense_count);
	sp->u_scb.cmd_scb[0] = req.status;

	switch (req.retsts) {

	case SCCMD_OK:
#ifdef	BSD_SCSI_SENSE_BUG
				sp->u_scb.cmd_scb[0] = 0;
#endif
				sp->error = SCG_NO_ERROR;	break;
	case SCCMD_TIMEOUT:	sp->error = SCG_TIMEOUT;	break;
	default:
	case SCCMD_BUSY:	sp->error = SCG_RETRYABLE;	break;
	case SCCMD_SENSE:	sp->error = SCG_RETRYABLE;	break;
	case SCCMD_UNKNOWN:	sp->error = SCG_FATAL;		break;
	}

	return (ret);
}
#define	sense	u_sense.Sense
