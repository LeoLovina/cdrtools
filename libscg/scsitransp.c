/* @(#)scsitransp.c	1.53 00/07/01 Copyright 1988,1995,2000 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)scsitransp.c	1.53 00/07/01 Copyright 1988,1995,2000 J. Schilling";
#endif
/*
 *	SCSI user level command transport routines (generic part).
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1988,1995,2000 J. Schilling
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

#include <mconfig.h>

#ifdef	HAVE_SYS_PARAM_H
#include <sys/param.h>	/* XXX nonportable to use u_char */
#endif
#include <stdio.h>
#include <standard.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <errno.h>
#include <timedefs.h>
#include <sys/ioctl.h>
#include <fctldefs.h>
#include <schily.h>

#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_version[]		= "0.1";	/* The global libscg version	*/
LOCAL	char	_scg_auth_schily[]	= "schily";	/* The author for this module	*/

#define	DEFTIMEOUT	20	/* Default timeout for SCSI command transport */

EXPORT	char	*scg_version	__PR((SCSI *scgp, int what));
EXPORT	char	*scg__version	__PR((SCSI *scgp, int what));
EXPORT	int	scsi_open	__PR((SCSI *scgp, char *device, int busno, int tgt, int tlun));
EXPORT	int	scsi_close	__PR((SCSI *scgp));
LOCAL	long	scsi_maxdma	__PR((SCSI *scgp, long amt));
EXPORT	BOOL	scsi_havebus	__PR((SCSI *scgp, int));
EXPORT	int	scsi_fileno	__PR((SCSI *scgp, int, int, int));
EXPORT	int	scsi_initiator_id __PR((SCSI *scgp));
EXPORT	int	scsi_isatapi	__PR((SCSI *scgp));
EXPORT	int	scsireset	__PR((SCSI *scgp));
EXPORT	void	*scsi_getbuf	__PR((SCSI *scgp, long));
EXPORT	void	scsi_freebuf	__PR((SCSI *scgp));
EXPORT	long	scsi_bufsize	__PR((SCSI *scgp, long));
EXPORT	void	scsi_setnonstderrs __PR((SCSI *scgp, const char **));
EXPORT	BOOL	scsi_yes	__PR((char *));
#ifdef	nonono
LOCAL	void	scsi_sighandler	__PR((int));
#endif
EXPORT	int	scsicmd		__PR((SCSI *scgp));
EXPORT	int	scsigetresid	__PR((SCSI *scgp));
LOCAL	void	scsitimes	__PR((SCSI *scgp));
LOCAL	BOOL	scsierr		__PR((SCSI *scgp));
LOCAL	int	scsicheckerr	__PR((SCSI *scgp));
EXPORT	void	scsiprinterr	__PR((SCSI *scgp));
EXPORT	void	scsiprintcdb	__PR((SCSI *scgp));
EXPORT	void	scsiprintwdata	__PR((SCSI *scgp));
EXPORT	void	scsiprintrdata	__PR((SCSI *scgp));
EXPORT	void	scsiprintresult	__PR((SCSI *scgp));
EXPORT	void	scsiprintstatus	__PR((SCSI *scgp));
EXPORT	void	scsiprbytes	__PR((char *, unsigned char *, int));
EXPORT	void	scsiprsense	__PR((unsigned char *, int));
EXPORT	int	scsi_sense_key	__PR((SCSI *scgp));
EXPORT	int	scsi_sense_code	__PR((SCSI *scgp));
EXPORT	int	scsi_sense_qual	__PR((SCSI *scgp));
EXPORT	void	scsiprintdev	__PR((struct scsi_inquiry *));

/*
 * Emulate the functionality of /dev/scg? with the local
 * SCSI user land implementation.
 */
#include "scsihack.c"

/*
 * Return version information for the SCSI transport code.
 * This has been introduced to make it easier to trace down problems
 * in applications.
 *
 * If scgp is NULL, return general library version information.
 * If scgp is != NULL, return version information for the low level transport.
 */
EXPORT char *
scg_version(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (scgp == (SCSI *)0) {
		switch (what) {

		case SCG_VERSION:
			return (_scg_version);
		/*
		 * If you changed this source, you are not allowed to
		 * return "schily" for the SCG_AUTHOR request.
		 */
		case SCG_AUTHOR:
			return (_scg_auth_schily);
		case SCG_SCCS_ID:
			return (sccsid);
		default:
			return ((char *)0);
		}
	}
	return (scg__version(scgp, what));
}

