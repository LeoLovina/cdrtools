/* @(#)drv_philips.c	1.1 97/05/20 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)drv_philips.c	1.1 97/05/20 Copyright 1997 J. Schilling";
#endif
/*
 *	CDR device implementation for
 *	Philips/Yamaha/Ricoh/Plasmon
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

#include <stdio.h>
#include <standard.h>

#include <scsireg.h>
#include <scsitransp.h>

#include "cdrecord.h"

LOCAL	int	philips_load	__PR((void));
LOCAL	int	philips_unload	__PR((void));
LOCAL	long	next_wr_addr_philips __PR((int track));
LOCAL	int	open_track_philips __PR((int track, int secsize, int sectype, int tracktype));
LOCAL	int	open_track_yamaha __PR((int track, int secsize, int sectype, int tracktype));

LOCAL	int	philips_attach	__PR((void));
LOCAL	int	plasmon_attach	__PR((void));
LOCAL	int	ricoh_attach	__PR((void));

cdr_t	cdr_philips_cdd521 = {
	0,
	CDR_TAO,
	drive_identify,
	philips_attach,
	drive_getdisktype,
	philips_load,
	philips_unload,
	recovery_needed,
	recover,
	speed_select_philips,
	select_secsize,
	next_wr_addr_philips,
	reserve_track,
	scsi_cdr_write,
	open_track_philips,
	close_track_philips,
	(int(*)__PR((int)))cmd_dummy,
	cmd_dummy,
	fixation,
};

cdr_t	cdr_philips_cdd522 = {
	0,
	CDR_TAO|CDR_DAO,
	drive_identify,
	philips_attach,
	drive_getdisktype,
	philips_load,
	philips_unload,
	recovery_needed,
	recover,
	speed_select_philips,
	select_secsize,
	next_wr_addr_philips,
	reserve_track,
	scsi_cdr_write,
	open_track_philips,
	close_track_philips,
	(int(*)__PR((int)))cmd_dummy,
	cmd_dummy,
	fixation,
};

cdr_t	cdr_plasmon_rf4100 = {
	0,
	CDR_TAO,
	drive_identify,
	plasmon_attach,
	drive_getdisktype,
	philips_load,
	philips_unload,
	recovery_needed,
	recover,
	speed_select_philips,
	select_secsize,
	next_wr_addr_philips,
	reserve_track,
	scsi_cdr_write,
	open_track_philips,
	close_track_philips,
	(int(*)__PR((int)))cmd_dummy,
	cmd_dummy,
	fixation,
};

cdr_t	cdr_yamaha_cdr100 = {
	0, 0,
	drive_identify,
	philips_attach,
	drive_getdisktype,
	cmd_dummy,
	philips_unload,
	recovery_needed,
	recover,
	speed_select_yamaha,
	select_secsize,
	next_wr_addr_philips,
	reserve_track,
	scsi_cdr_write,
	open_track_yamaha,
	close_track_philips,
	(int(*)__PR((int)))cmd_dummy,
	cmd_dummy,
	fixation,
};

cdr_t	cdr_ricoh_ro1420 = {
	0, 0,
	drive_identify,
	ricoh_attach,
	drive_getdisktype,
	scsi_load,
	scsi_unload,
	recovery_needed,
	recover,
	speed_select_yamaha,
	select_secsize,
	next_wr_addr_philips,
	reserve_track,
	scsi_cdr_write,
	open_track_philips,
	close_track_philips,
	(int(*)__PR((int)))cmd_dummy,
	cmd_dummy,
	fixation,
};

LOCAL int
philips_load()
{
	return (load_unload_philips(1));
}

LOCAL int
philips_unload()
{
	return (load_unload_philips(0));
}

LOCAL long
next_wr_addr_philips(track)
	int	track;
{
	long	fa;

/*	fa = first_writable_addr(0, 0, 0, 1);*/
	fa = first_writable_addr(0, 0, 0, 0);
	return (fa);
}

LOCAL int
open_track_philips(track, secsize, sectype, tracktype)
	int	track;
	int	secsize;
	int	sectype;
	int	tracktype;
{
	if (select_secsize(secsize) < 0)
		return (-1);

	if (write_track_info(sectype) < 0)
		return (-1);

	if (write_track(0, sectype) < 0)
		return (-1);

	return (0);
}

LOCAL int
open_track_yamaha(track, secsize, sectype, tracktype)
	int	track;
	int	secsize;
	int	sectype;
	int	tracktype;
{
	if (select_secsize(secsize) < 0)
		return (-1);

	if (write_track(0, sectype) < 0)
		return (-1);

	return (0);
}

