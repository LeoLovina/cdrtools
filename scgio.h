/* @(#)scgio.h	2.3 96/08/18 Copyright 1986 J. Schilling */
/*
 *	Definitions for the SCSI general driver 'scg'
 *
 *	Copyright (c) 1986 J. Schilling
 */
/* This program is free software; you can redistribute it and/or modify
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

#include <btorder.h>

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */
#elif	defined(_BIT_FIELDS_HTOL)	/* Motorola byteorder */
#else 
#error  One of _BIT_FIELDS_LTOH or _BIT_FIELDS_HTOL must be defined
#endif

/*
 * SCSI status completion block.
 */
#define	SCSI_EXTENDED_STATUS

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_status {
	u_char	vu_00	: 1;	/* vendor unique */
	u_char	chk	: 1;	/* check condition: sense data available */
	u_char	cm	: 1;	/* condition met */
	u_char	busy	: 1;	/* device busy or reserved */
	u_char	is	: 1;	/* intermediate status sent */
	u_char	vu_05	: 1;	/* vendor unique */
#define st_scsi2	vu_05	/* SCSI-2 modifier bit */
	u_char	vu_06	: 1;	/* vendor unique */
	u_char	st_rsvd	: 1;	/* reserved */

#ifdef	SCSI_EXTENDED_STATUS
#define	ext_st1	st_rsvd		/* extended status (next byte valid) */
	/* byte 1 */
	u_char	ha_er	: 1;	/* host adapter detected error */
	u_char	reserved: 6;	/* reserved */
	u_char	ext_st2	: 1;	/* extended status (next byte valid) */
	/* byte 2 */
	u_char	byte2;		/* third byte */
#endif	/* SCSI_EXTENDED_STATUS */
};

#else	/* Motorola byteorder */

struct	scsi_status {
	u_char	st_rsvd	: 1;	/* reserved */
	u_char	vu_06	: 1;	/* vendor unique */
	u_char	vu_05	: 1;	/* vendor unique */
#define st_scsi2	vu_05	/* SCSI-2 modifier bit */
	u_char	is	: 1;	/* intermediate status sent */
	u_char	busy	: 1;	/* device busy or reserved */
	u_char	cm	: 1;	/* condition met */
	u_char	chk	: 1;	/* check condition: sense data available */
	u_char	vu_00	: 1;	/* vendor unique */
#ifdef	SCSI_EXTENDED_STATUS
#define	ext_st1	st_rsvd		/* extended status (next byte valid) */
	/* byte 1 */
	u_char	ext_st2	: 1;	/* extended status (next byte valid) */
	u_char	reserved: 6;	/* reserved */
	u_char	ha_er	: 1;	/* host adapter detected error */
	/* byte 2 */
	u_char	byte2;		/* third byte */
#endif	/* SCSI_EXTENDED_STATUS */
};
#endif

/*
 * OLD Standard (Non Extended) SCSI Sense. Used mainly by the
 * Adaptec ACB 4000 which is the only controller that
 * does not support the Extended sense format.
 */
#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_sense {		/* scsi sense for error classes 0-6 */
	u_char	code	: 7;	/* error class/code */
	u_char	adr_val	: 1;	/* sense data is valid */
#ifdef	comment
	u_char	high_addr:5;	/* high byte of block addr */
	u_char	rsvd	: 3;
#else
	u_char	high_addr;	/* high byte of block addr */
#endif
	u_char	mid_addr;	/* middle byte of block addr */
	u_char	low_addr;	/* low byte of block addr */
};

#else	/* Motorola byteorder */

struct	scsi_sense {		/* scsi sense for error classes 0-6 */
	u_char	adr_val	: 1;	/* sense data is valid */
	u_char	code	: 7;	/* error class/code */
#ifdef	comment
	u_char	rsvd	: 3;
	u_char	high_addr:5;	/* high byte of block addr */
#else
	u_char	high_addr;	/* high byte of block addr */
#endif
	u_char	mid_addr;	/* middle byte of block addr */
	u_char	low_addr;	/* low byte of block addr */
};
#endif

/*
 * SCSI extended sense parameter block.
 */
