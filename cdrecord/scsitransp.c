/* @(#)scsitransp.c	1.12 97/03/02 Copyright 1988,1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)scsitransp.c	1.12 97/03/02 Copyright 1988,1995 J. Schilling";
#endif
/*
 *	SCSI user level command transport routines for
 *	the SCSI general driver 'scg'.
 *
 *	Copyright (c) 1988,1995 J. Schilling
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

#include <sys/param.h>	/* XXX nonportable to use u_char */
#include <stdio.h>
#include <standard.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "scgio.h"
#include "scsireg.h"
#include "scsitransp.h"

#ifdef	sun
#	define	HAVE_SCG	/* Currently only on SunOS/Solaris */
#endif

#define	DEFTIMEOUT	20	/* Default timeout for SCSI command transport */

/*
 * Need to move this into an scg driver ioctl.
 */
/*#define	MAX_DMA_SUN4M	(1024*1024)*/
#define	MAX_DMA_SUN4M	(124*1024)	/* Currently max working size */
/*#define	MAX_DMA_SUN4C	(126*1024)*/
#define	MAX_DMA_SUN4C	(124*1024)	/* Currently max working size */
#define	MAX_DMA_SUN3	(63*1024)
#define	MAX_DMA_SUN386	(32*1024)
#define	MAX_DMA_OTHER	(32*1024)

#define	ARCH_MASK	0xF0
#define	ARCH_SUN2	0x00
#define	ARCH_SUN3	0x10
#define	ARCH_SUN4	0x20
#define	ARCH_SUN386	0x30
#define	ARCH_SUN3X	0x40
#define	ARCH_SUN4C	0x50
#define	ARCH_SUN4E	0x60
#define	ARCH_SUN4M	0x70
#define	ARCH_SUNX	0x80

int	scsibus	= -1;
int	target	= -1;
int	lun	= -1;

int	kdebug;
int	debug;
int	silent;
int	verbose;
int	disre_disable = 0;
int	deftimeout = DEFTIMEOUT;
int	noparity;

struct	scg_cmd scmd;

LOCAL	long	scg_maxdma;
LOCAL	BOOL	scsi_running = FALSE;
LOCAL	char	*scsi_command;
LOCAL	struct timeval	cmdstart;
LOCAL	struct timeval	cmdstop;

EXPORT	int	scsi_open	__PR((void));
LOCAL	long	scsi_maxdma	__PR((void));
EXPORT	BOOL	scsi_havebus	__PR((int));
EXPORT	int	scsi_fileno	__PR((int, int, int));
EXPORT	void	*scsi_getbuf	__PR((long));
EXPORT	long	scsi_bufsize	__PR((long));
LOCAL	BOOL	scsi_yes	__PR((char *));
LOCAL	void	scsi_sighandler	__PR((int));
EXPORT	int	scsireset	__PR((void));
EXPORT	int	scsicmd		__PR((char *));
EXPORT	int	scsigetresid	__PR((void));
LOCAL	void	scsitimes	__PR((void));
LOCAL	BOOL	scsierr		__PR((void));
LOCAL	int	scsicheckerr	__PR((char *));
EXPORT	void	scsiprinterr	__PR((char *));
EXPORT	void	scsiprintstatus	__PR((void));
EXPORT	void	scsiprbytes	__PR((char *, unsigned char *, int));
EXPORT	void	scsiprsense	__PR((unsigned char *, int));
EXPORT	int	scsi_sense_key	__PR((void));
EXPORT	int	scsi_sense_code	__PR((void));
EXPORT	void	scsiprintdev	__PR((struct scsi_inquiry *));

#ifdef	HAVE_SCG
/*
 * We are using a "real" /dev/scg?
 */
#	define	scsi_send(f, cmdp)	ioctl((f), SCGIO_CMD, (cmdp))
#	define	MAX_SCG		8	/* Max # of SCSI controllers */

LOCAL	int	scgfiles[MAX_SCG];

#else
/*
 * We are emulating the functionality of /dev/scg? with the local
 * SCSI user land implementation.
 */
#	include	"scsihack.c"

#endif	/* HAVE_SCG */

#ifdef	HAVE_SCG
EXPORT
int scsi_open()
{
	register int	f;
	register int	i;
	register int	nopen = 0;
	char		devname[32];

	for (i=0; i < MAX_SCG; i++) {
		sprintf(devname, "/dev/scg%d", i);
		f = open(devname, 2);
		if (f < 0) {
			if (errno != ENOENT && errno != ENXIO)
				comerr("Cannot open '%s'.\n", devname);
		} else {
			nopen++;
		}
		scgfiles[i] = f;
	}
	return (nopen);
}

