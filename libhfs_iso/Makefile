#ident %W% %E% %Q%
###########################################################################
# Sample makefile for non-shared libraries
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		lib
TARGETLIB=	hfs
CPPOPTS +=	-DAPPLE_HYB
CFILES=		data.c block.c low.c file.c btree.c node.c record.c \
		volume.c hfs.c
LIBS=
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
