/* @(#)scsi_cmds.c	1.3 00/02/17 Copyright 1998,1999 Heiko Eissfeldt */
#ifndef lint
static char     sccsid[] =
"@(#)scsi_cmds.c	1.3 00/02/17 Copyright 1998,1999 Heiko Eissfeldt";

#endif
/* file for all SCSI commands
 * FUA (Force Unit Access) bit handling copied from Monty's cdparanoia.
 */
#define TESTSUBQFALLBACK	0

#include "config.h"
#include <stdio.h>
#include <standard.h>
#include <stdlib.h>
#include <strdefs.h>

#include <btorder.h>

#define        g5x_cdblen(cdb, len)    ((cdb)->count[0] = ((len) >> 16L)& 0xFF,\
                                (cdb)->count[1] = ((len) >> 8L) & 0xFF,\
                                (cdb)->count[2] = (len) & 0xFF)


#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>

#include <scg/scsitransp.h>

#include "mytype.h"
#include "cdda2wav.h"
#include "interface.h"
#include "byteorder.h"
#include "global.h"
#include "cdrecord.h"
#include "scsi_cmds.h"

unsigned char *bufferTOC;
subq_chnl *SubQbuffer;
unsigned char *cmd;

int SCSI_emulated_ATAPI_on(scgp)
	SCSI *scgp;
{
/*	return is_atapi;*/
	if (scsi_isatapi(scgp) > 0)
		return (TRUE);

	(void) allow_atapi(scgp, TRUE);
	return (allow_atapi(scgp, TRUE));
}

int heiko_mmc(scgp)
	SCSI *scgp;
{
        unsigned char	mode[0x100];
	int		was_atapi;
        struct  cd_mode_page_2A *mp;
	int retval;

        fillbytes((caddr_t)mode, sizeof(mode), '\0');

        was_atapi = allow_atapi(scgp, 1);
        scgp->silent++;
        mp = mmc_cap(scgp, mode);
        scgp->silent--;
        allow_atapi(scgp, was_atapi);
        if (mp == NULL)
                return (0);

        /* have a look at the capabilities */
	if (mp->cd_da_supported == 0) {
	  retval = -1;
	} else {
	  retval = 1 + mp->cd_da_accurate;
        }
	return retval;
}


int accepts_fua_bit;
unsigned char density = 0;
unsigned char orgmode4 = 0;
unsigned char orgmode10, orgmode11;

/* get current sector size from SCSI cdrom drive */
unsigned int 
get_orig_sectorsize(scgp, m4, m10, m11)
	SCSI *scgp;
	unsigned char *m4;
	unsigned char *m10; 
	unsigned char *m11;
{
      /* first get current values for density, etc. */

      static unsigned char *modesense = NULL;
   
      if (modesense == NULL) {
        modesense = (unsigned char *) malloc(12);
        if (modesense == NULL) {
          fprintf(stderr, "Cannot allocate memory for mode sense command in line %d\n", __LINE__);
          return 0;
        }
      }

      /* do the scsi cmd */
      if (scgp->verbose) fprintf(stderr, "\nget density and sector size...");
      if (mode_sense(scgp, modesense, 12, 0x01, 0) < 0)
	  fprintf(stderr, "get_orig_sectorsize mode sense failed\n");

      /* FIXME: some drives dont deliver block descriptors !!! */
      if (modesense[3] == 0)
        return 0;

      if (m4 != NULL)                       /* density */
        *m4 = modesense[4];
      if (m10 != NULL)                      /* MSB sector size */
        *m10 = modesense[10];
      if (m11 != NULL)                      /* LSB sector size */
        *m11 = modesense[11];

      return (modesense[10] << 8) + modesense[11];
}



/* switch CDROM scsi drives to given sector size  */
int set_sectorsize (scgp, secsize)
	SCSI *scgp;
	unsigned int secsize;
{
  static unsigned char mode [4 + 8];
  int retval;

  if (orgmode4 == 0xff) {
    get_orig_sectorsize(scgp, &orgmode4, &orgmode10, &orgmode11);
  }
  if (orgmode4 == 0x82 && secsize == 2048)
    orgmode4 = 0x81;

  /* prepare to read cds in the previous mode */

  fillbytes((caddr_t)mode, sizeof(mode), '\0');
  mode[ 3] = 8; 	       /* Block Descriptor Length */
  mode[ 4] = orgmode4; 	       /* normal density */
  mode[10] =  secsize >> 8;   /* block length "msb" */
  mode[11] =  secsize & 0xFF; /* block length lsb */

  if (scgp->verbose) fprintf(stderr, "\nset density and sector size...");
  /* do the scsi cmd */
  if ((retval = mode_select(scgp, mode, 12, 0, scgp->inq->data_format >= 2)) < 0)
        fprintf (stderr, "setting sector size failed\n");

  return retval;
}


