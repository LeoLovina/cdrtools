/* @(#)scsidefs.h	1.12 97/03/02 Copyright 1988 J. Schilling */
/*
 *	Definitions for SCSI devices i.e. for error strings in scsierrs.c
 *
 *	Copyright (c) 1988 J. Schilling
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

#define	DEV_UNKNOWN	0
#define	DEV_ACB40X0	1
#define	DEV_ACB4000	2
#define	DEV_ACB4010	3
#define	DEV_ACB4070	4
#define	DEV_ACB5500	5
#define	DEV_ACB4520A	6
#define	DEV_ACB4525	7
#define	DEV_MD21	8
#define	DEV_MD23	9
#define	DEV_NON_CCS_DSK	10
#define	DEV_CCS_GENDISK	11

#define	DEV_MT02	100
#define	DEV_SC4000	101

#define	DEV_RXT800S		200
#define	DEV_CDD_521		210
#define	DEV_CDD_522		211
#define	DEV_YAMAHA_CDR_100	220
#define	DEV_PLASMON_RF_4100	230

#define DEV_SONY_SMO	400

#define	DEV_HRSCAN	600
#define	DEV_MS300A	601

#define	old_acb(d)	(((d) == DEV_ACB40X0) || \
			 ((d) == DEV_ACB4000) || ((d) == DEV_ACB4010) || \
			 ((d) == DEV_ACB4070) || ((d) == DEV_ACB5500))

#define	ccs(d)		(!old_acb(d))

extern	int	dev;
