#ident @(#)dump.mk	1.3 00/12/09 
###########################################################################
#@@C@@
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		devdump
CPPOPTS +=	-DUSE_LIBSCHILY
CPPOPTS +=	-DUSE_LARGEFILES
CFILES=		dump.c
LIBS=		-lschily
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
