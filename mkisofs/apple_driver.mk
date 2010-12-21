#ident @(#)apple_driver.mk	1.3 10/12/19 
###########################################################################
#@@C@@
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		apple_driver
CPPOPTS +=	-DAPPLE_HYB
CPPOPTS +=	-I../libhfs_iso/
CPPOPTS	+=	-I../cdrecord
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_cdrtools\"

CFILES=		apple_driver.c
HFILES=		config.h mac_label.h mkisofs.h
LIBS=		-lschily
XMK_FILE=	apple_driver_man.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
