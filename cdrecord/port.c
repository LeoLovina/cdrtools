/* @(#)port.c	1.8 98/10/07 Copyright 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)port.c	1.8 98/10/07 Copyright 1995 J. Schilling";
#endif
/*
 *	Copyright (c) 1995 J. Schilling
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
#include <standard.h>
#include <stdxlib.h>
#include <timedefs.h>
#ifdef	NANOSEC
#include <poll.h>
#include <sys/systeminfo.h>

#ifdef	FMT
#include "fmt.h"
#endif

EXPORT	ulong	gethostid	__PR((void));
EXPORT	int	gethostname	__PR((char *name, int namelen));
EXPORT	int	getdomainname	__PR((char *name, int namelen));
EXPORT	int	usleep		__PR((int usec));

EXPORT ulong
gethostid()
{
	ulong	id;

	char	hbuf[257];
	sysinfo(SI_HW_SERIAL, hbuf, sizeof(hbuf));
	id = atoi(hbuf);
	return (id);
}

EXPORT int
gethostname(name, namelen)
	char	*name;
	int	namelen;
{
	if (sysinfo(SI_HOSTNAME, name, namelen) < 0)
		return (-1);
	return (0);
}

EXPORT int
getdomainname(name, namelen)
	char	*name;
	int	namelen;
{
	if (sysinfo(SI_SRPC_DOMAIN, name, namelen) < 0)
		return (-1);
	return (0);
}

#define	USE_POLL

EXPORT int
usleep(usec)
	int	usec;
{
	struct timespec ts;

	ts.tv_sec = usec / 1000000;
	ts.tv_nsec = (usec % 1000000) * 1000;

#if	defined(USE_POLL)

	if (poll(0, 0, usec/1000) < 0)
		comerr("poll delay failed.\n");

#elif	defined(USE_NANOSLEEP)

	nanosleep(&ts, 0);
#else
	if (ts.tv_sec == 0)
		ts.tv_sec = 1;
	sleep(ts.tv_sec);
#endif
	return (0);
}
#endif
