/* @(#)scsi_cdr.c	1.17 97/09/10 Copyright 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)scsi_cdr.c	1.17 97/09/10 Copyright 1995 J. Schilling";
#endif
/*
 *	SCSI command functions for cdrecord
 *
 *	Copyright (c) 1995 J. Schilling
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

#define	strindex(s1,s2)	strstr((s2), (s1))

struct	scg_cmd		scmd;
struct	scsi_inquiry	inq;
struct scsi_capacity cap = { 0, 2048 };
/*struct scsi_capacity cap = { 0, 2352 };*/

extern	int	scsibus;
extern	int	target;
extern	int	lun;

extern	int	silent;
extern	int	verbose;
extern	int	lverbose;

EXPORT	int	open_scsi	__PR((char *scsidev, int timeout,
								int be_verbose));
EXPORT	void	scsi_settimeout	__PR((int timeout));

EXPORT	BOOL	unit_ready	__PR((void));
EXPORT	int	test_unit_ready	__PR((void));
EXPORT	int	rezero_unit	__PR((void));
EXPORT	int	request_sense	__PR((void));
EXPORT	int	inquiry		__PR((caddr_t, int));
EXPORT	int	read_capacity	__PR((void));
EXPORT	int	scsi_load_unload __PR((int));
EXPORT	int	load_unload_philips __PR((int));
EXPORT	int	scsi_prevent_removal __PR((int));
EXPORT	int	scsi_start_stop_unit __PR((int, int));
EXPORT	int	scsi_set_speed	__PR((int readspeed, int writespeed));
EXPORT	int	qic02		__PR((int));
EXPORT	int	write_xg0	__PR((caddr_t, long, long, int));
EXPORT	int	write_xg1	__PR((caddr_t, long, long, int));
EXPORT	int	write_track	__PR((long, int));
EXPORT	int	scsi_flush_cache __PR((void));
EXPORT	int	read_toc	__PR((caddr_t, int, int, int, int));
EXPORT	int	read_toc_philips __PR((caddr_t, int, int, int, int));
EXPORT	int	read_header	__PR((caddr_t, long, int, int));
EXPORT	int	read_track_info	__PR((caddr_t, int, int));
EXPORT	int	read_track_info_philips	__PR((caddr_t, int, int));
EXPORT	int	close_track_philips __PR((int track));
EXPORT	int	fixation	__PR((int, int, int));
EXPORT	int	scsi_close_session __PR((int type));
EXPORT	int	recover		__PR((int));
EXPORT	int	first_writable_addr __PR((long *, int, int, int, int));
EXPORT	int	reserve_track	__PR((unsigned long));
EXPORT	int	mode_select	__PR((unsigned char *, int, int, int));
EXPORT	int	mode_sense	__PR((u_char *dp, int cnt, int page, int pcf));
EXPORT	int	speed_select_yamaha	__PR((int speed, int dummy));
EXPORT	int	speed_select_philips	__PR((int speed, int dummy));
EXPORT	int	write_track_info __PR((int));
EXPORT	int	read_tochdr	__PR((cdr_t *, int *, int *));
EXPORT	int	read_trackinfo	__PR((int, long *, struct msf *, int *, int *, int *));
EXPORT	int	read_session_offset __PR((long *));
EXPORT	int	read_session_offset_philips __PR((long *));
EXPORT	int	select_secsize	__PR((int));
EXPORT	BOOL	is_cddrive	__PR((void));
EXPORT	BOOL	is_unknown_dev	__PR((void));
EXPORT	int	read_scsi	__PR((caddr_t, long, int));
EXPORT	int	read_g0		__PR((caddr_t, long, int));
EXPORT	int	read_g1		__PR((caddr_t, long, int));
EXPORT	BOOL	getdev		__PR((BOOL));
EXPORT	void	printdev	__PR((void));
EXPORT	BOOL	do_inquiry	__PR((BOOL));
EXPORT	BOOL	recovery_needed	__PR((void));
EXPORT	int	scsi_load	__PR((void));
EXPORT	int	scsi_unload	__PR((void));
EXPORT	int	scsi_cdr_write	__PR((char *bufp, long sectaddr, long size, int blocks));
EXPORT	BOOL	is_mmc		__PR((void));
EXPORT	BOOL	mmc_check	__PR((BOOL *cdrrp, BOOL *cdwrp,
					BOOL *cdrrwp, BOOL *cdwrwp));


