/* @(#)drv_jvc.c	1.6 97/09/10 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)drv_jvc.c	1.6 97/09/10 Copyright 1997 J. Schilling";
#endif
/*
 *	CDR device implementation for
 *	JVC/TEAC
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
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>

#include <btorder.h>
#include <scgio.h>
#include <scsidefs.h>
#include <scsireg.h>

#include "cdrecord.h"
#include "scsitransp.h"

struct	scg_cmd		scmd;
struct	scsi_inquiry	inq;

/* just a hack */
long	lba_blocks;
long	lba_addr;

/*
 * macros for building MSF values from LBA
 */
#define LBA_MIN(x)	((x)/(60*75))
#define LBA_SEC(x)	(((x)%(60*75))/75)
#define LBA_FRM(x)	((x)%75)
#define MSF_CONV(a)	((((a)%100)/10)*16 + ((a)%10))

extern	int	scsibus;
extern	int	target;
extern	int	lun;

extern	int	silent;
extern	int	verbose;

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */
struct teac_mode_page_21 {		/* teac dummy selection */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x01 = 1 Byte */
	u_char	dummy		: 2;
	u_char	res		: 6;
};
#else
struct teac_mode_page_21 {		/* teac dummy selection */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x01 = 1 Byte */
	u_char	res		: 6;
	u_char	dummy		: 2;
};
#endif

struct teac_mode_page_31 {		/* teac speed selection */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x02 = 2 Byte */
	u_char	speed;
	u_char	res;
};

struct cdd_52x_mode_data {
	struct scsi_mode_header	header;
	union cdd_pagex	{
		struct teac_mode_page_21	teac_page21;
		struct teac_mode_page_31	teac_page31;
	} pagex;
};

LOCAL	int	teac_attach	__PR((void));
LOCAL	int	teac_getdisktype __PR((cdr_t *dp, dstat_t *dsp));
LOCAL	int	speed_select_teac __PR((int speed, int dummy));
LOCAL	int	next_wr_addr_jvc __PR((int track, long *ap));
LOCAL	int	write_teac_xg1	__PR((caddr_t, long, long, int, BOOL));
LOCAL	int	cdr_write_teac	__PR((char *bufp, long sectaddr, long size, int blocks));
LOCAL	int	open_track_jvc	__PR((int track, track_t *track_info));
LOCAL	int	teac_fixation	__PR((int onp, int dummy, int toctype));
LOCAL	int	close_track_teac __PR((int track));
LOCAL	int	teac_calibrate __PR((int toctype, int multi));
LOCAL	int	opt_power_judge	__PR((int judge));
LOCAL	int	clear_subcode	__PR((void));
LOCAL	int	set_limits	__PR((int lba, int length));
LOCAL	int	set_subcode	__PR((u_char *subcode_data, int length));
LOCAL	int	teac_freeze	__PR((int bp_flag));

cdr_t	cdr_teac_cdr50 = {
	0, 0,
	"teac_cdr50",
	"driver for Teac CD-R50S, JVC XR-W2010, Pinnacle RCD-5020",
	drive_identify,
	teac_attach,
	teac_getdisktype,
	scsi_load,
	scsi_unload,
	recovery_needed,
	recover,
	speed_select_teac,
	select_secsize,
	next_wr_addr_jvc,
	reserve_track,
	cdr_write_teac,
	open_track_jvc,
	close_track_teac,
	teac_calibrate,
	cmd_dummy,
	read_session_offset_philips,
	teac_fixation,
};

