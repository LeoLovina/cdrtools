/* @(#)scsihack.c	1.7 97/11/06 Copyright 1997 J. Schilling */
#ifndef lint
static	char _sccsid[] =
	"@(#)scsihack.c	1.7 97/11/06 Copyright 1997 J. Schilling";
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
 *	Currently available:
 *	Interface for Linux broken SCSI generic driver.
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

#ifdef	linux
#define	SCSI_IMPL		/* We have a SCSI implementation for Linux */

#include "scsi-linux-sg.c"

#endif	/* linux */

#if	defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define	SCSI_IMPL		/* We have a SCSI implementation for *BSD */

#include "scsi-bsd.c"

#endif	/* *BSD */

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

LOCAL	int	scsi_send	__PR((int f, struct scg_cmd *sp));

EXPORT
int scsi_open()
{
	comerrno(EX_BAD, "No SCSI transport implementation for this architecture.\n");
}

LOCAL long
scsi_maxdma()
{
	return	(0L);
}

EXPORT
BOOL scsi_havebus(busno)
	int	busno;
{
	return (FALSE);
}

EXPORT
int scsi_fileno(busno, tgt, tlun)
	int	busno;
	int	tgt;
	int	tlun;
{
	return (-1);
}

EXPORT
int scsireset()
{
	return (-1);
}

EXPORT void *
scsi_getbuf(amt)
	long	amt;
{
	return ((void *)0);
}

LOCAL int
scsi_send(f, sp)
	int		f;
	struct scg_cmd	*sp;
{
	return (-1);
}

#endif	/* SCSI_IMPL */
