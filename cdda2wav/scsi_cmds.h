/* @(#)scsi_cmds.h	1.3 00/06/02 Copyright 1998,1999 Heiko Eissfeldt */
/* header file for scsi_cmds.c */

extern unsigned char *cmd;

int SCSI_emulated_ATAPI_on __PR(( SCSI *scgp ));
unsigned char *Inquiry __PR(( SCSI *scgp ));
int TestForMedium __PR(( SCSI *scgp ));
void SpeedSelectSCSIMMC __PR((SCSI *scgp, unsigned speed));
void SpeedSelectSCSIYamaha __PR((SCSI *scgp, unsigned speed));
void SpeedSelectSCSISony __PR((SCSI *scgp, unsigned speed));
void SpeedSelectSCSIPhilipsCDD2600 __PR((SCSI *scgp, unsigned speed));
void SpeedSelectSCSINEC __PR((SCSI *scgp, unsigned speed));
void SpeedSelectSCSIToshiba __PR((SCSI *scgp, unsigned speed));
subq_chnl *ReadSubQSCSI __PR(( SCSI *scgp, unsigned char sq_format, unsigned char ltrack));
int ReadCddaMMC12 __PR((SCSI *scgp, UINT4 *p, unsigned lSector, unsigned SectorBurstVal ));
int ReadCdda12Matsushita __PR((SCSI *scgp, UINT4 *p, unsigned lSector, unsigned SectorBurstVal ));
int ReadCdda12 __PR((SCSI *scgp, UINT4 *p, unsigned lSector, unsigned SecorBurstVal ));
int ReadCdda10 __PR((SCSI *scgp, UINT4 *p, unsigned lSector, unsigned SecorBurstVal ));
int ReadStandard __PR((SCSI *scgp, UINT4 *p, unsigned lSector, unsigned SctorBurstVal ));
unsigned ReadTocSCSI __PR(( SCSI *scgp, TOC *toc ));
unsigned ReadFirstSessionTOCSony __PR(( SCSI *scgp, unsigned tracks ));
unsigned ReadFirstSessionTOCMMC __PR(( SCSI *scgp, unsigned tracks ));
void ReadTocTextSCSIMMC __PR(( SCSI *scgp ));
int Play_atSCSI __PR(( SCSI *scgp, unsigned int from_sector, unsigned int sectors));
int StopPlaySCSI __PR(( SCSI *scgp ));
void EnableCddaModeSelect __PR((SCSI *scgp, int fAudioMode));
int set_sectorsize __PR((SCSI *scgp, unsigned int secsize));
unsigned int
get_orig_sectorsize __PR((SCSI *scgp, unsigned char *m4, unsigned char *m10,
                    unsigned char *m11));
int heiko_mmc __PR(( SCSI *scgp ));
void init_scsibuf __PR(( SCSI *scgp, unsigned amt ));
int	myscsierr __PR(( SCSI *scgp ));

extern int accepts_fua_bit;
extern unsigned char density;
extern unsigned char orgmode4;
extern unsigned char orgmode10, orgmode11;