static const char *sd_cdd_521_error_str[] = {
	"\003\000tray out",				/* 0x03 */
	"\062\000write data error with CU",		/* 0x32 */	/* Yamaha */
	"\063\000monitor atip error",			/* 0x33 */
	"\064\000absorbtion control error",		/* 0x34 */
#ifdef	YAMAHA_CDR_100
	/* Is this the same ??? */
	"\120\000write operation in progress",		/* 0x50 */
#endif
	"\127\000unable to read TOC/PMA/Subcode/ATIP",	/* 0x57 */
	"\132\000operator medium removal request",	/* 0x5a */
	"\145\000verify failed",			/* 0x65 */
	"\201\000illegal track number",			/* 0x81 */
	"\202\000command now not valid",		/* 0x82 */
	"\203\000medium removal is prevented",		/* 0x83 */
	"\204\000tray out",				/* 0x84 */
	"\205\000track at one not in PMA",		/* 0x85 */
	"\240\000stopped on non data block",		/* 0xa0 */
	"\241\000invalid start adress",			/* 0xa1 */
	"\242\000attampt to cross track-boundary",	/* 0xa2 */
	"\243\000illegal medium",			/* 0xa3 */
	"\244\000disk write protected",			/* 0xa4 */
	"\245\000application code conflict",		/* 0xa5 */
	"\246\000illegal blocksize for command",	/* 0xa6 */
	"\247\000blocksize conflict",			/* 0xa7 */
	"\250\000illegal transfer length",		/* 0xa8 */
	"\251\000request for fixation failed",		/* 0xa9 */
	"\252\000end of medium reached",		/* 0xaa */
#ifdef	REAL_CDD_521
	"\253\000non reserved reserved track",		/* 0xab */
#else
	"\253\000illegal track number",			/* 0xab */
#endif
	"\254\000data track length error",		/* 0xac */
	"\255\000buffer under run",			/* 0xad */
	"\256\000illegal track mode",			/* 0xae */
	"\257\000optical power calibration error",	/* 0xaf */
	"\260\000calibration area almost full",		/* 0xb0 */
	"\261\000current program area empty",		/* 0xb1 */
	"\262\000no efm at search address",		/* 0xb2 */
	"\263\000link area encountered",		/* 0xb3 */
	"\264\000calibration area full",		/* 0xb4 */
	"\265\000dummy data blocks added",		/* 0xb5 */
	"\266\000block size format conflict",		/* 0xb6 */
	"\267\000current command aborted",		/* 0xb7 */
	"\270\000program area not empty",		/* 0xb8 */
#ifdef	YAMAHA_CDR_100
	/* Used while writing lead in in DAO */
	"\270\000write leadin in progress",		/* 0xb8 */
#endif
	"\271\000parameter list too large",		/* 0xb9 */
	"\277\000buffer overflow",			/* 0xbf */	/* Yamaha */
	"\300\000no barcode available",			/* 0xc0 */
	"\301\000barcode reading error",		/* 0xc1 */
	"\320\000recovery needed",			/* 0xd0 */
	"\321\000cannot recover track",			/* 0xd1 */
	"\322\000cannot recover pma",			/* 0xd2 */
	"\323\000cannot recover leadin",		/* 0xd3 */
	"\324\000cannot recover leadout",		/* 0xd4 */
	"\325\000cannot recover opc",			/* 0xd5 */
	"\326\000eeprom failure",			/* 0xd6 */
	"\340\000laser current over",			/* 0xe0 */	/* Yamaha */
	"\341\000servo adjustment over",		/* 0xe0 */	/* Yamaha */
	NULL
};

