#!/bin/sh -
#ident %W% %E% %Q%
###########################################################################
# Written 1998 by J. Schilling
###########################################################################
#
# Make Rule configuration script
#
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

echo '###########################################################################'
echo '#'
echo '#	Extended configuration definitions'
echo '#	This file is created automatically, do not change by hand.'
echo '#'
echo '#	Copyright (c) 1998 J. Schilling'
echo '#'
echo '###########################################################################'
echo 'XCONFIG= OK'

if [ -r /usr/include/camlib.h ]; then
	echo 'SCSILIB= -lcam'
fi
