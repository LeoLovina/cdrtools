/* @(#)diskid.c	1.2 98/04/15 Copyright 1998 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)diskid.c	1.2 98/04/15 Copyright 1998 J. Schilling";
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

/*
 * Tentative codes.
 */
LOCAL	char	m_postech[]	= "POSTECH Corporation";
LOCAL	char	m_ncolumbia[]	= "NIPPON COLUMBIA CO.,LTD.";
LOCAL	char	m_auvistar[]	= "Auvistar Industry Co.,Ltd.";
LOCAL	char	m_gigastore[]	= "GIGASTORAGE CORPORATION";
LOCAL	char	m_odc[]		= "OPTICAL DISC CORPRATION";
LOCAL	char	m_fornet[]	= "FORNET INTERNATIONAL PTE LTD.";
LOCAL	char	m_sony[]	= "SONY Corporation";
LOCAL	char	m_cis[]		= "CIS Technology Inc.";
LOCAL	char	m_cmc[]		= "CMC Magnetics Corporation";
LOCAL	char	m_csitaly[]	= "Computer Support Italy s.r.l.";
LOCAL	char	m_mmmm[]	= "Multi Media Masters & Machinary SA";
LOCAL	char	m_odm[]		= "Optical Disc Manufacturing Equipment";
LOCAL	char	m_ritek[]	= "Ritek Co.";

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
	/*
	 * Tentative codes.
	 */
	{{97, 26, 10}, m_postech },
	{{97, 47, 40}, m_postech },
	{{97, 30, 10}, m_ncolumbia },
	{{97, 48, 40}, m_ncolumbia },
	{{97, 28, 30}, m_auvistar },
	{{97, 46, 50}, m_auvistar },
	{{97, 28, 10}, m_gigastore },
	{{97, 49, 10}, m_gigastore },
	{{97, 26, 30}, m_odc },
	{{97, 26, 00}, m_fornet },
	{{97, 45, 00}, m_fornet },
	{{97, 26, 40}, m_fuji },
	{{97, 24, 10}, m_sony },
	{{97, 46, 10}, m_sony },
	{{97, 22, 40}, m_cis },
	{{97, 45, 40}, m_cis },
	{{97, 26, 60}, m_cmc },
	{{97, 46, 60}, m_cmc },
	{{97, 24, 20}, m_csitaly },
	{{97, 46, 30}, m_csitaly },
	{{97, 28, 20}, m_mmmm },
	{{97, 46, 20}, m_mmmm },
	{{97, 21, 40}, m_odm },
	{{97, 31, 00}, m_ritek },
	{{97, 47, 50}, m_ritek },
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