LOCAL long
scsi_maxdma()
{
	long	maxdma = 0L;

#ifdef	sun
	extern	long	gethostid __PR((void));
	int	cpu_type = gethostid() >> 24;

	switch (cpu_type & ARCH_MASK) {

	case ARCH_SUN4C:
	case ARCH_SUN4E:
		maxdma = MAX_DMA_SUN4C;
		break;

	case ARCH_SUN4M:
	case ARCH_SUNX:
		maxdma = MAX_DMA_SUN4M;
		break;

	case ARCH_SUN386:
		maxdma = MAX_DMA_SUN386;
		break;

	default:
		maxdma = MAX_DMA_SUN3;
	}
#else
	maxdma = MAX_DMA_OTHER;
#endif
	return (maxdma);
}

EXPORT
BOOL scsi_havebus(busno)
	int	busno;
{
	return (busno < 0 || busno >= MAX_SCG) ? FALSE : (scgfiles[busno] >= 0);
}

EXPORT
int scsi_fileno(busno, tgt, tlun)
	int	busno;
	int	tgt;
	int	tlun;
{
	return (busno < 0 || busno >= MAX_SCG) ? -1 : scgfiles[busno];
}

EXPORT void *
scsi_getbuf(amt)
	long	amt;
{
	if (scg_maxdma == 0)
		scg_maxdma = scsi_maxdma();

	if (amt <= 0 || amt > scg_maxdma)
		return ((void *)0);
	return (valloc((size_t)amt));
}

#endif	/* HAVE_SCG */

EXPORT long
scsi_bufsize(amt)
	long	amt;
{
	if (scg_maxdma == 0)
		scg_maxdma = scsi_maxdma();

	if (amt <= 0 || amt > scg_maxdma)
		return (scg_maxdma);
	return (amt);
}

LOCAL
BOOL scsi_yes(msg)
	char	*msg;
{
	char okbuf[10];

	printf("%s", msg);
	flush();
	if (getline(okbuf, sizeof(okbuf)) == EOF)
		exit(EX_BAD);
	if(streql(okbuf, "y") || streql(okbuf, "yes") ||
	   streql(okbuf, "Y") || streql(okbuf, "YES"))
		return(TRUE);
	else
		return(FALSE);
}

LOCAL void
scsi_sighandler(sig)
	int	sig;
{
	printf("\n");
	if (scsi_running) {
		printf("Running command: %s\n", scsi_command);
		printf("Resetting SCSI - Bus.\n");
		if (scsireset() < 0)
			errmsg("Cannot reset SCSI - Bus.\n");
	}
	if (scsi_yes("EXIT ? "))
		exit(sig);
}

EXPORT
int scsireset()
{
	int	f = scsi_fileno(scsibus, target, lun);

	return (ioctl(f, SCGIORESET, 0));
}

EXPORT
int scsicmd(name)
	char	*name;
{
	int	f;
	int	ret;

	f = scsi_fileno(scsibus, target, lun);
	scmd.kdebug = kdebug;
	if (scmd.timeout == 0 || scmd.timeout < deftimeout)
		scmd.timeout = deftimeout;
	if (disre_disable)
		scmd.flags &= ~SCG_DISRE_ENA;
	if (noparity)
		scmd.flags |= SCG_NOPARITY;

	if (verbose) {
		printf("\nExecuting '%s' command on Bus %d Target %d, Lun %d timeout %ds\n",
			name, scsibus, scmd.target, scmd.cdb.g0_cdb.lun,
			scmd.timeout);
		scsiprbytes("CDB: ", scmd.cdb.cmd_cdb, scmd.cdb_len);
	}

	if (scsi_running)
		raisecond("SCSI ALREADY RUNNING !!", 0L);
	gettimeofday(&cmdstart, (struct timeval *)0);
	scsi_command = name;
	scsi_running = TRUE;
	ret = scsi_send(f, &scmd);
	scsi_running = FALSE;
	scsitimes();
	if (ret < 0)
		comerr("Cannot send SCSI cmd via ioctl\n");
	return (scsicheckerr(name));
}

EXPORT
int scsigetresid()
{
	return (scmd.resid);
}

LOCAL
void scsitimes()
{
	gettimeofday(&cmdstop, (struct timeval *)0);
	cmdstop.tv_sec -= cmdstart.tv_sec;
	cmdstop.tv_usec -= cmdstart.tv_usec;
	while (cmdstop.tv_usec < 0) {
		cmdstop.tv_sec -= 1;
		cmdstop.tv_usec += 1000000;
	}
}

LOCAL
BOOL scsierr()
{
	register struct scg_cmd *cp = &scmd;

	if(cp->error != SCG_NO_ERROR ||
				cp->errno != 0 || *(u_char *)&cp->scb != 0)
		return (TRUE);
	return (FALSE);
}

LOCAL
int scsicheckerr(cmd)
	char	*cmd;
{
	register int ret;

	ret = 0;
	if(scsierr()) {
		if (!silent || verbose)
			scsiprinterr(cmd);
		ret = -1;
	}
	if ((!silent || verbose) && scmd.resid)
		printf("resid: %d\n", scmd.resid);
	return (ret);
}

