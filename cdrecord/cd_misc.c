/* @(#)cd_misc.c	1.2 98/03/12 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)cd_misc.c	1.2 98/03/12 Copyright 1997 J. Schilling";
#endif
/*
 *	Misc CD support routines
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

#include <standard.h>
#include <sys/types.h>

#include "cdrecord.h"

EXPORT	int	from_bcd		__PR((int b));
EXPORT	int	to_bcd			__PR((int i));
EXPORT	long	msf_to_lba		__PR((int m, int s, int f));
EXPORT	BOOL	lba_to_msf		__PR((long lba, msf_t *mp));

EXPORT int
from_bcd(b)
	int	b;
{
	return ((b & 0x0F) + 10 * (((b)>> 4) & 0x0F));
}

EXPORT int
to_bcd(i)
	int	i;
{
	return (i % 10 | ((i / 10) % 10) << 4);
}

EXPORT long
msf_to_lba(m, s, f)
	int	m;
	int	s;
	int	f;
{
	long	ret = m * 60 + s;

	ret *= 75;
	ret += f;
	if (m < 90)
		ret -= 150;
	else
		ret -= 450150;
	return (ret);
}

EXPORT BOOL
lba_to_msf(lba, mp)
	long	lba;
	msf_t	*mp;
{
	int	m;
	int	s;
	int	f;

	if (lba >= -150 && lba < 405000) {	/* lba <= 404849 */
		m = (lba + 150) / 60 / 75;
		s = (lba + 150 - m*60*75)  / 75;
		f = (lba + 150 - m*60*75 - s*75);

	} else if (lba >= -45150 && lba <= -151) {
		m = (lba + 450150) / 60 / 75;
		s = (lba + 450150 - m*60*75)  / 75;
		f = (lba + 450150 - m*60*75 - s*75);

	} else {
		mp->msf_min   = -1;
		mp->msf_sec   = -1;
		mp->msf_frame = -1;

		return (FALSE);
	}
	mp->msf_min   = m;
	mp->msf_sec   = s;
	mp->msf_frame = f;

	if (lba > 404849)			/* 404850 -> 404999: lead out */
		return (FALSE);
	return (TRUE);
}