LOCAL int
teac_getdisktype(dp, dsp)
	cdr_t	*dp;
	dstat_t	*dsp;
{
	struct	scsi_mode_header *mhp;
	struct	scsi_mode_page_header *mp;
	char			mode[256];
	int			len = 12;
	int			page = 0;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	(void)test_unit_ready();
	verbose++;
	if (mode_sense((u_char *)mode, len, page, 0) < 0) {	/* Page n current */
		verbose--;
printf("ERROR\n");
		return (-1);
	} else {
		len = ((struct scsi_mode_header *)mode)->sense_data_len + 1;
	}
	verbose--;
	if (((struct scsi_mode_header *)mode)->blockdesc_len >= 8) {

	mp = (struct scsi_mode_page_header *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
	}

printf("END\n");

	return (0);
}

LOCAL int
speed_select_teac(speed, dummy)
	int	speed;
	int	dummy;
{
	struct cdd_52x_mode_data md;
	int	count;
	int	status;

	fillbytes((caddr_t)&md, sizeof(md), '\0');

	count  = sizeof(struct scsi_mode_header) +
		sizeof(struct teac_mode_page_21);

	md.pagex.teac_page21.p_code = 0x21;
	md.pagex.teac_page21.p_len =  0x01;
	md.pagex.teac_page21.dummy = dummy?3:0;

	status = mode_select((u_char *)&md, count, 0, inq.data_format >= 2);
	if (status < 0)
		return (status);

	fillbytes((caddr_t)&md, sizeof(md), '\0');

	count  = sizeof(struct scsi_mode_header) +
		sizeof(struct teac_mode_page_31);

	speed >>= 1;
	md.pagex.teac_page31.p_code = 0x31;
	md.pagex.teac_page31.p_len =  0x02;
	md.pagex.teac_page31.speed = speed;

	return (mode_select((u_char *)&md, count, 0, inq.data_format >= 2));
}

LOCAL int
next_wr_addr_jvc(track, ap)
	int	track;
	long	*ap;
{
	return (0);
}

LOCAL int
write_teac_xg1(bp, addr, size, cnt, extwr)
	caddr_t	bp;		/* address of buffer */
	long	addr;		/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	cnt;		/* sectorcount */
	BOOL	extwr;		/* is an extended write */
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = bp;
	scmd.size = size;
	scmd.flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
/*	scmd.flags = SCG_DISRE_ENA;*/
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = SC_EWRITE;
	scmd.cdb.g1_cdb.lun = lun;
	g1_cdbaddr(&scmd.cdb.g1_cdb, addr);
	g1_cdblen(&scmd.cdb.g1_cdb, cnt);
	scmd.cdb.g1_cdb.vu_97 = extwr;

	if (scsicmd("write_teac_g1") < 0)
		return (-1);
	return (size - scmd.resid);
}

EXPORT int
cdr_write_teac(bufp, sectaddr, size, blocks)
	char	*bufp;
	long	sectaddr;
	long	size;
	int	blocks;
{
	int	ret;

	ret = write_teac_xg1(bufp, sectaddr, size, blocks, TRUE);
	if (ret < 0)
		return (ret);

	lba_addr += blocks;
	return (ret);
}

LOCAL int
open_track_jvc(track, track_info)
	int	track;
	track_t *track_info;
{
	int	i;
	int	lba;
	int	status;
	int	blocks;
	int	secspt;
	int	secsize;
	int	last_one;
	int	numbytes;
	int	num_written;
	int	numtransfer;
	u_char  *pregap_buf;
	u_char	subcode_buf[4];
extern	char	*buf;

	status = set_limits(-150, 150);
	if (status < 0)
		return (status);

	subcode_buf[0] = 1;		/* pregap ? */
	subcode_buf[1] = 0x41;		/* ctrl/addr */
	subcode_buf[2] = 1;		/* Q TNO */
	subcode_buf[3] = 0;		/* index */
	status = set_subcode(subcode_buf, 4);
	if (status < 0)
		return (status);

	secspt = track_info->secspt;
	secsize = track_info->secsize;
	pregap_buf = (u_char *)buf;
	fillbytes(buf, secspt*secsize, '\0');

	/* fill in the first 13 bytes with a code ???? */
	pregap_buf[0]  = 'T';
	pregap_buf[1]  = 'D';
	pregap_buf[2]  = 'I';
	pregap_buf[3]  = 0x01;
	pregap_buf[4]  = 'P';
	pregap_buf[5]  = 0x01;
	pregap_buf[6]  = 0x01;
	pregap_buf[7]  = 0x01;
	pregap_buf[8]  = 0x01;
	pregap_buf[9]  = 0x80;
	pregap_buf[10] = 0xFF;
	pregap_buf[11] = 0xFF;
	pregap_buf[12] = 0xFF;


	/* replicate this code for some reason */
	for (i=1; i < secspt; i++)
		movebytes(pregap_buf, &pregap_buf[i*secsize], 13);


	numtransfer = 145 / secspt;
	last_one = 145 % secspt;

	for (lba=-145, i=0; i < numtransfer; i++, lba += track_info->secspt) {
		/*
		 * now write the pregap code to the disk
		 */
		numbytes = secspt * secsize;
		num_written = write_teac_xg1((caddr_t)pregap_buf, lba, numbytes, secspt, TRUE);
		if (num_written != numbytes) {
			return (-1);
		}
	}
	if (last_one > 0){
		numbytes = last_one * secsize;
		num_written = write_teac_xg1((caddr_t)pregap_buf, lba, numbytes, last_one, TRUE);
		if (num_written != numbytes) {
			return(-1);
		}
	}

	blocks = track_info->tracksize/track_info->secsize +
		 (track_info->tracksize%track_info->secsize?1:0);
/* -----------------------------------------------------------------
 * This is just a hack for now because I don't understand the pregap length
 * calculation in checktsize.  There is definately a size problem.  I think
 * that checktsize may be wrong since its block size is the audio size
 * 2352.
----------------------------------------------------------------- */
lba_blocks=blocks; /* just a hack for now because I don't know how this works */

	/* -----------------------------------------------------------------
	 * set the limits for the new subcode - seems to apply to all
	 * of the data track.
	----------------------------------------------------------------- */
	status = set_limits(0, blocks+2);
	if (status < 0)
		return (status);

	subcode_buf[0] = 0;		/* pregap ? */
	subcode_buf[1] = 0x41;		/* ctrl/addr */
	subcode_buf[2] = 1;		/* Q TNO */
	subcode_buf[3] = 1;		/* index */
	status = set_subcode(subcode_buf, 4);
	return (status);

}