/* switch Toshiba/DEC and HP drives from/to cdda density */
void EnableCddaModeSelect (scgp, fAudioMode)
	SCSI *scgp;
	int fAudioMode;
{
  /* reserved, Medium type=0, Dev spec Parm = 0, block descriptor len 0 oder 8,
     Density (cd format) 
     (0=YellowBook, XA Mode 2=81h, XA Mode1=83h and raw audio via SCSI=82h),
     # blks msb, #blks, #blks lsb, reserved,
     blocksize, blocklen msb, blocklen lsb,
   */

  /* MODE_SELECT, page = SCSI-2  save page disabled, reserved, reserved, 
     parm list len, flags */
  static unsigned char mode [4 + 8] = { 
       /* mode section */
			    0, 
                            0, 0, 
                            8,       /* Block Descriptor Length */
       /* block descriptor */
                            0,       /* Density Code */
                            0, 0, 0, /* # of Blocks */
                            0,       /* reserved */
                            0, 0, 0};/* Blocklen */

  if (orgmode4 == 0 && fAudioMode) {
    if (0 == get_orig_sectorsize(scgp, &orgmode4, &orgmode10, &orgmode11)) {
        /* cannot retrieve density, sectorsize */
	orgmode10 = (CD_FRAMESIZE >> 8L);
	orgmode11 = (CD_FRAMESIZE & 0xFF);
    }
  }

  if (fAudioMode) {
    /* prepare to read audio cdda */
    mode [4] = density;  			/* cdda density */
    mode [10] = (CD_FRAMESIZE_RAW >> 8L);   /* block length "msb" */
    mode [11] = (CD_FRAMESIZE_RAW & 0xFF);  /* block length "lsb" */
  } else {
    /* prepare to read cds in the previous mode */
    mode [4] = orgmode4; /* 0x00; 			\* normal density */
    mode [10] = orgmode10; /* (CD_FRAMESIZE >> 8L);  \* block length "msb" */
    mode [11] = orgmode11; /* (CD_FRAMESIZE & 0xFF); \* block length lsb */
  }

  if (scgp->verbose) fprintf(stderr, "\nset density/sector size (EnableCddaModeSelect)...\n");
  /* do the scsi cmd */
  if (mode_select(scgp, mode, 12, 0, scgp->inq->data_format >= 2) < 0)
        fprintf (stderr, "Audio mode switch failed\n");
}


/* read CD Text information from the table of contents */
void ReadTocTextSCSIMMC ( scgp )
	SCSI *scgp;
{
    short int datalength;
    unsigned char *p = bufferTOC;

#if 1
  /* READTOC, MSF, format, res, res, res, Start track/session, len msb,
     len lsb, control */
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)bufferTOC;
        scmd->size = 4;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G1_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g1_cdb.cmd = 0x43;		/* Read TOC command */
        scmd->cdb.g1_cdb.lun = scgp->lun;
        scmd->cdb.g1_cdb.addr[0] = 5;		/* format field */
        scmd->cdb.g1_cdb.res6 = 0;	/* track/session is reserved */
        g1_cdblen(&scmd->cdb.g1_cdb, 4);

        scgp->silent++;
        if (scgp->verbose) fprintf(stderr, "\nRead TOC CD Text size ...");

	scgp->cmdname = "read toc size (text)";

        if (scsicmd(scgp) < 0) {
          scgp->silent--;
	  if (global.quiet != 1)
            fprintf (stderr, "Read TOC CD Text failed (probably not supported).\n");
	  p[0] = p[1] = '\0';
          return ;
        }
        scgp->silent--;

    datalength  = (p[0] << 8) | (p[1]);
    if (datalength <= 2) return;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)bufferTOC;
        scmd->size = 2+datalength;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G1_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g1_cdb.cmd = 0x43;		/* Read TOC command */
        scmd->cdb.g1_cdb.lun = scgp->lun;
        scmd->cdb.g1_cdb.addr[0] = 5;		/* format field */
        scmd->cdb.g1_cdb.res6 = 0;	/* track/session is reserved */
        g1_cdblen(&scmd->cdb.g1_cdb, 2+datalength);

        scgp->silent++;
        if (scgp->verbose) fprintf(stderr, "\nRead TOC CD Text data ...");

	scgp->cmdname = "read toc data (text)";

        if (scsicmd(scgp) < 0) {
          scgp->silent--;
	  if (global.quiet != 1)
            fprintf (stderr,  "Read TOC CD Text data failed (probably not supported).\n");
	  p[0] = p[1] = '\0';
          return ;
        }
        scgp->silent--;
#else
	{ FILE *fp;
	int read_;
	fp = fopen("PearlJam.cdtext", "rb");
	/*fp = fopen("celine.cdtext", "rb");*/
	if (fp == NULL) { perror(""); return; }
	fillbytes(bufferTOC, CD_FRAMESIZE, '\0');
	read_ = fread(bufferTOC, 1, CD_FRAMESIZE, fp );
fprintf(stderr, "read %d bytes. sizeof(bufferTOC)=%u\n", read_, CD_FRAMESIZE);
        datalength  = (bufferTOC[0] << 8) | (bufferTOC[1]);
	fclose(fp);
	}
