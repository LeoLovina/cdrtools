/* @(#)readcd.c	1.14 00/04/24 Copyright 1987 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)readcd.c	1.14 00/04/24 Copyright 1987 J. Schilling";
#endif
/*
 *	Skeleton for the use of the scg genearal SCSI - driver
 *
 *	Copyright (c) 1987 J. Schilling
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
#include <unixstd.h>
#include <stdxlib.h>
#include <strdefs.h>
#include <fctldefs.h>
#include <timedefs.h>

#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "cdrecord.h"

char	cdr_version[] = "1.8.1";

extern	BOOL	getlong		__PR((char *, long *, long, long));
extern	BOOL	getint		__PR((char *, int *, int, int));

typedef struct {
	long	start;
	long	end;
	char	*name;
} parm_t;

EXPORT	BOOL cvt_cyls		__PR((void));
EXPORT	BOOL cvt_bcyls		__PR((void));
EXPORT	void print_defect_list	__PR((void));
LOCAL	void usage		__PR((int ret));
EXPORT	int main		__PR((int ac, char **av));
LOCAL	int prstats		__PR((void));
LOCAL	void dorw		__PR((SCSI *scgp, char *filename, char* sectors));
LOCAL	void doit		__PR((SCSI *scgp));
#ifdef	used
LOCAL	int read_scsi_g1	__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
#endif

EXPORT	int	write_scsi	__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
EXPORT	int	write_g0	__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
EXPORT	int	write_g1	__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));

#ifdef	used
LOCAL	void	Xrequest_sense	__PR((SCSI *scgp));
#endif
LOCAL	int	rrr		__PR((SCSI *scgp, caddr_t bp, long addr, long cnt));
LOCAL	void	read_disk	__PR((SCSI	*scgp, parm_t *parmp));
LOCAL	void	write_disk	__PR((SCSI *scgp, parm_t *parmp));
LOCAL	int	choice		__PR((int n));
LOCAL	void	ra		__PR((SCSI *scgp));

#ifndef	TRUE
#	define	TRUE	1
#	define	FALSE	0
#endif

struct timeval	starttime;
struct timeval	stoptime;

char	*Sbuf;
long	Sbufsize;

int	help;
BOOL	is_suid;
BOOL	is_cdrom;
BOOL	do_write;

struct	scsi_format_data fmt;

/*XXX*/EXPORT	BOOL cvt_cyls(){ return (FALSE);}
/*XXX*/EXPORT	BOOL cvt_bcyls(){ return (FALSE);}
/*XXX*/EXPORT	void print_defect_list(){}

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:\treadcd [options]\n");
	error("options:\n");
	error("\t-version	print version information and exit\n");
	error("\tdev=target	SCSI target to use\n");
	error("\tf=filename	Name of file to read/write\n");
	error("\tsectors=range	Range of sectors to read/write\n");
	error("\t-w		Switch to write mode\n");
	error("\tkdebug=#,kd=#\tdo Kernel debugging\n");
	error("\t-v		increment general verbose level by one\n");
	error("\t-V		increment SCSI command transport verbose level by one\n");
	error("\t-silent,-s\tdo not print status of erreneous commands\n");
	exit(ret);
}	

