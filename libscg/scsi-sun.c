/* @(#)scsi-sun.c	1.54 00/07/01 Copyright 1988,1995,2000 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-sun.c	1.54 00/07/01 Copyright 1988,1995,2000 J. Schilling";
#endif
/*
 *	SCSI user level command transport routines for
 *	the SCSI general driver 'scg'.
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

#include <scg/scgio.h>

#include <libport.h>		/* Needed for gethostid() */
#ifdef	HAVE_SUN_DKIO_H
#	include <sun/dkio.h>

#	define	dk_cinfo	dk_conf
#	define	dki_slave	dkc_slave
#	define	DKIO_GETCINFO	DKIOCGCONF
#endif
#ifdef	HAVE_SYS_DKIO_H
#	include <sys/dkio.h>

#	define	DKIO_GETCINFO	DKIOCINFO
#endif

#define	TARGET(slave)	((slave) >> 3)
#define	LUN(slave)	((slave) & 07)

/*
 * Tht USCSI ioctl() is not usable on SunOS 4.x
 */
#ifdef	__SVR4
#	define	USE_USCSI
#endif

LOCAL	char	_scg_trans_version[] = "scg-1.54";	/* The version for /dev/scg	*/

#ifdef	USE_USCSI
LOCAL	int	scsi_uopen	__PR((SCSI *scgp, char *device, int busno, int tgt, int tlun));
LOCAL	int	scsi_uclose	__PR((SCSI *scgp));
LOCAL	int	scsi_ucinfo	__PR((int f, struct dk_cinfo *cp, int debug));
LOCAL	int	scsi_ugettlun	__PR((int f, int *tgtp, int *lunp));
LOCAL	long	scsi_umaxdma	__PR((SCSI *scgp, long amt));
LOCAL	BOOL	scsi_uhavebus	__PR((SCSI *scgp, int));
LOCAL	int	scsi_ufileno	__PR((SCSI *scgp, int, int, int));
LOCAL	int	scsi_uinitiator_id __PR((SCSI *scgp));
LOCAL	int	scsi_uisatapi	__PR((SCSI *scgp));
LOCAL	int	scsi_ureset	__PR((SCSI *scgp));
LOCAL	int	scsi_usend	__PR((SCSI *scgp, int f, struct scg_cmd *sp));
#endif

/*
 * Need to move this into an scg driver ioctl.
 */
/*#define	MAX_DMA_SUN4M	(1024*1024)*/
#define	MAX_DMA_SUN4M	(124*1024)	/* Currently max working size */
/*#define	MAX_DMA_SUN4C	(126*1024)*/
#define	MAX_DMA_SUN4C	(124*1024)	/* Currently max working size */
#define	MAX_DMA_SUN3	(63*1024)
#define	MAX_DMA_SUN386	(56*1024)
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

/*
 * We are using a "real" /dev/scg?
 */
#define	scsi_xsend(scgp, f, cmdp)	ioctl((f), SCGIO_CMD, (cmdp))
#define	MAX_SCG		16	/* Max # of SCSI controllers */
#define	MAX_TGT		16
#define	MAX_LUN		8

struct scg_local {
	union {
		int	SCGfiles[MAX_SCG];
#ifdef	USE_USCSI
		short	scgfiles[MAX_SCG][MAX_TGT][MAX_LUN];
#endif
	} u;
#ifdef	USE_USCSI
	int	isuscsi;
#endif
};
#define scglocal(p)	((struct scg_local *)((p)->local))
#define scgfiles(p)	(scglocal(p)->u.SCGfiles)

/*
 * Return version information for the low level SCSI transport code.
 * This has been introduced to make it easier to trace down problems
 * in applications.
 */
EXPORT char *
scg__version(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (scgp != (SCSI *)0) {
		switch (what) {

		case SCG_VERSION:
			return (_scg_trans_version);
		/*
		 * If you changed this source, you are not allowed to
		 * return "schily" for the SCG_AUTHOR request.
		 */
		case SCG_AUTHOR:
			return (_scg_auth_schily);
		case SCG_SCCS_ID:
			return (__sccsid);
		}
	}
	return ((char *)0);
}