#endif
}

/* read the start of the lead-out from the first session TOC */
unsigned ReadFirstSessionTOCSony ( scgp, tracks )
	SCSI *scgp;
	unsigned tracks;
{
  /* READTOC, MSF, format, res, res, res, Start track/session, len msb,
     len lsb, control */
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)bufferTOC;
        scmd->size = 4 + (tracks + 3) * 11;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G1_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g1_cdb.cmd = 0x43;		/* Read TOC command */
        scmd->cdb.g1_cdb.lun = scgp->lun;
        scmd->cdb.g1_cdb.res6 = 1;    		/* session */
        g1_cdblen(&scmd->cdb.g1_cdb, 4 + (tracks + 3) * 11);
        scmd->cdb.g1_cdb.vu_97 = 1;   		/* format */

        scgp->silent++;
        if (scgp->verbose) fprintf(stderr, "\nRead TOC first session ...");

	scgp->cmdname = "read toc first session";

        if (scsicmd(scgp) < 0) {
          scgp->silent--;
	  if (global.quiet != 1)
            fprintf (stderr, "Read TOC first session failed (probably not supported).\n");
          return 0;
        }
        scgp->silent--;

        if ((unsigned)((bufferTOC[0] << 8) | bufferTOC[1]) >= 4 + (tracks + 3) * 11 -2) {
          unsigned off;

          /* We want the entry with POINT = 0xA2, which has the start position
             of the first session lead out */
          off = 4 + 2 * 11 + 3;
          if (bufferTOC[off-3] == 1 && bufferTOC[off] == 0xA2) {
            unsigned retval;

            off = 4 + 2 * 11 + 8;
            retval = bufferTOC[off] >> 4;
	    retval *= 10; retval += bufferTOC[off] & 0xf;
	    retval *= 60;
	    off++;
            retval += 10 * (bufferTOC[off] >> 4) + (bufferTOC[off] & 0xf);
	    retval *= 75;
	    off++;
            retval += 10 * (bufferTOC[off] >> 4) + (bufferTOC[off] & 0xf);
	    retval -= 150;

            return retval;
          }
        }
        return 0;
}

/* read the start of the lead-out from the first session TOC */
unsigned ReadFirstSessionTOCMMC ( scgp, tracks )
	SCSI *scgp;
	unsigned tracks;
{

  /* READTOC, MSF, format, res, res, res, Start track/session, len msb,
     len lsb, control */
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)bufferTOC;
        scmd->size = 4 + (tracks + 3) * 11;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G1_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g1_cdb.cmd = 0x43;		/* Read TOC command */
        scmd->cdb.g1_cdb.lun = scgp->lun;
        scmd->cdb.g1_cdb.addr[0] = 2;		/* format */
        scmd->cdb.g1_cdb.res6 = 1;		/* session */
        g1_cdblen(&scmd->cdb.g1_cdb, 4 + (tracks + 3) * 11);

        scgp->silent++;
        if (scgp->verbose) fprintf(stderr, "\nRead TOC first session ...");

	scgp->cmdname = "read toc first session";

        if (scsicmd(scgp) < 0) {
          scgp->silent--;
	  if (global.quiet != 1)
            fprintf (stderr, "Read TOC first session failed (probably not supported).\n");
          return 0;
        }
        scgp->silent--;

        if ((unsigned)((bufferTOC[0] << 8) | bufferTOC[1]) >= 4 + (tracks + 3) * 11 - 2) {
          unsigned off;

          /* We want the entry with POINT = 0xA2, which has the start position
             of the first session lead out */
          off = 4 + 3;
          while (off < 4 + (tracks + 3) * 11 && bufferTOC[off] != 0xA2) {
            off += 11;
          }
          if (off < 4 + (tracks + 3) * 11) {
            off += 5;
            return (bufferTOC[off]*60 + bufferTOC[off+1])*75 + bufferTOC[off+2] - 150;
          }
        }
        return 0;
}