EXPORT long
scsi_bufsize(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	long	maxdma;

	maxdma = scsi_maxdma(scgp, amt);
	if (amt <= 0 || amt > maxdma)
		amt = maxdma;

	scgp->maxdma = maxdma;
	scgp->maxbuf = amt;

	return (amt);
}

EXPORT void
scsi_setnonstderrs(scgp, vec)
	SCSI	*scgp;
	const char **vec;
{
	scgp->nonstderrs = vec;
}

EXPORT
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

#ifdef	nonono
LOCAL void
scsi_sighandler(sig)
	int	sig;
{
	printf("\n");
	if (scsi_running) {
		printf("Running command: %s\n", scsi_command);
		printf("Resetting SCSI - Bus.\n");
		if (scsireset(scgp) < 0)
			errmsg("Cannot reset SCSI - Bus.\n");
	}
	if (scsi_yes("EXIT ? "))
		exit(sig);
}
#endif

EXPORT
int scsicmd(scgp)
	SCSI	*scgp;
{
		 int		f;
		 int		ret;
	register struct	scg_cmd	*scmd = scgp->scmd;

	f = scsi_fileno(scgp, scgp->scsibus, scgp->target, scgp->lun);
	scmd->kdebug = scgp->kdebug;
	if (scmd->timeout == 0 || scmd->timeout < scgp->deftimeout)
		scmd->timeout = scgp->deftimeout;
	if (scgp->disre_disable)
		scmd->flags &= ~SCG_DISRE_ENA;
	if (scgp->noparity)
		scmd->flags |= SCG_NOPARITY;

	scmd->u_sense.cmd_sense[0] = 0;		/* Paranioa */

	if (scgp->verbose) {
		printf("\nExecuting '%s' command on Bus %d Target %d, Lun %d timeout %ds\n",
			scgp->cmdname, scgp->scsibus, scgp->target, scmd->cdb.g0_cdb.lun,
			scmd->timeout);
		scsiprintcdb(scgp);
		if (scgp->verbose > 1)
			scsiprintwdata(scgp);
		flush();
	}

	if (scgp->running) {
		if (scgp->curcmdname) {
			error("Currently running '%s' command.\n",
							scgp->curcmdname);
		}
		raisecond("SCSI ALREADY RUNNING !!", 0L);
	}
	gettimeofday(scgp->cmdstart, (struct timezone *)0);
	scgp->curcmdname = scgp->cmdname;
	scgp->running = TRUE;
	ret = scsi_send(scgp, f, scmd);
	scgp->running = FALSE;
	scsitimes(scgp);
	if (ret < 0) {
		/*
		 * Old /dev/scg versions will not allow to access targets > 7.
		 * Include a workaround to make this non fatal.
		 */
		if (scgp->target < 8 || geterrno() != EINVAL)
			comerr("Cannot send SCSI cmd via ioctl\n");
		if (scmd->ux_errno == 0)
			scmd->ux_errno = geterrno();
		if (scmd->error == SCG_NO_ERROR)
			scmd->error = SCG_FATAL;
		if (scgp->debug) {
			errmsg("ret < 0 errno: %d ux_errno: %d error: %d\n",
					geterrno(), scmd->ux_errno, scmd->error);
		}
	}
	ret = scsicheckerr(scgp);
	if (scgp->verbose || (ret && scgp->silent <= 0)) {
		scsiprintresult(scgp);
	}
	return (ret);
}

EXPORT
int scsigetresid(scgp)
	SCSI	*scgp;
{
	return (scgp->scmd->resid);
}

LOCAL
void scsitimes(scgp)
	SCSI	*scgp;
{
	struct timeval	*stp = scgp->cmdstop;

	gettimeofday(stp, (struct timezone *)0);
	stp->tv_sec -= scgp->cmdstart->tv_sec;
	stp->tv_usec -= scgp->cmdstart->tv_usec;
	while (stp->tv_usec < 0) {
		stp->tv_sec -= 1;
		stp->tv_usec += 1000000;
	}
}

LOCAL BOOL
scsierr(scgp)
	SCSI	*scgp;
{
	register struct scg_cmd *cp = scgp->scmd;

	if(cp->error != SCG_NO_ERROR ||
				cp->ux_errno != 0 ||
				*(u_char *)&cp->scb != 0 ||
				cp->u_sense.cmd_sense[0] != 0)	/* Paranioa */
		return (TRUE);
	return (FALSE);
}

LOCAL int
scsicheckerr(scgp)
	SCSI	*scgp;
{
	register int ret;

	ret = 0;
	if(scsierr(scgp)) {
		if (!scgp->silent || scgp->verbose)
			scsiprinterr(scgp);
		ret = -1;
	}
	if ((!scgp->silent || scgp->verbose) && scgp->scmd->resid) {
		printf("resid: %d\n", scgp->scmd->resid);
		flush();
	}
	return (ret);
}