EXPORT int
scsi_open(scgp, device, busno, tgt, tlun)
	SCSI	*scgp;
	char	*device;
	int	busno;
	int	tgt;
	int	tlun;
{
	register int	f;
	register int	i;
	register int	nopen = 0;
	char		devname[32];

	if (busno >= MAX_SCG) {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Illegal value for busno '%d'", busno);
		return (-1);
	}

	if ((device != NULL && *device != '\0') || (busno == -2 && tgt == -2)) {
#ifdef	USE_USCSI
		return (scsi_uopen(scgp, device, busno, tgt, tlun));
#else
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Open by 'devname' not supported on this OS");
		return (-1);
#endif
	}

	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof(struct scg_local));
		if (scgp->local == NULL) {
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE, "No memory for scg_local");
			return (0);
		}

		for (i=0; i < MAX_SCG; i++) {
			scgfiles(scgp)[i] = -1;
		}
#ifdef	USE_USCSI
		scglocal(scgp)->isuscsi = 0;
#endif

	}


	for (i=0; i < MAX_SCG; i++) {
		sprintf(devname, "/dev/scg%d", i);
		f = open(devname, 2);
		if (f < 0) {
			if (errno != ENOENT && errno != ENXIO) {
				if (scgp->errstr)
					js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
						"Cannot open '%s'", devname);
				return (-1);
			}
		} else {
			nopen++;
		}
		scgfiles(scgp)[i] = f;
	}
#ifdef	USE_USCSI
	if (nopen <= 0)
		return (scsi_uopen(scgp, device, busno, tgt, tlun));
#endif
	return (nopen);
}

EXPORT int
scsi_close(scgp)
	SCSI	*scgp;
{
	register int	i;

	if (scgp->local == NULL)
		return (-1);
#ifdef	USE_USCSI
	if (scglocal(scgp)->isuscsi == 1)
		return (scsi_uclose(scgp));
#endif
	for (i=0; i < MAX_SCG; i++) {
		if (scgfiles(scgp)[i] >= 0)
			close(scgfiles(scgp)[i]);
		scgfiles(scgp)[i] = -1;
	}
	return (0);
}

LOCAL long
scsi_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	long	maxdma = MAX_DMA_OTHER;
#if	!defined(__i386_) && !defined(i386)
	int	cpu_type;
#endif

#ifdef	USE_USCSI
	long	l;

	if (scglocal(scgp)->isuscsi == 1) {
		l = scsi_umaxdma(scgp, amt);
		if (l > 0)
			return (l);
	}
#endif

#if	defined(__i386_) || defined(i386)
	maxdma = MAX_DMA_SUN386;
#else
	cpu_type = gethostid() >> 24;

	switch (cpu_type & ARCH_MASK) {

	case ARCH_SUN4C:
	case ARCH_SUN4E:
		maxdma = MAX_DMA_SUN4C;
		break;

	case ARCH_SUN4M:
	case ARCH_SUNX:
		maxdma = MAX_DMA_SUN4M;
		break;

	default:
		maxdma = MAX_DMA_SUN3;
	}
#endif

#ifndef	__SVR4
	/*
	 * SunOS 4.x allows esp hardware on VME boards and thus
	 * limits DMA on esp to 64k-1
	 */
	if (maxdma > MAX_DMA_SUN3)
		maxdma = MAX_DMA_SUN3;
#endif
	return (maxdma);
}

EXPORT BOOL
scsi_havebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	if (scgp->local == NULL)
		return (FALSE);
#ifdef	USE_USCSI
	if (scglocal(scgp)->isuscsi == 1)
		return (scsi_uhavebus(scgp, busno));
#endif
	return (busno < 0 || busno >= MAX_SCG) ? FALSE : (scgfiles(scgp)[busno] >= 0);
}

EXPORT int
scsi_fileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	if (scgp->local == NULL)
		return (-1);
#ifdef	USE_USCSI
	if (scglocal(scgp)->isuscsi == 1)
		return (scsi_ufileno(scgp, busno, tgt, tlun));
#endif

	return (busno < 0 || busno >= MAX_SCG) ? -1 : scgfiles(scgp)[busno];
}

