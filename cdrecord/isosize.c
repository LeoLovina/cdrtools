/* @(#)isosize.c	1.6 99/10/18 Copyright 1996 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)isosize.c	1.6 99/10/18 Copyright 1996 J. Schilling";
#endif
/*
 *	Copyright (c) 1996 J. Schilling
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

#include <sys/types.h>
#include <sys/stat.h>
#include <statdefs.h>
#include <unixstd.h>
#include <standard.h>
#include <utypes.h>
#include <intcvt.h>

#include "iso9660.h"

long	isosize		__PR((int f));

long
isosize(f)
	int	f;
{
	struct iso9660_voldesc		vd;
	struct iso9660_pr_voldesc	*vp;
	long				isize;
	struct stat			sb;
	long				mode;

	/*
	 * First check if a bad guy tries to call isosize()
	 * with an unappropriate file descriptor.
	 * return -1 in this case.
	 */
	if (isatty(f))
		return (-1L);
	if (fstat(f, &sb) < 0)
		return (-1L);
	mode = (long)(sb.st_mode & S_IFMT);
	if (!S_ISREG(mode) && !S_ISBLK(mode) && !S_ISCHR(mode))
		return (-1L);

	if (lseek(f, (off_t)(16L * 2048L), SEEK_SET) == -1)
		return (-1L);

	vp = (struct iso9660_pr_voldesc *) &vd;

	do {
		read(f, &vd, sizeof(vd));
		if (GET_UBYTE(vd.vd_type) == VD_PRIMARY)
			break;

	} while (GET_UBYTE(vd.vd_type) != VD_TERM);

	lseek(f, (off_t)0L, SEEK_SET);

	if (GET_UBYTE(vd.vd_type) != VD_PRIMARY)
		return (-1L);

	isize = GET_BINT(vp->vd_volume_space_size);
	isize *= GET_BSHORT(vp->vd_lbsize);
	return (isize);
}