#ifdef	comment
#define SC_CLASS_EXTENDED_SENSE 0x7     /* indicates extended sense */
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_ext_sense {	/* scsi extended sense for error class 7 */
	/* byte 0 */
	u_char	type	: 7;	/* fixed at 0x70 */
	u_char	adr_val	: 1;	/* sense data is valid */
	/* byte 1 */
	u_char	seg_num;	/* segment number, applies to copy cmd only */
	/* byte 2 */
	u_char	key	: 4;	/* sense key, see below */
	u_char		: 1;	/* reserved */
	u_char	ili	: 1;	/* incorrect length indicator */
	u_char	eom	: 1;	/* end of media */
	u_char	fil_mk	: 1;	/* file mark on device */
	/* bytes 3 through 7 */
	u_char	info_1;		/* information byte 1 */
	u_char	info_2;		/* information byte 2 */
	u_char	info_3;		/* information byte 3 */
	u_char	info_4;		/* information byte 4 */
	u_char	add_len;	/* number of additional bytes */
	/* bytes 8 through 13, CCS additions */
	u_char	optional_8;	/* CCS search and copy only */
	u_char	optional_9;	/* CCS search and copy only */
	u_char	optional_10;	/* CCS search and copy only */
	u_char	optional_11;	/* CCS search and copy only */
	u_char 	error_code;	/* error class & code */
	u_char	:8;		/* reserved */
};

#else	/* Motorola byteorder */

struct	scsi_ext_sense {	/* scsi extended sense for error class 7 */
	/* byte 0 */
	u_char	adr_val	: 1;	/* sense data is valid */
	u_char	type	: 7;	/* fixed at 0x70 */
	/* byte 1 */
	u_char	seg_num;	/* segment number, applies to copy cmd only */
	/* byte 2 */
	u_char	fil_mk	: 1;	/* file mark on device */
	u_char	eom	: 1;	/* end of media */
	u_char	ili	: 1;	/* incorrect length indicator */
	u_char		: 1;	/* reserved */
	u_char	key	: 4;	/* sense key, see below */
	/* bytes 3 through 7 */
	u_char	info_1;		/* information byte 1 */
	u_char	info_2;		/* information byte 2 */
	u_char	info_3;		/* information byte 3 */
	u_char	info_4;		/* information byte 4 */
	u_char	add_len;	/* number of additional bytes */
	/* bytes 8 through 13, CCS additions */
	u_char	optional_8;	/* CCS search and copy only */
	u_char	optional_9;	/* CCS search and copy only */
	u_char	optional_10;	/* CCS search and copy only */
	u_char	optional_11;	/* CCS search and copy only */
	u_char 	error_code;	/* error class & code */
	u_char	:8;		/* reserved */
};
#endif
/*
 * SCSI Operation codes. 
 */
#define SC_TEST_UNIT_READY	0x00
#define SC_REZERO_UNIT		0x01
#define SC_REQUEST_SENSE	0x03
#define SC_FORMAT		0x04
#define SC_FORMAT_TRACK		0x06
#define SC_REASSIGN_BLOCK	0x07		/* CCS only */
#define SC_SEEK			0x0b
#define SC_TRANSLATE		0x0f		/* ACB4000 only */
#define SC_INQUIRY		0x12		/* CCS only */
#define SC_MODE_SELECT		0x15
#define SC_RESERVE		0x16
#define SC_RELEASE		0x17
#define SC_MODE_SENSE		0x1a
#define SC_START		0x1b
#define SC_READ_DEFECT_LIST	0x37		/* CCS only, group 1 */
#define SC_READ_BUFFER          0x3c            /* CCS only, group 1 */
	/*
	 * Note, these two commands use identical command blocks for all
 	 * controllers except the Adaptec ACB 4000 which sets bit 1 of byte 1.
	 */
#define SC_READ			0x08
#define SC_WRITE		0x0a
#define SC_EREAD		0x28		/* 10 byte read */
#define SC_EWRITE		0x2a		/* 10 byte write */
#define SC_WRITE_VERIFY		0x2e            /* 10 byte write+verify */
#define SC_WRITE_FILE_MARK	0x10
#define SC_UNKNOWN		0xff		/* cmd list terminator */


/*
 * Messages that SCSI can send.
 */
#define SC_COMMAND_COMPLETE	0x00
#define SC_SYNCHRONOUS		0x01
#define SC_SAVE_DATA_PTR	0x02
#define SC_RESTORE_PTRS		0x03
#define SC_DISCONNECT		0x04
#define SC_ABORT		0x06
#define SC_MSG_REJECT		0x07
#define SC_NO_OP		0x08
#define SC_PARITY		0x09
#define SC_IDENTIFY		0x80
#define SC_DR_IDENTIFY		0xc0
#define SC_DEVICE_RESET		0x0c

