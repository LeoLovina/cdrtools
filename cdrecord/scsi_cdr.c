/* @(#)scsi_cdr.c	1.45 98/04/17 Copyright 1995 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)scsi_cdr.c	1.45 98/04/17 Copyright 1995 J. Schilling";
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
#include <mconfig.h>

#include <stdio.h>
#include <standard.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>

#include <utypes.h>
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
EXPORT	BOOL	wait_unit_ready	__PR((int secs));
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
EXPORT	int	read_disk_info	__PR((caddr_t, int));
EXPORT	int	read_track_info	__PR((caddr_t, int, int));
EXPORT	int	read_track_info_philips	__PR((caddr_t, int, int));
EXPORT	int	close_track_philips __PR((int track, track_t *trackp));
EXPORT	int	fixation	__PR((int, int, int, int tracks, track_t *trackp));
EXPORT	int	scsi_close_tr_session __PR((int type, int track));
EXPORT	int	scsi_blank	__PR((long addr, int blanktype));
EXPORT	int	recover		__PR((int));
EXPORT	int	first_writable_addr __PR((long *, int, int, int, int));
EXPORT	int	reserve_track	__PR((unsigned long));
EXPORT	BOOL	allow_atapi	__PR((BOOL new));
EXPORT	int	mode_select	__PR((u_char *, int, int, int));
EXPORT	int	mode_sense	__PR((u_char *dp, int cnt, int page, int pcf));
EXPORT	int	mode_select_sg0	__PR((u_char *, int, int, int));
EXPORT	int	mode_sense_sg0	__PR((u_char *dp, int cnt, int page, int pcf));
EXPORT	int	mode_select_g0	__PR((u_char *, int, int, int));
EXPORT	int	mode_select_g1	__PR((u_char *, int, int, int));
EXPORT	int	mode_sense_g0	__PR((u_char *dp, int cnt, int page, int pcf));
EXPORT	int	mode_sense_g1	__PR((u_char *dp, int cnt, int page, int pcf));
EXPORT	int	speed_select_yamaha	__PR((int speed, int dummy));
EXPORT	int	speed_select_philips	__PR((int speed, int dummy));
EXPORT	int	write_track_info __PR((int));
EXPORT	int	read_tochdr	__PR((cdr_t *, int *, int *));
EXPORT	int	read_trackinfo	__PR((int, long *, struct msf *, int *, int *, int *));
EXPORT	int	read_B0		__PR((BOOL isbcd, long *b0p, long *lop));
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
EXPORT	int	scsi_cdr_write	__PR((caddr_t bp, long sectaddr, long size, int blocks, BOOL islast));
EXPORT	struct cd_mode_page_2A * mmc_cap __PR((u_char *modep));
EXPORT	void	mmc_getval	__PR((struct cd_mode_page_2A *mp,
					BOOL *cdrrp, BOOL *cdwrp,
					BOOL *cdrrwp, BOOL *cdwrwp,
					BOOL *dvdp));
EXPORT	BOOL	is_mmc		__PR((void));
EXPORT	BOOL	mmc_check	__PR((BOOL *cdrrp, BOOL *cdwrp,
					BOOL *cdrrwp, BOOL *cdwrwp,
					BOOL *dvdp));
EXPORT	void	print_capabilities __PR((void));


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
	if (((struct scsi_ext_sense *)&scmd.sense)->key == SC_UNIT_ATTENTION) {
		if (test_unit_ready() >= 0)	/* alles OK */
			return (TRUE);
	}
						/* FALSE wenn NOT_READY */
	return (((struct scsi_ext_sense *)&scmd.sense)->key != SC_NOT_READY);
}

