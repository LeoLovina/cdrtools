#ident %W% %E% %Q%
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		cdrecord
CPPOPTS +=	-DFIFO
CFILES=		cdrecord.c cd_misc.c fifo.c isosize.c audiosize.c diskid.c \
		wm_session.c wm_track.c wm_packet.c \
		cdr_drv.c \
		drv_philips.c drv_sony.c drv_jvc.c drv_mmc.c \
		scsi_scan.c scsi_cdr.c scsitransp.c scsierrs.c \
		modes.c port.c
HFILES=		cdrecord.h iso9660.h auheader.h scgio.h scsidefs.h \
		scsireg.h scsitransp.h
#LIBS=		-lschily $(LIB_SOCKET)
LIBS=		-lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