EXPORT void
scsiprinterr(scgp)
	SCSI	*scgp;
{
	register struct scg_cmd *cp = scgp->scmd;
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
					(long)scgp->cmdstop->tv_sec,
					(long)scgp->cmdstop->tv_usec/1000,
								cp->timeout);
				err = errbuf;
				break;
	default:		sprintf(errbuf, "error: %d", cp->error);
				err = errbuf;
	}

	errmsgno(cp->ux_errno, "%s: scsi sendcmd: %s\n", scgp->cmdname, err);
	scsiprintcdb(scgp);

	if (cp->error <= SCG_RETRYABLE)
		scsiprintstatus(scgp);

	if (cp->scb.chk) {
		scsiprsense((u_char *)&cp->sense, cp->sense_count);
		scsierrmsg(scgp, &cp->sense, &cp->scb, -1, scgp->nonstderrs);
	}
	flush();
}

EXPORT void
scsiprintcdb(scgp)
	SCSI	*scgp;
{
	scsiprbytes("CDB: ", scgp->scmd->cdb.cmd_cdb, scgp->scmd->cdb_len);
}

EXPORT void
scsiprintwdata(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scmd->size > 0 && (scmd->flags & SCG_RECV_DATA) == 0) {
		printf("Sending %d (0x%X) bytes of data.\n",
			scmd->size, scmd->size);
		scsiprbytes("Write Data: ",
			(Uchar *)scmd->addr,
			scmd->size > 100 ? 100 : scmd->size);
	}
}

EXPORT void
scsiprintrdata(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scmd->size > 0 && (scmd->flags & SCG_RECV_DATA) != 0) {
		printf("Got %d (0x%X), expecting %d (0x%X) bytes of data.\n",
			scmd->size-scmd->resid, scmd->size-scmd->resid,
			scmd->size, scmd->size);
		scsiprbytes("Received Data: ",
			(Uchar *)scmd->addr,
			(scmd->size-scmd->resid) > 100 ?
			100 : (scmd->size-scmd->resid));
	}
}

EXPORT void
scsiprintresult(scgp)
	SCSI	*scgp;
{
	printf("cmd finished after %ld.%03lds timeout %ds\n",
		(long)scgp->cmdstop->tv_sec,
		(long)scgp->cmdstop->tv_usec/1000,
		scgp->scmd->timeout);
	if (scgp->verbose > 1)
		scsiprintrdata(scgp);
	flush();
}

EXPORT
void scsiprintstatus(scgp)
	SCSI	*scgp;
{
	register struct scg_cmd *cp = scgp->scmd;

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

EXPORT void
scsiprbytes(s, cp, n)
		char	*s;
	register u_char	*cp;
	register int	n;
{
	printf(s);
	while (--n >= 0)
		printf(" %02X", *cp++);
	printf("\n");
}

EXPORT void
scsiprsense(cp, n)
	u_char	*cp;
	int	n;
{
	scsiprbytes("Sense Bytes:", cp, n);
}

EXPORT int
scsi_sense_key(scgp)
	SCSI	*scgp;
{
	register struct scg_cmd *cp = scgp->scmd;
	int	key = -1;

	if(!scsierr(scgp))
		return (0);

	if (cp->sense.code >= 0x70)
		key = ((struct scsi_ext_sense*)&(cp->sense))->key;
	return(key);
}

EXPORT int
scsi_sense_code(scgp)
	SCSI	*scgp;
{
	register struct scg_cmd *cp = scgp->scmd;
	int	code = -1;

	if(!scsierr(scgp))
		return (0);

	if (cp->sense.code >= 0x70)
		code = ((struct scsi_ext_sense*)&(cp->sense))->sense_code;
	else
		code = cp->sense.code;
	return(code);
}

EXPORT int
scsi_sense_qual(scgp)
	SCSI	*scgp;
{
	register struct scg_cmd *cp = scgp->scmd;

	if(!scsierr(scgp))
		return (0);

	if (cp->sense.code >= 0x70)
		return (((struct scsi_ext_sense*)&(cp->sense))->qual_code);
	else
		return (0);
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
	case INQ_IT8_1:
		printf("IT8 1");		break;
	case INQ_IT8_2:
		printf("IT8 2");		break;
	case INQ_STARR:
		printf("Storage array");	break;
	case INQ_ENCL:
		printf("Enclosure services");	break;

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
