/* @(#)diskid.c	1.5 98/10/08 Copyright 1998 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)diskid.c	1.5 98/10/08 Copyright 1998 J. Schilling";
#endif
/*
 *	Disk Idientification Method
 *
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

#include <stdio.h>
#include <standard.h>
#include <sys/types.h>
#include <utypes.h>

#include "cdrecord.h"

EXPORT	void	pr_manufacturer		__PR((msf_t *mp));

struct disk_man {
	msf_t	mi_msf;
	char	*mi_name;
};

/*
 * Illegal (old) Manufacturer.
 */
LOCAL	char	m_ill[] = "Unknown old Manufacturer code";

/*
 * Permanent codes.
 */
LOCAL	char	m_leaddata[]	= "Lead Data Inc.";
LOCAL	char	m_fuji[]	= "FUJI Photo Film Co., Ltd.";
LOCAL	char	m_hitachi[]	= "Hitachi Maxell, Ltd.";
LOCAL	char	m_kodakjp[]	= "Kodak Japan Limited";
LOCAL	char	m_mitsui[]	= "Mitsui Chemicals, Inc.";
LOCAL	char	m_pioneer[]	= "Pioneer Video Corporation";
LOCAL	char	m_plasmon[]	= "Plasmon Data systems Ltd.";
LOCAL	char	m_princo[]	= "Princo Corporation";
LOCAL	char	m_ricoh[]	= "Ricoh Company Limited";
LOCAL	char	m_skc[]		= "SKC Co., Ltd.";
LOCAL	char	m_tyuden[]	= "Taiyo Yuden Company Limited";
LOCAL	char	m_tdk[]		= "TDK Corporation";
LOCAL	char	m_mitsubishi[]	= "Mitsubishi Chemical Corporation";
LOCAL	char	m_auvistar[]	= "Auvistar Industry Co.,Ltd.";
LOCAL	char	m_gigastore[]	= "GIGASTORAGE CORPORATION";
LOCAL	char	m_fornet[]	= "FORNET INTERNATIONAL PTE LTD.";
LOCAL	char	m_cmc[]		= "CMC Magnetics Corporation";
LOCAL	char	m_odm[]		= "Optical Disc Manufacturing Equipment";
LOCAL	char	m_ritek[]	= "Ritek Co.";

/*
 * Tentative codes.
 */
LOCAL	char	m_prodisc[]	= "Prodisc Technology Inc.";
LOCAL	char	m_postech[]	= "POSTECH Corporation";
LOCAL	char	m_ncolumbia[]	= "NIPPON COLUMBIA CO.,LTD.";
LOCAL	char	m_odc[]		= "OPTICAL DISC CORPRATION";
LOCAL	char	m_sony[]	= "SONY Corporation";
LOCAL	char	m_cis[]		= "CIS Technology Inc.";
LOCAL	char	m_csitaly[]	= "Computer Support Italy s.r.l.";
LOCAL	char	m_mmmm[]	= "Multi Media Masters & Machinary SA";

LOCAL	struct disk_man odman[] = {
	/*
	 * Illegal (old) codes.
	 */
	{{97, 25, 00}, m_ill },
	{{97, 25, 15}, m_ill },
	{{97, 27, 00}, "Old Ritek Co.???" },
	{{97, 27, 25}, m_ill },
	{{97, 30, 00}, m_ill },
	{{97, 33, 00}, m_ill },
	{{97, 35, 44}, m_ill },
	{{97, 39, 00}, m_ill },
	{{97, 45, 36}, "Old Kodak Photo CD" },
	{{97, 47, 00}, m_ill },
	{{97, 47, 30}, m_ill },
	{{97, 48, 14}, m_ill },
	{{97, 48, 33}, m_ill },
	{{97, 49, 00}, m_ill },
	{{97, 54, 00}, m_ill },
	{{97, 55, 06}, m_ill },
	{{97, 57, 00}, m_ill },
	/*
	 * List end marker
	 */
	{{00, 00, 00}, NULL },
};

