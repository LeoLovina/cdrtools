#ident @(#)Makefile	1.3 99/12/12 
###########################################################################
#@@C@@
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		mkhybrid
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
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
