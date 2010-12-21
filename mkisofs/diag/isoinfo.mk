#ident @(#)isoinfo.mk	1.16 10/12/19 
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
CPPOPTS +=	-DUSE_SCG
CPPOPTS +=	-I..
CPPOPTS +=	-I../../libscg
CPPOPTS +=	-I../../libscgcmd
CPPOPTS +=	-I../../libcdrdeflt
CPPOPTS +=	-DUSE_FIND
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DUSE_ICONV
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_cdrtools\"

CFILES=		isoinfo.c \
		scsi.c

LIBS=		-lsiconv -lscgcmd -lrscg -lscg $(LIB_VOLMGT) \
			-lcdrdeflt -ldeflt -lfind -lschily \
			$(LIB_ACL_TEST) $(SCSILIB) $(LIB_SOCKET) $(LIB_ICONV) $(LIB_INTL) $(LIB_INTL)

XMK_FILE=	isoinfo_man.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
