/* @(#)scsierrs.c	2.11 96/12/18 Copyright 1987-1996 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)scsierrs.c	2.11 96/12/18 Copyright 1987-1996 J. Schilling";
#endif
/*
 *	Error printing for scsitransp.c
 *
 *	Copyright (c) 1987-1996 J. Schilling
 */
/* This program is free software; you can redistribute it and/or modify
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

#include "scsireg.h"
#include "scsidefs.h"
#include "scgio.h"	/*XXX JS wird eigentlich nicht benoetigt!!	*/
			/*XXX JS kommt weg, wenn struct sense und status */
			/*XXX JS von scgio.h nach scsireg.h kommen	*/
#include "scsitransp.h"

#define	CTYPE_CCS	0
#define	CTYPE_MD21	1
#define	CTYPE_ACB4000	2
#define	CTYPE_SMO_C501	3

#define	SMO_C501
#define	CDD_521

EXPORT	char	*scsisensemsg	__PR((int, int, int));
EXPORT	void	scsierrmsg	__PR((struct scsi_sense *,
						struct scsi_status *, int));
static u_char sd_adaptec_keys[] = {
	0, 4, 4, 4,  2, 2, 4, 4,		/* 0x00-0x07 */
	4, 4, 4, 4,  4, 4, 4, 4,		/* 0x08-0x0f */
	4, 3, 3, 3,  3, 4, 3, 1,		/* 0x10-0x17 */
	1, 1, 3, 4,  3, 4, 3, 3,		/* 0x18-0x1f */
	5, 5, 5, 5,  5, 5, 5, 7,		/* 0x20-0x27 */
	6, 6, 6, 5,  4,11,11,11			/* 0x28-0x2f */
};
#define	MAX_ADAPTEC_KEYS (sizeof (sd_adaptec_keys))

static char *sd_adaptec_error_str[] = {
	"\031ECC error during verify",		/* 0x19 */
	"\032interleave error",			/* 0x1a */
	"\034bad format on drive",		/* 0x1c */
	"\035self test failed",			/* 0x1d */
	"\036defective track",			/* 0x1e */
	"\043volume overflow",			/* 0x23 */
	"\053set limit violation",		/* 0x2b */
	"\054error counter overflow",		/* 0x2c */
	"\055initiator detected error",		/* 0x2d */
	"\056scsi parity error",		/* 0x2e */
	"\057adapter parity error",		/* 0x2f */
	NULL
};

static char *sd_ccs_error_str[] = {
	"\000no sense",				/* 0x00 */
	"\001no index",				/* 0x01 */
	"\002no seek complete",			/* 0x02 */
	"\003write fault",			/* 0x03 */
	"\004drive not ready",			/* 0x04 */
	"\005drive not selected",		/* 0x05 */
	"\006no track 00",			/* 0x06 */
	"\007multiple drives selected",		/* 0x07 */
	"\010lun communication failure",	/* 0x08 */
	"\011servo error",			/* 0x09 */
	"\020ID CRC error",			/* 0x10 */
	"\021hard data error",			/* 0x11 */
	"\022no I.D. address mark",		/* 0x12 */
	"\023no data address mark",		/* 0x13 */
	"\024record not found",			/* 0x14 */
	"\025seek error",			/* 0x15 */
	"\026data sync mark error",		/* 0x16 */
	"\027soft data error (retries)",	/* 0x17 */
	"\030recoverable read error (ECC)",	/* 0x18 */
	"\031defect list error",		/* 0x19 */
	"\032parameter overrun",		/* 0x1a */
	"\033synchronous xfer error",		/* 0x1b */
	"\034no primary defect list",		/* 0x1c */
	"\035compare error",			/* 0x1d */
	"\036recoverable ID error",		/* 0x1e */
	"\040invalid command",			/* 0x20 */
	"\041illegal block address",		/* 0x21 */
	"\042illegal function for device type",	/* 0x22 */
	"\044illegal field in CDB",		/* 0x24 */
	"\045invalid lun",			/* 0x25 */
	"\046invalid param list",		/* 0x26 */
	"\047write protected",			/* 0x27 */
	"\050media changed",			/* 0x28 */
	"\051power on / reset",			/* 0x29 */
	"\052mode select param changed",	/* 0x2a */
	"\053host cannot disconnect",		/* 0x2b */
	"\054command sequence error",		/* 0x2c */
	"\060incompatible cartridge",		/* 0x30 */
	"\061medium format corrupted",		/* 0x31 */
	"\062No defect spare loc available",	/* 0x32 */
	"\100diagnostic/RAM failure",		/* 0x40 */
	"\101data path diag failure",		/* 0x41 */
	"\102power on failure",			/* 0x42 */
	"\103message reject error",		/* 0x43 */
	"\104internal controller error",	/* 0x44 */
	"\105select / reselect failure",	/* 0x45 */
	"\106unsuccessfull soft reset",		/* 0x46 */
	"\107scsi parity error",		/* 0x47 */
	"\110initiator detected error",		/* 0x48 */
	"\111inappropriate/illegal message",	/* 0x49 */
	"\113data error",			/* 0x4b */
	"\140second party status error",	/* 0x60 */
	NULL
};