/* read the table of contents from the cd and fill the TOC array */
unsigned ReadTocSCSI ( scgp, toc )
	SCSI *scgp;
	TOC *toc;
{
  unsigned i;
  unsigned tracks;

  /* first read the first and last track number */
  /* READTOC, MSF format flag, res, res, res, res, Start track, len msb,
     len lsb, flags */
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)bufferTOC;
        scmd->size = 4;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G1_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g1_cdb.cmd = 0x43;		/* read TOC command */
        scmd->cdb.g1_cdb.lun = scgp->lun;
        scmd->cdb.g1_cdb.res6 = 1;		/* start track */
        g1_cdblen(&scmd->cdb.g1_cdb, 4);

        if (scgp->verbose) fprintf(stderr, "\nRead TOC size (standard)...");
  /* do the scsi cmd (read table of contents) */

	scgp->cmdname = "read toc size";
  if (scsicmd(scgp) < 0)
      FatalError ("Read TOC size failed.\n");


  tracks = ((bufferTOC [3] ) - bufferTOC [2] + 2) ;
  if (tracks > MAXTRK) return 0;
  if (tracks == 0) return 0;


	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)bufferTOC;
        scmd->size = 4 + tracks * 8;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G1_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g1_cdb.cmd = 0x43;		/* read TOC command */
        scmd->cdb.g1_cdb.lun = scgp->lun;
        scmd->cdb.g1_cdb.res = 1;		/* MSF format */
        scmd->cdb.g1_cdb.res6 = 1;		/* start track */
        g1_cdblen(&scmd->cdb.g1_cdb, 4 + tracks * 8);

        if (scgp->verbose) fprintf(stderr, "\nRead TOC tracks (standard MSF)...");
        /* do the scsi cmd (read table of contents) */

	scgp->cmdname = "read toc tracks ";

  if (scsicmd(scgp) < 0) {
      /* fallback to LBA format for cd burners like Philips CD-522 */
	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)bufferTOC;
        scmd->size = 4 + tracks * 8;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G1_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g1_cdb.cmd = 0x43;		/* read TOC command */
        scmd->cdb.g1_cdb.lun = scgp->lun;
        scmd->cdb.g1_cdb.res = 0;		/* LBA format */
        scmd->cdb.g1_cdb.res6 = 1;		/* start track */
        g1_cdblen(&scmd->cdb.g1_cdb, 4 + tracks * 8);

        if (scgp->verbose) fprintf(stderr, "\nRead TOC tracks (standard LBA)...");
        /* do the scsi cmd (read table of contents) */

	scgp->cmdname = "read toc tracks ";
        if (scsicmd(scgp) < 0) {
          FatalError ("Read TOC tracks failed.\n");
        }
        for (i = 0; i < tracks; i++) {
          memcpy (&toc[i], bufferTOC + 4 + 8*i, 8);
          toc[i].ISRC[0] = 0;
          toc[i].dwStartSector = be32_to_cpu(toc[i].dwStartSector);
          if ( toc [i].bTrack != i+1 )
	    toc [i].bTrack = i+1;
        }
  } else {

    /* copy to our structure and convert start sector */
    for (i = 0; i < tracks; i++) {
      memcpy (&toc[i], bufferTOC + 4 + 8*i, 8);
      toc[i].ISRC[0] = 0;
      toc[i].dwStartSector = -150 + 75*60* bufferTOC[4 + 8*i + 5]+
                                    75*    bufferTOC[4 + 8*i + 6]+
                                           bufferTOC[4 + 8*i + 7];
      if ( toc [i].bTrack != i+1 )
	  toc [i].bTrack = i+1;
    }
  }
  return --tracks;           /* without lead-out */
}

/* ---------------- Read methods ------------------------------ */

/* Read max. SectorBurst of cdda sectors to buffer
   via standard SCSI-2 Read(10) command */
void ReadStandard (scgp, p, lSector, SectorBurstVal )
	SCSI *scgp;
	UINT4 *p;
	unsigned lSector;
	unsigned SectorBurstVal;
{
  /* READ10, flags, block1 msb, block2, block3, block4 lsb, reserved, 
     transfer len msb, transfer len lsb, block addressing mode */
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)p;
        scmd->size = SectorBurstVal*CD_FRAMESIZE_RAW;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G1_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g1_cdb.cmd = 0x28;		/* read 10 command */
        scmd->cdb.g1_cdb.lun = scgp->lun;
	scmd->cdb.g1_cdb.res |= (accepts_fua_bit == 1 ? 1 << 2 : 0);
        g1_cdbaddr(&scmd->cdb.g1_cdb, lSector);
        g1_cdblen(&scmd->cdb.g1_cdb, SectorBurstVal);
        if (scgp->verbose) fprintf(stderr, "\nReadStandard10 CDDA...");

	scgp->cmdname = "ReadStandard10";

	if (scsicmd(scgp) < 0)
		FatalError ("Read CD-ROM10 failed\n");
}

/* Read max. SectorBurst of cdda sectors to buffer
   via vendor-specific ReadCdda(10) command */
void ReadCdda10 (scgp, p, lSector, SectorBurstVal )
	SCSI *scgp;
	UINT4 *p;
	unsigned lSector;
	unsigned SectorBurstVal;
{
  /* READ10, flags, block1 msb, block2, block3, block4 lsb, reserved, 
     transfer len msb, transfer len lsb, block addressing mode */
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)p;
        scmd->size = SectorBurstVal*CD_FRAMESIZE_RAW;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G1_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g1_cdb.cmd = 0xd4;		/* Read audio command */
        scmd->cdb.g1_cdb.lun = scgp->lun;
	scmd->cdb.g1_cdb.res |= (accepts_fua_bit == 1 ? 1 << 2 : 0);
        g1_cdbaddr(&scmd->cdb.g1_cdb, lSector);
        g1_cdblen(&scmd->cdb.g1_cdb, SectorBurstVal);
        if (scgp->verbose) fprintf(stderr, "\nReadNEC10 CDDA...");

	scgp->cmdname = "Read10 NEC";

	if (scsicmd(scgp) < 0)
		FatalError ("Read CD-ROM10 (NEC) failed\n");

}


