#ident @(#)isodump.mk	1.3 00/12/09 
###########################################################################
#@@C@@
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		isodump
CPPOPTS +=	-DUSE_LIBSCHILY
CPPOPTS +=	-DUSE_LARGEFILES
CFILES=		isodump.c
LIBS=		-lschily
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
