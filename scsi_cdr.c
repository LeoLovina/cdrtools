/* @(#)scsi_cdr.c	1.1 96/02/04 Copyright 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)scsi_cdr.c	1.1 96/02/04 Copyright 1995 J. Schilling";
#endif  lint
/*
 *	SCSI command functions for cdrecord
 *
 *	Copyright (c) 1995 J. Schilling
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

/*
 * NOTICE:	The Philips CDD 521 has several firmware bugs.
 *		One of them is not to respond to a SCSI selection
 *		within 200ms if the general load on the
 *		SCSI bus is high. To deal with this problem
 *		most of the SCSI commands are send with the
 *		SCG_CMD_RETRY flag enabled.
 */
#include <stdio.h>
#include <standard.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/file.h>

#include <scgio.h>
#include <scsidefs.h>
#include <scsireg.h>

#include "cdrecord.h"
#include "scsitransp.h"

#define	strindex(s1,s2)	strstr((s2), (s1))

struct	scg_cmd		scmd;
struct	scsi_inquiry	inq;

extern	int	scsibus;
extern	int	target;
extern	int	lun;

extern	int	silent;
extern	int	verbose;


int
open_scsi(char *scsidev, int timeout)
{
	int	x1, x2, x3;
	int	n;
	extern	int	deftimeout;

	scsibus = 0;
	target = 6;
	lun = 0;

	deftimeout = timeout;

	n = sscanf(scsidev, "%d,%d,%d", &x1, &x2, &x3);
	if (n == 3) {
		scsibus = x1;
		target = x2;
		lun = x3;
	} else if (n == 2) {
		scsibus = 0;
		target = x1;
		lun = x2;
	} else {
		printf("WARNING: device not valid, trying to use default target...\n");
	}
	printf("scsidev: '%s'\n", scsidev);
	printf("scsibus: %d target: %d lun: %d\n", scsibus, target, lun);
	return (scsi_open());
}

void
scsi_settimeout(int timeout)
{
	extern	int	deftimeout;

	deftimeout = timeout / 100;
}

BOOL unit_ready()
{
	if (test_unit_ready() >= 0)		/* alles OK */
		return (TRUE);
	else if (scmd.error >= SCG_FATAL)	/* nicht selektierbar */
		return (FALSE);

	if (scmd.sense.code < 0x70) {		/* non extended Sense */
		if (scmd.sense.code == 0x04)	/* NOT_READY */
			return (FALSE);
		return (TRUE);
	}

						/* FALSE wenn NOT_READY */
	return (((struct scsi_ext_sense *)&scmd.sense)->key != SC_NOT_READY);
}

int
test_unit_ready()
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)0;
	scmd.size = 0;
	scmd.flags = SCG_DISRE_ENA | (silent ? SCG_SILENT:0);
	scmd.cdb_len = SC_G0_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g0_cdb.cmd = SC_TEST_UNIT_READY;
	scmd.cdb.g0_cdb.lun = lun;
	
	return (scsicmd("test unit ready"));
}

int
rezero_unit()
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)0;
	scmd.size = 0;
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G0_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g0_cdb.cmd = SC_REZERO_UNIT;
	scmd.cdb.g0_cdb.lun = lun;
	
	return (scsicmd("rezero unit"));
}

int
request_sense()
{
	char	sensebuf[CCS_SENSE_LEN];

	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = sensebuf;
	scmd.size = sizeof(sensebuf);;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G0_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g0_cdb.cmd = SC_REQUEST_SENSE;
	scmd.cdb.g0_cdb.lun = lun;
	scmd.cdb.g0_cdb.count = CCS_SENSE_LEN;
	
	if (scsicmd("request_sense") < 0)
		return (-1);
	scsiprsense((u_char *)sensebuf, CCS_SENSE_LEN - scmd.resid);
	return (0);
}

int
inquiry(bp, cnt)
	caddr_t	bp;
	int	cnt;
{
	fillbytes(bp, cnt, '\0');
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = bp;
	scmd.size = cnt;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G0_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g0_cdb.cmd = SC_INQUIRY;
	scmd.cdb.g0_cdb.lun = lun;
	scmd.cdb.g0_cdb.count = cnt;
	
	if (scsicmd("inquiry") < 0)
		return (-1);
	if (verbose) {
		scsiprbytes("Inquiry Data: ", (u_char *)bp, cnt - scmd.resid);
		printf("\n");
	}
	return (0);
}