EXPORT
void scsiprinterr(cmd)
	char	*cmd;
{
	register struct scg_cmd *cp = &scmd;
	register char	*err;
		char	errbuf[64];

	switch (cp->error) {

	case SCG_NO_ERROR :	err = "no error"; break;
	case SCG_RETRYABLE:	err = "retryable error"; break;
	case SCG_FATAL    :	err = "fatal error"; break;
				/*
				 * We need to cast timeval->* to long because
				 * of the broken sys/time.h in Linux.
				 */
	case SCG_TIMEOUT  :	sprintf(errbuf,
					"cmd timeout after %ld.%03ld (%d) s",
					(long)cmdstop.tv_sec, (long)cmdstop.tv_usec/1000,
								cp->timeout);
				err = errbuf;
				break;
	default:		sprintf(errbuf, "error: %d", cp->error);
				err = errbuf;
	}

	errmsgno(cp->errno, "%s: scsi sendcmd: %s\n", cmd, err);
	if (cp->error <= SCG_RETRYABLE)
		scsiprintstatus();

	if (cp->scb.chk) {
		scsiprsense((u_char *)&cp->sense, cp->sense_count);
		scsierrmsg(&cp->sense, &cp->scb, -1); 
	}
}

EXPORT
void scsiprintstatus()
{
	register struct scg_cmd *cp = &scmd;

	error("status: 0x%x ", *(u_char *)&cp->scb);
#ifdef	SCSI_EXTENDED_STATUS
	if (cp->scb.ext_st1)
		error("0x%x ", ((u_char *)&cp->scb)[1]);
	if (cp->scb.ext_st2)
		error("0x%x ", ((u_char *)&cp->scb)[2]);
#endif
	error("(");

	switch (*(u_char *)&cp->scb & 036) {

	case 0  : error("GOOD STATUS"); break;
	case 02 : error("CHECK CONDITION"); break;
	case 04 : error("CONDITION MET/GOOD"); break;
	case 010: error("BUSY"); break;
	case 020: error("INTERMEDIATE GOOD STATUS"); break;
	case 024: error("INTERMEDIATE CONDITION MET/GOOD"); break;
	case 030: error("RESERVATION CONFLICT"); break;
	default : error("Reserved"); break;
	}

#ifdef	SCSI_EXTENDED_STATUS
	if (cp->scb.ext_st1 && cp->scb.ha_er)
		error(" host adapter detected error");
#endif
	error(")\n");
}

void scsiprbytes(s, cp, n)
		char	*s;
	register u_char	*cp;
	register int	n;
{
	printf(s);
	while (--n >= 0)
		printf(" %02X", *cp++);
	printf("\n");
}

EXPORT
void scsiprsense(cp, n)
	u_char	*cp;
	int	n;
{
	scsiprbytes("Sense Bytes:", cp, n);
}

EXPORT int
scsi_sense_key()
{
	register struct scg_cmd *cp = &scmd;
	int	key = -1;

	if (cp->sense.code >= 0x70)
		key = ((struct scsi_ext_sense*)&(cp->sense))->key;
	return(key);
}

EXPORT int
scsi_sense_code()
{
	register struct scg_cmd *cp = &scmd;
	int	code = -1;

	if (cp->sense.code >= 0x70)
		code = ((struct scsi_ext_sense*)&(cp->sense))->error_code;
	else
		code = cp->sense.code;
	return(code);
}

EXPORT
void scsiprintdev(ip)
	struct	scsi_inquiry *ip;
{
	if (ip->removable)
		printf("Removable ");
	if (ip->data_format >= 2) {
		switch (ip->qualifier) {

		case INQ_DEV_PRESENT:		break;
		case INQ_DEV_NOTPR:
			printf("not present ");	break;
		case INQ_DEV_RES:
			printf("reserved ");	break;
		case INQ_DEV_NOTSUP:
			if (ip->type == INQ_NODEV) {
				printf("unsupported\n"); return;
			}
			printf("unsupported ");	break;
		default:
			printf("vendor specific %d ", ip->qualifier);
		}
	}
	switch (ip->type) {

	case INQ_DASD:
		printf("Disk");			break;
	case INQ_SEQD:
		printf("Tape");			break;
	case INQ_PRTD:
		printf("Printer");		break;
	case INQ_PROCD:
		printf("Processor");		break;
	case INQ_WORM:
		printf("WORM");			break;
	case INQ_ROMD:
		printf("CD-ROM");		break;
	case INQ_SCAN:
		printf("Scanner");		break;
	case INQ_OMEM:
		printf("Optical Storage");	break;
	case INQ_JUKE:
		printf("Juke Box");		break;
	case INQ_COMM:
		printf("Communication");	break;
	case INQ_NODEV:
		if (ip->data_format >= 2) {
			printf("unknown/no device");
			break;
		} else if (ip->qualifier == INQ_DEV_NOTSUP) {
			printf("unit not present");
			break;
		}
	default:
		printf("unknown device type 0x%x", ip->type);
	}
	printf("\n");
}