LOCAL int
close_track_teac(track)
	int	track;
{
	char	sector[3000];

	/*
	 * need read sector size
	 * XXX do we really need this ?
	 * XXX if we need this can we set blocks to 0 ?
	 */
lba_blocks++;
	return (write_teac_xg1(sector, lba_addr, 2048, 1, FALSE));
/*	return (0);*/
}



static const char *sd_teac50_error_str[] = {
	"\100\200diagnostic failure on component parts",	/* 40 80 */
	"\100\201diagnostic failure on memories",		/* 40 81 */
	"\100\202diagnostic failure on cd-rom ecc circuit",	/* 40 82 */
	"\100\203diagnostic failure on gate array",		/* 40 83 */
	"\100\204diagnostic failure on internal SCSI controller",	/* 40 84 */
	"\100\205diagnostic failure on servo processor",	/* 40 85 */
	"\100\206diagnostic failure on program rom",		/* 40 86 */
	"\100\220thermal sensor failure",			/* 40 90 */
	"\217\000a blank disk is detected by read toc",		/* 8F 00 */
	"\220\000pma less disk - not a recordable disk",	/* 90 00 */
	"\224\000toc less disk",				/* 94 00 */
	"\233\000opc initialize error",				/* 9B 00 */
	"\234\000opc execution eror",				/* 9C 00 */
	"\234\001alpc error - opc execution",			/* 9C 01 */
	"\234\002opc execution timeout",			/* 9C 02 */
	"\245\000disk application code does not match host application code",	/* A5 00 */
	"\255\000completed preview write",			/* AD 00 */
	"\257\000pca area full",				/* AF 00 */
	"\264\000full pma area",				/* B4 00 */
	"\265\000read address is atip area - blank",		/* B5 00 */
	"\266\000write address is efm area - aleady written",	/* B6 00 */
	"\272\000no write data - buffer empty",			/* BA 00 */
	"\273\000write emergency occurred",			/* BB 00 */
	"\314\000write request means mixed data mode",		/* CC 00 */
	"\315\000unable to ensure reliable writing with the inserted disk - unsupported disk",	/* CD 00 */
	"\316\000unable to ensure reliable writing as the inserted disk does not support speed",/* CE 00 */
	"\317\000unable to ensure reliable writing as the inserted disk has no char id code",	/* CF 00 */
	NULL
};

LOCAL int
teac_attach()
{
	scsi_setnonstderrs(sd_teac50_error_str);
	return (0);
}

LOCAL int
teac_fixation(onp, dummy, toctype)
	int onp;
	int dummy;
	int toctype;
{
	int total_lba;
	int status;
	u_char	subcode_buf[24];


	status = clear_subcode();
	if (status < 0)
		return (status);

	subcode_buf[0] = 0;		/* reserved */
	subcode_buf[1] = 0;		/* reserved */
	subcode_buf[2] = 0;		/* Q TNO */
/* obviously we need track 1 to track n here instead of just 1 */
	/* track 1 data */
	subcode_buf[3] = 0x41;		/* ctrl/adr */
	subcode_buf[4] = 1;		/* track no */
	subcode_buf[5] = 0;		/* start MSF=00:02:00 (LBA=0) */
	subcode_buf[6] = 2;
	subcode_buf[7] = 0;

	/* first track on disk */
	subcode_buf[8] = 0x41;		/* ctrl/adr */
	subcode_buf[9] = 0xa0;		/* point=A0 */
	subcode_buf[10] = 1;		/* first track */
	subcode_buf[11] = 0;		/* type: 0=CD-DA or CD-ROM */
	subcode_buf[12] = 0;		/* reserved */

	/* last track on disk */
	subcode_buf[13] = 0x41;		/* ctrl/adr */
	subcode_buf[14] = 0xa1;		/* point=A1 */
	subcode_buf[15] = 1;		/* last track */
	subcode_buf[16] = 0;		/* type: 0=CD-DA or CD-ROM */
	subcode_buf[17] = 0;		/* reserved */

	/* start of lead out */
	subcode_buf[18] = 0x41;		/* ctrl/adr */
	subcode_buf[19] = 0xa2;		/* point=A2 */
	/* start of lead out area in MSF */
	/* is this correct */
	total_lba = lba_blocks + 150 + 2;
	/* 00:00:00=-150(LBA) */
	subcode_buf[20] = MSF_CONV(LBA_MIN(total_lba));
	subcode_buf[21] = MSF_CONV(LBA_SEC(total_lba));
	subcode_buf[22] = MSF_CONV(LBA_FRM(total_lba));
	status = set_subcode(subcode_buf, 23);
	if (status < 0)
		return (status);

	/* now write the toc */
	status = teac_freeze(!onp);
	return (status);

}

