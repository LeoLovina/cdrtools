#ident "@(#)cdda2ogg.mk	1.2 10/02/11 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
INSMODE=	755
TARGET=		cdda2ogg
SCRFILE=	cdda2ogg
XMK_FILE=	cdda2ogg.mk1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.scr
###########################################################################