char	opts[]   = "kdebug#,kd#,verbose+,v+,Verbose,V+,silent,s,debug,help,h,version,dev*,f*,sectors*,w";

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	char	*dev = NULL;
	int	fcount;
	int	cac;
	char	* const *cav;
	int	scsibus	= 0;
	int	target	= 0;
	int	lun	= 0;
	int	silent	= 0;
	int	verbose	= 0;
	int	lverbose= 0;
	int	kdebug	= 0;
	int	debug	= 0;
	int	pversion = 0;
	SCSI	*scgp;
	char	*filename= NULL;
	char	*sectors = NULL;

	save_args(ac, av);

	cac = --ac;
	cav = ++av;

	if(getallargs(&cac, &cav, opts,
			&kdebug, &kdebug,
			&verbose, &verbose,
			&lverbose, &lverbose,
			&silent, &silent, &debug,
			&help, &help, &pversion,
			&dev, &filename, &sectors, &do_write) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (pversion) {
		printf("readcd %s (%s-%s-%s) Copyright (C) 1987, 1995-2000 Jörg Schilling\n",
								cdr_version,
								HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}

	fcount = 0;
	cac = ac;
	cav = av;

	while(getfiles(&cac, &cav, opts) > 0) {
		fcount++;
		if (fcount == 1) {
			if (*astoi(cav[0], &target) != '\0') {
				errmsgno(EX_BAD,
					"Target '%s' is not a Number.\n",
								cav[0]);
				usage(EX_BAD);
				/* NOTREACHED */
			}
		}
		if (fcount == 2) {
			if (*astoi(cav[0], &lun) != '\0') {
				errmsgno(EX_BAD,
					"Lun is '%s' not a Number.\n",
								cav[0]);
				usage(EX_BAD);
				/* NOTREACHED */
			}
		}
		if (fcount == 3) {
			if (*astoi(cav[0], &scsibus) != '\0') {
				errmsgno(EX_BAD,
					"Scsibus is '%s' not a Number.\n",
								cav[0]);
				usage(EX_BAD);
				/* NOTREACHED */
			}
		} else {
			scsibus = 0;
		}
		cac--;
		cav++;
	}
/*error("dev: '%s'\n", dev);*/

	if (dev) {
		char	errstr[80];

		if ((scgp = open_scsi(dev, errstr, sizeof(errstr), debug, lverbose)) == (SCSI *)0)
			comerr("%s%sCannot open SCSI driver.\n", errstr, errstr[0]?". ":"");
/*		scsi_settimeout(scgp, timeout);*/
	} else {
		if (scsibus == -1 && target >= 0 && lun >= 0)
			scsibus = 0;

		scgp = scsi_smalloc();
		scgp->debug = debug;
		scgp->kdebug = kdebug;
		scgp->scsibus = scsibus;
		scgp->target = target;
		scgp->lun = lun;

		if (!scsi_open(scgp, NULL, scsibus, target, lun))
			comerr("Cannot open SCSI driver.\n");

		scgp->scsibus = scsibus;
		scgp->target = target;
		scgp->lun = lun;
	}
	scgp->silent = silent;
	scgp->verbose = verbose;
	scgp->debug = debug;
	scgp->kdebug = kdebug;

	Sbufsize = scsi_bufsize(scgp, 256*1024L);
	if ((Sbuf = scsi_getbuf(scgp, Sbufsize)) == NULL)
		comerr("Cannot get SCSI I/O buffer.\n");

	is_suid = geteuid() != getuid();
	/*
	 * We don't need root privilleges anymore.
	 */
#ifdef	HAVE_SETREUID
	if (setreuid(-1, getuid()) < 0)
#else
#ifdef	HAVE_SETEUID
	if (seteuid(getuid()) < 0)
#else
	if (setuid(getuid()) < 0)
#endif
#endif
		comerr("Panic cannot set back efective uid.\n");

	/* code to use SCG */

	do_inquiry(scgp, FALSE);
	if (is_suid) {
		if (scgp->inq->type != INQ_ROMD)
			comerrno(EX_BAD, "Not root. Will only work on CD-ROM in suid mode\n");
	}

	if (filename || sectors) {
		dorw(scgp, filename, sectors);
	} else {
		doit(scgp);
	}
	return (0);
}

LOCAL int
prstats()
{
	int	sec;
	int	usec;
	int	tmsec;

	if (gettimeofday(&stoptime, (struct timezone *)0) < 0)
		comerr("Cannot get time\n");

	sec = stoptime.tv_sec - starttime.tv_sec;
	usec = stoptime.tv_usec - starttime.tv_usec;
	tmsec = sec*1000 + usec/1000;
#ifdef	lint
	tmsec = tmsec;	/* Bisz spaeter */
#endif
	if (usec < 0) {
		sec--;
		usec += 1000000;
	}

	error("Time total: %d.%03dsec\n", sec, usec/1000);
/*	return (sec + (usec / 500000));*/
	return (1000*sec + (usec / 1000));
}

LOCAL void
dorw(scgp, filename, sectors)
	SCSI	*scgp;
	char	*filename;
	char	*sectors;
{
	parm_t	params;
	char	*p = NULL;

	params.start = 0;
	params.end = -1;
	params.name = NULL;

	if (filename)
		params.name = filename;
	if (sectors)
		p = astol(sectors, &params.start);
	if (p && *p == '-')
		p = astol(++p, &params.end);
	if (p && *p != '\0')
		comerrno(EX_BAD, "Not a valid sector range '%s'\n", sectors);

	if (!wait_unit_ready(scgp, 60))
		comerr("Device not ready.\n");

	if (do_write)
		write_disk(scgp, &params);
	else
		read_disk(scgp, &params);
}

LOCAL void
doit(scgp)
	SCSI	*scgp;
{
	int	i = 0;

	for(;;) {
		if (!wait_unit_ready(scgp, 60))
			comerr("Device not ready.\n");

		printf("0:read 1:veri   2:erase   3:read buffer 4:cache 5:ovtime 6:cap\n");
		printf("7:wne  8:floppy 9:verify 10:checkcmds  11:read disk 12:write disk\n");
		printf("13:scsireset 14:seektest 15: readda\n");

		getint("Enter selection:", &i, 0, 15);
		switch (i) {

		case 11:	read_disk(scgp, 0);	break;
		case 12:	write_disk(scgp, 0);	break;
		case 15:	ra(scgp);		break;
		}
	}
}

#ifdef	used
LOCAL int
read_scsi_g1(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = bp;
/*	scmd->size = cnt*512;*/
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g1_cdb.cmd = 0x28;
	scmd->cdb.g1_cdb.lun = scgp->lun;
	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);
	
	scgp->cmdname = "read extended";

	return (scsicmd(scgp));
}
#endif