EXPORT int
open_scsi(scsidev, timeout, be_verbose)
	char	*scsidev;
	int	timeout;
	int	be_verbose;
{
	int	x1, x2, x3;
	int	n = 0;
	extern	int	deftimeout;

	if (timeout >= 0)
		deftimeout = timeout;

	if (scsidev != NULL)
		n = sscanf(scsidev, "%d,%d,%d", &x1, &x2, &x3);
	if (n == 3) {
		scsibus = x1;
		target = x2;
		lun = x3;
	} else if (n == 2) {
		scsibus = 0;
		target = x1;
		lun = x2;
	} else if (scsidev != NULL) {
		printf("WARNING: device not valid, trying to use default target...\n");
		scsibus = 0;
		target = 6;
		lun = 0;
	}
	if (be_verbose && scsidev != NULL) {
		printf("scsidev: '%s'\n", scsidev);
		printf("scsibus: %d target: %d lun: %d\n",
						scsibus, target, lun);
	}
	return (scsi_open());
}

EXPORT void
scsi_settimeout(timeout)
	int	timeout;
{
	extern	int	deftimeout;

	deftimeout = timeout / 100;
}

EXPORT BOOL
unit_ready()
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

EXPORT int
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

EXPORT int
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

EXPORT int
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

EXPORT int
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
	if (verbose)
		scsiprbytes("Inquiry Data   :", (u_char *)bp, cnt - scmd.resid);
	return (0);
}

EXPORT int
read_capacity()
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)&cap;
	scmd.size = sizeof(cap);
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0x25;	/* Read Capacity */
	scmd.cdb.g1_cdb.lun = lun;
	g1_cdblen(&scmd.cdb.g1_cdb, 0); /* Full Media */
	
	if (scsicmd("read capacity") < 0) {
		return (-1);
	} else {
		long	kb;
		long	mb;
		long	prmb;
		double	dkb;
		long	cbsize;
		long	cbaddr;

		cbsize = a_to_u_long(&cap.c_bsize);
		cbaddr = a_to_u_long(&cap.c_baddr);
		cap.c_bsize = cbsize;
		cap.c_baddr = cbaddr;

		if (silent)
			return (0);

		dkb =  (cap.c_baddr+1.0) * (cap.c_bsize/1024.0);
		kb = dkb;
		mb = dkb / 1024.0;
		prmb = dkb / 1000.0 * 1.024;
		printf("Capacity: %ld Blocks = %ld kBytes = %ld MBytes = %ld prMB\n",
			cap.c_baddr+1, kb, mb, prmb);
		printf("Sectorsize: %ld Bytes\n", cap.c_bsize);
	}
	return (0);
}

EXPORT int
scsi_load_unload(load)
	int	load;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G5_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g5_cdb.cmd = 0xA6;
	scmd.cdb.g5_cdb.lun = lun;
	scmd.cdb.g5_cdb.addr[1] = load?3:2;
	scmd.cdb.g5_cdb.count[2] = 0; /* slot # */
	
	if (scsicmd("medium load/unload") < 0)
		return (-1);
	return (0);
}

EXPORT int
load_unload_philips(load)
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


EXPORT int
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


EXPORT int
scsi_start_stop_unit(flg, loej)
	int	flg;
	int	loej;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G0_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g0_cdb.cmd = 0x1B;	/* Start Stop Unit */
	scmd.cdb.g0_cdb.lun = lun;
	scmd.cdb.g0_cdb.count = (flg ? 1:0) | (loej ? 2:0);
	
	return (scsicmd("start/stop unit"));
}

EXPORT int
scsi_set_speed(readspeed, writespeed)
	int	readspeed;
	int	writespeed;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G5_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g5_cdb.cmd = 0xBB;
	scmd.cdb.g5_cdb.lun = lun;
	i_to_short(&scmd.cdb.g5_cdb.addr[0], readspeed);
	i_to_short(&scmd.cdb.g5_cdb.addr[2], writespeed);

	if (scsicmd("set cd speed") < 0)
		return (-1);
	return (0);
}

EXPORT int
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

EXPORT int
write_xg0(bp, addr, size, cnt)
	caddr_t	bp;		/* address of buffer */
	long	addr;		/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
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

