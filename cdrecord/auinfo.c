/* @(#)auinfo.c	1.5 00/06/02 Copyright 1999 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)auinfo.c	1.5 00/06/02 Copyright 1999 J. Schilling";
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
#include <stdio.h>
#include <standard.h>
#include <strdefs.h>
#include <deflts.h>
#include <utypes.h>
#include <schily.h>

#include "cdrecord.h"

extern	int	debug;

EXPORT	void	auinfo		__PR((char *name, int track, track_t *trackp));
LOCAL	char 	*readtag	__PR((char *name));
EXPORT	void	setmcn		__PR((char *mcn, track_t *trackp));
LOCAL	void	isrc_illchar	__PR((char *isrc, int c));
EXPORT	void	setisrc		__PR((char *isrc, track_t *trackp));
EXPORT	void	setindex	__PR((char *tindex, track_t *trackp));

#ifdef	XXX
main(ac, av)
	int	ac;
	char	*av[];
{
/*	auinfo("/etc/default/cdrecord");*/
/*	auinfo("/mnt2/CD3/audio_01.inf");*/
	auinfo("/mnt2/CD3/audio_01.wav");
}
#endif

EXPORT void
auinfo(name, track, trackp)
	char	*name;
	int	track;
	track_t	*trackp;
{
	char	infname[1024];
	char	*p;
	track_t	*tp = &trackp[track];
	long	l;
	long	tno = -1;
	BOOL	isdao = !is_tao(&trackp[0]);

	strncpy(infname, name, sizeof(infname)-1);
	infname[sizeof(infname)-1] = '\0';
	p = strrchr(infname, '.');
	if (p != 0 && &p[4] < &name[sizeof(infname)]) {
		strcpy(&p[1], "inf");
	}

	if (defltopen(infname) == 0) {

		p = readtag("CDDB_DISKID=");

		p = readtag("MCN=");
		if (p && *p)
			setmcn(p, &trackp[0]);

		p = readtag("ISRC=");
		if (p && *p)
			setisrc(p, &trackp[track]);

		p = readtag("Albumtitle=");
		p = readtag("Tracknumber=");
		if (p && isdao)
			astol(p, &tno);

		p = readtag("Trackstart=");
		if (p && isdao) {
			l = -1L;
			astol(p, &l);
			if (track == 1 && tno == 1 && l > 0) {
				trackp[1].pregapsize = 150 + l;
				printf("Track1 Start: '%s' (%ld)\n", p, l);
			}
		}

		p = readtag("Tracklength=");

		p = readtag("Pre-emphasis=");
		if (p && *p) {
			if (strncmp(p, "yes", 3) == 0) {
				tp->flags |= TI_PREEMP;
				if (tp->tracktype == TOC_DA)
					tp->sectype = ST_AUDIO_PRE;

			} else if (strncmp(p, "no", 2) == 0) {
				tp->flags &= ~TI_PREEMP;
				if (tp->tracktype == TOC_DA)
					tp->sectype = ST_AUDIO_NOPRE;
			}
		}

		p = readtag("Channels=");
		p = readtag("Copy_permitted=");
		if (p && *p) {
			if (strncmp(p, "yes", 3) == 0)
				tp->flags |= TI_COPY;
		}
		p = readtag("Endianess=");
		p = readtag("Index=");
		if (p && *p && isdao)
			setindex(p, &trackp[track]);

		p = readtag("Index0=");
		if (p && isdao) {
			long ts;
			long ps;

			l = -2L;
			astol(p, &l);
			if (l == -1) {
				trackp[track+1].pregapsize = 0;
			} else if (l > 0) {
				ts = tp->tracksize / 2352;
				ps = ts - l;
				if (ps > 0)
					trackp[track+1].pregapsize = ps;
			}
		}
	}

}

LOCAL char *
readtag(name)
	char	*name;
{
	char	*p;

	p = defltread(name);
	if (p) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (debug)
			printf("%s	'%s'\n", name, p);
	}
	return (p);
}