#define	G0_MAXADDR	0x1FFFFFL

EXPORT int
write_scsi(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	if(addr <= G0_MAXADDR)
		return(write_g0(scgp, bp, addr, cnt));
	else
		return(write_g1(scgp, bp, addr, cnt));
}

EXPORT int
write_g0(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g0_cdb.cmd = SC_WRITE;
	scmd->cdb.g0_cdb.lun = scgp->lun;
	g0_cdbaddr(&scmd->cdb.g0_cdb, addr);
	scmd->cdb.g0_cdb.count = cnt;
	
	scgp->cmdname = "write_g0";

	return (scsicmd(scgp));
}

EXPORT int
write_g1(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g1_cdb.cmd = SC_EWRITE;
	scmd->cdb.g1_cdb.lun = scgp->lun;
	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);
	
	scgp->cmdname = "write_g1";

	return (scsicmd(scgp));
}

#ifdef	used
LOCAL void
Xrequest_sense(scgp)
	SCSI	*scgp;
{
	char	sense_buf[32];
	struct	scg_cmd ocmd;
	int	sense_count;
	char	*cmdsave;
	register struct	scg_cmd	*scmd = scgp->scmd;

	cmdsave = scgp->cmdname;

	movebytes(scmd, &ocmd, sizeof(*scmd));

	fillbytes((caddr_t)sense_buf, sizeof(sense_buf), '\0');

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = (caddr_t)sense_buf;
	scmd->size = sizeof(sense_buf);
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g1_cdb.cmd = 0x3;
	scmd->cdb.g1_cdb.lun = scgp->lun;
	scmd->cdb.g0_cdb.count = sizeof(sense_buf);
	
	scgp->cmdname = "request sense";

	scsicmd(scgp);

	sense_count = sizeof(sense_buf) - scsigetresid(scgp);
	movebytes(&ocmd, scmd, sizeof(*scmd));
	scmd->sense_count = sense_count;
	movebytes(sense_buf, (u_char *)&scmd->sense, scmd->sense_count);

	scgp->cmdname = cmdsave;
	scsiprinterr(scgp);
	scsiprintresult(scgp);	/* XXX restore key/code in future */
}
#endif

/*#define	MAX_RETRY	32*/
#define	MAX_RETRY	128