EXPORT int
write_xg1(bp, addr, size, cnt)
	caddr_t	bp;		/* address of buffer */
	long	addr;		/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	cnt;		/* sectorcount */
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
	
	if (scsicmd("write_g1") < 0)
		return (-1);
	return (size - scmd.resid);
}

EXPORT int
write_track(track, sectype)
	long	track;		/* track number 0 == new track */
	int	sectype;
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
	scmd.cdb.g1_cdb.res6 = sectype;
	
	if (scsicmd("write_track") < 0)
		return (-1);
	return (0);
}

EXPORT int
scsi_flush_cache()
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.timeout = 2 * 60;		/* Max: sizeof(CDR-cache)/150KB/s */
	scmd.cdb.g1_cdb.cmd = 0x35;
	scmd.cdb.g1_cdb.lun = lun;
	
	if (scsicmd("flush cache") < 0)
		return (-1);
	return (0);
}

EXPORT int
read_toc(bp, track, cnt, msf, fmt)
	caddr_t	bp;
	int	track;
	int	cnt;
	int	msf;
	int	fmt;

{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = bp;
	scmd.size = cnt;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0x43;
	scmd.cdb.g1_cdb.lun = lun;
	if (msf)
		scmd.cdb.g1_cdb.res = 1;
	scmd.cdb.g1_cdb.addr[0] = fmt & 0x0F;
	scmd.cdb.g1_cdb.res6 = track;
	g1_cdblen(&scmd.cdb.g1_cdb, cnt);

	if (scsicmd("read toc") < 0)
		return (-1);
	return (0);
}

EXPORT int
read_toc_philips(bp, track, cnt, msf, ses)
	caddr_t	bp;
	int	track;
	int	cnt;
	int	msf;
	int	ses;

{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = bp;
	scmd.size = cnt;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0x43;
	scmd.cdb.g1_cdb.lun = lun;
	if (msf)
		scmd.cdb.g1_cdb.res = 1;
	scmd.cdb.g1_cdb.res6 = track;
	g1_cdblen(&scmd.cdb.g1_cdb, cnt);
	if (ses)
		scmd.cdb.g1_cdb.vu_96 = 1;

	if (scsicmd("read toc") < 0)
		return (-1);
	return (0);
}

EXPORT int
read_header(bp, addr, cnt, msf)
	caddr_t	bp;
	long	addr;
	int	cnt;
	int	msf;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = bp;
	scmd.size = cnt;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0x44;
	scmd.cdb.g1_cdb.lun = lun;
	if (msf)
		scmd.cdb.g1_cdb.res = 1;
	g1_cdbaddr(&scmd.cdb.g1_cdb, addr);
	g1_cdblen(&scmd.cdb.g1_cdb, cnt);

	if (scsicmd("read header") < 0)
		return (-1);
	return (0);
}

EXPORT int
read_track_info(bp, track, cnt)
	caddr_t	bp;
	int	track;
	int	cnt;

{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = bp;
	scmd.size = cnt;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0x52;
	scmd.cdb.g1_cdb.lun = lun;
	scmd.cdb.g1_cdb.reladr = 1;
	g1_cdbaddr(&scmd.cdb.g1_cdb, track);
	g1_cdblen(&scmd.cdb.g1_cdb, cnt);

	if (scsicmd("read track info") < 0)
		return (-1);
	return (0);
}

EXPORT int
read_track_info_philips(bp, track, cnt)
	caddr_t	bp;
	int	track;
	int	cnt;

{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = bp;
	scmd.size = cnt;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0xE5;
	scmd.cdb.g1_cdb.lun = lun;
	g1_cdbaddr(&scmd.cdb.g1_cdb, track);
	g1_cdblen(&scmd.cdb.g1_cdb, cnt);

	if (scsicmd("read track info") < 0)
		return (-1);
	return (0);
}

/*
 * Needed for JVC too.
 */
EXPORT int
close_track_philips(track)
	int	track;
{
	return (scsi_flush_cache());
}

