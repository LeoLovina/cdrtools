/* @(#)scsireg.h	1.14 97/08/24 Copyright 1987 J. Schilling */
/*
 *	usefull definitions for dealing with CCS SCSI - devices
 *
 *	Copyright (c) 1987 J. Schilling
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

#ifndef	_SCSIREG_H
#define	_SCSIREG_H

#include <btorder.h>

/* 
 * Sense key values for extended sense.
 */
#define SC_NO_SENSE		0x00
#define SC_RECOVERABLE_ERROR	0x01
#define SC_NOT_READY		0x02
#define SC_MEDIUM_ERROR		0x03
#define SC_HARDWARE_ERROR	0x04
#define SC_ILLEGAL_REQUEST	0x05
#define SC_UNIT_ATTENTION	0x06
#define SC_WRITE_PROTECT	0x07
#define SC_BLANK_CHECK		0x08
#define SC_VENDOR_UNIQUE	0x09
#define SC_COPY_ABORTED		0x0A
#define SC_ABORTED_COMMAND	0x0B
#define SC_EQUAL		0x0C
#define SC_VOLUME_OVERFLOW	0x0D
#define SC_MISCOMPARE		0x0E
#define SC_RESERVED		0x0F

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_inquiry {
	u_char	type		: 5;	/*  0 */
	u_char	qualifier	: 3;	/*  0 */

	u_char	type_modifier	: 7;	/*  1 */
	u_char	removable	: 1;	/*  1 */

	u_char	ansi_version	: 3;	/*  2 */
	u_char	ecma_version	: 3;	/*  2 */
	u_char	iso_version	: 2;	/*  2 */

	u_char	data_format	: 4;	/*  3 */
	u_char	res3_54		: 2;	/*  3 */
	u_char	termiop		: 1;	/*  3 */
	u_char	aenc		: 1;	/*  3 */

	u_char	add_len		: 8;	/*  4 */
	u_char	sense_len	: 8;	/*  5 */ /* only Emulex ??? */
	u_char	res2		: 8;	/*  6 */

	u_char	softreset	: 1;	/*  7 */
	u_char	cmdque		: 1;
	u_char	res7_2		: 1;
	u_char	linked		: 1;
	u_char	sync		: 1;
	u_char	wbus16		: 1;
	u_char	wbus32		: 1;
	u_char	reladr		: 1;	/*  7 */

	char	vendor_info[8];		/*  8 */
	char	prod_ident[16];		/* 16 */
	char	prod_revision[4];	/* 32 */
#ifdef	comment
	char	vendor_uniq[20];	/* 36 */
	char	reserved[40];		/* 56 */
#endif
};					/* 96 */

#else					/* Motorola byteorder */

struct	scsi_inquiry {
	u_char	qualifier	: 3;	/*  0 */
	u_char	type		: 5;	/*  0 */

	u_char	removable	: 1;	/*  1 */
	u_char	type_modifier	: 7;	/*  1 */

	u_char	iso_version	: 2;	/*  2 */
	u_char	ecma_version	: 3;
	u_char	ansi_version	: 3;	/*  2 */

	u_char	aenc		: 1;	/*  3 */
	u_char	termiop		: 1;
	u_char	res3_54		: 2;
	u_char	data_format	: 4;	/*  3 */

	u_char	add_len		: 8;	/*  4 */
	u_char	sense_len	: 8;	/*  5 */ /* only Emulex ??? */
	u_char	res2		: 8;	/*  6 */
	u_char	reladr		: 1;	/*  7 */
	u_char	wbus32		: 1;
	u_char	wbus16		: 1;
	u_char	sync		: 1;
	u_char	linked		: 1;
	u_char	res7_2		: 1;
	u_char	cmdque		: 1;
	u_char	softreset	: 1;
	char	vendor_info[8];		/*  8 */
	char	prod_ident[16];		/* 16 */
	char	prod_revision[4];	/* 32 */
#ifdef	comment
	char	vendor_uniq[20];	/* 36 */
	char	reserved[40];		/* 56 */
#endif
};					/* 96 */
#endif

#define	info		vendor_info
#define	ident		prod_ident
#define	revision	prod_revision

/* Peripheral Device Qualifier */

#define	INQ_DEV_PRESENT	0x00		/* Physical device present */
#define	INQ_DEV_NOTPR	0x01		/* Physical device not present */
#define	INQ_DEV_RES	0x02		/* Reserved */
#define	INQ_DEV_NOTSUP	0x03		/* Logical unit not supported */

/* Peripheral Device Type */