LOCAL int
rrr(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	long	cnt;
{
	int	secsize = scgp->cap->c_bsize;
	int	try = 0;
	int	err;
	char	dummybuf[4000];

	errmsgno(EX_BAD, "Retrying from sector %d.\n", addr);
	while (cnt > 0) {
		error(".");

		do {
			if (try != 0) {
				if ((try % 8) == 0) {
					error("+");
					scgp->silent++;
					read_g1(scgp, dummybuf, scgp->cap->c_baddr, 1);
					scgp->silent--;
				} else if ((try % 4) == 0) {
					error("-");
					scgp->silent++;
					read_g1(scgp, dummybuf, 0, 1);
					scgp->silent--;
				} else {
					error("~");
					scgp->silent++;
					read_g1(scgp, dummybuf, choice(scgp->cap->c_baddr), 1);
					scgp->silent--;
				}
			}

			scgp->silent++;
			err = read_g1(scgp, bp, addr, 1);
			scgp->silent--;

			if (err < 0) {
				err = scgp->scmd->ux_errno;
/*				error("\n");*/
/*				errmsgno(err, "Cannot read source disk\n");*/
			} else {
				break;
			}
		} while (++try < MAX_RETRY);

		if (try >= MAX_RETRY) {
			error("\n");
			errmsgno(err, "Error not corrected.\n");
			return (-1);
		} else {
			if (try > 1) {
				error("\n");
				errmsgno(EX_BAD, "Error corrected after %d tries.\n", try);
			}
		}
		try = 0;
		cnt -= 1;
		addr += 1;
		bp += secsize;
	}
	return (0);
}

LOCAL void
read_disk(scgp, parmp)
	SCSI	*scgp;
	parm_t	*parmp;
{
	char	filename[512];
	char	*defname = "disk.out";
	FILE	*f;
	long	addr;
	long	cnt;
	long	end;
	int	nsec;
	long	start = 0L;
	int	err = 0;

	if (is_suid) {
		if (scgp->inq->type != INQ_ROMD)
			comerrno(EX_BAD, "Not root. Will only read from CD in suid mode\n");
	}

	filename[0] ='\0';
	read_capacity(scgp);
	end = scgp->cap->c_baddr + 1;

	if (end <= 0)
		end = 10000000;	/* Hack to read empty (e.g. blank=fast) disks */

	if (parmp) {
		if (parmp->name)
			defname = parmp->name;
		error("Copy from SCSI (%d,%d,%d) disk to file '%s'\n",
					scgp->scsibus, scgp->target, scgp->lun,
					defname);

		addr = start = parmp->start;
		if (parmp->end != -1 && parmp->end < end)
			end = parmp->end;
		cnt = Sbufsize / scgp->cap->c_bsize;
	}
else {

	error("Copy from SCSI (%d,%d,%d) disk to file\n",
					scgp->scsibus, scgp->target, scgp->lun);
	error("Enter filename [%s]: ", defname);flush();
	(void)getline(filename, sizeof(filename));

	addr = start;
	getlong("Enter starting sector for copy:", &addr, start, end-1);
	start = addr;
	cnt = end - addr;
	getlong("Enter number of sectors to copy:", &cnt, 1L, cnt);
	end = addr + cnt;

	cnt = Sbufsize / scgp->cap->c_bsize;
	getlong("Enter number of sectors per copy:", &cnt, 1L, cnt);
/*	error("end:  %8ld\n", end);*/
}

	if (filename[0] == '\0')
		strcpy(filename, defname);
	if (streql(filename, "-")) {
		f = stdout;
#if	defined(__CYGWIN32__) || defined(__EMX__)
		setmode(STDOUT_FILENO, O_BINARY);
#endif
	} else if ((f = fileopen(filename, "wcub")) == NULL)
		comerr("Cannot open '%s'.\n", filename);

	error("end:  %8ld\n", end);
	if (gettimeofday(&starttime, (struct timezone *)0) < 0)
		comerr("Cannot get start time\n");

	for(;addr < end; addr += cnt) {

		if ((addr + cnt) > end)
			cnt = end - addr;

		error("addr: %8d cnt: %d\r", addr, cnt);

/*		if (read_scsi(scgp, Sbuf, addr, cnt) < 0) {*/
		if (read_g1(scgp, Sbuf, addr, cnt) < 0) {
			err = scgp->scmd->ux_errno;
			error("\n");
			errmsgno(err, "Cannot read source disk\n");
			if (rrr(scgp, Sbuf, addr, cnt) < 0)
				goto out;
		}
		if (filewrite(f, Sbuf, cnt * scgp->cap->c_bsize) < 0) {
			err = geterrno();
			error("\n");
			errmsgno(err, "Cannot write '%s'\n", filename);
			break;
		}
	}
	error("addr: %8d", addr);
out:
	error("\n");
	nsec = prstats();
/*	error("%d\n",nsec);*/
	error("Read %.2f kB at %.1f kB/sec.\n",
		(addr - start)/(1024.0/scgp->cap->c_bsize),
		((addr - start)/(1024.0/scgp->cap->c_bsize)) / (0.001*nsec));
	if (err)
		exit(err);
}

LOCAL void
write_disk(scgp, parmp)
	SCSI	*scgp;
	parm_t	*parmp;
{
	char	filename[512];
	char	*defname = "disk.out";
	FILE	*f;
	long	addr = 0L;
	long	cnt;
	long	amt;
	long	end;
	int	nsec;
	int	start;

	if (is_suid)
		comerrno(EX_BAD, "Not root. Will not write in suid mode\n");

	read_capacity(scgp);
	end = scgp->cap->c_baddr + 1;

	if (end <= 0)
		end = 10000000;	/* Hack to write empty disks */

	if (parmp) {
		if (parmp->name)
			defname = parmp->name;
		error("Copy from file '%s' tp SCSI (%d,%d,%d) disk\n",
					defname,
					scgp->scsibus, scgp->target, scgp->lun);

		addr = start = parmp->start;
		if (parmp->end != -1 && parmp->end < end)
			end = parmp->end;
		cnt = Sbufsize / scgp->cap->c_bsize;
	}
else {

	error("Copy from file to SCSI (%d,%d,%d) disk\n",
					scgp->scsibus, scgp->target, scgp->lun);
	error("Enter filename [%s]: ", defname);flush();
	(void)getline(filename, sizeof(filename));
	error("Notice: reading from file always starts at file offset 0.\n");

	getlong("Enter starting sector for copy:", &addr, 0L, end-1);
	start = addr;
	cnt = end - addr;
	getlong("Enter number of sectors to copy:", &end, 1L, end);
	end = addr + cnt;

	cnt = Sbufsize / scgp->cap->c_bsize;
	getlong("Enter number of sectors per copy:", &cnt, 1L, cnt);
/*	error("end:  %8ld\n", end);*/
}

	if (filename[0] == '\0')
		strcpy(filename, defname);
	if (streql(filename, "-")) {
		f = stdin;
#if	defined(__CYGWIN32__) || defined(__EMX__)
		setmode(STDIN_FILENO, O_BINARY);
#endif
	} else if ((f = fileopen(filename, "rub")) == NULL)
		comerr("Cannot open '%s'.\n", filename);

	error("end:  %8ld\n", end);
	if (gettimeofday(&starttime, (struct timezone *)0) < 0)
		comerr("Cannot get start time\n");

	for(;addr < end; addr += cnt) {

		if ((addr + cnt) > end)
			cnt = end - addr;

		error("addr: %8d cnt: %d\r", addr, cnt);

		if ((amt = fileread(f, Sbuf, cnt * scgp->cap->c_bsize)) < 0)
			comerr("Cannot read '%s'\n", filename);
		if (amt == 0)
			break;
		if ((amt / scgp->cap->c_bsize) < cnt)
			cnt = amt / scgp->cap->c_bsize;
		if (write_scsi(scgp, Sbuf, addr, cnt) < 0)
			comerrno(scgp->scmd->ux_errno,
					"Cannot write destination disk\n");
	}
	error("addr: %8d\n", addr);
	nsec = prstats();
	error("Wrote %.2f kB at %.1f kB/sec.\n",
		(double)(addr - start)/(1024.0/scgp->cap->c_bsize),
		(double)((addr - start)/(1024.0/scgp->cap->c_bsize)) / nsec);
}

LOCAL int
choice(n)
	int	n;
{
#if	defined(HAVE_DRAND48)
	extern	double	drand48 __PR((void));

	return drand48() * n;
#else
#	if	defined(HAVE_RAND)
	extern	int	rand __PR((void));

	return rand() % n;
#	else
	return (0);
#	endif
#endif
}

LOCAL void
ra(scgp)
	SCSI	*scgp;
{
/*	char	filename[512];*/
	FILE	*f;
/*	long	addr = 0L;*/
/*	long	cnt;*/
/*	long	end;*/
/*	int	nsec;*/
/*	int	start;*/
/*	int	err = 0;*/

	select_secsize(scgp, 2352);
	read_capacity(scgp);
	fillbytes(Sbuf, 50*2352, 0);
	if (read_g1(scgp, Sbuf, 0, 50) < 0)
		errmsg("read CD\n");
	f = fileopen("DDA", "wctb");
/*	filewrite(f, Sbuf, 50 * 2352 - scsigetresid(scgp));*/
	filewrite(f, Sbuf, 50 * 2352 );
	fclose(f);
}

#ifdef	XXX
EXPORT int
read_da(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
/*	scmd->cdb.g5_cdb.cmd = SC_EREAD;*/
	scmd->cdb.g5_cdb.cmd = 0xd9;
	scmd->cdb.g5_cdb.lun = scgp->lun;
	g5_cdbaddr(&scmd->cdb.g1_cdb, addr);
	g5_cdblen(&scmd->cdb.g1_cdb, cnt);/* XXX subscript out of range ?? */
	
	scgp->cmdname = "read_g1";

	return (scsicmd(scgp));
}
#endif