/*
 * Standard SCSI control blocks.
 * These go in or out over the SCSI bus.
 */

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_g0cdb {		/* scsi group 0 command description block */
	u_char	cmd;		/* command code */
	u_char	high_addr : 5;	/* high part of block address */
	u_char	lun	  : 3;	/* logical unit number */
	u_char	mid_addr;	/* middle part of block address */
	u_char	low_addr;	/* low part of block address */
	u_char	count;		/* transfer length */
	u_char	link	  : 1;	/* link (another command follows) */
	u_char	fr	  : 1;	/* flag request (interrupt at completion) */
	u_char	rsvd	  : 4;	/* reserved */
	u_char	vu_56	  : 1;	/* vendor unique (byte 5 bit 6) */
	u_char	vu_57	  : 1;	/* vendor unique (byte 5 bit 7) */
};

#else	/* Motorola byteorder */

struct	scsi_g0cdb {		/* scsi group 0 command description block */
	u_char	cmd;		/* command code */
	u_char	lun	  : 3;	/* logical unit number */
	u_char	high_addr : 5;	/* high part of block address */
	u_char	mid_addr;	/* middle part of block address */
	u_char	low_addr;	/* low part of block address */
	u_char	count;		/* transfer length */
	u_char	vu_57	  : 1;	/* vendor unique (byte 5 bit 7) */
	u_char	vu_56	  : 1;	/* vendor unique (byte 5 bit 6) */
	u_char	rsvd	  : 4;	/* reserved */
	u_char	fr	  : 1;	/* flag request (interrupt at completion) */
	u_char	link	  : 1;	/* link (another command follows) */
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_g1cdb {		/* scsi group 1 command description block */
	u_char	cmd;		/* command code */
	u_char	reladr	  : 1;	/* address is relative */
	u_char	res	  : 4;	/* reserved bits 1-4 of byte 1 */
	u_char	lun	  : 3;	/* logical unit number */
	u_char	addr[4];	/* logical block address */
	u_char	res6;		/* reserved byte 6 */
	u_char	count[2];	/* transfer length */
	u_char	link	  : 1;	/* link (another command follows) */
	u_char	fr	  : 1;	/* flag request (interrupt at completion) */
	u_char	rsvd	  : 4;	/* reserved */
	u_char	vu_96	  : 1;	/* vendor unique (byte 5 bit 6) */
	u_char	vu_97	  : 1;	/* vendor unique (byte 5 bit 7) */
};

#else	/* Motorola byteorder */

struct	scsi_g1cdb {		/* scsi group 1 command description block */
	u_char	cmd;		/* command code */
	u_char	lun	  : 3;	/* logical unit number */
	u_char	res	  : 4;	/* reserved bits 1-4 of byte 1 */
	u_char	reladr	  : 1;	/* address is relative */
	u_char	addr[4];	/* logical block address */
	u_char	res6;		/* reserved byte 6 */
	u_char	count[2];	/* transfer length */
	u_char	vu_97	  : 1;	/* vendor unique (byte 5 bit 7) */
	u_char	vu_96	  : 1;	/* vendor unique (byte 5 bit 6) */
	u_char	rsvd	  : 4;	/* reserved */
	u_char	fr	  : 1;	/* flag request (interrupt at completion) */
	u_char	link	  : 1;	/* link (another command follows) */
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_g5cdb {		/* scsi group 5 command description block */
	u_char	cmd;		/* command code */
	u_char	reladr	  : 1;	/* address is relative */
	u_char	res	  : 4;	/* reserved bits 1-4 of byte 1 */
	u_char	lun	  : 3;	/* logical unit number */
	u_char	addr[4];	/* logical block address */
	u_char	res6;		/* reserved byte 6 */
	u_char	res7;		/* reserved byte 7 */
	u_char	res8;		/* reserved byte 8 */
	u_char	count[2];	/* transfer length */
	u_char	link	  : 1;	/* link (another command follows) */
	u_char	fr	  : 1;	/* flag request (interrupt at completion) */
	u_char	rsvd	  : 4;	/* reserved */
	u_char	vu_B6	  : 1;	/* vendor unique (byte B bit 6) */
	u_char	vu_B7	  : 1;	/* vendor unique (byte B bit 7) */
};

#else	/* Motorola byteorder */

struct	scsi_g5cdb {		/* scsi group 5 command description block */
	u_char	cmd;		/* command code */
	u_char	lun	  : 3;	/* logical unit number */
	u_char	res	  : 4;	/* reserved bits 1-4 of byte 1 */
	u_char	reladr	  : 1;	/* address is relative */
	u_char	addr[4];	/* logical block address */
	u_char	res6;		/* reserved byte 6 */
	u_char	res7;		/* reserved byte 7 */
	u_char	res8;		/* reserved byte 8 */
	u_char	count[2];	/* transfer length */
	u_char	vu_B7	  : 1;	/* vendor unique (byte B bit 7) */
	u_char	vu_B6	  : 1;	/* vendor unique (byte B bit 6) */
	u_char	rsvd	  : 4;	/* reserved */
	u_char	fr	  : 1;	/* flag request (interrupt at completion) */
	u_char	link	  : 1;	/* link (another command follows) */
};
#endif

#define	SC_G0_CDBLEN	6	/* Len of Group 0 commands */
#define	SC_G1_CDBLEN	10	/* Len of Group 1 commands */
#define	SC_G5_CDBLEN	12	/* Len of Group 5 commands */

#define	SCG_MAX_CMD	24	/* 24 bytes max. size is supported */
#define	SCG_MAX_STATUS	3	/* XXX (sollte 4 allign.) Mamimum Status Len */
#define	SCG_MAX_SENSE	32	/* Mamimum Sense Len for auto Req. Sense */

#define	DEF_SENSE_LEN	16	/* Default Sense Len */
#define	CCS_SENSE_LEN	18	/* Sense Len for CCS compatible devices */

struct	scg_cmd {
	caddr_t	addr;			/* Address of data in user space */
	int	size;			/* DMA count for data transfer */
	int	flags;			/* see below for definition */
	int	cdb_len;		/* Size of SCSI command in bytes */
					/* NOTE: rel 4 uses this field only */
					/* with commands not in group 1 or 2*/
	int	sense_len;		/* for intr() if -1 don't get sense */
	int	timeout;		/* timeout in seconds */
					/* NOTE: actual resolution depends */
					/* on driver implementation */
	int	debug;			/* driver debug level */
	int	resid;			/* Bytes not transfered */
	int	error;			/* Error code from scgintr() */
	int	errno;			/* UNIX error code */
#ifdef	comment
XXX	struct	scsi_status scb; ???	/* Status returnd by command */
#endif
	union {
		struct	scsi_status Scb;/* Status returnd by command */
		u_char	cmd_scb[SCG_MAX_STATUS];
	} u_scb;
#define	scb	u_scb.Scb
#ifdef	comment
XXX	struct	scsi_sense sense; ???	/* Sense bytes from command */
#endif
	union {
		struct	scsi_sense Sense;/* Sense bytes from command */
		u_char	cmd_sense[SCG_MAX_SENSE];
	} u_sense;
#define	sense	u_sense.Sense
	int	sense_count;		/* Number of bytes valid in sense */
	int	target;			/* SCSI target id */
	union {				/* SCSI command descriptor block */
		struct	scsi_g0cdb g0_cdb;
		struct	scsi_g1cdb g1_cdb;
		struct	scsi_g5cdb g5_cdb;
		u_char	cmd_cdb[SCG_MAX_CMD];
	} cdb;				/* 24 bytes max. size is supported */
};

#define	dma_read	flags		/* 1 if DMA to Sun, 0 otherwise */

/*
 * definition for flags field in scg_cmd struct
 */
#define	SCG_RECV_DATA	0x0001		/* DMA direction to Sun */
#define	SCG_DISRE_ENA	0x0002		/* enable disconnect/reconnect */
#define	SCG_SILENT	0x0004		/* be silent on errors */
#define	SCG_CMD_RETRY	0x0008		/* enable retries */
#define	SCG_NOPARITY	0x0010		/* disable parity for this command */

/*
 * definition for error field in scg_cmd struct
 *
 * The codes refer to SCSI general errors, not to device
 * specific errors.  Device specific errors are discovered
 * by checking the sense data.
 * The distinction between retryable and fatal is somewhat ad hoc.
 */
#define SCG_NO_ERROR	0		/* cdb transported without error */
#define SCG_RETRYABLE	1		/* any other case */
#define SCG_FATAL	2		/* could not select target */
#define SCG_TIMEOUT	3		/* driver timed out */

#define	g0_cdbaddr(cdb, a)	((cdb)->high_addr = (a) >> 16,\
				 (cdb)->mid_addr = ((a) >> 8) & 0xFF,\
				 (cdb)->low_addr = (a) & 0xFF)

#define	g1_cdbaddr(cdb, a)	((cdb)->addr[0] = (a) >> 24,\
				 (cdb)->addr[1] = ((a) >> 16)& 0xFF,\
				 (cdb)->addr[2] = ((a) >> 8) & 0xFF,\
				 (cdb)->addr[3] = (a) & 0xFF)