int
scsi_load_unload(load)
	int	load;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0xE7;
	scmd.cdb.g1_cdb.lun = lun;
	scmd.cdb.g1_cdb.count[1] = !load;
	
	if (scsicmd("medium load/unload") < 0)
		return (-1);
	return (0);
}


int
scsi_prevent_removal(prevent)
	int	prevent;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G0_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g0_cdb.cmd = 0x1E;
	scmd.cdb.g0_cdb.lun = lun;
	scmd.cdb.g0_cdb.count = prevent & 1;
	
	if (scsicmd("prevent/allow medium removal") < 0)
		return (-1);
	return (0);
}


int
scsi_start_stop_unit(flg)
	int	flg;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G0_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g0_cdb.cmd = 0x1B;	/* Start Stop Unit */
	scmd.cdb.g0_cdb.lun = lun;
	scmd.cdb.g0_cdb.count = flg & 0x1;
	
	return (scsicmd("start/stop unit"));
}

int
qic02(cmd)
	int	cmd;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)0;
	scmd.size = 0;
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G0_CDBLEN;
	scmd.sense_len = DEF_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g0_cdb.cmd = 0x0D;	/* qic02 Sysgen SC4000 */
	scmd.cdb.g0_cdb.lun = lun;
	scmd.cdb.g0_cdb.mid_addr = cmd;
	
	return (scsicmd("qic 02"));
}

int
write_xg0(bp, addr, size, cnt)
	caddr_t	bp;		/* address of buffer */
	long	addr;		/* disk address (sector) to put */
	int	size;		/* number of bytes to transfer */
	int	cnt;		/* sectorcount */
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = bp;
	scmd.size = size;
	scmd.flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
/*	scmd.flags = SCG_DISRE_ENA;*/
	scmd.cdb_len = SC_G0_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g0_cdb.cmd = SC_WRITE;
	scmd.cdb.g0_cdb.lun = lun;
	g0_cdbaddr(&scmd.cdb.g0_cdb, addr);
	scmd.cdb.g0_cdb.count = cnt;
	
	if (scsicmd("write_g0") < 0)
		return (-1);
	return (size - scmd.resid);
}

int
write_track(track, isaudio, preemp)
	long	track;		/* track number 0 == new track */
	int	isaudio;
	int	preemp;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
/*	scmd.flags = SCG_DISRE_ENA;*/
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0xE6;
	scmd.cdb.g1_cdb.lun = lun;
	g1_cdbaddr(&scmd.cdb.g1_cdb, track);
	scmd.cdb.g1_cdb.res6 = isaudio ? (preemp ? 5 : 4) : 1;
	
	if (scsicmd("write_track") < 0)
		return (-1);
	return (0);
}

int
scsi_flush_cache()
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0x35;
	scmd.cdb.g1_cdb.lun = lun;
	
	if (scsicmd("flush cache") < 0)
		return (-1);
	return (0);
}

int
fixation(onp, type)
	int	onp;	/* open next program area */
	int	type;	/* TOC type 0: CD-DA, 1: CD-ROM, 2: CD-ROM/XA1, 3: CD-ROM/XA2, 4: CDI */
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0xE9;
	scmd.cdb.g1_cdb.lun = lun;
	scmd.cdb.g1_cdb.count[1] = (onp ? 8 : 0) | type;
	
	if (scsicmd("fixation") < 0)
		return (-1);
	return (0);
}

int
recover()
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0xEC;
	scmd.cdb.g1_cdb.lun = lun;
	
	if (scsicmd("recover") < 0)
		return (-1);
	return (0);
}

int
reserve_track(len)
	unsigned long len;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0xE4;
	scmd.cdb.g1_cdb.lun = lun;
	i_to_long(&scmd.cdb.g1_cdb.addr[3], len);
	
	if (scsicmd("reserve_track") < 0)
		return (-1);
	return (0);
}

int
mode_select(dp, cnt, smp, pf)
	u_char	*dp;
	int	cnt;
	int	smp;
	int	pf;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)dp;
	scmd.size = cnt;
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G0_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g0_cdb.cmd = SC_MODE_SELECT;
	scmd.cdb.g0_cdb.lun = lun;
	scmd.cdb.g0_cdb.high_addr = smp ? 1 : 0 | pf ? 0x10 : 0;
	scmd.cdb.g0_cdb.count = cnt;

	printf("%s ", smp?"Save":"Set ");
	scsiprbytes("Mode Parameters", dp, cnt);

	return (scsicmd("mode select"));
}

