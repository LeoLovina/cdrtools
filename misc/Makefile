#ident %W% %E% %Q%
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		readcd
CPPOPTS +=	-I../cdrecord -DUSE_PG
CFILES=		readcd.c cd_misc.c io.c \
		scsi_cdr.c \
		modes.c misc.c
HFILES=		
#LIBS=		-lschily $(LIB_SOCKET)
LIBS=		-lscg -lschily $(SCSILIB)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