LOCAL	struct disk_man dman[] = {
	/*
	 * Permanent codes.
	 */
	{{97, 26, 50}, m_leaddata },
	{{97, 48, 60}, m_leaddata },
	{{97, 26, 40}, m_fuji },
	{{97, 46, 40}, m_fuji },
	{{97, 25, 20}, m_hitachi },
	{{97, 47, 10}, m_hitachi },
	{{97, 27, 40}, m_kodakjp },
	{{97, 48, 10}, m_kodakjp },
	{{97, 27, 50}, m_mitsui },
	{{97, 48, 50}, m_mitsui },
	{{97, 27, 30}, m_pioneer },
	{{97, 48, 30}, m_pioneer },
	{{97, 27, 10}, m_plasmon },
	{{97, 48, 20}, m_plasmon },
	{{97, 27, 20}, m_princo },
	{{97, 47, 20}, m_princo },
	{{97, 27, 60}, m_ricoh },
	{{97, 48, 00}, m_ricoh },
	{{97, 26, 20}, m_skc },
	{{97, 24, 00}, m_tyuden },
	{{97, 46, 00}, m_tyuden },
	{{97, 32, 00}, m_tdk },
	{{97, 49, 00}, m_tdk },
	{{97, 34, 20}, m_mitsubishi },
	{{97, 50, 20}, m_mitsubishi },
	{{97, 28, 30}, m_auvistar },
	{{97, 46, 50}, m_auvistar },
	{{97, 28, 10}, m_gigastore },
	{{97, 49, 10}, m_gigastore },
	{{97, 26, 00}, m_fornet },
	{{97, 45, 00}, m_fornet },
	{{97, 26, 60}, m_cmc },
	{{97, 46, 60}, m_cmc },
	{{97, 21, 40}, m_odm },
	{{97, 31, 00}, m_ritek },
	{{97, 47, 50}, m_ritek },
	/*
	 * Tentative codes.
	 */
	{{97, 32, 10}, m_prodisc },
	{{97, 47, 60}, m_prodisc },
	{{97, 26, 10}, m_postech },
	{{97, 47, 40}, m_postech },
	{{97, 30, 10}, m_ncolumbia },
	{{97, 48, 40}, m_ncolumbia },
	{{97, 26, 30}, m_odc },
	{{97, 24, 10}, m_sony },
	{{97, 46, 10}, m_sony },
	{{97, 22, 40}, m_cis },
	{{97, 45, 40}, m_cis },
	{{97, 24, 20}, m_csitaly },
	{{97, 46, 30}, m_csitaly },
	{{97, 28, 20}, m_mmmm },
	{{97, 46, 20}, m_mmmm },
	/*
	 * List end marker
	 */
	{{00, 00, 00}, NULL },
};

EXPORT void
pr_manufacturer(mp)
	msf_t	*mp;
{
	struct disk_man * dp;
	int	frame;
	int	type;
	char	*tname;

	type = mp->msf_frame % 10;
	frame = mp->msf_frame - type;
	if (type < 5) {
		tname = "Cyanine, AZO or similar";
	} else {
		tname = "Phthalocyanine or similar";
	}

	dp = odman;
	while (dp->mi_msf.msf_min != 0) {
		if (mp->msf_min == dp->mi_msf.msf_min &&
				mp->msf_sec == dp->mi_msf.msf_sec &&
				mp->msf_frame == dp->mi_msf.msf_frame) {
			printf("Disk type unknown\n");
			printf("Manufacturer: %s\n", dp->mi_name);
			return;
		}
		dp++;
	}
	dp = dman;
	while (dp->mi_msf.msf_min != 0) {
		if (mp->msf_min == dp->mi_msf.msf_min &&
				mp->msf_sec == dp->mi_msf.msf_sec &&
				frame == dp->mi_msf.msf_frame) {
			printf("Disk type: %s\n", tname);
			printf("Manufacturer: %s\n", dp->mi_name);
			return;
		}
		dp++;
	}
	printf("Disk type unknown\n");
	printf("Manufacturer unknown (not in table)\n");
}