EXPORT int
scsi_initiator_id(scgp)
	SCSI	*scgp;
{
	int		f = scsi_fileno(scgp, scgp->scsibus, scgp->target, scgp->lun);
	int		id = -1;
#ifdef	DKIO_GETCINFO
	struct dk_cinfo	conf;
#endif

#ifdef	USE_USCSI
	if (scglocal(scgp)->isuscsi == 1)
		return (scsi_uinitiator_id(scgp));
#endif

#ifdef	DKIO_GETCINFO
	if (ioctl(f, DKIO_GETCINFO, &conf) < 0)
		return (id);
	if (TARGET(conf.dki_slave) != -1)
		id = TARGET(conf.dki_slave);
#endif
	return (id);
}

EXPORT int
scsi_isatapi(scgp)
	SCSI	*scgp;
{
#ifdef	USE_USCSI
	if (scglocal(scgp)->isuscsi == 1)
		return (scsi_uisatapi(scgp));
#endif
	return (FALSE);
}

EXPORT int
scsireset(scgp)
	SCSI	*scgp;
{
	int	f = scsi_fileno(scgp, scgp->scsibus, scgp->target, scgp->lun);

#ifdef	USE_USCSI
	if (scglocal(scgp)->isuscsi == 1)
		return (scsi_ureset(scgp));
#endif
	return (ioctl(f, SCGIORESET, 0));
}

EXPORT void *
scsi_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	if (amt <= 0 || amt > scsi_bufsize(scgp, amt))
		return ((void *)0);
	scgp->bufbase = (void *)valloc((size_t)amt);
	return (scgp->bufbase);
}

EXPORT void
scsi_freebuf(scgp)
	SCSI	*scgp;
{
	if (scgp->bufbase)
		free(scgp->bufbase);
	scgp->bufbase = NULL;
}

LOCAL int
scsi_send(scgp, f, sp)
	SCSI		*scgp;
	int		f;
	struct scg_cmd	*sp;
{
#ifdef	USE_USCSI
	if (scglocal(scgp)->isuscsi == 1)
		return (scsi_usend(scgp, f, sp));
#endif

	return (ioctl(f, SCGIO_CMD, sp));
}

/*--------------------------------------------------------------------------*/
/*
 *	This is Sun USCSI interface code ...
 */
#ifdef	USE_USCSI
#include <strdefs.h>

#include <sys/scsi/impl/uscsi.h>

/*
 * Bit Mask definitions, for use accessing the status as a byte.
 */
#define	STATUS_MASK			0x3E
#define	STATUS_GOOD			0x00
#define	STATUS_CHECK			0x02

#define	STATUS_RESERVATION_CONFLICT	0x18
#define	STATUS_TERMINATED		0x22

#ifdef	nonono
#define	STATUS_MASK			0x3E
#define	STATUS_GOOD			0x00
#define	STATUS_CHECK			0x02

#define	STATUS_MET			0x04
#define	STATUS_BUSY			0x08
#define	STATUS_INTERMEDIATE		0x10
#define	STATUS_SCSI2			0x20
#define	STATUS_INTERMEDIATE_MET		0x14
#define	STATUS_RESERVATION_CONFLICT	0x18
#define	STATUS_TERMINATED		0x22
#define	STATUS_QFULL			0x28
#define	STATUS_ACA_ACTIVE		0x30
#endif

LOCAL int
scsi_uopen(scgp, device, busno, tgt, tlun)
	SCSI	*scgp;
	char	*device;
	int	busno;
	int	tgt;
	int	tlun;
{
	register int	f;
	register int	b;
	register int	t;
	register int	l;
	register int	nopen = 0;
	char		devname[32];

	if (scgp->overbose)
		error("Warning: Using USCSI interface.\n");

	if (busno >= MAX_SCG || tgt >= MAX_TGT || tlun >= MAX_LUN) {
		errno = EINVAL;
		if (scgp->errstr)
		    js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
		       "Illegal value for busno, target or lun '%d,%d,%d'",
		       busno, tgt, tlun);

		return (-1);
	}
	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof(struct scg_local));
		if (scgp->local == NULL)
			return (0);
		scglocal(scgp)->isuscsi = 0;
	}
	if (scglocal(scgp)->isuscsi == 0) {
		for (b=0; b < MAX_SCG; b++) {
			for (t=0; t < MAX_TGT; t++) {
				for (l=0; l < MAX_LUN ; l++)
					scglocal(scgp)->u.scgfiles[b][t][l] = (short)-1;
			}
		}
		scglocal(scgp)->isuscsi = 1;
	}

	if (device != NULL && strcmp(device, "USCSI") == 0)
		goto uscsiscan;

	if ((device != NULL && *device != '\0') || (busno == -2 && tgt == -2))
		goto openbydev;

