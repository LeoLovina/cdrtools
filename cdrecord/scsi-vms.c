/* @(#)scsi-vms.c	1.3 98/10/06 Copyright 1997 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-vms.c	1.3 98/10/06 Copyright 1997 J. Schilling";
#endif
/*
 *	Interface for the VMS generic SCSI implementation.
 *
 *	This is a hack, that tries to emulate the functionality
 *	of the scg driver.
 *
 *	Copyright (c) 1997 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <iodef.h>
#include <ssdef.h>
#include <descrip.h>
#include <starlet.h>
#include <string.h>
#include <LIB$ROUTINES.H>

#define MAX_SCG 	8	/* Max # of SCSI controllers */
#define MAX_TGT 	8
#define MAX_LUN 	8

#define MAX_DMA_VMS	(96*512)/* Check if this is not too big */
#define MAX_TMO_VMS	255	/* max value for OpenVMS/AXP 7.1 ehh*/


LOCAL	int	scsi_send	__PR((int f, struct scg_cmd *sp));
LOCAL	int	do_scsi_cmd	__PR((int f, struct scg_cmd *sp));
LOCAL	int	do_scsi_sense	__PR((int f, struct scg_cmd *sp));

#define DEVICE_NAMELEN 7

struct SCSI$DESC {
	Uint	SCSI$L_OPCODE;		/* SCSI Operation Code */
	Uint	SCSI$L_FLAGS;		/* SCSI Flags Bit Map */
	char	*SCSI$A_CMD_ADDR;	/* ->SCSI command buffer */
	Uint	SCSI$L_CMD_LEN; 	/* SCSI command length, bytes */
	char	*SCSI$A_DATA_ADDR;	/* ->SCSI data buffer */
	Uint	SCSI$L_DATA_LEN;	/* SCSI data length, bytes */
	Uint	SCSI$L_PAD_LEN; 	/* SCSI pad length, bytes */
	Uint	SCSI$L_PH_CH_TMOUT;	/* SCSI phase change timeout */
	Uint	SCSI$L_DISCON_TMOUT;	/* SCSI disconnect timeout */
	Uint	SCSI$L_RES_1;		/* Reserved */
	Uint	SCSI$L_RES_2;		/* Reserved */
	Uint	SCSI$L_RES_3;		/* Reserved */
	Uint	SCSI$L_RES_4;		/* Reserved */
	Uint	SCSI$L_RES_5;		/* Reserved */
	Uint	SCSI$L_RES_6;		/* Reserved */
};

#ifdef __ALPHA
#pragma member_alignment save
#pragma nomember_alignment
#endif

struct SCSI$IOSB {
	Ushort	SCSI$W_VMS_STAT;	/* VMS status code */
	Ulong	SCSI$L_IOSB_TFR_CNT;	/* Actual #bytes transferred */
	char	SCSI$B_IOSB_FILL_1;
	Uchar	SCSI$B_IOSB_STS;	/* SCSI device status */
};

#ifdef __ALPHA
#pragma member_alignment restore
#endif

#define SCSI$K_GOOD_STATUS	0
#define SCSI$K_CHECK_CONDITION 0x2
#define SCSI$K_CONDITION_MET 0x4
#define SCSI$K_BUSY 0x8
#define SCSI$K_INTERMEDIATE 0x10
#define SCSI$K_INTERMEDIATE_C_MET 0x14
#define SCSI$K_RESERVATION_CONFLICT 0x18
#define SCSI$K_COMMAND_TERMINATED 0x22
#define SCSI$K_QUEUE_FULL 0x28


#define SCSI$K_WRITE		0X0	/* direction of transfer=write */
#define SCSI$K_READ		0X1	/* direction of transfer=read */
#define SCSI$K_FL_ENAB_DIS	0X2	/* enable disconnects */
#define SCSI$K_FL_ENAB_SYNC	0X4	/* enable sync */
#define GK_EFN			0	/* Event flag number */

static char	gk_device[8];		/* XXX JS hoffentlich gibt es keinen Ueberlauf */
static Ushort	gk_chan;
static Ushort	transfer_length;
static int	i;
static int	status;
static $DESCRIPTOR(gk_device_desc, gk_device);
static struct SCSI$IOSB gk_iosb ;
static struct SCSI$DESC gk_desc;