EXPORT BOOL
wait_unit_ready(secs)
	int	secs;
{
	int	i;
	int	c;
	int	k;
	int	ret;

	silent++;
	ret = test_unit_ready();		/* eat up unit attention */
	silent--;

	if (ret >= 0)				/* success that's enough */
		return (TRUE);

	silent++;
	for (i=0; i < secs && (ret = test_unit_ready()) < 0; i++) {
		if (scmd.scb.busy != 0) {
			sleep(1);
			continue;
		}
		c = scsi_sense_code();
		k = scsi_sense_key();
		if (k == SC_NOT_READY && (c == 0x3A || c == 0x30)) {
			scsiprinterr("test_unit_ready");
			silent--;
			return (FALSE);
		}
		sleep(1);
	}
	silent--;
	if (ret < 0)
		return (FALSE);
	return (TRUE);
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
read_toc_philips(bp, track, cnt, msf, fmt)
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
	scmd.cdb.g1_cdb.res6 = track;
	g1_cdblen(&scmd.cdb.g1_cdb, cnt);

	if (fmt & 1)
		scmd.cdb.g1_cdb.vu_96 = 1;
	if (fmt & 2)
		scmd.cdb.g1_cdb.vu_97 = 1;

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
read_disk_info(bp, cnt)
	caddr_t	bp;
	int	cnt;

{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = bp;
	scmd.size = cnt;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.timeout = 4 * 60;		/* Needs up to 2 minutes */
	scmd.cdb.g1_cdb.cmd = 0x51;
	scmd.cdb.g1_cdb.lun = lun;
	g1_cdblen(&scmd.cdb.g1_cdb, cnt);

	if (scsicmd("read disk info") < 0)
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
	scmd.timeout = 4 * 60;		/* Needs up to 2 minutes */
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
close_track_philips(track, trackp)
	int	track;
	track_t	*trackp;
{
	return (scsi_flush_cache());
}

EXPORT int
fixation(onp, dummy, type, tracks, trackp)
	int	onp;	/* open next program area */
	int	dummy;
	int	type;	/* TOC type 0: CD-DA, 1: CD-ROM, 2: CD-ROM/XA1, 3: CD-ROM/XA2, 4: CDI */
	int	tracks;
	track_t	*trackp;
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
scsi_close_tr_session(type, track)
	int	type;
	int	track;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.timeout = 8 * 60;		/* Needs up to 4 minutes */
	scmd.cdb.g1_cdb.cmd = 0x5B;
	scmd.cdb.g1_cdb.lun = lun;
	scmd.cdb.g1_cdb.addr[0] = type;
	scmd.cdb.g1_cdb.addr[3] = track;
	
	if (scsicmd("close track/session") < 0)
		return (-1);
	return (0);
}

EXPORT int
scsi_blank(addr, blanktype)
	long	addr;
	int	blanktype;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G5_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.timeout = 160 * 60; /* full blank at 1x could take 80 minutes */
	scmd.cdb.g5_cdb.cmd = 0xA1;	/* Blank */
	scmd.cdb.g0_cdb.high_addr = blanktype;
	g1_cdbaddr(&scmd.cdb.g5_cdb, addr);

	return (scsicmd("blank unit"));
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

/*
 * XXX First try to handle ATAPI:
 * XXX ATAPI cannot handle SCSI 6 byte commands.
 * XXX We try to simulate 6 byte mode sense/select.
 */
LOCAL BOOL	is_atapi;

EXPORT BOOL
allow_atapi(new)
	BOOL	new;
{
	BOOL	old = is_atapi;
	u_char	mode[256];

	if (new == old)
		return (old);

	silent++;
	if (new &&
	    mode_sense_g1(mode, 8, 0x3F, 0) < 0) {	/* All pages current */
		new = FALSE;
	}
	silent--;

	is_atapi = new;
	return (old);
}

EXPORT int
mode_select(dp, cnt, smp, pf)
	u_char	*dp;
	int	cnt;
	int	smp;
	int	pf;
{
	if (is_atapi)
		return (mode_select_sg0(dp, cnt, smp, pf));
	return (mode_select_g0(dp, cnt, smp, pf));
}

EXPORT int
mode_sense(dp, cnt, page, pcf)
	u_char	*dp;
	int	cnt;
	int	page;
	int	pcf;
{
	if (is_atapi)
		return (mode_sense_sg0(dp, cnt, page, pcf));
	return (mode_sense_g0(dp, cnt, page, pcf));
}

/*
 * Simulate mode select g0 with mode select g1.
 */
EXPORT int
mode_select_sg0(dp, cnt, smp, pf)
	u_char	*dp;
	int	cnt;
	int	smp;
	int	pf;
{
	u_char	xmode[256+4];
	int	amt = cnt;

	movebytes(&dp[4], &xmode[8], cnt-4);
	if (amt < 4) {		/* Data length. medium type & VU */
		amt += 1;
	} else {
		amt += 4;
	}
	xmode[0] = 0;
	xmode[1] = 0;
	xmode[2] = dp[1];
	xmode[3] = dp[2];
	xmode[4] = 0;
	xmode[5] = 0;
	i_to_short(&xmode[6], dp[3]);

	if (verbose) scsiprbytes("Mode Parameters (un-converted)", dp, cnt);

	return (mode_select_g1(xmode, amt, smp, pf));
}

/*
 * Simulate mode sense g0 with mode sense g1.
 */
EXPORT int
mode_sense_sg0(dp, cnt, page, pcf)
	u_char	*dp;
	int	cnt;
	int	page;
	int	pcf;
{
	u_char	xmode[256+4];
	int	amt = cnt;
	int	len;

	fillbytes((caddr_t)xmode, sizeof(scmd), '\0');
	if (amt < 4) {		/* Data length. medium type & VU */
		amt += 1;
	} else {
		amt += 4;
	}
	if (mode_sense_g1(xmode, amt, page, pcf) < 0)
		return (-1);

	amt -= scsigetresid();
	if (amt > 4)
		movebytes(&xmode[8], &dp[4], amt-4);
	len = a_to_u_short(xmode);
	if (len == 0) {
		dp[0] = 0;
	} else if (len < 6) {
		if (len > 2)
			len = 2;
		dp[0] = len;
	} else {
		dp[0] = len - 3;
	}
	dp[1] = xmode[2];
	dp[2] = xmode[3];
	len = a_to_u_short(&xmode[6]);
	dp[3] = len;

	if (verbose) scsiprbytes("Mode Sense Data (converted)", dp, cnt - scmd.resid);
	return (0);
}

EXPORT int
mode_select_g0(dp, cnt, smp, pf)
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

	if (verbose) {
		printf("%s ", smp?"Save":"Set ");
		scsiprbytes("Mode Parameters", dp, cnt);
	}

	return (scsicmd("mode select g0"));
}

