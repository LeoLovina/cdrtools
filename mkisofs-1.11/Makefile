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
CPPOPTS	+=	-DUSE_SCG -I../cdrecord
CFILES=		mkisofs.c tree.c write.c hash.c rock.c exclude.c multi.c \
		files.c match.c name.c fnmatch.c eltorito.c \
		scsi.c \
		scsi_cdr.c cd_misc.c scsitransp.c scsierrs.c \
		modes.c port.c
LIBS=		-lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