#ifdef SMO_C501
static char *sd_smo_c501_error_str[] = {
	"\012disk not inserted",		/* 0x0a */
	"\013load/unload failure",		/* 0x0b */
	"\014spindle failure",			/* 0x0c */
	"\015focus failure",			/* 0x0d */
	"\016tracking failure",			/* 0x0e */
	"\017bias magnet failure",		/* 0x0f */
	"\043illegal function for medium type",	/* 0x23 */
/*XXX*/	"\070recoverable write error",		/* 0x38 */
/*XXX*/	"\071write error recovery failed",	/* 0x39 */
	"\072defect list update failed",	/* 0x3a */
	"\075defect list not available",	/* 0x3d */
	"\200limited laser life",		/* 0x80 */
	"\201laser focus coil over-current",	/* 0x81 */
	"\202laser tracking coil over-current",	/* 0x82 */
	"\203temperature alarm",		/* 0x83 */
	NULL
};

#ifdef	OOO
static char *Osd_smo_c501_error_str[] = {
	"\001no index",				/* 0x01 */
	"\002seek incomplete",			/* 0x02 */
	"\003write fault",			/* 0x03 */
	"\004drive not ready",			/* 0x04 */
	"\005drive not selected",		/* 0x05 */
	"\006no track 0",			/* 0x06 */
	"\007multiple drives selected",		/* 0x07 */
	"\010lun failure",			/* 0x08 */
	"\011servo error",			/* 0x09 */
	"\012disk not inserted",		/* 0x0a */
	"\013load/unload failure",		/* 0x0b */
	"\014spindle failure",			/* 0x0c */
	"\015focus failure",			/* 0x0d */
	"\016tracking failure",			/* 0x0e */
	"\017bias magnet failure",		/* 0x0f */
	"\020ID CRC error",			/* 0x10 */
	"\021unrecoverable read error",		/* 0x11 */
	"\025seek error",			/* 0x15 */
	"\030recoverable read error",		/* 0x18 */
	"\040invalid command",			/* 0x20 */
	"\041invalid logical block address",	/* 0x21 */
	"\043illegal function for medium",	/* 0x23 */
	"\044invalid cdb",			/* 0x24 */
	"\045invalid lun",			/* 0x25 */
	"\046invalid param list",		/* 0x26 */
	"\047write protected",			/* 0x27 */
	"\050media changed",			/* 0x28 */
	"\051reset occured",			/* 0x29 */
	"\052mode select changed",		/* 0x2a */
	"\060incompatible cartridge",		/* 0x30 */
	"\061medium format corrupted",		/* 0x31 */
	"\062No defect spare loc available",	/* 0x32 */
	"\070recoverable write error",		/* 0x38 */
	"\071write error recovery failed",	/* 0x39 */
	"\072defect list update failed",	/* 0x3a */
	"\075defect list not available",	/* 0x3d */
	"\102power on diagnostic failed",	/* 0x42 */
	"\103message reject error",		/* 0x43 */
	"\104internal controller error",	/* 0x44 */
	"\107scsi parity error",		/* 0x47 */
	"\110initiator detected error",		/* 0x48 */
	"\111inappropriate/illegal message",	/* 0x49 */
	"\200limited laser life",		/* 0x80 */
	"\201laser focus coil over-current",	/* 0x81 */
	"\202laser tracking coil failure",	/* 0x82 */
	"\203temperature alarm",		/* 0x83 */
	NULL
};
#endif /* OOO */
#endif