struct disk_rcap {
	msf_t	ci_msf;
	long	ci_cap;
	long	ci_rcap;
};

LOCAL	struct disk_rcap rcap[] = {

	{{97, 25, 00}, 359849, 374002 },	/* TDK 80 Minuten	     */
	{{97, 26, 60}, 337350, 349030 },	/* Koch grün CD-R74PRO	     */
	{{97, 26, 50}, 337050, 351205 },	/* Saba			     */
	{{97, 22, 40}, 336631, 349971 },	/* Targa grün CD-R74	     */
	{{97, 27, 31}, 336601, 351379 },	/* Pioneer blau CDM-W74S     */
	{{97, 26, 40}, 336225, 346210 },	/* Fuji Silver Disk	     */
	{{97, 31, 00}, 336225, 345460 },	/* Arita grün		     */
	{{97, 25, 28}, 336075, 352879 },	/* Maxell gold CD-R74G	     */
	{{97, 24, 00}, 336075, 346741 },	/* Philips grün CD-R74	     */
	{{97, 22, 41}, 335206, 349385 },	/* Octek grün		     */
	{{97, 34, 20}, 335100, 342460 },	/* Verbatim DataLifePlus     */
	{{97, 25, 21}, 335100, 346013 },	/* Maxell grün CD-R74XL	     */
	{{97, 27, 00}, 335100, 353448 },	/* TDK grün CD-RXG74	     */
	{{97, 27, 33}, 335100, 351336 },	/* Pioneer RDD-74A	     */
	{{97, 26, 60}, 334259, 349036 },	/* BASF grün		     */
	{{97, 28, 20}, 333976, 346485 },	/* Koch  grün  CD-R74 PRO    */

	{{97, 32, 00}, 333975, 345736 },	/* Imation 3M		     */
	{{97, 32, 00}, 333975, 348835 },	/* TDK Reflex X     CD-R74   */

	{{97, 30, 18}, 333899, 344857 },	/* HiSpace  grün	     */
	{{97, 27, 65}, 333750, 348343 },	/* Ricoh gold		     */
	{{97, 27, 00}, 333750, 336246 },	/* BestMedia grün   CD-R74   */
	{{97, 27, 28}, 333491, 347473 },	/* Fuji grün (alt)	     */
	{{97, 24, 48}, 333491, 343519 },	/* BASF (alt)		     */
	{{97, 27, 55}, 333235, 343270 },	/* Teac gold CD-R74	     */
	{{97, 27, 45}, 333226, 343358 },	/* Kodak gold		     */
	{{97, 28, 20}, 333226, 346483 },	/* SAST grün		     */
	{{97, 27, 45}, 333226, 343357 },	/* Mitsumi gold		     */
	{{97, 28, 25}, 333226, 346481 },	/* Cedar Grün		     */
	{{97, 23, 00}, 333226, 346206 },	/* Fuji grün (alt)	     */
	{{97, 33, 00}, 333225, 349623 },	/* DataFile Albrechts	     */
	{{97, 27, 19}, 332850, 348442 },	/* Plasmon gold PCD-R74	     */
	{{97, 32, 00}, 96600,  106502 },	/* TDK 80mm (for music only) */

	/*
	 * List end marker
	 */
	{{00, 00, 00}, 0L, 0L },
};

EXPORT long
disk_rcap(mp, maxblock)
	msf_t	*mp;
	long	maxblock;
{
	struct disk_rcap * dp;

	dp = rcap;
	while (dp->ci_msf.msf_min != 0) {
		if (mp->msf_min == dp->ci_msf.msf_min &&
				mp->msf_sec == dp->ci_msf.msf_sec &&
				mp->msf_frame == dp->ci_msf.msf_frame &&
				maxblock == dp->ci_cap)
			return (dp->ci_rcap);
		dp++;
	}
	return (0L);
}