struct cdd_52x_mode_page_21 {	/* write track information */
	u_char	parsave		: 1;
	u_char			: 1;
	u_char	p_code		: 6;
	u_char	p_len;			/* 0x0E = 14 Bytes */
	u_char	res_2;
	u_char	sectype;
	u_char	track;
	u_char	ISRC[9];
	u_char	res[2];

};

struct cdd_52x_mode_page_23 {	/* speed selection */
	u_char	parsave		: 1;
	u_char			: 1;
	u_char	p_code		: 6;
	u_char	p_len;			/* 0x06 = 6 Bytes */
	u_char	speed;
	u_char	dummy;
	u_char	res[4];

};

struct yamaha_mode_page_31 {	/* drive configuration */
	u_char	parsave		: 1;
	u_char			: 1;
	u_char	p_code		: 6;
	u_char	p_len;			/* 0x02 = 2 Bytes */
	u_char	res;
	u_char	speed		: 4;
	u_char	dummy		: 4;

};

struct cdd_52x_mode_data {
	struct scsi_mode_header	header;
	union cdd_pagex	{
		struct cdd_52x_mode_page_21	page21;
		struct cdd_52x_mode_page_23	page23;
		struct yamaha_mode_page_31	page31;
	} pagex;
};

int
speed_select(speed, dummy)
	int	speed;
	int	dummy;
{
	struct cdd_52x_mode_data md;
	int	count;

	fillbytes((caddr_t)&md, sizeof(md), '\0');

	if (is_yamaha()) {
		count  = sizeof(struct scsi_mode_header) +
			sizeof(struct yamaha_mode_page_31);

		speed >>= 1;
		md.pagex.page31.p_code = 0x31;
		md.pagex.page31.p_len =  0x02;
		md.pagex.page31.speed = speed;
		md.pagex.page31.dummy = dummy;
	} else {
		count  = sizeof(struct scsi_mode_header) +
			sizeof(struct cdd_52x_mode_page_23);

		md.pagex.page23.p_code = 0x23;
		md.pagex.page23.p_len =  0x06;
		md.pagex.page23.speed = speed;
		md.pagex.page23.dummy = dummy;
	}
	
	return (mode_select((u_char *)&md, count, 0, 1));
}

int
write_track_info(isaudio, preemp)
	int	isaudio;
	int	preemp;
{
	struct cdd_52x_mode_data md;
	int	count = sizeof(struct scsi_mode_header) +
			sizeof(struct cdd_52x_mode_page_21);

	fillbytes((caddr_t)&md, sizeof(md), '\0');
	md.pagex.page21.p_code = 0x21;
	md.pagex.page21.p_len =  0x0E;
				/* is sectype ok ??? */
	md.pagex.page21.sectype = isaudio ? (preemp ? 5 : 4) : 1;
	md.pagex.page21.track = 0;	/* 0 : create new track */
	
	return (mode_select((u_char *)&md, count, 0, 1));
}

int
select_secsize(secsize)
	int	secsize;
{
	struct scsi_mode_data md;
	int	count = sizeof(struct scsi_mode_header) +
			sizeof(struct scsi_mode_blockdesc);

	fillbytes((caddr_t)&md, sizeof(md), '\0');
	md.header.blockdesc_len = 8;
	i_to_3_byte(md.blockdesc.lblen, secsize);
	
	return (mode_select((u_char *)&md, count, 0, 1));
}

int	dev = DEV_CDD_521;

BOOL
is_cdrecorder()
{
	return (dev == DEV_CDD_521 ||
		dev == DEV_YAMAHA_CDR_100);
}

BOOL
is_yamaha()
{
	return (dev == DEV_YAMAHA_CDR_100);
}


#define	DEBUG
#ifdef	DEBUG
#define	G0_MAXADDR	0x1FFFFFL

/*struct scsi_capacity cap = { 0, 2048 };*/
struct scsi_capacity cap = { 0, 2352 };

int
read_scsi(bp, addr, cnt)
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	if(addr <= G0_MAXADDR)
		return(read_g0(bp, addr, cnt));
	else
		return(read_g1(bp, addr, cnt));
}

int
read_g0(bp, addr, cnt)
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	if (cap.c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = bp;
	scmd.size = cnt*cap.c_bsize;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G0_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g0_cdb.cmd = SC_READ;
	scmd.cdb.g0_cdb.lun = lun;
	g0_cdbaddr(&scmd.cdb.g0_cdb, addr);
	scmd.cdb.g0_cdb.count = cnt;
/*	scmd.cdb.g0_cdb.vu_56 = 1;*/
	
	return (scsicmd("read_g0"));
}

