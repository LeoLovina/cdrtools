/* @(#)defaults.c	1.6 01/02/21 Copyright 1998 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)defaults.c	1.6 01/02/21 Copyright 1998 J. Schilling";
#endif
/*
 *	Copyright (c) 1998 J. Schilling
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
#include <stdxlib.h>
#include <unixstd.h>
#include <strdefs.h>
#include <stdio.h>
#include <standard.h>
#include <deflts.h>
#include <utypes.h>
#include <schily.h>
#include "cdrecord.h"

EXPORT	void	cdr_defaults	__PR((char **devp, int *speedp, long *fsp));
LOCAL	void	cdr_xdefaults	__PR((char **devp, int *speedp, long *fsp));
LOCAL	char *	strsv		__PR((char* s));

EXPORT void
cdr_defaults(devp, speedp, fsp)
	char	**devp;
	int	*speedp;
	long	*fsp;
{
	char	*dev	= *devp;
	int	speed	= *speedp;
	long	fs	= *fsp;

	if (!dev) {
		*devp = getenv("CDR_DEVICE");

		if (!*devp && defltopen("/etc/default/cdrecord") == 0) {
			dev = defltread("CDR_DEVICE=");
			if (dev != NULL)
				*devp = strsv(dev);
		}
	}
	if (*devp)
		cdr_xdefaults(devp, &speed, &fs);

	if (speed < 0) {
		char	*p = getenv("CDR_SPEED");

		if (!p) {
			if (defltopen("/etc/default/cdrecord") == 0) {
				p = defltread("CDR_SPEED=");
			}
		}
		if (p) {
			speed = atoi(p);
			if (speed < 0)
				comerrno(EX_BAD, "Bad speed environment.\n");
		}
	}
	if (speed >= 0)
		*speedp = speed;

	if (fs < 0L) {
		char	*p = getenv("CDR_FIFOSIZE");

		if (!p) {
			if (defltopen("/etc/default/cdrecord") == 0) {
				p = defltread("CDR_FIFOSIZE=");
			}
		}
		if (p) {
			if (getnum(p, &fs) != 1)
				comerrno(EX_BAD, "Bad fifo size environment.\n");
		}
	}
	if (fs > 0L)
		*fsp = fs;

	defltclose();
}

LOCAL void
cdr_xdefaults(devp, speedp, fsp)
	char	**devp;
	int	*speedp;
	long	*fsp;
{
	char	dname[256];
	char	*p = *devp;
	char	*x = ",:/@";

	while (*x) {
		if (strchr(p, *x))
			return;
		x++;
	}
	js_snprintf(dname, sizeof(dname), "%s=", p);
	if (defltopen("/etc/default/cdrecord") != 0)
		return;

	p = defltread(dname);
	if (p != NULL) {
		while (*p == '\t')
			p++;
		if ((x = strchr(p, '\t')) != NULL)
			*x = '\0';
		*devp = strsv(p);
		if (x) {
			p = ++x;
			while (*p == '\t')
				p++;
			if ((x = strchr(p, '\t')) != NULL)
				*x = '\0';
			if (*speedp < 0)
				*speedp = atoi(p);
		}
		if (x) {
			p = ++x;
			while (*p == '\t')
				p++;
			if ((x = strchr(p, '\t')) != NULL)
				*x = '\0';
			if (*fsp < 0L) {
				if (getnum(p, fsp) != 1)
					comerrno(EX_BAD,
					"Bad fifo size in defaults.\n");
			}
		}
		if (x) {
			p = ++x;
			while (*p == '\t')
				p++;
			if ((x = strchr(p, '\t')) != NULL)
				*x = '\0';
			if (strcmp(p, "\"\"")) {
				extern	char	*driveropts;

				if (driveropts == NULL)
					driveropts = strsv(p);
			}
		}
	}
}

LOCAL char *
strsv(s)
	char	*s;
{
	char	*p;
	int len = strlen(s);

	p = malloc(len+1);
	if (p)
		strcpy(p, s);
	return (p);
}