EXPORT int
mode_select_g1(dp, cnt, smp, pf)
	u_char	*dp;
	int	cnt;
	int	smp;
	int	pf;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)dp;
	scmd.size = cnt;
	scmd.flags = SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0x55;
	scmd.cdb.g1_cdb.lun = lun;
	scmd.cdb.g0_cdb.high_addr = smp ? 1 : 0 | pf ? 0x10 : 0;
	g1_cdblen(&scmd.cdb.g1_cdb, cnt);

	if (verbose) {
		printf("%s ", smp?"Save":"Set ");
		scsiprbytes("Mode Parameters", dp, cnt);
	}

	return (scsicmd("mode select g1"));
}

EXPORT int
mode_sense_g0(dp, cnt, page, pcf)
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

	if (scsicmd("mode sense g0") < 0)
		return (-1);
	if (verbose) scsiprbytes("Mode Sense Data", dp, cnt - scmd.resid);
	return (0);
}

EXPORT int
mode_sense_g1(dp, cnt, page, pcf)
	u_char	*dp;
	int	cnt;
	int	page;
	int	pcf;
{
	fillbytes((caddr_t)&scmd, sizeof(scmd), '\0');
	scmd.addr = (caddr_t)dp;
	scmd.size = cnt;
	scmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd.cdb_len = SC_G1_CDBLEN;
	scmd.sense_len = CCS_SENSE_LEN;
	scmd.target = target;
	scmd.cdb.g1_cdb.cmd = 0x5A;
	scmd.cdb.g1_cdb.lun = lun;
#ifdef	nonono
	scmd.cdb.g0_cdb.high_addr = 1<<4;	/* DBD Disable Block desc. */
#endif
	scmd.cdb.g1_cdb.addr[0] = (page&0x3F) | ((pcf<<6)&0xC0);
	g1_cdblen(&scmd.cdb.g1_cdb, cnt);

	if (scsicmd("mode sense g1") < 0)
		return (-1);
	if (verbose) scsiprbytes("Mode Sense Data", dp, cnt - scmd.resid);
	return (0);
}

struct cdd_52x_mode_page_21 {	/* write track information */
		MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0E = 14 Bytes */
	u_char	res_2;
	u_char	sectype;
	u_char	track;
	u_char	ISRC[9];
	u_char	res[2];
};

struct cdd_52x_mode_page_23 {	/* speed selection */
		MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x06 = 6 Bytes */
	u_char	speed;
	u_char	dummy;
	u_char	res[4];

};

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct yamaha_mode_page_31 {	/* drive configuration */
		MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x02 = 2 Bytes */
	u_char	res;
	Ucbit	dummy		: 4;
	Ucbit	speed		: 4;
};

#else				/* Motorola byteorder */

struct yamaha_mode_page_31 {	/* drive configuration */
		MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x02 = 2 Bytes */
	u_char	res;
	Ucbit	speed		: 4;
	Ucbit	dummy		: 4;
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
	Uchar	len[2];
	Uchar	first;
	Uchar	last;
};