int
read_g1(bp, addr, cnt)
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	if (cap.c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = bp;
	scmd.size = cnt*cap.c_bsize;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = SC_EREAD;
	scmd.cdb.g1_cdb.lun = lun;
	g1_cdbaddr(&scmd.cdb.g1_cdb, addr);
	g1_cdblen(&scmd.cdb.g1_cdb, cnt);
	
	return (scsicmd("read_g1"));
}
#endif	/* DEBUG */

BOOL getdev(print)
	BOOL	print;
{
	BOOL	got_inquiry = TRUE;

	fillbytes((caddr_t)&inq, sizeof(inq), '\0');
	dev = DEV_UNKNOWN;
	silent++;
	(void)unit_ready();
	if (scmd.error >= SCG_FATAL &&
				!(scmd.scb.chk && scmd.sense_count > 0)) {
		silent--;
		return (FALSE);
	}


/*	if (scmd.error < SCG_FATAL || scmd.scb.chk && scmd.sense_count > 0){*/

	if (inquiry((caddr_t)&inq, sizeof(inq)) < 0) {
		got_inquiry = FALSE;
		if (verbose) {
			printf(
		"error: %d scb.chk: %d sense_count: %d sense.code: 0x%x\n",
				scmd.error, scmd.scb.chk,
				scmd.sense_count, scmd.sense.code);
		}
			/*
			 * Folgende Kontroller kennen das Kommando
			 * INQUIRY nicht:
			 *
			 * ADAPTEC	ACB-4000, ACB-4010, ACB 4070
			 * SYSGEN	SC4000
			 *
			 * Leider reagieren ACB40X0 und ACB5500 identisch
			 * wenn drive not ready (code == not ready),
			 * sie sind dann nicht zu unterscheiden.
			 */

		if (scmd.scb.chk && scmd.sense_count == 4) {
			/* Test auf SYSGEN			 */
			(void)qic02(0x12);	/* soft lock on  */
			if (qic02(1) < 0) {	/* soft lock off */
				dev = DEV_ACB40X0;
/*				dev = acbdev();*/
			} else {
				dev = DEV_SC4000;
				inq.type = INQ_SEQD;
				inq.removable = 1;
			}
		}
	}
	switch (inq.type) {

	case INQ_DASD:
		if (inq.add_len == 0) {
			if (dev == DEV_UNKNOWN && got_inquiry) {
				dev = DEV_ACB5500;
				strcpy(inq.vendor_info,
					"ADAPTEC ACB-5500        FAKE");

			} else switch (dev) {

			case DEV_ACB40X0:
				strcpy(inq.vendor_info,
					"ADAPTEC ACB-40X0        FAKE");
				break;
			case DEV_ACB4000:
				strcpy(inq.vendor_info,
					"ADAPTEC ACB-4000        FAKE");
				break;
			case DEV_ACB4010:
				strcpy(inq.vendor_info,
					"ADAPTEC ACB-4010        FAKE");
				break;
			case DEV_ACB4070:
				strcpy(inq.vendor_info,
					"ADAPTEC ACB-4070        FAKE");
				break;
			}
		} else if (inq.add_len < 31) {
			dev = DEV_NON_CCS_DSK;

		} else if (strindex("EMULEX", inq.info)) {
			if (strindex("MD21", inq.ident))
				dev = DEV_MD21;
			if (strindex("MD23", inq.ident))
				dev = DEV_MD23;
			else
				dev = DEV_CCS_GENDISK;
		} else if (strindex("ADAPTEC", inq.info)) {
			if (strindex("ACB-4520", inq.ident))
				dev = DEV_ACB4520A;
			if (strindex("ACB-4525", inq.ident))
				dev = DEV_ACB4525;
			else
				dev = DEV_CCS_GENDISK;
		} else if (strindex("SONY", inq.info) &&
					strindex("SMO-C501", inq.ident)) {
			dev = DEV_SONY_SMO;
		} else {
			dev = DEV_CCS_GENDISK;
		}
		break;

	case INQ_SEQD:
		if (dev == DEV_SC4000) {
			strcpy(inq.vendor_info,
				"SYSGEN  SC4000          FAKE");
		} else if (inq.add_len == 0 &&
					inq.removable &&
						inq.ansi_version == 1) {
			dev = DEV_MT02;
			strcpy(inq.vendor_info,
				"EMULEX  MT02            FAKE");
		}
		break;

/*	case INQ_OPTD:*/
	case INQ_ROMD:
	case INQ_WORM:
		if (strindex("RXT-800S", inq.ident))
			dev = DEV_RXT800S;
		if (strindex("PHILIPS", inq.info) ||
				strindex("IMS", inq.info) ||
				strindex("KODAK", inq.info) ||
				strindex("HP", inq.info))
			dev = DEV_CDD_521;
		if (strindex("YAMAHA", inq.info))
			dev = DEV_YAMAHA_CDR_100;
		break;

	case INQ_PROCD:
		if (strindex("BERTHOLD", inq.info)) {
			if (strindex("", inq.ident))
				dev = DEV_HRSCAN;
		}
		break;

	case INQ_SCAN:
		dev = DEV_MS300A;
		break;
	}
	silent--;
	if (!print)
		return (TRUE);

	if (dev == DEV_UNKNOWN && !got_inquiry) {
#ifdef	PRINT_INQ_ERR
		/*
		 * Der String 'inquiry" kann falsch sein!
		 * Das ist dann, wenn ein Gerät angeschlossen ist,
		 * das nonextended sense liefert und mit dem
		 * qic02() Kommando nicht identifiziert werden kann.
		 */
		scsiprinterr("inquiry");
#endif
		return (FALSE);
	}

	printf("Device type    : ");
	scsiprintdev(&inq);
	printf("Version        : %d\n", inq.ansi_version);
	printf("Response Format: %d\n", inq.data_format);
	if (inq.data_format >= 2) {
		printf("Capabilities   : ");
		if (inq.aenc)		printf("AENC ");
		if (inq.termiop)	printf("TERMIOP ");
		if (inq.reladr)		printf("RELADR ");
		if (inq.wbus32)		printf("WBUS32 ");
		if (inq.wbus16)		printf("WBUS16 ");
		if (inq.sync)		printf("SYNC ");
		if (inq.linked)		printf("LINKED ");
		if (inq.cmdque)		printf("CMDQUE ");
		if (inq.softreset)	printf("SOFTRESET ");
		printf("\n");
	}
	if (inq.add_len >= 31 ||
			inq.info[0] || inq.ident[0] || inq.revision[0]) {
		printf("Vendor_info    : '%.8s'\n", inq.info);
		printf("Identifikation : '%.16s'\n", inq.ident);
		printf("Revision       : '%.4s'\n", inq.revision);
	}
	return (TRUE);
}