uscsiscan:
	if (busno >= 0 && tgt >= 0 && tlun >= 0) {

		if (busno >= MAX_SCG || tgt >= MAX_TGT || tlun >= MAX_LUN)
			return (-1);

		js_snprintf(devname, sizeof(devname), "/dev/rdsk/c%dt%dd%ds2",
			busno, tgt, tlun);
		f = open(devname, O_RDONLY | O_NDELAY);
		if (f < 0) {
			js_snprintf(scgp->errstr,
				    SCSI_ERRSTR_SIZE,
				"Cannot open '%s'", devname);
			return (0);
		}
		scglocal(scgp)->u.scgfiles[busno][tgt][tlun] = f;
		return 1;
	} else {

		for (b=0; b < MAX_SCG; b++) {
			for (t=0; t < MAX_TGT; t++) {
				for (l=0; l < MAX_LUN ; l++) {
					js_snprintf(devname, sizeof(devname),
						"/dev/rdsk/c%dt%dd%ds2",
						b, t, l);
					f = open(devname, O_RDONLY | O_NDELAY);
					if (f < 0 && errno != ENOENT
							&& errno != ENXIO
							&& errno != ENODEV) {
						if (scgp->errstr)
							js_snprintf(scgp->errstr,
							    SCSI_ERRSTR_SIZE,
							    "Cannot open '%s'", devname);
					}
					if (f < 0 && l == 0)
						break;
					if (f >= 0) {
						nopen ++;
						if (scglocal(scgp)->u.scgfiles[b][t][l] == -1)
						scglocal(scgp)->u.scgfiles[b][t][l] = f;
					else
						close(f);
					}
				}
			}
		}
	}
openbydev:
	if (nopen == 0) {
		int	target;
		int	lun;

		if (device == NULL || device[0] == '\0')
			return (0);

		f = open(device, O_RDONLY | O_NDELAY);
		if (f < 0) {
			js_snprintf(scgp->errstr,
				    SCSI_ERRSTR_SIZE,
				"Cannot open '%s'", device);
			return (0);
		}

		if (busno < 0)
			busno = 0;	/* Use Fake number if not specified */
		scgp->scsibus = busno;

		if (scsi_ugettlun(f, &target, &lun) >= 0) {
			if (tgt >= 0 && tlun >= 0) {
				if (tgt != target || tlun != lun) {
					close(f);
					return (0);
				}
			}
			tgt = target;
			tlun = lun;
		} else {
			if (tgt < 0 || tlun < 0) {
				close(f);
				return (0);
			}
		}
		scgp->target = tgt;
		scgp->lun = tlun;

		if (scglocal(scgp)->u.scgfiles[busno][tgt][tlun] == -1)
			scglocal(scgp)->u.scgfiles[busno][tgt][tlun] = f;

		return (++nopen);
	}
	return (nopen);
}

LOCAL int
scsi_uclose(scgp)
	SCSI	*scgp;
{
	register int	f;
	register int	b;
	register int	t;
	register int	l;

	if (scgp->local == NULL)
		return (-1);

	for (b=0; b < MAX_SCG; b++) {
		for (t=0; t < MAX_TGT; t++) {
			for (l=0; l < MAX_LUN ; l++) {
				f = scglocal(scgp)->u.scgfiles[b][t][l];
				if (f >= 0)
					close(f);
				scglocal(scgp)->u.scgfiles[b][t][l] = (short)-1;
			}
		}
	}
	return (0);
}