/* Read max. SectorBurst of cdda sectors to buffer
   via vendor-specific ReadCdda(12) command */
void ReadCdda12 (scgp, p, lSector, SectorBurstVal )
	SCSI *scgp;
	UINT4 *p;
	unsigned lSector;
	unsigned SectorBurstVal;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)p;
        scmd->size = SectorBurstVal*CD_FRAMESIZE_RAW;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G5_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g5_cdb.cmd = 0xd8;		/* read audio command */
        scmd->cdb.g5_cdb.lun = scgp->lun;
	scmd->cdb.g5_cdb.res |= (accepts_fua_bit == 1 ? 1 << 2 : 0);
        g5_cdbaddr(&scmd->cdb.g5_cdb, lSector);
        g5_cdblen(&scmd->cdb.g5_cdb, SectorBurstVal);

        if (scgp->verbose) fprintf(stderr, "\nReadSony12 CDDA...");

	scgp->cmdname = "Read12";

	if (scsicmd(scgp) < 0)
		FatalError ("Read CD-ROM12 failed\n");

}

/* Read max. SectorBurst of cdda sectors to buffer
   via vendor-specific ReadCdda(12) command */
/*
> It uses a 12 Byte CDB with 0xd4 as opcode, the start sector is coded as
> normal and the number of sectors is coded in Byte 8 and 9 (begining with 0).
*/

void ReadCdda12Matsushita (scgp, p, lSector, SectorBurstVal )
	SCSI *scgp;
	UINT4 *p;
	unsigned lSector;
	unsigned SectorBurstVal;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

        fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)p;
        scmd->size = SectorBurstVal*CD_FRAMESIZE_RAW;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G5_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g5_cdb.cmd = 0xd4;		/* read audio command */
        scmd->cdb.g5_cdb.lun = scgp->lun;
	scmd->cdb.g5_cdb.res |= (accepts_fua_bit == 1 ? 1 << 2 : 0);
        g5_cdbaddr(&scmd->cdb.g5_cdb, lSector);
        g5_cdblen(&scmd->cdb.g5_cdb, SectorBurstVal);

        if (scgp->verbose) fprintf(stderr, "\nReadMatsushita12 CDDA...");

	scgp->cmdname = "Read12Matsushita";

	if (scsicmd(scgp) < 0)
		FatalError ("Read CD-ROM12 (Matsushita) failed\n");

}

/* Read max. SectorBurst of cdda sectors to buffer
   via MMC standard READ CD command */
void ReadCddaMMC12 (scgp, p, lSector, SectorBurstVal )
	SCSI *scgp;
	UINT4 *p;
	unsigned lSector;
	unsigned SectorBurstVal;
{
	register struct	scg_cmd	*scmd;
	int i;
  for (i = 5; i > 0; i--) {	
	scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)p;
        scmd->size = SectorBurstVal*CD_FRAMESIZE_RAW;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G5_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g5_cdb.cmd = 0xbe;		/* read cd command */
        scmd->cdb.g5_cdb.lun = scgp->lun;
        scmd->cdb.g5_cdb.res = 1 << 1; /* expected sector type field CDDA */
        g5_cdbaddr(&scmd->cdb.g5_cdb, lSector);
        g5x_cdblen(&scmd->cdb.g5_cdb, SectorBurstVal);
	scmd->cdb.g5_cdb.count[3] = 1 << 4;	/* User data */

        if (scgp->verbose) fprintf(stderr, "\nReadMMC12 CDDA...");

	scgp->cmdname = "ReadCD MMC 12";

	if (scsicmd(scgp) >= 0)
		break;
  }
  if (i == 0) FatalError("ReadCD MMC 12 failed");
}

/* Read the Sub-Q-Channel to SubQbuffer. This is the method for
 * drives that do not support subchannel parameters. */
#ifdef	PROTOTYPES
static subq_chnl *ReadSubQFallback (SCSI *scgp, unsigned char sq_format, unsigned char track)
#else
static subq_chnl *ReadSubQFallback ( scgp, sq_format, track )
	SCSI *scgp;
	unsigned char sq_format;
	unsigned char track;