#define	INQ_DASD	0x00		/* Direct-access device (disk) */
#define	INQ_SEQD	0x01		/* Sequential-access device (tape) */
#define	INQ_PRTD	0x02 		/* Printer device */
#define	INQ_PROCD	0x03 		/* Processor device */
#define	INQ_OPTD	0x04		/* Write once device (optical disk) */
#define	INQ_WORM	0x04		/* Write once device (optical disk) */
#define	INQ_ROMD	0x05		/* CD-ROM device */
#define	INQ_SCAN	0x06		/* Scanner device */
#define	INQ_OMEM	0x07		/* Optical Memory device */
#define	INQ_JUKE	0x08		/* Medium Changer device (jukebox) */
#define	INQ_COMM	0x09		/* Communications device */
#define	INQ_IT8_1	0x0A		/* IT8 */
#define	INQ_IT8_2	0x0B		/* IT8 */
#define	INQ_STARR	0x0C		/* Storage array device */
#define	INQ_ENCL	0x0D		/* Enclosure services device */
#define	INQ_NODEV	0x1F		/* Unknown or no device */
#define	INQ_NOTPR	0x1F		/* Logical unit not present (SCSI-1) */

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_header {
	u_char	sense_data_len	: 8;
	u_char	medium_type;
	u_char	res2		: 4;
	u_char	cache		: 1;
	u_char	res		: 2;
	u_char	write_prot	: 1;
	u_char	blockdesc_len;
};

#else					/* Motorola byteorder */

struct scsi_mode_header {
	u_char	sense_data_len	: 8;
	u_char	medium_type;
	u_char	write_prot	: 1;
	u_char	res		: 2;
	u_char	cache		: 1;
	u_char	res2		: 4;
	u_char	blockdesc_len;
};
#endif

struct scsi_modesel_header {
	u_char	sense_data_len	: 8;
	u_char	medium_type;
	u_char	res2		: 8;
	u_char	blockdesc_len;
};

struct scsi_mode_blockdesc {
	u_char	density;
	u_char	nlblock[3];
	u_char	res		: 8;
	u_char	lblen[3];
};

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct acb_mode_data {
	u_char	listformat;
	u_char	ncyl[2];
	u_char	nhead;
	u_char	start_red_wcurrent[2];
	u_char	start_precomp[2];
	u_char	landing_zone;
	u_char	step_rate;
	u_char			: 2;
	u_char	hard_sec	: 1;
	u_char	fixed_media	: 1;
	u_char			: 4;
	u_char	sect_per_trk;
};

#else					/* Motorola byteorder */

struct acb_mode_data {
	u_char	listformat;
	u_char	ncyl[2];
	u_char	nhead;
	u_char	start_red_wcurrent[2];
	u_char	start_precomp[2];
	u_char	landing_zone;
	u_char	step_rate;
	u_char			: 4;
	u_char	fixed_media	: 1;
	u_char	hard_sec	: 1;
	u_char			: 2;
	u_char	sect_per_trk;
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_header {
	u_char	p_code		: 6;
	u_char	res		: 1;
	u_char	parsave		: 1;
	u_char	p_len;
};

/*
 * This is a hack that allows mode pages without
 * any further bitfileds to be defined bitorder independent.
 */
#define	MP_P_CODE			\
		p_code		: 6;	\
	u_char	p_res		: 1;	\
	u_char	parsave		: 1

#else					/* Motorola byteorder */

struct scsi_mode_page_header {
	u_char	parsave		: 1;
	u_char	res		: 1;
	u_char	p_code		: 6;
	u_char	p_len;
};

/*
 * This is a hack that allows mode pages without
 * any further bitfileds to be defined bitorder independent.
 */
#define	MP_P_CODE			\
		parsave		: 1;	\
	u_char	p_res		: 1;	\
	u_char	p_code		: 6

#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_01 {		/* Error recovery Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0A = 12 Bytes */
	u_char	disa_correction	: 1;	/* Byte 2 */
	u_char	term_on_rec_err	: 1;
	u_char	report_rec_err	: 1;
	u_char	en_early_corr	: 1;
	u_char	read_continuous	: 1;
	u_char	tranfer_block	: 1;
	u_char	en_auto_reall_r	: 1;
	u_char	en_auto_reall_w	: 1;	/* Byte 2 */
	u_char	rd_retry_count;		/* Byte 3 */
	u_char	correction_span;
	char	head_offset_count;
	char	data_strobe_offset;
	u_char	res;
	u_char	wr_retry_count;
	u_char	res_tape[2];
	u_char	recov_timelim[2];
};

#else					/* Motorola byteorder */

struct scsi_mode_page_01 {		/* Error recovery Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0A = 12 Bytes */
	u_char	en_auto_reall_w	: 1;	/* Byte 2 */
	u_char	en_auto_reall_r	: 1;
	u_char	tranfer_block	: 1;
	u_char	read_continuous	: 1;
	u_char	en_early_corr	: 1;
	u_char	report_rec_err	: 1;
	u_char	term_on_rec_err	: 1;
	u_char	disa_correction	: 1;	/* Byte 2 */
	u_char	rd_retry_count;		/* Byte 3 */
	u_char	correction_span;
	char	head_offset_count;
	char	data_strobe_offset;
	u_char	res;
	u_char	wr_retry_count;
	u_char	res_tape[2];
	u_char	recov_timelim[2];
};
#endif


#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_02 {		/* Device dis/re connect Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0E = 16 Bytes */
	u_char	buf_full_ratio;
	u_char	buf_empt_ratio;
	u_char	bus_inact_limit[2];
	u_char	disc_time_limit[2];
	u_char	conn_time_limit[2];
	u_char	max_burst_size[2];	/* Start SCSI-2 */
	u_char	data_tr_dis_ctl	: 2;
	u_char			: 6;
	u_char	res[3];
};