LOCAL int
scsi_ucinfo(f, cp, debug)
	int		f;
	struct dk_cinfo *cp;
	int		debug;
{
	fillbytes(cp, sizeof(*cp), '\0');

	if (ioctl(f, DKIOCINFO, cp) < 0)
		return (-1);

	if (debug <= 0)
		return (0);

	printf("cname:		'%s'\n", cp->dki_cname);
	printf("ctype:		0x%04hX %hd\n", cp->dki_ctype, cp->dki_ctype);
	printf("cflags:		0x%04hX\n", cp->dki_flags);
	printf("cnum:		%hd\n", cp->dki_cnum);
#ifdef	__EVER__
	printf("addr:		%d\n", cp->dki_addr);
	printf("space:		%d\n", cp->dki_space);
	printf("prio:		%d\n", cp->dki_prio);
	printf("vec:		%d\n", cp->dki_vec);
#endif
	printf("dname:		'%s'\n", cp->dki_dname);
	printf("unit:		%d\n", cp->dki_unit);
	printf("slave:		%d %04o Tgt: %d Lun: %d\n",
				cp->dki_slave, cp->dki_slave,
				TARGET(cp->dki_slave), LUN(cp->dki_slave));
	printf("partition:	%hd\n", cp->dki_partition);
	printf("maxtransfer:	%d (%d)\n",
				cp->dki_maxtransfer,
				cp->dki_maxtransfer * DEV_BSIZE);
	return (0);
}

LOCAL int
scsi_ugettlun(f, tgtp, lunp)
	int	f;
	int	*tgtp;
	int	*lunp;
{
	struct dk_cinfo ci;

	if (scsi_ucinfo(f, &ci, 0) < 0)
		return (-1);
	if (tgtp)
		*tgtp = TARGET(ci.dki_slave);
	if (lunp)
		*lunp = LUN(ci.dki_slave);
	return (0);
}

LOCAL long
scsi_umaxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	register int	b;
	register int	t;
	register int	l;
	long		maxdma = -1L;
	int		f;
	struct dk_cinfo ci;

	if (scgp->local == NULL)
		return (-1L);

	for (b=0; b < MAX_SCG; b++) {
		for (t=0; t < MAX_TGT; t++) {
			for (l=0; l < MAX_LUN ; l++) {
				if ((f = scglocal(scgp)->u.scgfiles[b][t][l]) < 0)
					continue;
				if (scsi_ucinfo(f, &ci, 0) < 0)
					continue;
				if (maxdma < 0)
					maxdma = (long)(ci.dki_maxtransfer * DEV_BSIZE);
				if (maxdma > (long)(ci.dki_maxtransfer * DEV_BSIZE))
					maxdma = (long)(ci.dki_maxtransfer * DEV_BSIZE);
			}
		}
	}
	return (maxdma);
}

LOCAL BOOL
scsi_uhavebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	register int	t;
	register int	l;

	if (scgp->local == NULL || busno < 0 || busno >= MAX_SCG)
		return (FALSE);

	for (t=0; t < MAX_TGT; t++) {
		for (l=0; l < MAX_LUN ; l++)
			if (scglocal(scgp)->u.scgfiles[busno][t][l] >= 0)
				return (TRUE);
	}
	return (FALSE);
}

LOCAL int
scsi_ufileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	if (scgp->local == NULL ||
	    busno < 0 || busno >= MAX_SCG ||
	    tgt < 0 || tgt >= MAX_TGT ||
	    tlun < 0 || tlun >= MAX_LUN)
		return (-1);

	return ((int)scglocal(scgp)->u.scgfiles[busno][tgt][tlun]);
}

LOCAL int
scsi_uinitiator_id(scgp)
	SCSI	*scgp;
{
	return (-1);
}

LOCAL int 
scsi_uisatapi(scgp)
	SCSI	*scgp;
{
	char		devname[32];
	char		symlinkname[MAXPATHLEN];
	int		len;
	int		f = scsi_fileno(scgp, scgp->scsibus, scgp->target, scgp->lun);
	struct dk_cinfo ci;

	if (ioctl(f, DKIOCINFO, &ci) < 0)
		return (-1);

	js_snprintf(devname, sizeof(devname), "/dev/rdsk/c%dt%dd%ds2",
		scgp->scsibus, scgp->target, scgp->lun);

	symlinkname[0] = '\0';
	len = readlink(devname, symlinkname, sizeof(symlinkname));
	if (len > 0)
		symlinkname[len] = '\0';

	if (len >= 0 && strstr(symlinkname, "ide") != NULL)
		return (TRUE);
	else
		return (FALSE);
}