#ifdef CDD_521
static char *sd_cdd_521_error_str[] = {
	"\003tray out",				/* 0x03 */
	"\062write data error with CU",		/* 0x32 */	/* Yamaha */
	"\063monitor atip error",		/* 0x33 */
	"\064absorbtion control error",		/* 0x34 */
	"\072medium not present",		/* 0x3a */
	"\075invalid bits in identify message",	/* 0x3d */
	"\120write append error",		/* 0x50 */
#ifdef	YAMAHA_CDR_100
	/* Is this the same ??? */
	"\120write operation in progress",	/* 0x50 */
#endif
	"\123medium load or eject failure",	/* 0x53 */
	"\127unable to read TOC/PMA/Subcode/ATIP",	/* 0x57 */
	"\132operator medium removal request",	/* 0x5a */
	"\143end of user area encountered on this track",/* 0x63 */
	"\144illegal mode for this track",	/* 0x64 */
	"\145verify failed",			/* 0x65 */
	"\201illegal track number",		/* 0x81 */
	"\202command now not valid",		/* 0x82 */
	"\203medium removal is prevented",	/* 0x83 */
	"\204tray out",				/* 0x84 */
	"\205track at one not in PMA",		/* 0x85 */
	"\240stopped on non data block",	/* 0xa0 */
	"\241invalid start adress",		/* 0xa1 */
	"\242attampt to cross track-boundary",	/* 0xa2 */
	"\243illegal medium",			/* 0xa3 */
	"\244disk write protected",		/* 0xa4 */
	"\245application code conflict",	/* 0xa5 */
	"\246illegal blocksize for command",	/* 0xa6 */
	"\247blocksize conflict",		/* 0xa7 */
	"\250illegal transfer length",		/* 0xa8 */
	"\251request for fixation failed",	/* 0xa9 */
	"\252end of medium reached",		/* 0xaa */
#ifdef	REAL_CDD_521
	"\253non reserved reserved track",	/* 0xab */
#else
	"\253illegal track number",		/* 0xab */
#endif
	"\254data track length error",		/* 0xac */
	"\255buffer under run",			/* 0xad */
	"\256illegal track mode",		/* 0xae */
	"\257optical power calibration error",	/* 0xaf */
	"\260calibration area almost full",	/* 0xb0 */
	"\261current program area empty",	/* 0xb1 */
	"\262no efm at search address",		/* 0xb2 */
	"\263link area encountered",		/* 0xb3 */
	"\264calibration area full",		/* 0xb4 */
	"\265dummy data blocks added",		/* 0xb5 */
	"\266block size format conflict",	/* 0xb6 */
	"\267current command aborted",		/* 0xb7 */
	"\270program area not empty",		/* 0xb8 */
#ifdef	YAMAHA_CDR_100
	/* Used while writing lead in in DAO */
	"\270write leadin in progress",		/* 0xb8 */
#endif
	"\271parameter list too large",		/* 0xb9 */
	"\277buffer overflow",			/* 0xbf */	/* Yamaha */
	"\300no barcode available",		/* 0xc0 */
	"\301barcode reading error",		/* 0xc1 */
	"\320recovery needed",			/* 0xd0 */
	"\321cannot recover track",		/* 0xd1 */
	"\322cannot recover pma",		/* 0xd2 */
	"\323cannot recover leadin",		/* 0xd3 */
	"\324cannot recover leadout",		/* 0xd4 */
	"\325cannot recover opc",		/* 0xd5 */
	"\326eeprom failure",			/* 0xd6 */
	"\340laser current over",		/* 0xe0 */	/* Yamaha */
	"\341servo adjustment over",		/* 0xe0 */	/* Yamaha */
	NULL
};
#endif

