/* @(#)global.h	1.2 99/12/19 Copyright 1998,1999 Heiko Eissfeldt */
/* Global Variables */

#ifdef  MD5_SIGNATURES
#include "md5.h"
#endif

typedef struct index_list {
  struct index_list *next;
  int frameoffset;
} index_list;

typedef struct global {

  char dev_name [200];		/* device name */
  char aux_name [200];		/* device name */
  char fname_base[200];

  int have_forked;
  int parent_died;
  int audio;
  struct soundfile *audio_out;
  int cooked_fd;
  int no_file;
  int no_infofile;
  int no_cddbfile;
  int quiet;
  int verbose;
  int scsi_silent;
  int scsi_verbose;
  int multiname;
  int sh_bits;
  int Remainder;
  int SkippedSamples;
  int OutSampleSize;
  int need_big_endian;
  int need_hostorder;
  int channels;
  unsigned long iloop;
  unsigned long nSamplesDoneInTrack;
  unsigned overlap;
  int useroverlap;
  unsigned nsectors;
  unsigned buffers;
  unsigned shmsize;
  long pagesize;
  int outputendianess;
  int findminmax;
  int maxamp[2];
  int minamp[2];
  unsigned speed;
  int userspeed;
  int ismono;
  int findmono;
  int swapchannels;
  int deemphasize;
  int gui;
  long playback_rate;
  int target; /* SCSI Id to be used */
  int lun;    /* SCSI Lun to be used */
  UINT4 cddb_id;
  unsigned char *cdindex_id;
  unsigned char *creator;
  unsigned char *copyright_message;
  unsigned char *disctitle;
  unsigned char *tracktitle[100];
  unsigned char *trackcreator[100];
  index_list *trackindexlist[100];

#ifdef  MD5_SIGNATURES
  unsigned int md5blocksize, md5count;
  MD5_CTX context;
  unsigned char MD5_result[16];
#endif

#ifdef	ECHO_TO_SOUNDCARD
  int soundcard_fd;
  int echo;
#endif
} global_t;

extern global_t global;