#define	g1_cdblen(cdb, len)	((cdb)->count[0] = ((len) >> 8) & 0xFF,\
				 (cdb)->count[1] = (len) & 0xFF)

#define	i_to_long(a, i)		(((u_char *)(a))[0] = ((i) >> 24)& 0xFF,\
				 ((u_char *)(a))[1] = ((i) >> 16)& 0xFF,\
				 ((u_char *)(a))[2] = ((i) >> 8) & 0xFF,\
				 ((u_char *)(a))[3] = (i) & 0xFF)

#define	i_to_3_byte(a, i)	(((u_char *)(a))[0] = ((i) >> 16)& 0xFF,\
				 ((u_char *)(a))[1] = ((i) >> 8) & 0xFF,\
				 ((u_char *)(a))[2] = (i) & 0xFF)

#define	i_to_short(a, i)	(((u_char *)(a))[0] = ((i) >> 8) & 0xFF,\
				 ((u_char *)(a))[1] = (i) & 0xFF)

#define	a_to_u_short(a)	((unsigned short) \
			((((unsigned char*) a)[1]       & 0xFF) | \
			 (((unsigned char*) a)[0] << 8  & 0xFF00)))

#define	a_to_3_byte(a)	((unsigned long) \
			((((unsigned char*) a)[2]       & 0xFF) | \
			 (((unsigned char*) a)[1] << 8  & 0xFF00) | \
			 (((unsigned char*) a)[0] << 16 & 0xFF0000)))