void printdev()
{
	printf("Device seems to be: ");

	switch (dev) {

	case DEV_UNKNOWN:	printf("unknown");		break;
	case DEV_ACB40X0:	printf("Adaptec 4000/4010/4070");break;
	case DEV_ACB4000:	printf("Adaptec 4000");		break;
	case DEV_ACB4010:	printf("Adaptec 4010");		break;
	case DEV_ACB4070:	printf("Adaptec 4070");		break;
	case DEV_ACB5500:	printf("Adaptec 5500");		break;
	case DEV_ACB4520A:	printf("Adaptec 4520A");	break;
	case DEV_ACB4525:	printf("Adaptec 4525");		break;
	case DEV_MD21:		printf("Emulex MD21");		break;
	case DEV_MD23:		printf("Emulex MD23");		break;
	case DEV_NON_CCS_DSK:	printf("Generic NON CCS Disk");	break;
	case DEV_CCS_GENDISK:	printf("Generic CCS Disk");	break;
	case DEV_SONY_SMO:	printf("Sony SMO-C501");	break;
	case DEV_MT02:		printf("Emulex MT02");		break;
	case DEV_SC4000:	printf("Sysgen SC4000");	break;
	case DEV_RXT800S:	printf("Maxtor RXT800S");	break;
	case DEV_HRSCAN:	printf("Berthold HR-Scanner");	break;
	case DEV_MS300A:	printf("Microtek MS300A");	break;

	case DEV_CDD_521:	printf("Philips CDD521");	break;
	case DEV_YAMAHA_CDR_100:printf("Yamaha CDR-100");	break;

	default:		printf("Missing Entry");	break;

	}
	printf(".\n");

}

BOOL
do_inquiry(print)
	int	print;
{
	if (getdev(print)) {
		printdev();
		return (TRUE);
	} else {
		return (FALSE);
	}
}

BOOL
recovery_needed()
{
	int err;

	silent++;
	err = test_unit_ready();
	silent--;

	if (err >= 0)
		return (FALSE);
	else if (scmd.error >= SCG_FATAL)	/* nicht selektierbar */
		return (FALSE);

	if (scmd.sense.code < 0x70)		/* non extended Sense */
		return (FALSE);
	return (((struct scsi_ext_sense *)&scmd.sense)->error_code == 0xD0);
}