struct trackdesc {
	Uchar	res0;

#if defined(_BIT_FIELDS_LTOH)		/* Intel byteorder */
	Ucbit	control		: 4;
	Ucbit	adr		: 4;
#else					/* Motorola byteorder */
	Ucbit	adr		: 4;
	Ucbit	control		: 4;
#endif

	Uchar	track;
	Uchar	res3;
	Uchar	addr[4];
};

struct diskinfo {
	struct tocheader	hd;
	struct trackdesc	desc[1];
};

struct siheader {
	Uchar	len[2];
	Uchar	finished;
	Uchar	unfinished;
};

struct sidesc {
	Uchar	sess_number;
	Uchar	res1;
	Uchar	track;
	Uchar	res3;
	Uchar	addr[4];
};

struct sinfo {
	struct siheader	hd;
	struct sidesc	desc[1];
};

struct trackheader {
	Uchar	mode;
	Uchar	res[3];
	Uchar	addr[4];
};
#define	TRM_ZERO	0
#define	TRM_USER_ECC	1	/* 2048 bytes user data + 288 Bytes ECC/EDC */
#define	TRM_USER	2	/* All user data (2336 bytes) */

struct ftrackdesc {
	Uchar	sess_number;

#if defined(_BIT_FIELDS_LTOH)		/* Intel byteorder */
	Ucbit	control		: 4;
	Ucbit	adr		: 4;
#else					/* Motorola byteorder */
	Ucbit	adr		: 4;
	Ucbit	control		: 4;
#endif

	Uchar	track;
	Uchar	point;
	Uchar	amin;
	Uchar	asec;
	Uchar	aframe;
	Uchar	res7;
	Uchar	pmin;
	Uchar	psec;
	Uchar	pframe;
};