EXPORT
int scsi_open(device, busno, tgt, tlun)
	char	*device;
	int	busno;
	int	tgt;
	int	tlun;
{
	char	devname[DEVICE_NAMELEN];
	char	buschar;
	char	buschar1;

	if (*device != '\0' || (busno == -2 && tgt == -2)) 
		return (-1);

	if (busno >= 0 && tgt >= 0 && tlun >= 0) {
		if (busno >= MAX_SCG || tgt >= MAX_TGT || tlun >= MAX_LUN)
			return (-1);
	}
	switch (scsibus) {

	case 0:	buschar = 'd';
		buschar1 = 'a';
		break;
	case 1:	buschar = 'd';
		buschar1 = 'b';
		break;
	case 2:	buschar = 'd';
		buschar1 = 'c';
		break;
	case 3:	buschar = 'd';
		buschar1 = 'd';
		break;
	case 4:	buschar = 'g';
		buschar1 = 'a';
		break;
	case 5:	buschar = 'g';
		buschar1 = 'b';
		break;
	case 6:	buschar = 'g';
		buschar1 = 'c';
		break;
	case 7:	buschar = 'g';
		buschar1 = 'd';
		break;
	default : comerrno(EX_BAD, "Wrong scsibus-#");
		/* Will not return */
		return (-1);
	}
	/* XXX JS XXX devname length snprintf ??? */
	sprintf(devname, "%ck%c%d0%d:", buschar, buschar1, target, lun);
	strcpy (gk_device, devname);
	status = sys$assign(&gk_device_desc, &gk_chan, 0, 0);
	if (!(status & 1)) {
		printf("Unable to access scsi-device \"%s\"\n", &gk_device[0]);
		return (-1);
	}
	return (status);
}

LOCAL long
scsi_maxdma()
{
	return (MAX_DMA_VMS);
}

EXPORT
BOOL scsi_havebus(busno)
	int	busno;
{
	if (gk_chan <= 0)
		return (FALSE);
	return (TRUE);
}

EXPORT
int scsi_fileno(busno, tgt, tlun)
	int	busno;
	int	tgt;
	int	tlun;
{
	if (gk_chan <= 0)
		return (-1);
	return (gk_chan);
}

EXPORT
int scsi_isatapi()
{
	return (FALSE);
}

EXPORT
int scsireset()
{
	return (-1);
}

EXPORT void *
scsi_getbuf(amt)
	long	amt;
{
	void	*ret;

	if (scg_maxdma == 0)
		scg_maxdma = scsi_maxdma();

	if (amt <= 0 || amt > scg_maxdma)
		return ((void *)0);
	if (debug)
		printf("scsi_getbuf: %ld bytes\n", amt);
	ret = malloc((size_t)(amt));	/* XXX JS XXX valloc() ??? */
	return (ret);
}

LOCAL int
do_scsi_cmd(f, sp)
	int		f;
	struct scg_cmd	*sp;
{
	char		*cmdadr;
	int		notcmdretry;
	int		len;
	int		severity;

	/* XXX JS XXX This cannot be OK */
	notcmdretry = (sp->flags & SCG_CMD_RETRY)^SCG_CMD_RETRY;
	/* error corrected ehh	*/
/* XXX JS Wenn das notcmdretry Flag bei VMS auch 0x08 ist und Du darauf hoffst,
   XXX	Dasz ich den Wert nie aendere, dann ist das richtig.
   XXX Siehe unten: Das gleiche gilt fuer SCG_RECV_DATA und SCG_DISRE_ENA !!!
*/

	cmdadr = (char *)sp->cdb.cmd_cdb;
	/* XXX JS XXX This cannot be OK */
	gk_desc.SCSI$L_FLAGS = ((sp->flags & SCG_RECV_DATA) |
				(sp->flags & SCG_DISRE_ENA)|
				notcmdretry);
				/* XXX siehe oben, das ist ein bitweises oder!!! */
	gk_desc.SCSI$A_DATA_ADDR = sp->addr;
	gk_desc.SCSI$L_DATA_LEN = sp->size;
	gk_desc.SCSI$A_CMD_ADDR = cmdadr;
	gk_desc.SCSI$L_CMD_LEN = sp->cdb_len;
	gk_desc.SCSI$L_PH_CH_TMOUT = sp->timeout;
	gk_desc.SCSI$L_DISCON_TMOUT = sp->timeout;
	if (gk_desc.SCSI$L_PH_CH_TMOUT > MAX_TMO_VMS)
	    gk_desc.SCSI$L_PH_CH_TMOUT = MAX_TMO_VMS;
	if (gk_desc.SCSI$L_DISCON_TMOUT > MAX_TMO_VMS)
	    gk_desc.SCSI$L_DISCON_TMOUT = MAX_TMO_VMS;
	gk_desc.SCSI$L_OPCODE = 1;	/* SCSI Operation Code */
	gk_desc.SCSI$L_PAD_LEN = 0;	/* SCSI pad length, bytes */
	gk_desc.SCSI$L_RES_1 = 0;	/* Reserved */
	gk_desc.SCSI$L_RES_2 = 0;	/* Reserved */
	gk_desc.SCSI$L_RES_3 = 0;	/* Reserved */
	gk_desc.SCSI$L_RES_4 = 0;	/* Reserved */
	gk_desc.SCSI$L_RES_5 = 0;	/* Reserved */
	gk_desc.SCSI$L_RES_6 = 0;	/* Reserved */
	if (debug) {
		printf("***********************************************************\n");
		printf("SCSI VMS-I/O parameters\n");
		printf("OPCODE: %d", gk_desc.SCSI$L_OPCODE);
		printf(" FLAGS: %d\n", gk_desc.SCSI$L_FLAGS);
		printf("CMD:");
		for (i = 0; i < gk_desc.SCSI$L_CMD_LEN; i++) {
			printf("%x ", sp->cdb.cmd_cdb[i]);
		}
		printf("\n");
		printf("DATA_LEN: %d\n", gk_desc.SCSI$L_DATA_LEN);
		printf("PH_CH_TMOUT: %d", gk_desc.SCSI$L_PH_CH_TMOUT);
		printf(" DISCON_TMOUT: %d\n", gk_desc.SCSI$L_DISCON_TMOUT);
	}
	status = sys$qiow(GK_EFN, gk_chan, IO$_DIAGNOSE, &gk_iosb, 0, 0,
			&gk_desc, sizeof(gk_desc), 0, 0, 0, 0);