static const char *sd_ro1420_error_str[] = {
	"\004\000logical unit is in process of becoming ready", /* 04 00 */
	"\011\200radial skating error",				/* 09 80 */
	"\011\201sledge servo failure",				/* 09 81 */
	"\011\202pll no lock",					/* 09 82 */
	"\011\203servo off track",				/* 09 83 */
	"\011\204atip sync error",				/* 09 84 */
	"\011\205atip/subcode jumped error",			/* 09 85 */
	"\127\300subcode not found",				/* 57 C0 */
	"\127\301atip not found",				/* 57 C1 */
	"\127\302no atip or subcode",				/* 57 C2 */
	"\127\303pma error",					/* 57 C3 */
	"\127\304toc read error",				/* 57 C4 */
	"\127\305disk informatoion error",			/* 57 C5 */
	"\144\200read in leadin",				/* 64 80 */
	"\144\201read in leadout",				/* 64 81 */
	"\201\000illegal track",				/* 81 00 */
	"\202\000command not now valid",			/* 82 00 */
	"\220\000reserve track check error",			/* 90 00 */
	"\220\001verify blank error",				/* 90 01 */
	"\221\001mode of last track error",			/* 91 01 */
	"\222\000header search error",				/* 92 00 */
	"\230\001header monitor error",				/* 98 01 */
	"\230\002edc error",					/* 98 02 */
	"\230\003read link, run-in run-out",			/* 98 03 */
	"\230\004last one block error",				/* 98 04 */
	"\230\005illegal blocksize",				/* 98 05 */
	"\230\006not all data transferred",			/* 98 06 */
	"\230\007cdbd over run error",				/* 98 07 */
	"\240\000stopped on non_data block",			/* A0 00 */
	"\241\000invalid start address",			/* A1 00 */
	"\243\000illegal medium",				/* A3 00 */
	"\246\000illegal blocksize for command",		/* A6 00 */
	"\251\000request for fixation failed",			/* A9 00 */
	"\252\000end of medium reached",			/* AA 00 */
	"\253\000illegal track number",				/* AB 00 */
	"\255\000buffer underrun",				/* AD 00 */
	"\256\000illegal track mode",				/* AE 00 */
	"\257\200power range error",				/* AF 80 */
	"\257\201moderation error",				/* AF 81 */
	"\257\202beta upper range error",			/* AF 82 */
	"\257\203beta lower range error",			/* AF 83 */
	"\257\204alpha upper range error",			/* AF 84 */
	"\257\205alpha lower range error",			/* AF 85 */
	"\257\206alpha and power range error",			/* AF 86 */
	"\260\000calibration area almost full",			/* B0 00 */
	"\261\000current program area empty",			/* B1 00 */
	"\262\000no efm at search address",			/* B2 00 */
	"\264\000calibration area full",			/* B4 00 */
	"\265\000dummy blocks added",				/* B5 00 */
	"\272\000write audio on reserved track",		/* BA 00 */
	"\302\200syscon rom error",				/* C2 80 */
	"\302\201syscon ram error",				/* C2 81 */
	"\302\220efm encoder error",				/* C2 90 */
	"\302\221efm decoder error",				/* C2 91 */
	"\302\222servo ic error",				/* C2 92 */
	"\302\223motor controller error",			/* C2 93 */
	"\302\224dac error",					/* C2 94 */
	"\302\225syscon eeprom error",				/* C2 95 */
	"\302\240block decoder communication error",		/* C2 A0 */
	"\302\241block encoder communication error",		/* C2 A1 */
	"\302\242block encoder/decoder path error",		/* C2 A2 */
	"\303\000CD-R engine selftest error",			/* C3 xx */
	"\304\000buffer parity error",				/* C4 00 */
	"\305\000data transfer error",				/* C5 00 */
	"\340\00012V failure",					/* E0 00 */
	"\341\000undefined syscon error",			/* E1 00 */
	"\341\001syscon communication error",			/* E1 01 */
	"\341\002unknown syscon error",				/* E1 02 */
	"\342\000syscon not ready",				/* E2 00 */
	"\343\000command rejected",				/* E3 00 */
	"\344\000command not accepted",				/* E4 00 */
	"\345\000verify error at beginning of track",		/* E5 00 */
	"\345\001verify error at ending of track",		/* E5 01 */
	"\345\002verify error at beginning of lead-in",		/* E5 02 */
	"\345\003verify error at ending of lead-in",		/* E5 03 */
	"\345\004verify error at beginning of lead-out",	/* E5 04 */
	"\345\005verify error at ending of lead-out",		/* E5 05 */
	"\377\000command phase timeout error",			/* FF 00 */
	"\377\001data in phase timeout error",			/* FF 01 */
	"\377\002data out phase timeout error",			/* FF 02 */
	"\377\003status phase timeout error",			/* FF 03 */
	"\377\004message in phase timeout error",		/* FF 04 */
	"\377\005message out phase timeout error",		/* FF 05 */
	NULL
};

LOCAL int
philips_attach()
{
	scsi_setnonstderrs(sd_cdd_521_error_str);
	return (0);
}

LOCAL int
plasmon_attach()
{
	extern struct scsi_inquiry	inq;

	inq.ansi_version = 1;	/* Currect the ly */

	scsi_setnonstderrs(sd_cdd_521_error_str);
	return (0);
}

LOCAL int
ricoh_attach()
{
	scsi_setnonstderrs(sd_ro1420_error_str);
	return (0);
}