static char *sd_sense_keys[] = {
	"No Additional Sense",		/* 0x00 */
	"Recovered Error",		/* 0x01 */
	"Not Ready",			/* 0x02 */
	"Medium Error",			/* 0x03 */
	"Hardware Error",		/* 0x04 */
	"Illegal Request",		/* 0x05 */
	"Unit Attention",		/* 0x06 */
	"Data Protect",			/* 0x07 */
	"Blank Check",			/* 0x08 */
	"Vendor Unique",		/* 0x09 */
	"Copy Aborted",			/* 0x0a */
	"Aborted Command",		/* 0x0b */
	"Equal",			/* 0x0c */
	"Volume Overflow",		/* 0x0d */
	"Miscompare",			/* 0x0e */
	"Reserved"			/* 0x0f */
};

static char *sd_cmds[] = {
	"\000test unit ready",		/* 0x00 */
	"\001rezero",			/* 0x01 */
	"\003request sense",		/* 0x03 */
	"\004format",			/* 0x04 */
	"\007reassign",			/* 0x07 */
	"\010read",			/* 0x08 */
	"\012write",			/* 0x0a */
	"\013seek",			/* 0x0b */
	"\022inquiry",			/* 0x12 */
	"\025mode select",		/* 0x15 */
	"\026reserve",			/* 0x16 */
	"\027release",			/* 0x17 */
	"\030copy",			/* 0x18 */
	"\032mode sense",		/* 0x1a */
	"\033start/stop",		/* 0x1b */
	"\036door lock",		/* 0x1e */
	"\067read defect data",		/* 0x37 */
	NULL
};

EXPORT
char	*scsisensemsg(ctype, class, code)
	register int	ctype;
	register int	class;
	register int	code;
{
	register int i;
	register char **vec  = (char **)NULL;

	code |= class<<4;
	switch (ctype) {
	case DEV_MD21:
		vec = sd_ccs_error_str;
		break;

	case DEV_ACB40X0:
	case DEV_ACB4000:
	case DEV_ACB4010:
	case DEV_ACB4070:
	case DEV_ACB5500:
		vec = sd_adaptec_error_str;
		break;

#ifdef	SMO_C501
	case DEV_SONY_SMO:
		vec = sd_smo_c501_error_str;
		break;
#endif

#ifdef	CDD_521
	case DEV_CDD_521:
	case DEV_YAMAHA_CDR_100:
		vec = sd_cdd_521_error_str;
		break;
#endif
	default:
		vec = sd_ccs_error_str;
	}
	if (vec == (char **)NULL)
		return ("");

	for (i = 0; i < 2; i++) {
		while (*vec != (char *) NULL) {
			if (code == (u_char)*vec[0]) {
				return (char *)((int)(*vec)+1);
			} else
				vec++;
		}
		if (*vec == (char *) NULL)
			vec = sd_ccs_error_str;
	}
	return ("invalid sense code");
}

