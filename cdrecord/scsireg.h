/* @(#)scsireg.h	1.12 96/12/31 Copyright 1987 J. Schilling */
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