#endif
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)SubQbuffer;
        scmd->size = 24;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G1_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g1_cdb.cmd = 0x42;		/* Read SubQChannel */
						/* use LBA */
        scmd->cdb.g1_cdb.lun = scgp->lun;
        scmd->cdb.g1_cdb.addr[0] = 0x40; 	/* SubQ info */
        scmd->cdb.g1_cdb.addr[1] = 0;	 	/* parameter list: all */
        scmd->cdb.g1_cdb.res6 = track;		/* track number */
        g1_cdblen(&scmd->cdb.g1_cdb, 24);

        if (scgp->verbose) fprintf(stderr, "\nRead Subchannel_dumb...");

	scgp->cmdname = "Read Subchannel_dumb";

	if (scsicmd(scgp) < 0) {
	  fprintf( stderr, "Read SubQ failed\n");
	}

	/* check, if the requested format is delivered */
	{ unsigned char *p = (unsigned char *) SubQbuffer;
	  if ((((unsigned)p[2] << 8) | p[3]) /* LENGTH */ > ULONG_C(11) &&
	    (p[5] >> 4) /* ADR */ == sq_format) {
	    return SubQbuffer;
	  }
	}

	/* FIXME: we might actively search for the requested info ... */
	return NULL;
}

/* Read the Sub-Q-Channel to SubQbuffer */
#ifdef	PROTOTYPES
subq_chnl *ReadSubQSCSI (SCSI *scgp, unsigned char sq_format, unsigned char track)
#else
subq_chnl *ReadSubQSCSI ( scgp, sq_format, track )
	SCSI *scgp;
	unsigned char sq_format;
	unsigned char track;
#endif
{
        int resp_size;
	register struct	scg_cmd	*scmd = scgp->scmd;

        switch (sq_format) {
          case GET_POSITIONDATA:
            resp_size = 16;
	    track = 0;
          break;
          case GET_CATALOGNUMBER:
            resp_size = 24;
	    track = 0;
          break;
          case GET_TRACK_ISRC:
            resp_size = 24;
          break;
          default:
                fprintf(stderr, "ReadSubQSCSI: unknown format %d\n", sq_format);
                return NULL;
        }

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->addr = (caddr_t)SubQbuffer;
        scmd->size = resp_size;
        scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
        scmd->cdb_len = SC_G1_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g1_cdb.cmd = 0x42;
						/* use LBA */
        scmd->cdb.g1_cdb.lun = scgp->lun;
        scmd->cdb.g1_cdb.addr[0] = 0x40; 	/* SubQ info */
        scmd->cdb.g1_cdb.addr[1] = sq_format;	/* parameter list: all */
        scmd->cdb.g1_cdb.res6 = track;		/* track number */
        g1_cdblen(&scmd->cdb.g1_cdb, resp_size);

        if (scgp->verbose) fprintf(stderr, "\nRead Subchannel...");

	scgp->cmdname = "Read Subchannel";

  if (scsicmd(scgp) < 0) {
    /* in case of error do a fallback for dumb firmwares */
    return ReadSubQFallback(scgp, sq_format, track);
  }

  return SubQbuffer;
}

/********* non standardized speed selects ***********************/

void SpeedSelectSCSIToshiba (scgp, speed)
	SCSI *scgp;
	unsigned speed;
{
  static unsigned char mode [4 + 3];
  unsigned char *page = mode + 4;
  int retval;

  fillbytes((caddr_t)mode, sizeof(mode), '\0');
  /* the first 4 mode bytes are zero. */
  page[0] = 0x20;
  page[1] = 1;
  page[2] = speed;   /* 0 for single speed, 1 for double speed (3401) */

  if (scgp->verbose) fprintf(stderr, "\nspeed select Toshiba...");

  scgp->silent++;
  /* do the scsi cmd */
  if ((retval = mode_select(scgp, mode, 7, 0, scgp->inq->data_format >= 2)) < 0)
        fprintf (stderr, "speed select Toshiba failed\n");
  scgp->silent--;
}

void SpeedSelectSCSINEC (scgp, speed)
	SCSI *scgp;
	unsigned speed;
{
  static unsigned char mode [4 + 8];
  unsigned char *page = mode + 4;
  int retval;
	register struct	scg_cmd	*scmd = scgp->scmd;

  fillbytes((caddr_t)mode, sizeof(mode), '\0');
  /* the first 4 mode bytes are zero. */
  page [0] = 0x0f; /* page code */
  page [1] = 6;    /* parameter length */
  /* bit 5 == 1 for single speed, otherwise double speed */
  page [2] = speed == 1 ? 1 << 5 : 0;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
  scmd->addr = (caddr_t)mode;
  scmd->size = 12;
  scmd->flags = SCG_DISRE_ENA;
  scmd->cdb_len = SC_G1_CDBLEN;
  scmd->sense_len = CCS_SENSE_LEN;
  scmd->target = scgp->target;
  scmd->cdb.g1_cdb.cmd = 0xC5;
  scmd->cdb.g1_cdb.lun = scgp->lun;
  scmd->cdb.g1_cdb.addr[0] = 0 ? 1 : 0 | 1 ? 0x10 : 0;
  g1_cdblen(&scmd->cdb.g1_cdb, 12);

  if (scgp->verbose) fprintf(stderr, "\nspeed select NEC...");
  /* do the scsi cmd */

	scgp->cmdname = "speed select NEC";

  if ((retval = scsicmd(scgp)) < 0)
        fprintf(stderr ,"speed select NEC failed\n");
}