#else					/* Motorola byteorder */

struct scsi_mode_page_02 {		/* Device dis/re connect Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0E = 16 Bytes */
	u_char	buf_full_ratio;
	u_char	buf_empt_ratio;
	u_char	bus_inact_limit[2];
	u_char	disc_time_limit[2];
	u_char	conn_time_limit[2];
	u_char	max_burst_size[2];	/* Start SCSI-2 */
	u_char			: 6;
	u_char	data_tr_dis_ctl	: 2;
	u_char	res[3];
};
#endif

#define	DTDC_DATADONE	0x01		/*
					 * Target may not disconnect once
					 * data transfer is started until
					 * all data successfully transferred.
					 */

#define	DTDC_CMDDONE	0x03		/*
					 * Target may not disconnect once
					 * data transfer is started until
					 * command completed.
					 */


#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_03 {		/* Direct access format Paramters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x16 = 24 Bytes */
	u_char	trk_per_zone[2];
	u_char	alt_sec_per_zone[2];
	u_char	alt_trk_per_zone[2];
	u_char	alt_trk_per_vol[2];
	u_char	sect_per_trk[2];
	u_char	bytes_per_phys_sect[2];
	u_char	interleave[2];
	u_char	trk_skew[2];
	u_char	cyl_skew[2];
	u_char			: 3;
	u_char	inhibit_save	: 1;
	u_char	fmt_by_surface	: 1;
	u_char	removable	: 1;
	u_char	hard_sec	: 1;
	u_char	soft_sec	: 1;
	u_char	res[3];
};

#else					/* Motorola byteorder */

