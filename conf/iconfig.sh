#!/bin/sh -
#ident %W% %E% %Q%
###########################################################################
# Written 1998 by J. Schilling
###########################################################################
# Include file configuration script
###########################################################################
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
###########################################################################

echo '/*'
echo ' *	Extended configuration file'
echo ' *	This file is created automatically, do not change by hand.'
echo ' *'
echo ' *	Copyright (c) 1998 J. Schilling'
echo ' */'
echo '#define	HAVE_XCONFIG_H'

if [ -r /usr/include/linux/pg.h -o -r /usr/src/linux/include/linux/pg.h ]; then
	echo '#define	HAVE_LINUX_PG_H'
fi
if [ -r /usr/include/camlib.h ]; then
	echo '#define	HAVE_CAMLIB_H'
fi
