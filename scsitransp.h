/* @(#)scsitransp.h	1.3 96/02/04 Copyright 1995 J. Schilling */
/*
 *	Definitions for commands that use functions from scsitransp.c
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
 * From scsitransp.c:
 */
extern	int	scsi_open	__PR((void));
extern	int	scsi_fileno	__PR((int));
extern	int	scsicmd		__PR((char *));
extern	int	scsigetresid	__PR((void));
extern	void	scsiprinterr	__PR((char *));
extern	void	scsiprintstatus	__PR((void));
extern	void	scsiprbytes	__PR((char *, unsigned char *, int));
extern	void	scsiprsense	__PR((unsigned char *, int));
extern	void	scsiprintdev	__PR((struct scsi_inquiry *));

/*
 * From scsierrmsg.c:
 */
extern	char	*scsisensemsg	__PR((int, int, int));
extern	void	scsiserrmsg	__PR((struct scsi_sense *,
						struct scsi_status *, int));
