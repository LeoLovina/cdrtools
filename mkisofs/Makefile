#ident %W% %E% %Q%
###########################################################################
# Sample makefile for general application programs
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		mkisofs
#CPPOPTS +=	-DADD_FILES
CPPOPTS	+=	-DHAVE_CONFIG_H -DUSE_LIBSCHILY -DUSE_SCG \
		'-DAPPID_DEFAULT="MKISOFS ISO 9660 FILESYSTEM BUILDER & CDRECORD CD-R/DVD CREATOR"' \
		-I../cdrecord
CFILES=		mkisofs.c tree.c write.c hash.c rock.c multi.c \
		joliet.c match.c name.c fnmatch.c eltorito.c boot.c \
		getopt.c getopt1.c \
		scsi.c \
		scsi_cdr.c cd_misc.c \
		modes.c
LIBS=		-lscg -lschily $(SCSILIB)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