EXPORT int
fixation(onp, dummy, type)
	int	onp;	/* open next program area */
	int	dummy;
	int	type;	/* TOC type 0: CD-DA, 1: CD-ROM, 2: CD-ROM/XA1, 3: CD-ROM/XA2, 4: CDI */
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.timeout = 8 * 60;		/* Needs up to 4 minutes */
	scmd.cdb.g1_cdb.cmd = 0xE9;
	scmd.cdb.g1_cdb.lun = lun;
	scmd.cdb.g1_cdb.count[1] = (onp ? 8 : 0) | type;
	
	if (scsicmd("fixation") < 0)
		return (-1);
	return (0);
}

EXPORT int
scsi_close_session(type)
	int	type;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.timeout = 8 * 60;		/* Needs up to 4 minutes */
	scmd.cdb.g1_cdb.cmd = 0x5B;
	scmd.cdb.g1_cdb.lun = lun;
/*	scmd.cdb.g1_cdb.addr[0] = type;*/
	scmd.cdb.g1_cdb.addr[0] = 2;
	
	if (scsicmd("close session") < 0)
		return (-1);
	return (0);
}

EXPORT int
recover(track)
	int	track;
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

struct	fwa {
	char	len;
	char	addr[4];
	char	res;
};

EXPORT int
first_writable_addr(ap, track, isaudio, preemp, npa)
	long	*ap;
	int	track;
	int	isaudio;
	int	preemp;
	int	npa;
{
	struct	fwa	fwa;

	fillbytes((caddr_t)&fwa, sizeof(fwa), '\0');
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)&fwa;
	scmd.size = sizeof(fwa);
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0xE2;
	scmd.cdb.g1_cdb.lun = lun;
	scmd.cdb.g1_cdb.addr[0] = track;
	scmd.cdb.g1_cdb.addr[1] = isaudio ? (preemp ? 5 : 4) : 1;

	scmd.cdb.g1_cdb.count[0] = npa?1:0;
	scmd.cdb.g1_cdb.count[1] = sizeof(fwa);
	
	if (scsicmd("first writeable address") < 0)
		return (-1);

	if (ap)
		*ap = a_to_u_long(fwa.addr);
	return (0);
}

EXPORT int
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

EXPORT int
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
	if (verbose)
		scsiprbytes("Mode Parameters", dp, cnt);

	return (scsicmd("mode select"));
}

EXPORT int
mode_sense(dp, cnt, page, pcf)
	u_char	*dp;
	int	cnt;
	int	page;
	int	pcf;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)dp;
	scmd.size = 0xFF;
	scmd.size = cnt;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G0_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g0_cdb.cmd = SC_MODE_SENSE;
	scmd.cdb.g0_cdb.lun = lun;
#ifdef	nonono
	scmd.cdb.g0_cdb.high_addr = 1<<4;	/* DBD Disable Block desc. */
#endif
	scmd.cdb.g0_cdb.mid_addr = (page&0x3F) | ((pcf<<6)&0xC0);
	scmd.cdb.g0_cdb.count = page ? 0xFF : 24;
	scmd.cdb.g0_cdb.count = cnt;

	if (scsicmd("mode sense") < 0)
		return (-1);
	if (verbose) scsiprbytes("Mode Sense Data", dp, cnt - scmd.resid);
	return (0);
}

struct cdd_52x_mode_page_21 {	/* write track information */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0E = 14 Bytes */
	u_char	res_2;
	u_char	sectype;
	u_char	track;
	u_char	ISRC[9];
	u_char	res[2];
};

struct cdd_52x_mode_page_23 {	/* speed selection */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x06 = 6 Bytes */
	u_char	speed;
	u_char	dummy;
	u_char	res[4];

};

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct yamaha_mode_page_31 {	/* drive configuration */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x02 = 2 Bytes */
	u_char	res;
	u_char	dummy		: 4;
	u_char	speed		: 4;
};

#else				/* Motorola byteorder */

struct yamaha_mode_page_31 {	/* drive configuration */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x02 = 2 Bytes */
	u_char	res;
	u_char	speed		: 4;
	u_char	dummy		: 4;
};
#endif

struct cdd_52x_mode_data {
	struct scsi_mode_header	header;
	union cdd_pagex	{
		struct cdd_52x_mode_page_21	page21;
		struct cdd_52x_mode_page_23	page23;
		struct yamaha_mode_page_31	page31;
	} pagex;
};