struct fdiskinfo {
	struct tocheader	hd;
	struct ftrackdesc	desc[1];
};


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
	if (read_toc(xb, 0, sizeof(struct tocheader), 0, FMT_TOC) < 0) {
		if (silent == 0)
			errmsgno(EX_BAD, "Cannot read TOC header\n");
		return (-1);
	}
	len = a_to_u_short(tp->len) + sizeof(struct tocheader)-2;
	if (len >= 4) {
		if (fp)
			*fp = tp->first;
		if (lp)
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
	if (read_toc(xb, track, sizeof(struct diskinfo), 0, FMT_TOC) < 0) {
		errmsgno(EX_BAD, "Cannot read TOC\n");
		return (-1);
	}
	len = a_to_u_short(dp->hd.len) + sizeof(struct tocheader)-2;
	if (len <  sizeof(struct diskinfo))
		return (-1);

	if (offp)
		*offp = a_to_u_long(dp->desc[0].addr);
	if (adrp)
		*adrp = dp->desc[0].adr;
	if (controlp)
		*controlp = dp->desc[0].control;

	if (msfp) {
		silent++;
		if (read_toc(xb, track, sizeof(struct diskinfo), 1, FMT_TOC)
									>= 0) {
			msfp->msf_min = dp->desc[0].addr[1];
			msfp->msf_sec = dp->desc[0].addr[2];
			msfp->msf_frame = dp->desc[0].addr[3];
		} else {
			msfp->msf_min = 0;
			msfp->msf_sec = 0;
			msfp->msf_frame = 0;
		}
		silent--;
	}

	if (modep == NULL)
		return (0);

	if (track == 0xAA) {
		*modep = -1;
		return (0);
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
	return (0);
}

EXPORT	int
read_B0(isbcd, b0p, lop)
	BOOL	isbcd;
	long	*b0p;
	long	*lop;
{
	struct	fdiskinfo *dp;
	struct	ftrackdesc *tp;
	char	xb[8192];
	char	*pe;
	int	len;
	long	l;

	dp = (struct fdiskinfo *)xb;

	fillbytes((caddr_t)xb, sizeof(xb), '\0');
	if (read_toc_philips(xb, 1, sizeof(struct tocheader), 0, FMT_FULLTOC) < 0) {
		return (-1);
	}
	len = a_to_u_short(dp->hd.len) + sizeof(struct tocheader)-2;
	if (len <  sizeof(struct fdiskinfo))
		return (-1);
	if (read_toc_philips(xb, 1, len, 0, FMT_FULLTOC) < 0) {
		return (-1);
	}
	if (verbose) {
		scsiprbytes("TOC data: ", (Uchar *)xb,
			len > sizeof(xb) - scsigetresid() ? sizeof(xb) - scsigetresid() : len);

		tp = &dp->desc[0];
		pe = &xb[len];

		while ((char *)tp < pe) {
			scsiprbytes("ENT: ", (Uchar *)tp, 11);
			tp++;
		}
	}
	tp = &dp->desc[0];
	pe = &xb[len];

	for (; (char *)tp < pe; tp++) {
		if (tp->sess_number != dp->hd.last)
			continue;
		if (tp->point != 0xB0)
			continue;
		if (verbose)
			scsiprbytes("B0: ", (Uchar *)tp, 11);
		if (isbcd) {
			l = msf_to_lba(from_bcd(tp->amin),
				from_bcd(tp->asec),
				from_bcd(tp->aframe));
		} else {
			l = msf_to_lba(tp->amin,
				tp->asec,
				tp->aframe);
		}
		if (b0p)
			*b0p = l;

		if (verbose)
			printf("B0 start: %ld\n", l);

		if (isbcd) {
			l = msf_to_lba(from_bcd(tp->pmin),
				from_bcd(tp->psec),
				from_bcd(tp->pframe));
		} else {
			l = msf_to_lba(tp->pmin,
				tp->psec,
				tp->pframe);
		}

		if (verbose)
			printf("B0 lout: %ld\n", l);
		if (lop)
			*lop = l;
		return (0);
	}
	return (-1);
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
	if (read_toc_philips((caddr_t)xb, 0, sizeof(struct siheader), 0, FMT_SINFO) < 0)
		return (-1);
	len = a_to_u_short(sp->hd.len) + sizeof(struct siheader)-2;
	if (len > sizeof(xb)) {
		errmsgno(EX_BAD, "Session info too big.\n");
		return (-1);
	}
	if (read_toc_philips((caddr_t)xb, 0, len, 0, FMT_SINFO) < 0)
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
		return (read_g0(bp, addr, cnt));
	else
		return (read_g1(bp, addr, cnt));
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
	} else if (verbose) {
		int	i;
		int	len = inq.add_len + 5;
		Uchar	ibuf[256+5];
		Uchar	*ip = (Uchar *)&inq;
		Uchar	c;

		if (len > sizeof (inq) && inquiry((caddr_t)ibuf, inq.add_len+5) >= 0) {
			len = inq.add_len+5 - scsigetresid();
			ip = ibuf;
		} else {
			len = sizeof (inq);
		}
		printf("Inquiry Data   : ");
		for (i = 0; i < len; i++) {
			c = ip[i];
			if (c >= ' ' && c < 0177)
				printf("%c", c);
			else
				printf(".");
		}
		printf("\n");
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
			if (strindex("XR-W2001", inq.ident))
				dev = DEV_TEAC_CD_R50S;
			else if (strindex("XR-W2010", inq.ident))
				dev = DEV_TEAC_CD_R50S;
			else if (strindex("R2626", inq.ident))
				dev = DEV_TEAC_CD_R50S;

		} else if (strindex("MITSBISH", inq.info)) {

#ifdef	XXXX_REALLY
			/* It's MMC compliant */
			if (strindex("CDRW226", inq.ident))
				dev = DEV_MMC_CDRW;
#endif

		} else if (strindex("MITSUMI", inq.info)) {
			/* Don't know any product string */
			dev = DEV_CDD_522;

		} else if (strindex("PHILIPS", inq.info) ||
				strindex("IMS", inq.info) ||
				strindex("KODAK", inq.info) ||
				strindex("HP", inq.info)) {

			if (strindex("CDD521/00", inq.ident))
				dev = DEV_CDD_521_OLD;
			else if (strindex("CDD521", inq.ident))
				dev = DEV_CDD_521;

			if (strindex("CDD522", inq.ident))
				dev = DEV_CDD_522;
			if (strindex("PCD225", inq.ident))
				dev = DEV_CDD_522;
			if (strindex("KHSW/OB", inq.ident))	/* PCD600 */
				dev = DEV_CDD_522;
			if (strindex("CDR-240", inq.ident))
				dev = DEV_CDD_2000;

			if (strindex("CDD20", inq.ident))
				dev = DEV_CDD_2000;
			if (strindex("CDD26", inq.ident))
				dev = DEV_CDD_2600;

			if (strindex("C4324/C4325", inq.ident))
				dev = DEV_CDD_2000;
			if (strindex("CD-Writer 6020", inq.ident))
				dev = DEV_CDD_2600;

		} else if (strindex("PINNACLE", inq.info)) {
			if (strindex("RCD-1000", inq.ident))
				dev = DEV_TEAC_CD_R50S;
			if (strindex("RCD5020", inq.ident))
				dev = DEV_TEAC_CD_R50S;
			if (strindex("RCD5040", inq.ident))
				dev = DEV_TEAC_CD_R50S;
			if (strindex("RCD 4X4", inq.ident))
				dev = DEV_TEAC_CD_R50S;

		} else if (strindex("PIONEER", inq.info)) {
			if (strindex("CD-WO DW-S114X", inq.ident))
				dev = DEV_PIONEER_DW_S114X;
			else if (strindex("DVD-R DVR-S101", inq.ident))
				dev = DEV_PIONEER_DVDR_S101;

		} else if (strindex("PLASMON", inq.info)) {
			if (strindex("RF4100", inq.ident))
				dev = DEV_PLASMON_RF_4100;
			else if (strindex("CDR4220", inq.ident))
				dev = DEV_CDD_2000;

		} else if (strindex("PLEXTOR", inq.info)) {
			if (strindex("CD-R   PX-R24CS", inq.ident))
				dev = DEV_RICOH_RO_1420C;

		} else if (strindex("RICOH", inq.info)) {
			if (strindex("RO-1420C", inq.ident))
				dev = DEV_RICOH_RO_1420C;

		} else if (strindex("SAF", inq.info)) {	/* Smart & Friendly */
			if (strindex("CD-R2004", inq.ident) ||
			    strindex("CD-R2006 ", inq.ident))
				dev = DEV_SONY_CDU_924;
			else if (strindex("CD-R2006PLUS", inq.ident))
				dev = DEV_TEAC_CD_R50S;
			else if (strindex("CD-RW226", inq.ident))
				dev = DEV_TEAC_CD_R50S;
			else if (strindex("CD-R4012", inq.ident))
				dev = DEV_TEAC_CD_R50S;

		} else if (strindex("SONY", inq.info)) {
			if (strindex("CD-R   CDU92", inq.ident) ||
			    strindex("CD-R   CDU94", inq.ident))
				dev = DEV_SONY_CDU_924;

		} else if (strindex("TEAC", inq.info)) {
			if (strindex("CD-R50S", inq.ident) ||
			    strindex("CD-R55S", inq.ident))
				dev = DEV_TEAC_CD_R50S;

		} else if (strindex("TRAXDATA", inq.info) ||
				strindex("Traxdata", inq.info)) {
			if (strindex("CDR4120", inq.ident))
				dev = DEV_TEAC_CD_R50S;

		} else if (strindex("T.YUDEN", inq.info)) {
			if (strindex("CD-WO EW-50", inq.ident))
				dev = DEV_CDD_521;

		} else if (strindex("WPI", inq.info)) {	/* Wearnes */
			if (strindex("CDR-632P", inq.ident))
				dev = DEV_CDD_2600;

		} else if (strindex("YAMAHA", inq.info)) {
			if (strindex("CDR10", inq.ident))
				dev = DEV_YAMAHA_CDR_100;
			if (strindex("CDR200", inq.ident))
				dev = DEV_YAMAHA_CDR_400;
			if (strindex("CDR400", inq.ident))
				dev = DEV_YAMAHA_CDR_400;

		} else if (strindex("MATSHITA", inq.info)) {
			if (strindex("CD-R   CW-7501", inq.ident))
				dev = DEV_MATSUSHITA_7501;
			if (strindex("CD-R   CW-7502", inq.ident))
				dev = DEV_MATSUSHITA_7502;
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
			BOOL	dvd	 = FALSE;

			dev = DEV_CDROM;

			if (mmc_check(&cdrr, &cdwr, &cdrrw, &cdwrw, &dvd))
				dev = DEV_MMC_CDROM;
			if (cdwr)
				dev = DEV_MMC_CDR;
			if (cdwrw)
				dev = DEV_MMC_CDRW;
			if (dvd)
				dev = DEV_MMC_DVD;
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
	case DEV_MMC_DVD:	printf("Generic mmc2 DVD");	break;
	case DEV_CDD_521:	printf("Philips CDD-521");	break;
	case DEV_CDD_521_OLD:	printf("Philips old CDD-521");	break;
	case DEV_CDD_522:	printf("Philips CDD-522");	break;
	case DEV_CDD_2000:	printf("Philips CDD-2000");	break;
	case DEV_CDD_2600:	printf("Philips CDD-2600");	break;
	case DEV_YAMAHA_CDR_100:printf("Yamaha CDR-100");	break;
	case DEV_YAMAHA_CDR_400:printf("Yamaha CDR-400");	break;
	case DEV_PLASMON_RF_4100:printf("Plasmon RF-4100");	break;
	case DEV_SONY_CDU_924:	printf("Sony CDU-924S");	break;
	case DEV_RICOH_RO_1420C:printf("Ricoh RO-1420C");	break;
	case DEV_TEAC_CD_R50S:	printf("Teac CD-R50S");		break;
	case DEV_MATSUSHITA_7501:printf("Matsushita CW-7501");	break;
	case DEV_MATSUSHITA_7502:printf("Matsushita CW-7502");	break;

	case DEV_PIONEER_DW_S114X: printf("Pioneer DW-S114X");	break;
	case DEV_PIONEER_DVDR_S101:printf("Pioneer DVD-R S101");break;

	default:		printf("Missing Entry for dev %d",
							 dev);	break;

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
scsi_cdr_write(bp, sectaddr, size, blocks, islast)
	caddr_t	bp;		/* address of buffer */
	long	sectaddr;	/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	blocks;		/* sector count */
	BOOL	islast;		/* last write for track */
{
	return (write_xg1(bp, sectaddr, size, blocks));
}

EXPORT struct cd_mode_page_2A *
mmc_cap(modep)
	u_char	*modep;
{
	int	len;
	int	val;
	u_char	mode[0x100];
	struct	cd_mode_page_2A *mp;
	struct	cd_mode_page_2A *mp2;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	if (!get_mode_params(0x2A, "CD capabilities",
			mode, (u_char *)0, (u_char *)0, (u_char *)0, &len))
		return (NULL);

	if (len == 0)			/* Pre SCSI-3/mmc drive	 	*/
		return (NULL);

	mp = (struct cd_mode_page_2A *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	/*
	 * Do some heuristics against pre SCSI-3/mmc VU page 2A
	 */
	if (mp->p_len < 0x14)
		return (NULL);

	val = a_to_u_short(mp->max_read_speed);
	if (val != 0 && val < 176)
		return (NULL);

	val = a_to_u_short(mp->cur_read_speed);
	if (val != 0 && val < 176)
		return (NULL);

	len -= sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len;
	if (modep)
		mp2 = (struct cd_mode_page_2A *)modep;
	else
		mp2 = malloc(len);
	if (mp2)
		movebytes(mp, mp2, len);

	return (mp2);
}

EXPORT void
mmc_getval(mp, cdrrp, cdwrp, cdrrwp, cdwrwp, dvdp)
	struct	cd_mode_page_2A *mp;
	BOOL	*cdrrp;
	BOOL	*cdwrp;
	BOOL	*cdrrwp;
	BOOL	*cdwrwp;
	BOOL	*dvdp;
{
	if (cdrrp)
		*cdrrp = (mp->cd_r_read != 0);	/* SCSI-3/mmc CD	*/
	if (cdwrp)
		*cdwrp = (mp->cd_r_write != 0);	/* SCSI-3/mmc CD-R	*/

	if (cdrrwp)
		*cdrrwp = (mp->cd_rw_read != 0);/* SCSI-3/mmc CD	*/
	if (cdwrwp)
		*cdwrwp = (mp->cd_rw_write != 0);/* SCSI-3/mmc CD-RW	*/
	if (dvdp) {
		*dvdp = 			/* SCSI-3/mmc2 DVD 	*/
		(mp->dvd_ram_read + mp->dvd_r_read  + mp->dvd_rom_read +
		 mp->dvd_ram_write + mp->dvd_r_write) != 0;
	}
}

EXPORT BOOL
is_mmc()
{
	return (mmc_check(NULL, NULL, NULL, NULL, NULL));
}

EXPORT BOOL
mmc_check(cdrrp, cdwrp, cdrrwp, cdwrwp, dvdp)
	BOOL	*cdrrp;
	BOOL	*cdwrp;
	BOOL	*cdrrwp;
	BOOL	*cdwrwp;
	BOOL	*dvdp;
{
	u_char	mode[0x100];
	BOOL	was_atapi;
	struct	cd_mode_page_2A *mp;

	if (inq.type != INQ_ROMD)
		return (FALSE);

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	was_atapi = allow_atapi(TRUE);
	silent++;
	mp = mmc_cap(mode);
	silent--;
	allow_atapi(was_atapi);
	if (mp == NULL)
		return (FALSE);

	mmc_getval(mp, cdrrp, cdwrp, cdrrwp, cdwrwp, dvdp);

	return (TRUE);			/* Generic SCSI-3/mmc CD	*/
}

#define	DOES(what,flag)	printf("  Does %s%s\n", flag?"":"not ",what);
#define	IS(what,flag)	printf("  Is %s%s\n", flag?"":"not ",what);
#define	VAL(what,val)	printf("  %s: %d\n", what, val[0]*256 + val[1]);
#define	SVAL(what,val)	printf("  %s: %s\n", what, val);

EXPORT void
print_capabilities()
{
	BOOL	was_atapi;
	u_char	mode[0x100];
	struct	cd_mode_page_2A *mp;
static	const	char	*bclk[4] = {"32", "16", "24", "24 (I2S)"};
static	const	char	*load[8] = {"caddy", "tray", "pop-up", "reserved(3)",
				"disc changer", "cartridge changer",
				"reserved(6)", "reserved(7)" };


	if (inq.type != INQ_ROMD)
		return;

	fillbytes((caddr_t)mode, sizeof(mode), '\0');

	was_atapi = allow_atapi(TRUE);	/* Try to switch to 10 byte mode cmds */
	silent++;
	mp = mmc_cap(mode);
	silent--;
	allow_atapi(was_atapi);
	if (mp == NULL)
		return;

	printf ("\nDrive capabilities, per page 2A:\n\n");

	DOES("read CD-R media", mp->cd_r_read);
	DOES("write CD-R media", mp->cd_r_write);
	DOES("read CD-RW media", mp->cd_rw_read);
	DOES("write CD-RW media", mp->cd_rw_write);
	DOES("read DVD-ROM media", mp->dvd_rom_read);
	DOES("read DVD-R media", mp->dvd_r_read);
	DOES("write DVD-R media", mp->dvd_r_write);
	DOES("read DVD-RAM media", mp->dvd_ram_read);
	DOES("write DVD-RAM media", mp->dvd_ram_write);
	DOES("support test writing", mp->test_write);
	printf("\n");
	DOES("read Mode 2 Form 1 blocks", mp->mode_2_form_1);
	DOES("read Mode 2 Form 2 blocks", mp->mode_2_form_2);
	DOES("read digital audio blocks", mp->cd_da_supported);
	if (mp->cd_da_supported) 
		DOES("restart non-streamed digital audio reads accurately", mp->cd_da_accurate);
	DOES("read multi-session CDs", mp->multi_session);
	DOES("read fixed-packet CD media using Method 2", mp->method2);
	DOES("read CD bar code", mp->read_bar_code);
	DOES("read R-W subcode information", mp->rw_supported);
	if (mp->rw_supported) 
		DOES("return R-W subcode de-interleaved and error-corrected", mp->rw_deint_corr);
	DOES("return CD media catalog number", mp->UPC);
	DOES("return CD ISRC information", mp->ISRC);
	DOES("support C2 error pointers", mp->c2_pointers);
	DOES("deliver composite A/V data", mp->composite);
	printf("\n");
	DOES("play audio CDs", mp->audio_play);
	if (mp->audio_play) {
		VAL("Number of volume control levels", mp->num_vol_levels);
		DOES("support individual volume control setting for each channel", mp->sep_chan_vol);
		DOES("support independent mute setting for each channel", mp->sep_chan_mute);
		DOES("support digital output on port 1", mp->digital_port_1);
		DOES("support digital output on port 2", mp->digital_port_2);
		if (mp->digital_port_1 || mp->digital_port_2) {
			DOES("send digital data LSB-first", mp->LSBF);
			DOES("set LRCK high for left-channel data", mp->RCK);
			DOES("have valid data on falling edge of clock", mp->BCK);
			SVAL("Length of data in BCLKs", bclk[mp->length]);
		}
	}
	printf("\n");
	SVAL("Loading mechanism type", load[mp->loading_type]);
	DOES("support ejection of CD via START/STOP command", mp->eject);
	DOES("lock media on power up via prevent jumper", mp->prevent_jumper);
	DOES("allow media to be locked in the drive via PREVENT/ALLOW command", mp->lock);
	IS("currently in a media-locked state", mp->lock_state);
	DOES("have load-empty-slot-in-changer feature", mp->sw_slot_sel);
	DOES("support Individual Disk Present feature", mp->disk_present_rep);
	printf("\n");
	VAL("Maximum read  speed in kB/s", mp->max_read_speed);
	VAL("Current read  speed in kB/s", mp->cur_read_speed);
	VAL("Maximum write speed in kB/s", mp->max_write_speed);
	VAL("Current write speed in kB/s", mp->cur_write_speed);
	VAL("Buffer size in KB", mp->buffer_size);

	return;			/* Generic SCSI-3/mmc CD	*/
}