LOCAL int
scsi_ureset(scgp)
	SCSI	*scgp;
{
	int	f = scsi_fileno(scgp, scgp->scsibus, scgp->target, scgp->lun);
	struct uscsi_cmd req;

	fillbytes(&req, sizeof(req), '\0');
	req.uscsi_flags = USCSI_RESET | USCSI_SILENT;	/* reset target */

	return (ioctl(f, USCSICMD, &req));
}

LOCAL int
scsi_usend(scgp, f, sp)
	SCSI		*scgp;
	int		f;
	struct scg_cmd	*sp;
{
	struct uscsi_cmd req;
	int		ret;

	if (f < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}

	fillbytes(&req, sizeof(req), '\0');

	req.uscsi_flags = USCSI_SILENT | USCSI_RQENABLE;

	if (sp->flags & SCG_RECV_DATA) {
		req.uscsi_flags |= USCSI_READ;
	} else if (sp->size > 0) {
		req.uscsi_flags |= USCSI_WRITE;
	}
	req.uscsi_buflen	= sp->size;
	req.uscsi_bufaddr	= sp->addr;
	req.uscsi_timeout	= sp->timeout;
	req.uscsi_cdblen	= sp->cdb_len;
	req.uscsi_rqbuf		= (caddr_t) sp->u_sense.cmd_sense;
	req.uscsi_rqlen		= sizeof (sp->u_sense.cmd_sense);
	req.uscsi_cdb		= (caddr_t) &sp->cdb;

	errno = 0;
	ret = ioctl(f, USCSICMD, &req);

	if (scgp->debug) {
		printf("ret: %d errno: %d (%s)\n", ret, errno, errmsgstr(errno));
		printf("uscsi_flags:     0x%x\n", req.uscsi_flags);
		printf("uscsi_status:    0x%x\n", req.uscsi_status);
		printf("uscsi_timeout:   %d\n", req.uscsi_timeout);
		printf("uscsi_bufaddr:   0x%lx\n", (long)req.uscsi_bufaddr);
		printf("uscsi_buflen:    %d\n", req.uscsi_buflen);
		printf("uscsi_resid:     %d\n", req.uscsi_resid);
		printf("uscsi_rqlen:     %d\n", req.uscsi_rqlen);
		printf("uscsi_rqstatus:  0x%x\n", req.uscsi_rqstatus);
		printf("uscsi_rqresid:   %d\n", req.uscsi_rqresid);
		printf("uscsi_rqbuf:     0x%lx\n", (long)req.uscsi_rqbuf);
	}
	if (ret < 0) {
		sp->ux_errno = geterrno();
		/*
		 * Check if SCSI command cound not be send at all.
		 */
		if (sp->ux_errno == ENOTTY && scsi_isatapi(scgp) == TRUE) {
			if (scgp->debug)
				printf("ENOTTY atapi: %d\n", scsi_isatapi(scgp));
			sp->error = SCG_FATAL;
			return (0);
		}
		if (errno == ENOTTY || errno == ENXIO ||
		    errno == EINVAL || errno == EACCES) {
			return (-1);
		}
	} else {
		sp->ux_errno = 0;
	}
	ret			= 0;
	sp->sense_count		= req.uscsi_rqlen - req.uscsi_rqresid;
	sp->resid		= req.uscsi_resid;
	sp->u_scb.cmd_scb[0]	= req.uscsi_status;

	if ((req.uscsi_status & STATUS_MASK) == STATUS_GOOD) {
		sp->error = SCG_NO_ERROR;
		return (0);
	}
	if (req.uscsi_rqstatus == 0 &&
	    ((req.uscsi_status & STATUS_MASK) == STATUS_CHECK)) {
		sp->error = SCG_RETRYABLE;
		return (0);
	}
	if (req.uscsi_status & (STATUS_TERMINATED |
				 STATUS_RESERVATION_CONFLICT)) {
		sp->error = SCG_FATAL;
	}
	if (req.uscsi_status != 0) {
		sp->error = SCG_RETRYABLE;
	}

	return (ret);
}
#endif	/* USE_USCSI */