EXPORT int
speed_select_yamaha(speed, dummy)
	int	speed;
	int	dummy;
{
	struct cdd_52x_mode_data md;
	int	count;

	fillbytes((caddr_t)&md, sizeof(md), '\0');

	count  = sizeof(struct scsi_mode_header) +
		sizeof(struct yamaha_mode_page_31);

	speed >>= 1;
	md.pagex.page31.p_code = 0x31;
	md.pagex.page31.p_len =  0x02;
	md.pagex.page31.speed = speed;
	md.pagex.page31.dummy = dummy?1:0;
	
	return (mode_select((u_char *)&md, count, 0, inq.data_format >= 2));
}

EXPORT int
speed_select_philips(speed, dummy)
	int	speed;
	int	dummy;
{
	struct cdd_52x_mode_data md;
	int	count;

	fillbytes((caddr_t)&md, sizeof(md), '\0');

	count  = sizeof(struct scsi_mode_header) +
		sizeof(struct cdd_52x_mode_page_23);

	md.pagex.page23.p_code = 0x23;
	md.pagex.page23.p_len =  0x06;
	md.pagex.page23.speed = speed;
	md.pagex.page23.dummy = dummy?1:0;
	
	return (mode_select((u_char *)&md, count, 0, inq.data_format >= 2));
}

EXPORT int
write_track_info(sectype)
	int	sectype;
{
	struct cdd_52x_mode_data md;
	int	count = sizeof(struct scsi_mode_header) +
			sizeof(struct cdd_52x_mode_page_21);

	fillbytes((caddr_t)&md, sizeof(md), '\0');
	md.pagex.page21.p_code = 0x21;
	md.pagex.page21.p_len =  0x0E;
				/* is sectype ok ??? */
	md.pagex.page21.sectype = sectype;
	md.pagex.page21.track = 0;	/* 0 : create new track */
	
	return (mode_select((u_char *)&md, count, 0, inq.data_format >= 2));
}

struct tocheader {
	char	len[2];
	char	first;
	char	last;
};

struct trackdesc {
	char	res0;

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */
	u_char	control:4;
	u_char	adr:4;
#else				/* Motorola byteorder */
	u_char	adr:4;
	u_char	control:4;
#endif

	char	track;
	char	res3;
	char	addr[4];
};

struct diskinfo {
	struct tocheader	hd;
	struct trackdesc	desc[1];
};

struct siheader {
	char	len[2];
	char	finished;
	char	unfinished;
};

struct sidesc {
	char	sess_number;
	char	res1;
	char	track;
	char	res3;
	char	addr[4];
};

struct sinfo {
	struct siheader	hd;
	struct sidesc	desc[1];
};

struct trackheader {
	char	mode;
	char	res[3];
	char	addr[4];
};
#define	TRM_ZERO	0
#define	TRM_USER_ECC	1	/* 2048 bytes user data + 288 Bytes ECC/EDC */
#define	TRM_USER	2	/* All user data (2336 bytes) */


EXPORT	int
read_tochdr(dp, fp, lp)
	cdr_t	*dp;
	int	*fp;
	int	*lp;
{
	struct	tocheader *tp;
	char	xb[256];
	int	len;

	tp = (struct tocheader *)xb;

	fillbytes((caddr_t)xb, sizeof(xb), '\0');
	if (read_toc(xb, 0, sizeof(struct tocheader), 0, FMT_TOC) < 0)
		comerrno(EX_BAD, "Cannot read TOC header\n");
	len = a_to_u_short(tp->len) + sizeof(struct tocheader)-2;
	if (len >= 4) {
		*fp = tp->first;
		*lp = tp->last;
		return (0);
	}
	return (-1);
}
	