void SpeedSelectSCSIPhilipsCDD2600 (scgp, speed)
	SCSI *scgp;
	unsigned speed;
{
  /* MODE_SELECT, page = SCSI-2  save page disabled, reserved, reserved,
     parm list len, flags */
  static unsigned char mode [4 + 8];
  unsigned char *page = mode + 4;
  int retval;

  fillbytes((caddr_t)mode, sizeof(mode), '\0');
  /* the first 4 mode bytes are zero. */
  page[0] = 0x23;
  page[1] = 6;
  page[2] = page [4] = speed;
  page[3] = 1;

  if (scgp->verbose) fprintf(stderr, "\nspeed select Philips...");
  /* do the scsi cmd */
  if ((retval = mode_select(scgp, mode, 12, 0, scgp->inq->data_format >= 2)) < 0)
        fprintf (stderr, "speed select PhilipsCDD2600 failed\n");
}

void SpeedSelectSCSISony (scgp, speed)
	SCSI *scgp;
	unsigned speed;
{
  static unsigned char mode [4 + 4];
  unsigned char *page = mode + 4;
  int retval;

  fillbytes((caddr_t)mode, sizeof(mode), '\0');
  /* the first 4 mode bytes are zero. */
  page[0] = 0x31;
  page[1] = 2;
  page[2] = speed;

  if (scgp->verbose) fprintf(stderr, "\nspeed select Sony...");
  /* do the scsi cmd */
  scgp->silent++;
  if ((retval = mode_select(scgp, mode, 8, 0, scgp->inq->data_format >= 2)) < 0)
        fprintf (stderr, "speed select Sony failed\n");
  scgp->silent--;
}

void SpeedSelectSCSIYamaha (scgp, speed)
	SCSI *scgp;
	unsigned speed;
{
  static unsigned char mode [4 + 4];
  unsigned char *page = mode + 4;
  int retval;

  fillbytes((caddr_t)mode, sizeof(mode), '\0');
  /* the first 4 mode bytes are zero. */
  page[0] = 0x31;
  page[1] = 2;
  page[2] = speed;

  if (scgp->verbose) fprintf(stderr, "\nspeed select Yamaha...");
  /* do the scsi cmd */
  if ((retval = mode_select(scgp, mode, 8, 0, scgp->inq->data_format >= 2)) < 0)
        fprintf (stderr, "speed select Yamaha failed\n");
}

void SpeedSelectSCSIMMC (scgp, speed)
	SCSI *scgp;
	unsigned speed;
{
  int spd;
	register struct	scg_cmd	*scmd = scgp->scmd;

   if (speed == 0 || speed == 0xFFFF) {
      spd = 0xFFFF;
   } else {
      spd = (1764 * speed) / 10;
   }
	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
        scmd->flags = SCG_DISRE_ENA;
        scmd->cdb_len = SC_G5_CDBLEN;
        scmd->sense_len = CCS_SENSE_LEN;
        scmd->target = scgp->target;
        scmd->cdb.g5_cdb.cmd = 0xBB;
        scmd->cdb.g5_cdb.lun = scgp->lun;
        i_to_2_byte(&scmd->cdb.g5_cdb.addr[0], spd);
        i_to_2_byte(&scmd->cdb.g5_cdb.addr[2], 0xffff);

        if (scgp->verbose) fprintf(stderr, "\nspeed select MMC...");

	scgp->cmdname = "set cd speed";

	scgp->silent++;
        if (scsicmd(scgp) < 0) {
		if (scsi_sense_key(scgp) == 0x05 &&
		    scsi_sense_code(scgp) == 0x20 &&
		    scsi_sense_qual(scgp) == 0x00) {
			/* this optional command is not implemented */
		} else {
			scsiprinterr(scgp);
                	fprintf (stderr, "speed select MMC failed\n");
		}
	}
	scgp->silent--;
}

/* request vendor brand and model */
unsigned char *Inquiry ( scgp )
	SCSI *scgp;
{
  static unsigned char *Inqbuffer = NULL;
	register struct	scg_cmd	*scmd = scgp->scmd;

  if (Inqbuffer == NULL) {
    Inqbuffer = (unsigned char *) malloc(42);
    if (Inqbuffer == NULL) {
      fprintf(stderr, "Cannot allocate memory for inquiry command in line %d\n", __LINE__);
        return NULL;
    }
  }

  fillbytes(Inqbuffer, 42, '\0');
	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
  scmd->addr = (caddr_t)Inqbuffer;
  scmd->size = 42;
  scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
  scmd->cdb_len = SC_G0_CDBLEN;
  scmd->sense_len = CCS_SENSE_LEN;
  scmd->target = scgp->target;
  scmd->cdb.g0_cdb.cmd = SC_INQUIRY;
  scmd->cdb.g0_cdb.lun = scgp->lun;
  scmd->cdb.g0_cdb.count = 42;
        
	scgp->cmdname = "inquiry";

  if (scsicmd(scgp) < 0)
     return (NULL);

  /* define structure with inquiry data */
  memcpy(scgp->inq, Inqbuffer, sizeof(*scgp->inq)); 

  if (scgp->verbose)
     scsiprbytes("Inquiry Data   :", (u_char *)Inqbuffer, 22 - scmd->resid);

  return (Inqbuffer);
}

