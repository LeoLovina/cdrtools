#ident @(#)Makefile	1.5 00/01/17 Copyr 1995 J. Schilling
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		cdrecord
CPPOPTS +=	-DFIFO -DAUINFO
CFILES=		cdrecord.c cd_misc.c fifo.c isosize.c audiosize.c auinfo.c \
		diskid.c \
		wm_session.c wm_track.c wm_packet.c \
		defaults.c \
		cdr_drv.c \
		drv_philips.c drv_sony.c drv_jvc.c drv_mmc.c \
		drv_simul.c \
		scsi_scan.c scsi_cdr.c \
		modes.c misc.c
HFILES=		cdrecord.h iso9660.h auheader.h scsi_scan.h \
		scsimmc.h \
		../include/scg/scgio.h ../include/scg/scsidefs.h \
		../include/scg/scsireg.h ../include/scg/scsitransp.h
#LIBS=		-lschily $(LIB_SOCKET)
LIBS=		-ldeflt -lscg -lschily $(SCSILIB)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