EXPORT	int
read_trackinfo(track, offp, msfp, adrp, controlp, modep)
	int	track;
	long	*offp;
	struct msf *msfp;
	int	*adrp;
	int	*controlp;
	int	*modep;
{
	struct	diskinfo *dp;
	char	xb[256];
	int	len;

	dp = (struct diskinfo *)xb;

	fillbytes((caddr_t)xb, sizeof(xb), '\0');
	if (read_toc(xb, track, sizeof(struct diskinfo), 0, FMT_TOC) < 0)
		comerrno(EX_BAD, "Cannot read TOC\n");
	len = a_to_u_short(dp->hd.len) + sizeof(struct tocheader)-2;
	if (len <  sizeof(struct diskinfo))
		return (-1);

	*offp = a_to_u_long(dp->desc[0].addr);
	*adrp = dp->desc[0].adr;
	*controlp = dp->desc[0].control;

	silent++;
	if (read_toc(xb, track, sizeof(struct diskinfo), 1, FMT_TOC) >= 0) {
		msfp->msf_min = dp->desc[0].addr[1];
		msfp->msf_sec = dp->desc[0].addr[2];
		msfp->msf_frame = dp->desc[0].addr[3];
	} else {
		msfp->msf_min = 0;
		msfp->msf_sec = 0;
		msfp->msf_frame = 0;
	}
	silent--;

	if (track == 0xAA) {
		*modep = -1;
		return(0);
	}

	fillbytes((caddr_t)xb, sizeof(xb), '\0');

	silent++;
	if (read_header(xb, *offp, 8, 0) >= 0) {
		*modep = xb[0];
	} else if (read_track_info_philips(xb, track, 14) >= 0) {
		*modep = xb[0xb] & 0xF;
	} else {
		*modep = -1;
	}
	silent--;
	return(0);
}
	
/*
 * Return address of first track in last session (SCSI-3/mmc version).
 */
EXPORT int
read_session_offset(offp)
	long	*offp;
{
	struct	diskinfo *dp;
	char	xb[256];
	int	len;

	dp = (struct diskinfo *)xb;

	fillbytes((caddr_t)xb, sizeof(xb), '\0');
	if (read_toc((caddr_t)xb, 0, sizeof(struct tocheader), 0, FMT_SINFO) < 0)
		return (-1);

	if (verbose)
		scsiprbytes("tocheader: ",
		(u_char *)xb, sizeof(struct tocheader) - scsigetresid());

	len = a_to_u_short(dp->hd.len) + sizeof(struct tocheader)-2;
	if (len > sizeof(xb)) {
		errmsgno(EX_BAD, "Session info too big.\n");
		return (-1);
	}
	if (read_toc((caddr_t)xb, 0, len, 0, FMT_SINFO) < 0)
		return (-1);

	if (verbose)
		scsiprbytes("tocheader: ",
			(u_char *)xb, len - scsigetresid());

	dp = (struct diskinfo *)xb;
	if (offp)
		*offp = a_to_u_long(dp->desc[0].addr);
	return (0);
}

/*
 * Return address of first track in last session (pre SCSI-3 version).
 */
EXPORT int
read_session_offset_philips(offp)
	long	*offp;
{
	struct	sinfo *sp;
	char	xb[256];
	int	len;

	sp = (struct sinfo *)xb;

	fillbytes((caddr_t)xb, sizeof(xb), '\0');
	if (read_toc_philips((caddr_t)xb, 0, sizeof(struct siheader), 0, 1) < 0)
		return (-1);
	len = a_to_u_short(sp->hd.len) + sizeof(struct siheader)-2;
	if (len > sizeof(xb)) {
		errmsgno(EX_BAD, "Session info too big.\n");
		return (-1);
	}
	if (read_toc_philips((caddr_t)xb, 0, len, 0, 1) < 0)
		return (-1);
	/*
	 * Old drives return the number of finished sessions in first/finished
	 * a descriptor is returned for each session. 
	 * New drives return the number of the first and last session
	 * one descriptor for the last finished session is returned
	 * as in SCSI-3
	 * In all cases the lowest session number is set to 1.
	 */
	sp = (struct sinfo *)xb;
	if (offp)
		*offp = a_to_u_long(sp->desc[sp->hd.finished-1].addr);
	return (0);
}

EXPORT int
select_secsize(secsize)
	int	secsize;
{
	struct scsi_mode_data md;
	int	count = sizeof(struct scsi_mode_header) +
			sizeof(struct scsi_mode_blockdesc);

	(void)test_unit_ready();	/* clear any error situation */

	fillbytes((caddr_t)&md, sizeof(md), '\0');
	md.header.blockdesc_len = 8;
	i_to_3_byte(md.blockdesc.lblen, secsize);
	
	return (mode_select((u_char *)&md, count, 0, inq.data_format >= 2));
}

int	dev = DEV_CDD_521;

