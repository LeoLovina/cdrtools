#ident %W% %E% %Q%
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		cdrecord
#CPPOPTS +=	-DSOL2
CFILES=		cdrecord.c isosize.c scsi_cdr.c scsitransp.c scsierrs.c port.c
HFILES=		cdrecord.h scgio.h scsidefs.h scsireg.h scsitransp.h
LIBS=		-lschily $(LIB_SOCKET)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