#undef	sense	/*XXX JS Hack, solange scgio.h noch nicht fertig ist */
EXPORT void
scsierrmsg(sense, status, error_code)
	register struct scsi_sense *sense;
	register struct scsi_status *status;
	int error_code;
{
	char *sensemsg, *cmdname, *sensekey;
#define	ext_sense	((struct scsi_ext_sense* ) sense)
	register int blkno = 0;
	register int class;
	register int code;
	int key = 0;
	int blkvalid = 0;
	int fm = 0;
	int eom = 0;
	extern int dev;

	sensekey = sensemsg = "[]";
	if (sense->code >= 0x70) {
		if (error_code >= 0)
			class = error_code;
		else
			class = ext_sense->error_code;
		code = class & 0x0F;
		class >>= 4;
	} else {
		class = sense->code >> 4;
		code = sense->code & 0x0F;
	}
	if (status->chk == 0) {
		sensemsg = "no sense";
	} else {
		if (sense->code >= 0x70) {
			key = ext_sense->key;
			if (key < 0x10)
				sensekey = sd_sense_keys[ext_sense->key];
			else
				sensekey = "invalid sensekey";
			blkno = (ext_sense->info_1 << 24) |
				(ext_sense->info_2 << 16) |
				(ext_sense->info_3 << 8) |
				ext_sense->info_4;
			fm = ext_sense->fil_mk;
			eom = ext_sense->eom;
		} else {
			key = -1;
			sensekey = "[]";
			blkno = (sense->high_addr << 16) |
				(sense->mid_addr << 8) |
				sense->low_addr;
			fm = eom = 0;
		}
		blkvalid = sense->adr_val;
		sensemsg = scsisensemsg(dev, class, code);
	}
/*
	if (un->un_cmd < sizeof(scsi_cmds)) {
		cmdname = scsi_cmds[un->un_cmd];
	} else {
		cmdname = "unknown cmd";
	}
*/
	cmdname = "";
	printf("%sSense Key: 0x%x %s, Code 0x%x%x %s%s%s blk %d %s%s%s\n",
		cmdname, key, sensekey,
		class, code, *sensemsg?"(":"", sensemsg, *sensemsg?")":"",
		blkno, blkvalid?"valid ":"",
		fm?"file mark detected ":"", eom?"end of media":"");
}

#ifdef	DEBUG
print_err(code, ctype)
{
	register int i;
	register char **vec  = (char **)NULL;

	switch (ctype) {
	case CTYPE_MD21:
	case CTYPE_CCS:
		vec = sd_ccs_error_str;
		break;

	case CTYPE_ACB4000:
		vec = sd_adaptec_error_str;
		break;

#ifdef	SMO_C501
	case CTYPE_SMO_C501:
		vec = sd_smo_c501_error_str;
		break;
#endif

#ifdef	CDD_521
	case DEV_CDD_521:
		vec = sd_cdd_521_error_str;
		break;
#endif
	}
	printf("error code: 0x%x", code);
	if (vec == (char **)NULL)
		return;

	for (i = 0; i < 2; i++) {
		while (*vec != (char *) NULL) {
			if (code == (u_char)*vec[0]) {
				printf(" (%s)", (char *)((int)(*vec)+1));
				return;
			} else
				vec++;
		}
		if (*vec == (char *) NULL)
			vec = sd_ccs_error_str;
	}
}


int	dev;

main()
{
	int	i;

#ifdef ACB
	for (i = 0; i < 0x30; i++) {
/*		printf("Code: 0x%x	Key: 0x%x	", i, sd_adaptec_keys[i]);*/
		printf("Key: 0x%x %-16s ", sd_adaptec_keys[i],
					sd_sense_keys[sd_adaptec_keys[i]] );
		print_err(i, CTYPE_ACB4000);
		printf("\n");
	}
#else
/*	for (i = 0; i < 0x84; i++) {*/
	for (i = 0; i < 0xd8; i++) {
/*		print_err(i, CTYPE_SMO_C501);*/
		print_err(i, DEV_CDD_521);
		printf("\n");
	}
#endif
}
#endif
