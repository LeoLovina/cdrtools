#ident @(#)isoinfo.mk	1.4 01/01/19 
###########################################################################
#@@C@@
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		isoinfo
CPPOPTS +=	-DUSE_LIBSCHILY
CPPOPTS +=	-DUSE_LARGEFILES
CFILES=		isoinfo.c
LIBS=		-lunls -lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