LOCAL int
teac_calibrate(toctype, multi)
	int toctype;
	int multi;
{
	int status;

	status = clear_subcode();
	if (status < 0)
		return (status);

	status = opt_power_judge(1);
	if (status < 0)
		return (status);

	status = opt_power_judge(0);
	return (status);
}

/*--------------------------------------------------------------------------*/
#define SC_SET_LIMITS		0xb3		/* teac 12 byte command */
#define SC_SET_SUBCODE		0xc2		/* teac 10 byte command */
#define SC_READ_PMA		0xc4		/* teac 10 byte command */
#define SC_READ_DISK_INFO	0xc7		/* teac 10 byte command */
#define SC_FREEZE		0xe3		/* teac 12 byte command */
#define SC_OPC_EXECUTE		0xec		/* teac 12 byte command */
#define SC_CLEAR_SUBCODE	0xe4		/* teac 12 byte command */

/* -----------------------------------------------------------------
 * Optimum power calibration for Teac Drives.
----------------------------------------------------------------- */
LOCAL int
opt_power_judge(judge)
	int judge;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)0;
	scmd.size = 0;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G5_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.timeout = 30;

	scmd.cdb.g5_cdb.cmd = SC_OPC_EXECUTE;
	scmd.cdb.g5_cdb.lun = lun;
	scmd.cdb.g5_cdb.reladr = judge; /* Judge the Disc */

	return (scsicmd("opt_power_judge"));
}

/* -----------------------------------------------------------------
 * Clear subcodes for Teac Drives.
----------------------------------------------------------------- */
LOCAL int
clear_subcode()
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)0;
	scmd.size = 0;
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G5_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;

	scmd.cdb.g5_cdb.cmd = SC_CLEAR_SUBCODE;
	scmd.cdb.g5_cdb.lun = lun;
	scmd.cdb.g5_cdb.addr[3] = 0x80;

	return (scsicmd("clear_subcode"));
}

/* -----------------------------------------------------------------
 * Set limits for command linking for Teac Drives.
----------------------------------------------------------------- */
LOCAL int
set_limits(lba, length)
	int	lba;
	int	length;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)0;
	scmd.size = 0;
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G5_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;

	scmd.cdb.g5_cdb.cmd = SC_SET_LIMITS;
	scmd.cdb.g5_cdb.lun = lun;
	i_to_long(&scmd.cdb.g5_cdb.addr[0], lba);
	i_to_long(&scmd.cdb.g5_cdb.count[0], length);

	return (scsicmd("set_limits"));
}

/* -----------------------------------------------------------------
 * Set subcode for Teac Drives.
----------------------------------------------------------------- */
LOCAL int
set_subcode(subcode_data, length)
	u_char	*subcode_data;
	int	length;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)subcode_data;
	scmd.size = length;
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;

	scmd.cdb.g1_cdb.cmd = SC_SET_SUBCODE;
	scmd.cdb.g1_cdb.lun = lun;
	g1_cdblen(&scmd.cdb.g1_cdb, length);
	return (scsicmd("set_subcode"));
}

LOCAL int
read_disk_info(data, length, type)
	u_char	*data;
	int	length;
	int	type;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)data;
	scmd.size = length;
	scmd.flags = SCG_RECV_DATA |SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;

	scmd.cdb.g1_cdb.cmd = SC_READ_DISK_INFO;
	scmd.cdb.g1_cdb.lun = lun;
	return (scsicmd("read_disk_info"));
}

/* -----------------------------------------------------------------
 * Perform the freeze command for Teac Drives.
----------------------------------------------------------------- */
LOCAL int
teac_freeze(bp_flag)
	int	bp_flag;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)0;
	scmd.size = 0;
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G5_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.timeout = 8 * 60;		/* Needs up to 4 minutes */

	scmd.cdb.g5_cdb.cmd = SC_FREEZE;
	scmd.cdb.g5_cdb.lun = lun;
	scmd.cdb.g5_cdb.addr[3] = bp_flag ? 0x80 : 0;
	return (scsicmd("teac_freeze"));
}