EXPORT BOOL
is_cddrive()
{
	return (inq.type == INQ_ROMD || inq.type == INQ_WORM);
}

EXPORT BOOL
is_unknown_dev()
{
	return (dev == DEV_UNKNOWN);
}

#define	DEBUG
#ifdef	DEBUG
#define	G0_MAXADDR	0x1FFFFFL

EXPORT int
read_scsi(bp, addr, cnt)
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	if(addr <= G0_MAXADDR && cnt < 256)
		return(read_g0(bp, addr, cnt));
	else
		return(read_g1(bp, addr, cnt));
}

EXPORT int
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

EXPORT int
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

EXPORT BOOL
getdev(print)
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

		/*
		 * Start of CD-Recorders:
		 */
		if (strindex("GRUNDIG", inq.info)) {
			if (strindex("CDR100IPW", inq.ident))
				dev = DEV_CDD_2000;

		} else if (strindex("JVC", inq.info)) {
			if (strindex("XR-W2010", inq.ident))
				dev = DEV_TEAC_CD_R50S;

		} else if (strindex("MITSUMI", inq.info)) {
			/* Don't know any product string */
			dev = DEV_CDD_522;

		} else if (strindex("PHILIPS", inq.info) ||
				strindex("IMS", inq.info) ||
				strindex("KODAK", inq.info) ||
				strindex("HP", inq.info)) {

			if (strindex("CDD521", inq.ident))
				dev = DEV_CDD_521;

			if (strindex("CDD522", inq.ident))
				dev = DEV_CDD_522;
			if (strindex("PCD225", inq.ident))
				dev = DEV_CDD_522;
			if (strindex("KHSW/OB", inq.ident))	/* PCD600 */
				dev = DEV_CDD_522;

			if (strindex("CDD20", inq.ident))
				dev = DEV_CDD_2000;
			if (strindex("CDD26", inq.ident))
				dev = DEV_CDD_2600;

			if (strindex("C4324/C4325", inq.ident))
				dev = DEV_CDD_2000;
			if (strindex("CD-Writer 6020", inq.ident))
				dev = DEV_CDD_2600;

		} else if (strindex("PINNACLE", inq.info)) {
			if (strindex("RCD-5020", inq.ident))
				dev = DEV_TEAC_CD_R50S;
			if (strindex("RCD5040", inq.ident))
				dev = DEV_TEAC_CD_R50S;

		} else if (strindex("PLASMON", inq.info)) {
			if (strindex("RF4100", inq.ident))
				dev = DEV_PLASMON_RF_4100;
			else if (strindex("CDR4220", inq.ident))
				dev = DEV_CDD_2000;

		} else if (strindex("RICOH", inq.info)) {
			if (strindex("RO-1420C", inq.ident))
				dev = DEV_RICOH_RO_1420C;

		} else if (strindex("SAF", inq.info)) {	/* Smart & Friendly */
			if (strindex("CD-R2004", inq.ident))
				dev = DEV_SONY_CDU_924;

		} else if (strindex("SONY", inq.info)) {
			if (strindex("CD-R   CDU92", inq.ident) ||
			    strindex("CD-R   CDU94", inq.ident))
				dev = DEV_SONY_CDU_924;

		} else if (strindex("TEAC", inq.info)) {
			if (strindex("CD-R50S", inq.ident))
				dev = DEV_TEAC_CD_R50S;

		} else if (strindex("T.YUDEN", inq.info)) {
			if (strindex("CD-WO", inq.ident))
				dev = DEV_CDD_521;

		} else if (strindex("YAMAHA", inq.info)) {
			if (strindex("CDR10", inq.ident))
				dev = DEV_YAMAHA_CDR_100;
			if (strindex("CDR200", inq.ident))
				dev = DEV_YAMAHA_CDR_400;
			if (strindex("CDR400", inq.ident))
				dev = DEV_YAMAHA_CDR_400;
		}
		if (dev == DEV_UNKNOWN) {
			/*
			 * We do not have Manufacturer strings for
			 * the following drives.
			 */
			if (strindex("CDS615E", inq.ident))	/* Olympus */
				dev = DEV_SONY_CDU_924;
		}
		if (dev == DEV_UNKNOWN && inq.type == INQ_ROMD) {
			BOOL	cdrr	 = FALSE;
			BOOL	cdwr	 = FALSE;
			BOOL	cdrrw	 = FALSE;
			BOOL	cdwrw	 = FALSE;

			dev = DEV_CDROM;

			if (mmc_check(&cdrr, &cdwr, &cdrrw, &cdwrw))
				dev = DEV_MMC_CDROM;
			if (cdwr)
				dev = DEV_MMC_CDR;
			if (cdwrw)
				dev = DEV_MMC_CDRW;
		}

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