struct scsi_mode_page_03 {		/* Direct access format Paramters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x16 = 24 Bytes */
	u_char	trk_per_zone[2];
	u_char	alt_sec_per_zone[2];
	u_char	alt_trk_per_zone[2];
	u_char	alt_trk_per_vol[2];
	u_char	sect_per_trk[2];
	u_char	bytes_per_phys_sect[2];
	u_char	interleave[2];
	u_char	trk_skew[2];
	u_char	cyl_skew[2];
	u_char	soft_sec	: 1;
	u_char	hard_sec	: 1;
	u_char	removable	: 1;
	u_char	fmt_by_surface	: 1;
	u_char	inhibit_save	: 1;
	u_char			: 3;
	u_char	res[3];
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_04 {		/* Rigid disk Geometry Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x16 = 24 Bytes */
	u_char	ncyl[3];
	u_char	nhead;
	u_char	start_precomp[3];
	u_char	start_red_wcurrent[3];
	u_char	step_rate[2];
	u_char	landing_zone[3];
	u_char	rot_pos_locking	: 2;	/* Start SCSI-2 */
	u_char			: 6;	/* Start SCSI-2 */
	u_char	rotational_off;
	u_char	res1;
	u_char	rotation_rate[2];
	u_char	res2[2];
};

#else					/* Motorola byteorder */

struct scsi_mode_page_04 {		/* Rigid disk Geometry Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x16 = 24 Bytes */
	u_char	ncyl[3];
	u_char	nhead;
	u_char	start_precomp[3];
	u_char	start_red_wcurrent[3];
	u_char	step_rate[2];
	u_char	landing_zone[3];
	u_char			: 6;	/* Start SCSI-2 */
	u_char	rot_pos_locking	: 2;	/* Start SCSI-2 */
	u_char	rotational_off;
	u_char	res1;
	u_char	rotation_rate[2];
	u_char	res2[2];
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_05 {		/* Flexible disk Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x1E = 32 Bytes */
	u_char	transfer_rate[2];
	u_char	nhead;
	u_char	sect_per_trk;
	u_char	bytes_per_phys_sect[2];
	u_char	ncyl[2];
	u_char	start_precomp[2];
	u_char	start_red_wcurrent[2];
	u_char	step_rate[2];
	u_char	step_pulse_width;
	u_char	head_settle_delay[2];
	u_char	motor_on_delay;
	u_char	motor_off_delay;
	u_char	spc		: 4;
	u_char			: 4;
	u_char			: 5;
	u_char	mo		: 1;
	u_char	ssn		: 1;
	u_char	trdy		: 1;
	u_char	write_compensation;
	u_char	head_load_delay;
	u_char	head_unload_delay;
	u_char	pin_2_use	: 4;
	u_char	pin_34_use	: 4;
	u_char	pin_1_use	: 4;
	u_char	pin_4_use	: 4;
	u_char	rotation_rate[2];
	u_char	res[2];
};

#else					/* Motorola byteorder */

struct scsi_mode_page_05 {		/* Flexible disk Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x1E = 32 Bytes */
	u_char	transfer_rate[2];
	u_char	nhead;
	u_char	sect_per_trk;
	u_char	bytes_per_phys_sect[2];
	u_char	ncyl[2];
	u_char	start_precomp[2];
	u_char	start_red_wcurrent[2];
	u_char	step_rate[2];
	u_char	step_pulse_width;
	u_char	head_settle_delay[2];
	u_char	motor_on_delay;
	u_char	motor_off_delay;
	u_char	trdy		: 1;
	u_char	ssn		: 1;
	u_char	mo		: 1;
	u_char			: 5;
	u_char			: 4;
	u_char	spc		: 4;
	u_char	write_compensation;
	u_char	head_load_delay;
	u_char	head_unload_delay;
	u_char	pin_34_use	: 4;
	u_char	pin_2_use	: 4;
	u_char	pin_4_use	: 4;
	u_char	pin_1_use	: 4;
	u_char	rotation_rate[2];
	u_char	res[2];
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_07 {		/* Verify Error recovery */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0A = 12 Bytes */
	u_char	disa_correction	: 1;	/* Byte 2 */
	u_char	term_on_rec_err	: 1;
	u_char	report_rec_err	: 1;
	u_char	en_early_corr	: 1;
	u_char	res		: 4;	/* Byte 2 */
	u_char	ve_retry_count;		/* Byte 3 */
	u_char	ve_correction_span;
	char	res2[5];		/* Byte 5 */
	u_char	ve_recov_timelim[2];	/* Byte 10 */
};

#else					/* Motorola byteorder */

struct scsi_mode_page_07 {		/* Verify Error recovery */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0A = 12 Bytes */
	u_char	res		: 4;	/* Byte 2 */
	u_char	en_early_corr	: 1;
	u_char	report_rec_err	: 1;
	u_char	term_on_rec_err	: 1;
	u_char	disa_correction	: 1;	/* Byte 2 */
	u_char	ve_retry_count;		/* Byte 3 */
	u_char	ve_correction_span;
	char	res2[5];		/* Byte 5 */
	u_char	ve_recov_timelim[2];	/* Byte 10 */
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_08 {		/* Caching Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0A = 12 Bytes */
	u_char	disa_rd_cache	: 1;	/* Byte 2 */
	u_char	muliple_fact	: 1;
	u_char	en_wt_cache	: 1;
	u_char	res		: 5;	/* Byte 2 */
	u_char	wt_ret_pri	: 4;	/* Byte 3 */
	u_char	demand_rd_ret_pri: 4;	/* Byte 3 */
	u_char	disa_pref_tr_len[2];	/* Byte 4 */
	u_char	min_pref[2];		/* Byte 6 */
	u_char	max_pref[2];		/* Byte 8 */
	u_char	max_pref_ceiling[2];	/* Byte 10 */
};

#else					/* Motorola byteorder */

struct scsi_mode_page_08 {		/* Caching Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0A = 12 Bytes */
	u_char	res		: 5;	/* Byte 2 */
	u_char	en_wt_cache	: 1;
	u_char	muliple_fact	: 1;
	u_char	disa_rd_cache	: 1;	/* Byte 2 */
	u_char	demand_rd_ret_pri: 4;	/* Byte 3 */
	u_char	wt_ret_pri	: 4;
	u_char	disa_pref_tr_len[2];	/* Byte 4 */
	u_char	min_pref[2];		/* Byte 6 */
	u_char	max_pref[2];		/* Byte 8 */
	u_char	max_pref_ceiling[2];	/* Byte 10 */
};
#endif

struct scsi_mode_page_09 {		/* Peripheral device Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* >= 0x06 = 8 Bytes */
	u_char	interface_id[2];	/* Byte 2 */
	u_char	res[4];			/* Byte 4 */
	u_char	vendor_specific[1];	/* Byte 8 */
};

#define	PDEV_SCSI	0x0000		/* scsi interface */
#define	PDEV_SMD	0x0001		/* SMD interface */
#define	PDEV_ESDI	0x0002		/* ESDI interface */
#define	PDEV_IPI2	0x0003		/* IPI-2 interface */
#define	PDEV_IPI3	0x0004		/* IPI-3 interface */

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_0A {		/* Common device Control Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x06 = 8 Bytes */
	u_char	rep_log_exeption: 1;	/* Byte 2 */
	u_char	res		: 7;	/* Byte 2 */
	u_char	dis_queuing	: 1;	/* Byte 3 */
	u_char	queuing_err_man	: 1;
	u_char	res2		: 2;
	u_char	queue_alg_mod	: 4;	/* Byte 3 */
	u_char	EAENP		: 1;	/* Byte 4 */
	u_char	UAENP		: 1;
	u_char	RAENP		: 1;
	u_char	res3		: 4;
	u_char	en_ext_cont_all	: 1;	/* Byte 4 */
	u_char	res4		: 8;
	u_char	ready_aen_hold_per[2];	/* Byte 6 */
};

#else					/* Motorola byteorder */

struct scsi_mode_page_0A {		/* Common device Control Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x06 = 8 Bytes */
	u_char	res		: 7;	/* Byte 2 */
	u_char	rep_log_exeption: 1;	/* Byte 2 */
	u_char	queue_alg_mod	: 4;	/* Byte 3 */
	u_char	res2		: 2;
	u_char	queuing_err_man	: 1;
	u_char	dis_queuing	: 1;	/* Byte 3 */
	u_char	en_ext_cont_all	: 1;	/* Byte 4 */
	u_char	res3		: 4;
	u_char	RAENP		: 1;
	u_char	UAENP		: 1;
	u_char	EAENP		: 1;	/* Byte 4 */
	u_char	res4		: 8;
	u_char	ready_aen_hold_per[2];	/* Byte 6 */
};
#endif

#define	CTRL_QMOD_RESTRICT	0x0
#define	CTRL_QMOD_UNRESTRICT	0x1


struct scsi_mode_page_0B {		/* Medium Types Supported Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x06 = 8 Bytes */
	u_char	res[2];			/* Byte 2 */
	u_char	medium_one_supp;	/* Byte 4 */
	u_char	medium_two_supp;	/* Byte 5 */
	u_char	medium_three_supp;	/* Byte 6 */
	u_char	medium_four_supp;	/* Byte 7 */
};

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_0C {		/* Notch & Partition Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x16 = 24 Bytes */
	u_char	res		: 6;	/* Byte 2 */
	u_char	logical_notch	: 1;
	u_char	notched_drive	: 1;	/* Byte 2 */
	u_char	res2;			/* Byte 3 */
	u_char	max_notches[2];		/* Byte 4  */
	u_char	active_notch[2];	/* Byte 6  */
	u_char	starting_boundary[4];	/* Byte 8  */
	u_char	ending_boundary[4];	/* Byte 12 */
	u_char	pages_notched[8];	/* Byte 16 */
};

#else					/* Motorola byteorder */

struct scsi_mode_page_0C {		/* Notch & Partition Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x16 = 24 Bytes */
	u_char	notched_drive	: 1;	/* Byte 2 */
	u_char	logical_notch	: 1;
	u_char	res		: 6;	/* Byte 2 */
	u_char	res2;			/* Byte 3 */
	u_char	max_notches[2];		/* Byte 4  */
	u_char	active_notch[2];	/* Byte 6  */
	u_char	starting_boundary[4];	/* Byte 8  */
	u_char	ending_boundary[4];	/* Byte 12 */
	u_char	pages_notched[8];	/* Byte 16 */
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_0D {		/* CD-ROM Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x06 = 8 Bytes */
	u_char	res;			/* Byte 2 */
	u_char	inact_timer_mult: 4;	/* Byte 3 */
	u_char	res2		: 4;	/* Byte 3 */
	u_char	s_un_per_m_un[2];	/* Byte 4  */
	u_char	f_un_per_s_un[2];	/* Byte 6  */
};

#else					/* Motorola byteorder */

struct scsi_mode_page_0D {		/* CD-ROM Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x06 = 8 Bytes */
	u_char	res;			/* Byte 2 */
	u_char	res2		: 4;	/* Byte 3 */
	u_char	inact_timer_mult: 4;	/* Byte 3 */
	u_char	s_un_per_m_un[2];	/* Byte 4  */
	u_char	f_un_per_s_un[2];	/* Byte 6  */
};
#endif

struct sony_mode_page_20 {		/* Sony Format Mode Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0A = 12 Bytes */
	u_char	format_mode;
	u_char	format_type;
#define	num_bands	user_band_size	/* Gilt bei Type 1 */
	u_char	user_band_size[4];	/* Gilt bei Type 0 */
	u_char	spare_band_size[2];
	u_char	res[2];
};

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct toshiba_mode_page_20 {		/* Toshiba Speed Control Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x01 = 3 Bytes */
	u_char	speed		: 1;
	u_char	res		: 7;
};

#else					/* Motorola byteorder */

struct toshiba_mode_page_20 {		/* Toshiba Speed Control Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x01 = 3 Bytes */
	u_char	res		: 7;
	u_char	speed		: 1;
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct ccs_mode_page_38 {		/* CCS Caching Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0E = 14 Bytes */

	u_char	cache_table_size: 4;	/* Byte 3 */
	u_char	cache_en	: 1;
	u_char	res2		: 1;
	u_char	wr_index_en	: 1;
	u_char	res		: 1;	/* Byte 3 */
	u_char	threshold;		/* Byte 4 Prefetch threshold */
	u_char	max_prefetch;		/* Byte 5 Max. prefetch */
	u_char	max_multiplier;		/* Byte 6 Max. prefetch multiplier */
	u_char	min_prefetch;		/* Byte 7 Min. prefetch */
	u_char	min_multiplier;		/* Byte 8 Min. prefetch multiplier */
	u_char	res3[8];		/* Byte 9 */
};

#else					/* Motorola byteorder */

struct ccs_mode_page_38 {		/* CCS Caching Parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x0E = 14 Bytes */