EXPORT void
setmcn(mcn, trackp)
	char	*mcn;
	track_t	*trackp;
{
	char	*p;

	if (strlen(mcn) != 13)
		comerrno(EX_BAD, "MCN '%s' has illegal length.\n", mcn);

	for (p = mcn; *p; p++) {
		if (*p < '0' || *p > '9')
			comerrno(EX_BAD, "MCN '%s' contains illegal character '%c'.\n", mcn, *p);
	}
	p = malloc(14);
	strcpy (p, mcn);
	trackp->isrc = p;

	if (debug)
		printf("Track %d MCN: '%s'\n", trackp->trackno, trackp->isrc);
}

LOCAL	char	upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

LOCAL void
isrc_illchar(isrc, c)
	char	*isrc;
	int	c;
{
	errmsgno(EX_BAD, "ISRC '%s' contains illegal character '%c'.\n", isrc, c);
}

EXPORT void
setisrc(isrc, trackp)
	char	*isrc;
	track_t	*trackp;
{
	char	ibuf[13];
	char	*ip;
	char	*p;
	int	i;
	int	len;

	if ((len = strlen(isrc)) != 12) {
		for (p = isrc, i = 0; *p; p++) {
			if (*p == '-')
				i++;
		}
		if (((len - i) != 12) || i > 3)
			comerrno(EX_BAD, "ISRC '%s' has illegal length.\n", isrc);
	}

	/*
	 * The country code.
	 */
	for (p = isrc, ip = ibuf, i = 0; i < 2; p++, i++) {
		*ip++ = *p;
		if (!strchr(upper, *p)) {
/*			goto illchar;*/
			isrc_illchar(isrc, *p);
			if (*p >= '0' && *p <= '9')
				continue;
			exit(EX_BAD);
		}
	}
	if (*p == '-')
		p++;

	/*
	 * The XXX code.
	 */
	for (i = 0; i < 3; p++, i++) {
		*ip++ = *p;
		if (strchr(upper, *p))
			continue;
		if (*p >= '0' && *p <= '9')
			continue;
		goto illchar;
	}
	if (*p == '-')
		p++;

	/*
	 * The Year and the recording number.
	 */
	for (i = 0; i < 7; p++, i++) {
		*ip++ = *p;
		if (*p >= '0' && *p <= '9')
			continue;
		if (*p == '-' && i == 2) {
			ip--;
			i--;
			continue;
		}
		goto illchar;
	}
	*ip = '\0';
	p = malloc(13);
	strcpy (p, ibuf);
	trackp->isrc = p;

	if (debug)
		printf("Track %d ISRC: '%s'\n", trackp->trackno, trackp->isrc);
	return;
illchar:
	isrc_illchar(isrc, *p);
	exit(EX_BAD);
}

EXPORT void
setindex(tindex, trackp)
	char	*tindex;
	track_t	*trackp;
{
	char	*p;
	int	i;
	int	nindex;
	long	idx;
	long	*idxlist;

	idxlist = (long *)malloc(100*sizeof(long));
	p = tindex;
	idxlist[0] = 0;
	i = 0;
	while (*p) {
		p = astol(p, &idx);
		if (*p != '\0' && *p != ' ' && *p != '\t' && *p != ',')
			goto illchar;
		i++;
		if (i > 99)
			comerrno(EX_BAD, "Too many indices for track %d\n", trackp->trackno);
		idxlist[i] = idx;
		if (*p == ',')
			p++;
		while (*p == ' ' || *p == '\t')
			p++;
	}
	nindex = i;

	if (debug)
		printf("Track %d %d Index: '%s'\n", trackp->trackno, i, tindex);

	if (debug) for (i=0; i <= nindex; i++)
		printf("%d: %ld\n", i, idxlist[i]);

	trackp->nindex = nindex;
	trackp->tindex = idxlist;
	if (debug && nindex > 1)
		exit(1);
	return;
illchar:
	comerrno(EX_BAD, "Index '%s' contains illegal character '%c'.\n", tindex, *p);
}