#define SC_CLASS_EXTENDED_SENSE 0x07
#define TESTUNITREADY_CMD 0
#define TESTUNITREADY_CMDLEN 6

#define ADD_SENSECODE 12
#define ADD_SC_QUALIFIER 13
#define NO_MEDIA_SC 0x3a
#define NO_MEDIA_SCQ 0x00

int TestForMedium ( scgp )
	SCSI *scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

  if (interface != GENERIC_SCSI) {
    return 1;
  }

  /* request READY status */
	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
  scmd->addr = (caddr_t)0;
  scmd->size = 0;
  scmd->flags = SCG_DISRE_ENA | (1 ? SCG_SILENT:0);
  scmd->cdb_len = SC_G0_CDBLEN;
  scmd->sense_len = CCS_SENSE_LEN;
  scmd->target = scgp->target;
  scmd->cdb.g0_cdb.cmd = SC_TEST_UNIT_READY;
  scmd->cdb.g0_cdb.lun = scgp->lun;
        
  if (scgp->verbose) fprintf(stderr, "\ntest unit ready...");
  scgp->silent++;

	scgp->cmdname = "test unit ready";

  if (scsicmd(scgp) >= 0) {
    scgp->silent--;
    return 1;
  }
  scgp->silent--;

  if (scmd->sense.code >= SC_CLASS_EXTENDED_SENSE) {
    return 
      scmd->u_sense.cmd_sense[ADD_SENSECODE] != NO_MEDIA_SC ||
      scmd->u_sense.cmd_sense[ADD_SC_QUALIFIER] != NO_MEDIA_SCQ;
  } else {
    /* analyse status. */
    /* 'check condition' is interpreted as not ready. */
    return (scmd->u_scb.cmd_scb[0] & 0x1e) != 0x02;
  }
}

int StopPlaySCSI ( scgp )
	SCSI *scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
  scmd->addr = NULL;
  scmd->size = 0;
  scmd->flags = SCG_DISRE_ENA;
  scmd->cdb_len = SC_G0_CDBLEN;
  scmd->sense_len = CCS_SENSE_LEN;
  scmd->target = scgp->target;
  scmd->cdb.g0_cdb.cmd = 0x1b;
  scmd->cdb.g0_cdb.lun = scgp->lun;

  if (scgp->verbose) fprintf(stderr, "\nstop audio play");
  /* do the scsi cmd */

	scgp->cmdname = "stop audio play";

  return scsicmd(scgp) >= 0 ? 0 : -1;
}

int Play_atSCSI ( scgp, from_sector, sectors)
	SCSI *scgp;
	unsigned int from_sector;
	unsigned int sectors;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
  scmd->addr = NULL;
  scmd->size = 0;
  scmd->flags = SCG_DISRE_ENA;
  scmd->cdb_len = SC_G1_CDBLEN;
  scmd->sense_len = CCS_SENSE_LEN;
  scmd->target = scgp->target;
  scmd->cdb.g1_cdb.cmd = 0x47;
  scmd->cdb.g1_cdb.lun = scgp->lun;
  scmd->cdb.g1_cdb.addr[1] = (from_sector + 150) / (60*75);
  scmd->cdb.g1_cdb.addr[2] = ((from_sector + 150) / 75) % 60;
  scmd->cdb.g1_cdb.addr[3] = (from_sector + 150) % 75;
  scmd->cdb.g1_cdb.res6 = (from_sector + 150 + sectors) / (60*75);
  scmd->cdb.g1_cdb.count[0] = ((from_sector + 150 + sectors) / 75) % 60;
  scmd->cdb.g1_cdb.count[1] = (from_sector + 150 + sectors) % 75;

  if (scgp->verbose) fprintf(stderr, "\nplay sectors...");
  /* do the scsi cmd */

	scgp->cmdname = "play sectors";

  return scsicmd(scgp) >= 0 ? 0 : -1;
}

static caddr_t scsibuffer;	/* page aligned scsi transfer buffer */

void init_scsibuf __PR((SCSI *, unsigned));

void init_scsibuf(scgp, amt)
	SCSI *scgp;
	unsigned amt;
{
	if (scsibuffer != NULL) {
		fprintf(stderr, "the SCSI transfer buffer has already been allocated!\n");
		exit(3);
	}
	scsibuffer = scsi_getbuf(scgp, amt);
	if (scsibuffer == NULL) {
		fprintf(stderr, "could not get SCSI transfer buffer!\n");
		exit(3);
	}
}