	u_char	res		: 1;	/* Byte 3 */
	u_char	wr_index_en	: 1;
	u_char	res2		: 1;
	u_char	cache_en	: 1;
	u_char	cache_table_size: 4;	/* Byte 3 */
	u_char	threshold;		/* Byte 4 Prefetch threshold */
	u_char	max_prefetch;		/* Byte 5 Max. prefetch */
	u_char	max_multiplier;		/* Byte 6 Max. prefetch multiplier */
	u_char	min_prefetch;		/* Byte 7 Min. prefetch */
	u_char	min_multiplier;		/* Byte 8 Min. prefetch multiplier */
	u_char	res3[8];		/* Byte 9 */
};
#endif

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct cd_mode_page_05 {		/* write parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x32 = 50 Bytes */
	u_char	write_type	:4;	/* Session write type (PACKET/TAO...)*/
	u_char	test_write	:1;	/* Do not actually write data	     */
	u_char	res_2		:3;
	u_char	track_mode	:4;	/* Track mode (Q-sub control nibble) */
	u_char	copy		:1;	/* 1st higher gen of copy prot track ~*/
	u_char	fp		:1;	/* Fixed packed (if in packet mode)  */
	u_char	multi_session	:2;	/* Multi session write type	     */
	u_char	dbtype		:4;	/* Data block type		     */
	u_char	res_4		:4;	/* Reserved			     */
	u_char	res_56[2];		/* Reserved			     */
	u_char	host_appl_code	:6;	/* Host application code of disk     */
	u_char	res_7		:2;	/* Reserved			     */
	u_char	session_format;		/* Session format (DA/CDI/XA)	     */
	u_char	res_9;			/* Reserved			     */
	u_char	packet_size[4];		/* # of user datablocks/fixed packet */
	u_char	audio_pause_len[2];	/* # of blocks where index is zero   */
	u_char	media_cat_number[16];	/* Media catalog Number (MCN)	     */
	u_char	ISRC[14];		/* ISRC for this track		     */
	u_char	sub_header[4];
	u_char	vendor_uniq[4];
};

#else				/* Motorola byteorder */

struct cd_mode_page_05 {		/* write parameters */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x32 = 50 Bytes */
	u_char	res_2		:3;
	u_char	test_write	:1;	/* Do not actually write data	     */
	u_char	write_type	:4;	/* Session write type (PACKET/TAO...)*/
	u_char	multi_session	:2;	/* Multi session write type	     */
	u_char	fp		:1;	/* Fixed packed (if in packet mode)  */
	u_char	copy		:1;	/* 1st higher gen of copy prot track */
	u_char	track_mode	:4;	/* Track mode (Q-sub control nibble) */
	u_char	res_4		:4;	/* Reserved			     */
	u_char	dbtype		:4;	/* Data block type		     */
	u_char	res_56[2];		/* Reserved			     */
	u_char	res_7		:2;	/* Reserved			     */
	u_char	host_appl_code	:6;	/* Host application code of disk     */
	u_char	session_format;		/* Session format (DA/CDI/XA)	     */
	u_char	res_9;			/* Reserved			     */
	u_char	packet_size[4];		/* # of user datablocks/fixed packet */
	u_char	audio_pause_len[2];	/* # of blocks where index is zero   */
	u_char	media_cat_number[16];	/* Media catalog Number (MCN)	     */
	u_char	ISRC[14];		/* ISRC for this track		     */
	u_char	sub_header[4];
	u_char	vendor_uniq[4];
};

#endif

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct cd_mode_page_2A {		/* CD Cap / mech status */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x14 = 20 Bytes */
	u_char	cd_r_read	:1;	/* Reads CD-R  media		     */
	u_char	cd_rw_read	:1;	/* Reads CD-RW media		     */
	u_char	method2		:1;	/* Reads fixed packet method2 media  */
	u_char	res_2		:5;
	u_char	cd_r_write	:1;	/* Supports writing CD-R  media	     */
	u_char	cd_rw_write	:1;	/* Supports writing CD-RW media	     */
	u_char	test_write	:1;	/* Supports emulation write	     */
	u_char	res_3		:5;	/* Reseved			     */
	u_char	audio_play	:1;	/* Supports Audio play operation     */
	u_char	composite	:1;	/* Deliveres composite A/V stream    */
	u_char	digital_port_2	:1;	/* Supports digital output on port 2 */
	u_char	digital_port_1	:1;	/* Supports digital output on port 1 */
	u_char	mode_2_form_1	:1;	/* Reads Mode-2 form 1 media (XA)    */
	u_char	mode_2_form_2	:1;	/* Reads Mode-2 form 2 media	     */
	u_char	multi_session	:1;	/* Reads multi-session media	     */
	u_char	res_4		:1;	/* Reserved			     */
	u_char	cd_da_supported	:1;	/* Reads audio data with READ CD cmd */
	u_char	cd_da_accurate	:1;	/* READ CD data stream is accurate   */
	u_char	rw_supported	:1;	/* Reads R-W sub channel information */
	u_char	rw_deint_corr	:1;	/* Reads de-interleved R-W sub chan  */
	u_char	c2_pointers	:1;	/* Supports C2 error pointers	     */
	u_char	ISRC		:1;	/* Reads ISRC information	     */
	u_char	UPC		:1;	/* Reads media catalog number (UPC)  */
	u_char	read_bar_code	:1;	/* Supports reading bar codes	     */
	u_char	lock		:1;	/* PREVENT/ALLOW may lock media	     */
	u_char	lock_state	:1;	/* Lock state 0=unlocked 1=locked    */
	u_char	prevent_jumper	:1;	/* State of prev/allow jumper 0=pres */
	u_char	eject		:1;	/* Ejects disc/cartr with STOP LoEj  */
	u_char	res_6_4		:1;	/* Reserved			     */
	u_char	loading_type	:3;	/* Loading mechanism type	     */
	u_char	sep_chan_vol	:1;	/* Vol controls each channel separat */
	u_char	sep_chan_mute	:1;	/* Mute controls each channel separat*/
	u_char	disk_present_rep:1;	/* Changer supports disk present rep */
	u_char	sw_slot_sel	:1;	/* Load empty slot in changer	     */
	u_char	res_7		:4;	/* Reserved			     */
	u_char	max_read_speed[2];	/* Max. read speed in KB/s	     */
	u_char	num_vol_levels[2];	/* # of supported volume levels	     */
	u_char	buffer_size[2];		/* Buffer size for the data in KB    */
	u_char	cur_read_speed[2];	/* Current read speed in KB/s	     */
	u_char	res_16;			/* Reserved			     */
	u_char	res_17_0	:1;	/* Reserved			     */
	u_char	BCK		:1;	/* Data valid on falling edge of BCK */
	u_char	RCK		:1;	/* Set: HIGH high LRCK=left channel  */
	u_char	LSBF		:1;	/* Set: LSB first Clear: MSB first   */
	u_char	length		:2;	/* 0=32BCKs 1=16BCKs 2=24BCKs 3=24I2c*/
	u_char	res_17		:2;	/* Reserved			     */
	u_char	max_write_speed[2];	/* Max. write speed supported in KB/s*/
	u_char	cur_write_speed[2];	/* Current write speed in KB/s	     */
};

#else				/* Motorola byteorder */

struct cd_mode_page_2A {		/* CD Cap / mech status */
	u_char	MP_P_CODE;		/* parsave & pagecode */
	u_char	p_len;			/* 0x14 = 20 Bytes */
	u_char	res_2		:5;
	u_char	method2		:1;	/* Reads fixed packet method2 media  */
	u_char	cd_rw_read	:1;	/* Reads CD-RW media		     */
	u_char	cd_r_read	:1;	/* Reads CD-R  media		     */
	u_char	res_3		:5;	/* Reseved			     */
	u_char	test_write	:1;	/* Supports emulation write	     */
	u_char	cd_rw_write	:1;	/* Supports writing CD-RW media	     */
	u_char	cd_r_write	:1;	/* Supports writing CD-R  media	     */
	u_char	res_4		:1;	/* Reserved			     */
	u_char	multi_session	:1;	/* Reads multi-session media	     */
	u_char	mode_2_form_2	:1;	/* Reads Mode-2 form 2 media	     */
	u_char	mode_2_form_1	:1;	/* Reads Mode-2 form 1 media (XA)    */
	u_char	digital_port_1	:1;	/* Supports digital output on port 1 */
	u_char	digital_port_2	:1;	/* Supports digital output on port 2 */
	u_char	composite	:1;	/* Deliveres composite A/V stream    */
	u_char	audio_play	:1;	/* Supports Audio play operation     */
	u_char	read_bar_code	:1;	/* Supports reading bar codes	     */
	u_char	UPC		:1;	/* Reads media catalog number (UPC)  */
	u_char	ISRC		:1;	/* Reads ISRC information	     */
	u_char	c2_pointers	:1;	/* Supports C2 error pointers	     */
	u_char	rw_deint_corr	:1;	/* Reads de-interleved R-W sub chan  */
	u_char	rw_supported	:1;	/* Reads R-W sub channel information */
	u_char	cd_da_accurate	:1;	/* READ CD data stream is accurate   */
	u_char	cd_da_supported	:1;	/* Reads audio data with READ CD cmd */
	u_char	loading_type	:3;	/* Loading mechanism type	     */
	u_char	res_6_4		:1;	/* Reserved			     */
	u_char	eject		:1;	/* Ejects disc/cartr with STOP LoEj  */
	u_char	prevent_jumper	:1;	/* State of prev/allow jumper 0=pres */
	u_char	lock_state	:1;	/* Lock state 0=unlocked 1=locked    */
	u_char	lock		:1;	/* PREVENT/ALLOW may lock media	     */
	u_char	res_7		:4;	/* Reserved			     */
	u_char	sw_slot_sel	:1;	/* Load empty slot in changer	     */
	u_char	disk_present_rep:1;	/* Changer supports disk present rep */
	u_char	sep_chan_mute	:1;	/* Mute controls each channel separat*/
	u_char	sep_chan_vol	:1;	/* Vol controls each channel separat */
	u_char	max_read_speed[2];	/* Max. read speed in KB/s	     */
	u_char	num_vol_levels[2];	/* # of supported volume levels	     */
	u_char	buffer_size[2];		/* Buffer size for the data in KB    */
	u_char	cur_read_speed[2];	/* Current read speed in KB/s	     */
	u_char	res_16;			/* Reserved			     */
	u_char	res_17		:2;	/* Reserved			     */
	u_char	length		:2;	/* 0=32BCKs 1=16BCKs 2=24BCKs 3=24I2c*/
	u_char	LSBF		:1;	/* Set: LSB first Clear: MSB first   */
	u_char	RCK		:1;	/* Set: HIGH high LRCK=left channel  */
	u_char	BCK		:1;	/* Data valid on falling edge of BCK */
	u_char	res_17_0	:1;	/* Reserved			     */
	u_char	max_write_speed[2];	/* Max. write speed supported in KB/s*/
	u_char	cur_write_speed[2];	/* Current write speed in KB/s	     */
};

#endif

#define	LT_CADDY	0
#define	LT_TRAY		1
#define	LT_POP_UP	2
#define	LT_RES3		3
#define	LT_CHANGER_IND	4
#define	LT_CHANGER_CART	5
#define	LT_RES6		6
#define	LT_RES7		7


struct scsi_mode_data {
	struct scsi_mode_header		header;
	struct scsi_mode_blockdesc	blockdesc;
	union	pagex	{
		struct acb_mode_data		acb;
		struct scsi_mode_page_01	page1;
		struct scsi_mode_page_02	page2;
		struct scsi_mode_page_03	page3;
		struct scsi_mode_page_04	page4;
		struct scsi_mode_page_05	page5;
		struct scsi_mode_page_07	page7;
		struct scsi_mode_page_08	page8;
		struct scsi_mode_page_09	page9;
		struct scsi_mode_page_0A	pageA;
		struct scsi_mode_page_0B	pageB;
		struct scsi_mode_page_0C	pageC;
		struct scsi_mode_page_0D	pageD;
		struct sony_mode_page_20	sony20;
		struct toshiba_mode_page_20	toshiba20;
		struct ccs_mode_page_38		ccs38;
	} pagex;
};

struct scsi_capacity {
	long	c_baddr;		/* must convert byteorder!! */
	long	c_bsize;		/* must convert byteorder!! */
};

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_def_header {
	u_char		:8;
	u_char	format	:3;
	u_char	gdl	:1;
	u_char	mdl	:1;
	u_char		:3;
	u_char	length[2];
};

#else					/* Motorola byteorder */

struct scsi_def_header {
	u_char		:8;
	u_char		:3;
	u_char	mdl	:1;
	u_char	gdl	:1;
	u_char	format	:3;
	u_char	length[2];
};
#endif


#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_format_header {
	u_char	res		: 8;	/* Adaptec 5500: 1 --> format track */
	u_char	vu		: 1;
	u_char			: 3;
	u_char	serr		: 1;	/* Stop on error		    */
	u_char	dcert		: 1;	/* Disable certification	    */
	u_char	dmdl		: 1;	/* Disable manufacturer defect list */
	u_char	enable		: 1;	/* Enable to use the next 3 bits    */
	u_char	length[2];		/* Length of following list in bytes*/
};

#else					/* Motorola byteorder */

struct scsi_format_header {
	u_char	res		: 8;	/* Adaptec 5500: 1 --> format track */
	u_char	enable		: 1;	/* Enable to use the next 3 bits    */
	u_char	dmdl		: 1;	/* Disable manufacturer defect list */
	u_char	dcert		: 1;	/* Disable certification	    */
	u_char	serr		: 1;	/* Stop on error		    */
	u_char			: 3;
	u_char	vu		: 1;
	u_char	length[2];		/* Length of following list in bytes*/
};
#endif

struct	scsi_def_bfi {
	u_char	cyl[3];
	u_char	head;
	u_char	bfi[4];
};

struct	scsi_def_phys {
	u_char	cyl[3];
	u_char	head;
	u_char	sec[4];
};

struct	scsi_def_list {
	struct	scsi_def_header	hd;
	union {
			u_char		list_block[1][4];
		struct	scsi_def_bfi	list_bfi[1];
		struct	scsi_def_phys	list_phys[1];
	} def_list;
};

struct	scsi_format_data {
	struct scsi_format_header hd;
	union {
			u_char		list_block[1][4];
		struct	scsi_def_bfi	list_bfi[1];
		struct	scsi_def_phys	list_phys[1];
	} def_list;
};

#define	def_block	def_list.list_block
#define	def_bfi		def_list.list_bfi
#define	def_phys	def_list.list_phys

#define	SC_DEF_BLOCK	0
#define	SC_DEF_BFI	4
#define	SC_DEF_PHYS	5
#define	SC_DEF_VU	6
#define	SC_DEF_RES	7

struct	scsi_send_diag_cmd {
	u_char	cmd;
	u_char	addr[4];
	u_char		:8;
};

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_sector_header {
	u_char	cyl[2];
	u_char	head;
	u_char	sec;
	u_char		:5;
	u_char	rp	:1;
	u_char	sp	:1;
	u_char	dt	:1;
};

#else					/* Motorola byteorder */

struct	scsi_sector_header {
	u_char	cyl[2];
	u_char	head;
	u_char	sec;
	u_char	dt	:1;
	u_char	sp	:1;
	u_char	rp	:1;
	u_char		:5;
};
#endif

#endif	/* _SCSIREG_H */
