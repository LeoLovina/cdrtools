/* @(#)scsihack.c	1.25 00/01/22 Copyright 1997 J. Schilling */
#ifndef lint
static	char _sccsid[] =
	"@(#)scsihack.c	1.25 00/01/22 Copyright 1997 J. Schilling";
#endif
/*
 *	Interface for other generic SCSI implementations.
 *	To add a new hack, add something like:
 *
 *	#ifdef	__FreeBSD__
 *	#define	SCSI_IMPL
 *	#include some code
 *	#endif
 *
 *	Warning: you may change this source or add new SCSI tranport
 *	implementations, but if you do that you need to change the
 *	_scg_version and _scg_auth* string that are returned by the
 *	SCSI transport code.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *	If your version has been integrated into the main steam release,
 *	the return value will be set to "schily".
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

LOCAL	int	scsi_send	__PR((SCSI *scgp, int f, struct scg_cmd *sp));


#ifdef	linux
#define	SCSI_IMPL		/* We have a SCSI implementation for Linux */

#ifdef	not_needed		/* We now have a local vrersion of pg.h  */
#ifndef	HAVE_LINUX_PG_H		/* If we are compiling on an old version */
#	undef	USE_PG_ONLY	/* there is no 'pg' driver and we cannot */
#	undef	USE_PG		/* include <linux/pg.h> which is needed  */
#endif				/* by the pg transport code.		 */
#endif

#ifdef	USE_PG_ONLY
#include "scsi-linux-pg.c"
#else
#include "scsi-linux-sg.c"
#endif

#endif	/* linux */

#if	defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define	SCSI_IMPL		/* We have a SCSI implementation for *BSD */

#include "scsi-bsd.c"

#endif	/* *BSD */

#if	defined(__bsdi__)	/* We have a SCSI implementation for BSD/OS 3.x (and later?) */
# include <sys/param.h>
# if (_BSDI_VERSION >= 199701)
#  define	SCSI_IMPL

#  include "scsi-bsd-os.c"

# endif	/* BSD/OS >= 3.0 */
#endif /* BSD/OS */

#ifdef	__sgi
#define	SCSI_IMPL		/* We have a SCSI implementation for SGI */

#include "scsi-sgi.c"

#endif	/* SGI */

#ifdef	__hpux
#define	SCSI_IMPL		/* We have a SCSI implementation for HP-UX */

#include "scsi-hpux.c"

#endif	/* HP-UX */

#if	defined(_IBMR2) || defined(_AIX)
#define	SCSI_IMPL		/* We have a SCSI implementation for AIX */

#include "scsi-aix.c"

#endif	/* AIX */

#if	defined(__NeXT__)
#define	SCSI_IMPL		/* We have a SCSI implementation for NextStep */

#include "scsi-next.c"

#endif	/* NEXT */

#if	defined(__osf__)
#define	SCSI_IMPL		/* We have a SCSI implementation for OSF/1 */

#include "scsi-osf.c"

#endif	/* OSF/1 */

#ifdef	VMS
#define	SCSI_IMPL		/* We have a SCSI implementation for VMS */

#include "scsi-vms.c"

#endif	/* VMS */

#ifdef	OPENSERVER
#define	SCSI_IMPL		/* We have a SCSI implementation for SCO OpenServer */

#include "scsi-openserver.c"

#endif  /* SCO */

#ifdef	UNIXWARE
#define	SCSI_IMPL		/* We have a SCSI implementation for SCO UnixWare */

#include "scsi-unixware.c"

#endif  /* UNIXWARE */

#ifdef	__OS2
#define	SCSI_IMPL		/* We have a SCSI implementation for OS/2 */

#include "scsi-os2.c"

#endif  /* OS/2 */

#ifdef	__BEOS__
#define	SCSI_IMPL		/* Yep, BeOS does that funky scsi stuff */
#include "scsi-beos.c"
#endif

#ifdef	__CYGWIN32__
#define	SCSI_IMPL		/* Yep, we support WNT and W9? */
#include "scsi-wnt.c"
#endif

#ifdef	__NEW_ARCHITECTURE
#define	SCSI_IMPL		/* We have a SCSI implementation for XXX */
/*
 * Add new hacks here
 */
#include "scsi-new-arch.c"
#endif

#ifndef	SCSI_IMPL
/*
 * This is to make scsitranp.c compile on all architectures.
 * This does not mean that you may use it, but you can see
 * if other problems exist.
 */

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsihack.c-1.25";	/* The version for this transport*/

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
			return (_sccsid);
		}
	}
	return ((char *)0);
}

EXPORT
int scsi_open(scgp, device, busno, tgt, tlun)
	SCSI	*scgp;
	char	*device;
	int	busno;
	int	tgt;
	int	tlun;
{
	comerrno(EX_BAD, "No SCSI transport implementation for this architecture.\n");
	return (-1);	/* Keep lint happy */
}

EXPORT int
scsi_close(scgp)
	SCSI	*scgp;
{
	return (-1);
}

LOCAL long
scsi_maxdma(scgp)
	SCSI	*scgp;
{
	return	(0L);
}

EXPORT void
scsi_freebuf(scgp)
	SCSI	*scgp;
{
}

EXPORT
BOOL scsi_havebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	return (FALSE);
}

EXPORT
int scsi_fileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	return (-1);
}

EXPORT int
scsi_initiator_id(scgp)
	SCSI	*scgp;
{
	return (-1);
}

EXPORT
int scsi_isatapi(scgp)
	SCSI	*scgp;
{
	return (FALSE);
}

EXPORT
int scsireset(scgp)
	SCSI	*scgp;
{
	return (-1);
}

EXPORT void *
scsi_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	return ((void *)0);
}

LOCAL int
scsi_send(scgp, f, sp)
	SCSI		*scgp;
	int		f;
	struct scg_cmd	*sp;
{
	return (-1);
}

#endif	/* SCSI_IMPL */
