/* @(#)scsi.c	1.13 00/02/10 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)scsi.c	1.13 00/02/10 Copyright 1997 J. Schilling";
#endif
/*
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

#ifdef	USE_SCG
#include <mconfig.h>

#include <stdio.h>
#include <standard.h>
#include <stdxlib.h>
#include <unixstd.h>

#include "mkisofs.h"
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "cdrecord.h"

/*
 * NOTICE:	You should not make BUF_SIZE more than
 *		the buffer size of the CD-Recorder.
 *
 * Do not set BUF_SIZE to be more than 126 KBytes
 * if you are running cdrecord on a sun4c machine.
 *
 * WARNING:	Philips CDD 521 dies if BUF_SIZE is to big.
 */
#define	BUF_SIZE	(62*1024)	/* Must be a multiple of 2048	   */

LOCAL	SCSI	*scgp;
LOCAL	long	bufsize;		/* The size of the transfer buffer */

EXPORT	int	readsecs	__PR((int startsecno, void *buffer, int sectorcount));
EXPORT	int	scsidev_open	__PR((char *path));
EXPORT	int	scsidev_close	__PR((void));

EXPORT int
readsecs(startsecno, buffer, sectorcount)
	int	startsecno;
	void	*buffer;
	int	sectorcount;
{
	int	f;
	int	secsize;	/* The drive's SCSI sector size		*/
	long	amount;		/* The number of bytes to be transfered	*/
	long	secno;		/* The sector number to read from	*/
	long	secnum;		/* The number of sectors to read	*/
	char	*bp;
	long	amt;

	if (in_image == NULL) {
		/*
		 * We are using the standard CD-ROM sectorsize of 2048 bytes
		 * while the drive may be switched to 512 bytes per sector.
		 *
		 * XXX We assume that secsize is no more than SECTOR_SIZE
		 * XXX and that SECTOR_SIZE / secsize is not a fraction.
		 */
		secsize = scgp->cap->c_bsize;
		amount = sectorcount * SECTOR_SIZE;
		secno = startsecno * (SECTOR_SIZE / secsize);
		bp = buffer;

		while (amount > 0) {
			amt = amount;
			if (amount > bufsize)
				amt = bufsize;
			secnum = amt / secsize;

			if (read_scsi(scgp, bp, secno, secnum) < 0 ||
						scsigetresid(scgp) != 0) {
#ifdef	OLD
				return (-1);
#else
				comerr("Read error on old image\n");
#endif
			}

			amount -= secnum * secsize;
			bp     += secnum * secsize;
			secno  += secnum;
		}
		return (SECTOR_SIZE * sectorcount);
	}

	f = fileno(in_image);

	if (lseek(f, (off_t)startsecno * SECTOR_SIZE, 0) == (off_t)-1) {
#ifdef	USE_LIBSCHILY
		comerr("Seek error on old image\n");
#else
		fprintf(stderr,"Seek error on old image\n");
		exit(10);
#endif
	}
	if( read(f, buffer, (sectorcount * SECTOR_SIZE))
	    != (sectorcount * SECTOR_SIZE) )
	{
#ifdef	USE_LIBSCHILY
		comerr("Read error on old image\n");
#else
		fprintf(stderr," Read error on old image\n");
		exit(10);
#endif
	}
	return sectorcount * SECTOR_SIZE;
}

EXPORT int
scsidev_open(path)
	char	*path;
{
	char	errstr[80];
	char	*buf;	/* ignored, bit OS/2 ASPI layer needs memory which
			   has been allocated by scsi_getbuf()		   */

			/* path, debug, verboseopen */
	scgp = open_scsi(path, errstr, sizeof(errstr), 0, 0);
	if (scgp == 0) {
		errmsg("%s%sCannot open SCSI driver.\n", errstr, errstr[0]?". ":"");
		return (-1);
	}

	bufsize = scsi_bufsize(scgp, BUF_SIZE);
	if ((buf = scsi_getbuf(scgp, bufsize)) == NULL) {
		errmsg("Cannot get SCSI I/O buffer.\n");
		close_scsi(scgp);
		return (-1);
	}

	bufsize = (bufsize / SECTOR_SIZE) * SECTOR_SIZE;

	allow_atapi(scgp, TRUE);

	if (!wait_unit_ready(scgp, 60)) {/* Eat Unit att / Wait for drive */
		scgp->silent--;
		return (-1);
	}

	scgp->silent++;
	read_capacity(scgp);	/* Set Capacity/Sectorsize for I/O */
	scgp->silent--;

	return (1);
}

EXPORT int
scsidev_close()
{
	if (in_image == NULL) {
		return (close_scsi(scgp));
	} else {
		return (fclose(in_image));
	}
}

#endif	/* USE_SCG */