	if (debug) {
		printf("qiow-status: %i\n", status);
		printf("VMS status code %i\n",gk_iosb.SCSI$W_VMS_STAT);
		printf("Actual #bytes transferred %i\n",gk_iosb.SCSI$L_IOSB_TFR_CNT);
		printf("SCSI device status %i\n",gk_iosb.SCSI$B_IOSB_STS);
	}

	if (!(status & 1)) {		/* Fehlerindikation fuer sys$qiow() */
		sp->ux_errno = geterrno();
		/* schwerwiegender nicht SCSI bedingter Fehler => return(-1) */
		if (sp->ux_errno == ENOTTY || sp->ux_errno == ENXIO ||
		    sp->ux_errno == EINVAL || sp->ux_errno == EACCES) {
			return (-1);
		}
	} else {
		sp->ux_errno = 0;
	}

	if (gk_iosb.SCSI$W_VMS_STAT == SS$_NORMAL) {
		sp->error = SCG_NO_ERROR;
		sp->resid = 0;
		return(0);
	}

	severity = gk_iosb.SCSI$W_VMS_STAT & 0x7;

	if (severity < 4) {
		sp->error = SCG_RETRYABLE;
		sp->ux_errno = EIO;
		sp->u_scb.cmd_scb[0] = gk_iosb.SCSI$B_IOSB_STS;
		sp->resid = gk_desc.SCSI$L_DATA_LEN - gk_iosb.SCSI$L_IOSB_TFR_CNT;
		return(0);
	}

	if (gk_iosb.SCSI$W_VMS_STAT == SS$_TIMEOUT) {
		sp->error = SCG_TIMEOUT;
		sp->ux_errno = EIO;
		sp->resid = gk_desc.SCSI$L_DATA_LEN;
		return(0);
	}

	sp->error = SCG_FATAL;
	sp->ux_errno = EIO;
	sp->resid = gk_desc.SCSI$L_DATA_LEN;
	return(0);
}

LOCAL int
do_scsi_sense(f, sp)
	int		f;
	struct scg_cmd	*sp;
{
	int		ret;
	struct scg_cmd	s_cmd;

	if (sp->sense_len > SCG_MAX_SENSE)
		sp->sense_len = SCG_MAX_SENSE;

	fillbytes((caddr_t)&s_cmd, sizeof(s_cmd), '\0');
	s_cmd.addr = (char *)sp->u_sense.cmd_sense;
	s_cmd.size = sp->sense_len;
	s_cmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	s_cmd.cdb_len = SC_G0_CDBLEN;
	s_cmd.sense_len = CCS_SENSE_LEN;
	s_cmd.target = target;
	s_cmd.cdb.g0_cdb.cmd = SC_REQUEST_SENSE;
	s_cmd.cdb.g0_cdb.lun = sp->cdb.g0_cdb.lun;
	s_cmd.cdb.g0_cdb.count = sp->sense_len;
	ret = do_scsi_cmd(f, &s_cmd);

	if (ret < 0)
		return (ret);
	if (s_cmd.u_scb.cmd_scb[0] & 02) {
		/* XXX ??? Check condition on request Sense ??? */
	}
	sp->sense_count = sp->sense_len - s_cmd.resid;
	return (ret);
}

LOCAL int
scsi_send(f, sp)
	int		f;
	struct scg_cmd	*sp;
{
	int	ret;

	if (f < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}
	ret = do_scsi_cmd(f, sp);
	if (ret < 0)
		return (ret);
	if (sp->u_scb.cmd_scb[0] & 02)
		ret = do_scsi_sense(f, sp);
	return (ret);
}
