#ident @(#)isoinfo.mk	1.2 99/12/27 
###########################################################################
#@@C@@
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		isoinfo
#CPPOPTS +=	-DADD_FILES
CPPOPTS +=	-DUSE_LIBSCHILY
CFILES=		isoinfo.c
LIBS=		-lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
