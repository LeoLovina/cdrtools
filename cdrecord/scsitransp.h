/* @(#)scsitransp.h	1.14 98/08/30 Copyright 1995 J. Schilling */
/*
 *	Definitions for commands that use functions from scsitransp.c
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
 * From scsitransp.c:
 */
extern	int	scsi_open	__PR((char *device, int busno, int tgt, int tlun));
extern	BOOL	scsi_havebus	__PR((int));
extern	int	scsi_fileno	__PR((int, int, int));
extern	int	scsi_isatapi	__PR((void));
extern	int	scsireset	__PR((void));
extern	void	*scsi_getbuf	__PR((long));
extern	long	scsi_bufsize	__PR((long));
extern	void	scsi_setnonstderrs __PR((const char **));
extern	int	scsicmd		__PR((char *));
extern	int	scsigetresid	__PR((void));
extern	void	scsiprinterr	__PR((char *));
extern	void	scsiprintcdb	__PR((void));
extern	void	scsiprintwdata	__PR((void));
extern	void	scsiprintrdata	__PR((void));
extern	void	scsiprintresult	__PR((void));
extern	void	scsiprintstatus	__PR((void));
extern	void	scsiprbytes	__PR((char *, unsigned char *, int));
extern	void	scsiprsense	__PR((unsigned char *, int));
extern	int	scsi_sense_key	__PR((void));
extern	int	scsi_sense_code	__PR((void));
extern	int	scsi_sense_qual	__PR((void));
#ifdef	_SCSIREG_H
extern	void	scsiprintdev	__PR((struct scsi_inquiry *));
#endif

/*
 * From scsierrmsg.c:
 */
extern	const char	*scsisensemsg	__PR((int, int, int,
						const char **, char *));
#ifdef	_SCGIO_H
extern	void		scsierrmsg	__PR((struct scsi_sense *,
						struct scsi_status *,
						int, const char **));
#endif