EXPORT void
printdev()
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

	case DEV_CDROM:		printf("Generic CD-ROM");	break;
	case DEV_MMC_CDROM:	printf("Generic mmc CD-ROM");	break;
	case DEV_MMC_CDR:	printf("Generic mmc CD-R");	break;
	case DEV_MMC_CDRW:	printf("Generic mmc CD-RW");	break;
	case DEV_CDD_521:	printf("Philips CDD521");	break;
	case DEV_CDD_522:	printf("Philips CDD522");	break;
	case DEV_CDD_2000:	printf("Philips CDD2000");	break;
	case DEV_CDD_2600:	printf("Philips CDD2600");	break;
	case DEV_YAMAHA_CDR_100:printf("Yamaha CDR-100");	break;
	case DEV_YAMAHA_CDR_400:printf("Yamaha CDR-400");	break;
	case DEV_PLASMON_RF_4100:printf("Plasmon RF-4100");	break;
	case DEV_SONY_CDU_924:	printf("Sony CDU924S");		break;
	case DEV_RICOH_RO_1420C:printf("Ricoh RO-1420C");	break;
	case DEV_TEAC_CD_R50S:	printf("Teac CD-R50S");		break;

	default:		printf("Missing Entry");	break;

	}
	printf(".\n");

}

EXPORT BOOL
do_inquiry(print)
	int	print;
{
	if (getdev(print)) {
		if (print)
			printdev();
		return (TRUE);
	} else {
		return (FALSE);
	}
}

EXPORT BOOL
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
	return (((struct scsi_ext_sense *)&scmd.sense)->sense_code == 0xD0);
}

EXPORT int
scsi_load()
{
	return (scsi_start_stop_unit(1, 1));
}

EXPORT int
scsi_unload()
{
	return (scsi_start_stop_unit(0, 1));
}

EXPORT int
scsi_cdr_write(bufp, sectaddr, size, blocks)
	char	*bufp;
	long	sectaddr;
	long	size;
	int	blocks;
{
	return (write_xg0(bufp, 0, size, blocks));
}

EXPORT BOOL
is_mmc()
{
	return (mmc_check(NULL, NULL, NULL, NULL));
}

EXPORT BOOL
mmc_check(cdrrp, cdwrp, cdrrwp, cdwrwp)
	BOOL	*cdrrp;
	BOOL	*cdwrp;
	BOOL	*cdrrwp;
	BOOL	*cdwrwp;
{
	u_char	mode[0x100];
	int	len;
	struct	cd_mode_page_2A *mp;
extern	struct	scsi_inquiry	inq;

	if (inq.type != INQ_ROMD)
		return (FALSE);

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	silent++;
	if (!get_mode_params(0x2A, "CD capabilities",
			mode, (u_char *)0, (u_char *)0, (u_char *)0, &len)) {
		silent--;
		return (FALSE);
	}
		silent--;

	if (len == 0)			/* Pre SCSI-3/mmc drive	 	*/
		return (FALSE);

	mp = (struct cd_mode_page_2A *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	if (mp->p_len < 0x14)
		return (FALSE);
	if (a_to_u_short(mp->max_read_speed) < 176 ||
			a_to_u_short(mp->cur_read_speed) < 176)
		return (FALSE);

	if (cdrrp)
		*cdrrp = (mp->cd_r_read != 0);	/* SCSI-3/mmc CD	*/
	if (cdwrp)
		*cdwrp = (mp->cd_r_write != 0);	/* SCSI-3/mmc CD-R	*/

	if (cdrrwp)
		*cdrrwp = (mp->cd_rw_read != 0);/* SCSI-3/mmc CD	*/
	if (cdwrwp)
		*cdwrwp = (mp->cd_rw_write != 0);/* SCSI-3/mmc CD-RW	*/

	return (TRUE);			/* Generic SCSI-3/mmc CD	*/
}
