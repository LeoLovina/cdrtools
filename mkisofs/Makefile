#ident %W% %E% %Q%
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
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		mkisofs
HARDLINKS=	mkhybrid
#CPPOPTS +=	-DADD_FILES
CPPOPTS +=	-DAPPLE_HYB
CPPOPTS +=	-DSORTING
CPPOPTS +=	-I../libhfs_iso/
CPPOPTS	+=	-DHAVE_CONFIG_H -DUSE_LIBSCHILY -DUSE_SCG \
		'-DAPPID_DEFAULT="MKISOFS ISO 9660/HFS FILESYSTEM BUILDER & CDRECORD CD-R/DVD CREATOR"' \
		-I../cdrecord
CFILES=		mkisofs.c tree.c write.c hash.c rock.c multi.c \
		joliet.c match.c name.c fnmatch.c eltorito.c boot.c \
		getopt.c getopt1.c \
		scsi.c \
		scsi_cdr.c cd_misc.c \
		modes.c \
		apple.c volume.c desktop.c mac_label.c
HFILES=		apple.h bootinfo.h config.h defaults.h diskmbr.h exclude.h \
		fnmatch.h getopt.h iso9660.h mac_label.h mactypes.h match.h \
		mkisofs.h sunlabel.h vms.h 

LIBS=		-lhfs -lfile -lunls -lscg -lschily $(SCSILIB)
XMK_FILE=	Makefile.man hybridman.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
