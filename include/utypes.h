/* @(#)utypes.h	1.2 98/01/23 Copyright 1997 J. Schilling */
/*
 *	Definitions for some unsigned types
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

#ifndef	_UTYPES_H
#define	_UTYPES_H

typedef	unsigned long	Ulong;
typedef	unsigned int	Uint;
typedef	unsigned short	Ushort;
typedef	unsigned char	Uchar;

#ifdef	_AIX

typedef unsigned int	Ucbit;

#else

typedef unsigned char	Ucbit;

#endif

#endif	/* _UTYPES_H */