#define	a_to_u_long(a)	((unsigned long) \
			((((unsigned char*) a)[3]       & 0xFF) | \
			 (((unsigned char*) a)[2] << 8  & 0xFF00) | \
			 (((unsigned char*) a)[1] << 16 & 0xFF0000) | \
			 (((unsigned char*) a)[0] << 24 & 0xFF000000)))


/* Peripheral Device Type */

#define	SC_DASD			0x00
#define	SC_SEQD			0x01
#define	SC_PRTD			0x02 
#define	SC_PROCD		0x03 
#define	SC_OPTD			0x04
#define	SC_ROMD			0x05
#define	SC_SCAN			0x06
#define	SC_OMEM			0x07
#define	SC_JUKE			0x08
#define	SC_COMM			0x09
#define	SC_NOTPR		0x7F

#if	defined(SVR4)
#include <sys/ioccom.h>
#endif
#if	defined(__STDC__) || defined(SVR4)
#define	SCGIOCMD	_IOWR('G', 1, struct scg_cmd)	/* do a SCSI cmd   */
#define	SCGIORESET	_IO('G', 2)			/* reset SCSI bus  */
#define	SCGIOGDISRE	_IOR('G', 4, int)		/* get sc disre Val*/
#define	SCGIOSDISRE	_IOW('G', 5, int)		/* set sc disre Val*/
#define	SCGIOIDBG	_IO('G', 100)			/* Inc Debug Val   */
#define	SCGIODDBG	_IO('G', 101)			/* Dec Debug Val   */
#define	SCGIOGDBG	_IOR('G', 102, int)		/* get Debug Val   */
#define	SCGIOSDBG	_IOW('G', 103, int)		/* set Debug Val   */
#define	SCIOGDBG	_IOR('G', 104, int)		/* get sc Debug Val*/
#define	SCIOSDBG	_IOW('G', 105, int)		/* set sc Debug Val*/
#else
#define	SCGIOCMD	_IOWR(G, 1, struct scg_cmd)	/* do a SCSI cmd   */
#define	SCGIORESET	_IO(G, 2)			/* reset SCSI bus  */
#define	SCGIOGDISRE	_IOR(G, 4, int)			/* get sc disre Val*/
#define	SCGIOSDISRE	_IOW(G, 5, int)			/* set sc disre Val*/
#define	SCGIOIDBG	_IO(G, 100)			/* Inc Debug Val   */
#define	SCGIODDBG	_IO(G, 101)			/* Dec Debug Val   */
#define	SCGIOGDBG	_IOR(G, 102, int)		/* get Debug Val   */
#define	SCGIOSDBG	_IOW(G, 103, int)		/* set Debug Val   */
#define	SCIOGDBG	_IOR(G, 104, int)		/* get sc Debug Val*/
#define	SCIOSDBG	_IOW(G, 105, int)		/* set sc Debug Val*/
#endif

#define	SCGIO_CMD	SCGIOCMD	/* backward ccompatibility */
