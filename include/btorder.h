/* @(#)btorder.h	1.1 96/08/19 Copyright 1996 J. Schilling */
/*
 *	Definitions for Bitordering
 *
 *	Copyright (c) 1996 J. Schilling
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


#ifndef	_BTORDER_H
#define	_BTORDER_H

#include <sys/types.h>			/* try to load isa_defs.h on Solaris */

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */
#elif	defined(_BIT_FIELDS_HTOL)	/* Motorola byteorder */
#else
#	if defined(sun3) || defined(mc68000) || \
	   defined(sun4) || defined(sparc)
#		define _BIT_FIELDS_HTOL
#	endif

#	if defined(__i386_) || defined(i386)
#		define _BIT_FIELDS_LTOH
#endif
#endif

#endif	/* _BTORDER_H */
