/*
 * Program mkisofs.c - generate iso9660 filesystem  based upon directory
 * tree on hard disk.

   Written by Eric Youngdale (1993).

   Copyright 1993 Yggdrasil Computing, Incorporated

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

static char rcsid[] ="$Id: mkisofs.c,v 1.32 1999/03/07 21:48:49 eric Exp $";

#include "config.h"
#include <errno.h>
#include "mkisofs.h"
#include "match.h"

#ifdef linux
#include <getopt.h>
#else
#include "getopt.h"
#endif

#include "iso9660.h"
#include <ctype.h>

#ifndef VMS
#include <time.h>
#else
#include <sys/time.h>
#include "vms.h"
#endif

#include <stdlib.h>
#include <sys/stat.h>

#ifndef VMS
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif
#include <fctldefs.h>

#include "exclude.h"

#ifdef	no_more_needed
#ifdef __NetBSD__
#include <sys/time.h>
#include <sys/resource.h>
#endif
#endif	/* no_more_needed */

#ifdef	USE_LIBSCHILY
#include <standard.h>
#endif

struct directory * root = NULL;

static char version_string[] = "mkisofs 1.12";

char * outfile;
FILE * discimage;
unsigned int next_extent = 0;
unsigned int last_extent = 0;
unsigned int session_start = 0;
unsigned int path_table_size = 0;
unsigned int path_table[4] = {0,};
unsigned int path_blocks = 0;


unsigned int jpath_table_size = 0;
unsigned int jpath_table[4] = {0,};
unsigned int jpath_blocks = 0;

struct iso_directory_record root_record;
struct iso_directory_record jroot_record;

char * extension_record = NULL;
int extension_record_extent = 0;
int extension_record_size = 0;

/* These variables are associated with command line options */
int use_eltorito = 0;
int hard_disk_boot = 0;
int not_bootable = 0;
int no_emul_boot = 0;
int load_addr = 0;
int load_size = 0;
int boot_info_table = 0;
int use_sparcboot = 0;
int use_genboot = 0;
int use_RockRidge = 0;
int use_Joliet = 0;
int verbose = 1;
int gui = 0;
int all_files  = 1;		/* New default is to include all files */
int follow_links = 0;
int rationalize = 0;
int generate_tables = 0;
int print_size = 0;
int split_output = 0;
char * preparer = PREPARER_DEFAULT;
char * publisher = PUBLISHER_DEFAULT;
char * appid = APPID_DEFAULT;
char * copyright = COPYRIGHT_DEFAULT;
char * biblio = BIBLIO_DEFAULT;
char * abstract = ABSTRACT_DEFAULT;
char * volset_id = VOLSET_ID_DEFAULT;
char * volume_id = VOLUME_ID_DEFAULT;
char * system_id = SYSTEM_ID_DEFAULT;
char * boot_catalog = BOOT_CATALOG_DEFAULT;
char * boot_image = BOOT_IMAGE_DEFAULT;
char * genboot_image = BOOT_IMAGE_DEFAULT;
int volume_set_size = 1;
int volume_sequence_number = 1;

int jhide_trans_tbl;		 /* Hide TRANS.TBL from Joliet tree */
int hide_rr_moved;		 /* Name RR_MOVED .rr_moved in Rock Ridge tree */
int omit_period = 0;             /* Violates iso9660, but these are a pain */
int transparent_compression = 0; /* So far only works with linux */
int omit_version_number = 0;     /* May violate iso9660, but noone uses vers*/
int no_rr = 0;			 /* Do not use RR attributes from old session*/
int RR_relocation_depth = 6;     /* Violates iso9660, but most systems work */
int full_iso9660_filenames = 0;  /* Used with Amiga.  Disc will not work with
				  DOS */
int allow_untranslated = 0;  /* Minimal (only truncation of 30+ characters)
				translation of filenames. This is for HP-UX,
				which does not recognize ANY extentions
				(Rock Ridge, Joliet), causing pain when
				loading software. pfs_mount can be used
				to read the extensions, but the untranslated
				filenames can be read by the "native" cdfs
				mounter. Completely violates iso9660. */
int allow_leading_dots = 0;	 /* DOS cannot read names with leading dots */
int split_SL_component = 1;	 /* circumvent a bug in the SunOS driver */
int split_SL_field = 1;		 /* circumvent a bug in the SunOS */

char *trans_tbl = "TRANS.TBL";	 /* default name for translation table */

struct rcopts{
  char * tag;
  char ** variable;
};

struct rcopts rcopt[] = {
  {"PREP", &preparer},
  {"PUBL", &publisher},
  {"APPI", &appid},
  {"COPY", &copyright},
  {"BIBL", &biblio},
  {"ABST", &abstract},
  {"VOLS", &volset_id},
  {"VOLI", &volume_id},
  {"SYSI", &system_id},
  {NULL, NULL}
};

/*
 * In case it isn't obvious, the option handling code was ripped off from GNU-ld.
 */
struct ld_option
{
  /* The long option information.  */
  struct option opt;
  /* The short option with the same meaning ('\0' if none).  */
  char shortopt;
  /* The name of the argument (NULL if none).  */
  const char *arg;
  /* The documentation string.  If this is NULL, this is a synonym for
     the previous option.  */
  const char *doc;
  enum
    {
      /* Use one dash before long option name.  */
      ONE_DASH,
      /* Use two dashes before long option name.  */
      TWO_DASHES,
      /* Don't mention this option in --help output.  */
      NO_HELP
    } control;
};

/* Codes used for the long options with no short synonyms.  150 isn't
   special; it's just an arbitrary non-ASCII char value.  */
#define OPTION_HELP			150
#define OPTION_QUIET			151
#define OPTION_NOSPLIT_SL_COMPONENT	152
#define OPTION_NOSPLIT_SL_FIELD		153
#define OPTION_PRINT_SIZE		154
#define OPTION_SPLIT_OUTPUT		155
#define OPTION_ABSTRACT			156
#define OPTION_BIBLIO			157
#define OPTION_COPYRIGHT		158
#define OPTION_SYSID			159
#define OPTION_VOLSET			160
#define OPTION_VOLSET_SIZE		161
#define OPTION_VOLSET_SEQ_NUM		162
#define OPTION_I_HIDE			163
#define OPTION_J_HIDE			164
#define OPTION_LOG_FILE			165
#define OPTION_PVERSION			166
#define OPTION_NOBAK			167
#define OPTION_SPARCLABEL		168
#define OPTION_HARD_DISK_BOOT		169
#define OPTION_NO_EMUL_BOOT		170
#define OPTION_NO_BOOT			171
#define OPTION_BOOT_LOAD_ADDR		172
#define OPTION_BOOT_LOAD_SIZE		173
#define OPTION_BOOT_INFO_TABLE		174
#define OPTION_HIDE_TRANS_TBL		175
#define OPTION_HIDE_RR_MOVED		176
#define OPTION_GUI			177
#define OPTION_TRANS_TBL		178
#define OPTION_P_LIST			179
#define OPTION_I_LIST			180
#define OPTION_J_LIST			181
#define OPTION_X_LIST			182
#define OPTION_NO_RR			183

static const struct ld_option ld_options[] =
{
  { {"all-files", no_argument, NULL, 'a'},
      'a', NULL, "Process all files (don't skip backup files)", ONE_DASH },
  { {"nobak", no_argument, NULL, OPTION_NOBAK},
      '\0', NULL, "Do not include backup files", ONE_DASH },
  { {"no-bak", no_argument, NULL, OPTION_NOBAK},
      '\0', NULL, "Do not include backup files", ONE_DASH },
  { {"abstract", required_argument, NULL, OPTION_ABSTRACT},
      '\0', "FILE", "Set Abstract filename" , ONE_DASH },
  { {"appid", required_argument, NULL, 'A'},
      'A', "ID", "Set Application ID" , ONE_DASH },
  { {"biblio", required_argument, NULL, OPTION_BIBLIO},
      '\0', "FILE", "Set Bibliographic filename" , ONE_DASH },
  { {"copyright", required_argument, NULL, OPTION_COPYRIGHT},
      '\0', "FILE", "Set Copyright filename" , ONE_DASH },
  { {"eltorito-boot", required_argument, NULL, 'b'},
      'b', "FILE", "Set El Torito boot image name" , ONE_DASH },
  { {"sparc-boot", required_argument, NULL, 'B'},
      'B', "FILES", "Set sparc boot image names" , ONE_DASH },
  { {"generic-boot", required_argument, NULL, 'G'},
      'G', "FILE", "Set generic boot image name" , ONE_DASH },
  { {"sparc-label", required_argument, NULL, OPTION_SPARCLABEL},
      '\0', "label text", "Set sparc boot disk label" , ONE_DASH },
  { {"eltorito-catalog", required_argument, NULL, 'c'},
      'c', "FILE", "Set El Torito boot catalog name" , ONE_DASH },
  { {"cdrecord-params", required_argument, NULL, 'C'},
      'C', "PARAMS", "Magic paramters from cdrecord" , ONE_DASH },
  { {"omit-period", no_argument, NULL, 'd'},
      'd', NULL, "Omit trailing periods from filenames", ONE_DASH },
  { {"disable-deep-relocation", no_argument, NULL, 'D'},
      'D', NULL, "Disable deep directory relocation", ONE_DASH },
  { {"follow-links", no_argument, NULL, 'f'},
      'f', NULL, "Follow symbolic links", ONE_DASH },
  { {"help", no_argument, NULL, OPTION_HELP},
      '\0', NULL, "Print option help", ONE_DASH },
  { {"hide", required_argument, NULL, OPTION_I_HIDE},
      '\0', "GLOBFILE", "Hide ISO9660/RR file" , ONE_DASH },
  { {"hide-list", required_argument, NULL, OPTION_I_LIST},
      '\0', "FILE", "File with list of ISO9660/RR files to hide" , ONE_DASH },
  { {"hide-joliet", required_argument, NULL, OPTION_J_HIDE},
      '\0', "GLOBFILE", "Hide Joliet file" , ONE_DASH },
  { {"hide-joliet-list", required_argument, NULL, OPTION_J_LIST},
      '\0', "FILE", "File with list of Joliet files to hide" , ONE_DASH },
  { {"hide-joliet-trans-tbl", no_argument, NULL, OPTION_HIDE_TRANS_TBL},
      '\0', NULL, "Hide TRANS.TBL from Joliet tree" , ONE_DASH },
  { {"hide-rr-moved", no_argument, NULL, OPTION_HIDE_RR_MOVED},
      '\0', NULL, "Rename RR_MOVED to .rr_moved in Rock Ridge tree" , ONE_DASH },
  { {"gui", no_argument, NULL, OPTION_GUI},
      '\0', NULL, "Switch behaviour for GUI", ONE_DASH },
  { {NULL, required_argument, NULL, 'i'},
      'i', "ADD_FILES", "No longer supported" , TWO_DASHES },
  { {"joliet", no_argument, NULL, 'J'},
      'J', NULL, "Generate Joliet directory information", ONE_DASH },
  { {"full-iso9660-filenames", no_argument, NULL, 'l'},
      'l', NULL, "Allow full 32 character filenames for iso9660 names", ONE_DASH },
  { {"allow-leading-dots", no_argument, NULL, 'L'},
      'L', NULL, "Allow iso9660 filenames to start with '.'", ONE_DASH },
  { {"log-file", required_argument, NULL, OPTION_LOG_FILE},
      '\0', "LOG_FILE", "Re-direct messages to LOG_FILE", ONE_DASH },
  { {"exclude", required_argument, NULL, 'm'},
      'm', "GLOBFILE", "Exclude file name" , ONE_DASH },
  { {"exclude-list", required_argument, NULL, OPTION_X_LIST},
      '\0', "FILE", "File with list of file names to exclude" , ONE_DASH },
  { {"prev-session", required_argument, NULL, 'M'},
      'M', "FILE", "Set path to previous session to merge" , ONE_DASH },
  { {"omit-version-number", no_argument, NULL, 'N'},
      'N', NULL, "Omit version number from iso9660 filename", ONE_DASH },
  { {"no-rr", no_argument, NULL, OPTION_NO_RR},
      0, NULL, "Inhibit reading of Rock Ridge attributes from previous session" , ONE_DASH },
  { {"no-split-symlink-components", no_argument, NULL, OPTION_NOSPLIT_SL_COMPONENT},
      0, NULL, "Inhibit splitting symlink components" , ONE_DASH },
  { {"no-split-symlink-fields", no_argument, NULL, OPTION_NOSPLIT_SL_FIELD},
      0, NULL, "Inhibit splitting symlink fields" , ONE_DASH },
  { {"output", required_argument, NULL, 'o'},
      'o', "FILE", "Set output file name" , ONE_DASH },
  { {"path-list", required_argument, NULL, OPTION_P_LIST},
      '\0', "FILE", "File with list of pathnames to process" , ONE_DASH },
  { {"preparer", required_argument, NULL, 'p'},
      'p', "PREP", "Set Volume preparer" , ONE_DASH },
  { {"print-size", no_argument, NULL, OPTION_PRINT_SIZE},
      '\0', NULL, "Print estimated filesystem size and exit", ONE_DASH },
  { {"publisher", required_argument, NULL, 'P'},
      'P', "PUB", "Set Volume publisher" , ONE_DASH },
  { {"quiet", no_argument, NULL, OPTION_QUIET},
      '\0', NULL, "Run quietly", ONE_DASH },
  { {"rational-rock", no_argument, NULL, 'r'},
      'r', NULL, "Generate rationalized Rock Ridge directory information", ONE_DASH },
  { {"rock", no_argument, NULL, 'R'},
      'R', NULL, "Generate Rock Ridge directory information", ONE_DASH },
  { {"split-output", no_argument, NULL, OPTION_SPLIT_OUTPUT},
      '\0', NULL, "Split output into files of approx. 1GB size", ONE_DASH },
  { {"sysid", required_argument, NULL, OPTION_SYSID},
      '\0', "ID", "Set System ID" , ONE_DASH },
  { {"translation-table", no_argument, NULL, 'T'},
      'T', NULL, "Generate translation tables for systems that don't understand long filenames", ONE_DASH },
  { {"table-name", required_argument, NULL, OPTION_TRANS_TBL},
      '\0', "TABLE_NAME", "Translation table file name", ONE_DASH },
  { {"untranslated-filenames", no_argument, NULL, 'U'},
      'U', NULL, "Allow Untranslated filenames (for HPUX & AIX). Forces on -d, -l, -L, -N", ONE_DASH },
  { {"verbose", no_argument, NULL, 'v'},
      'v', NULL, "Verbose", ONE_DASH },
  { {"version", no_argument, NULL, OPTION_PVERSION},
      '\0', NULL, "Print the current version", ONE_DASH },
  { {"volid", required_argument, NULL, 'V'},
      'V', "ID", "Set Volume ID" , ONE_DASH },
  { {"volset", required_argument, NULL, OPTION_VOLSET},
      '\0', "ID", "Set Volume set ID" , ONE_DASH },
  { {"volset-size", required_argument, NULL, OPTION_VOLSET_SIZE},
      '\0', "#", "Set Volume set size" , ONE_DASH },
  { {"volset-seqno", required_argument, NULL, OPTION_VOLSET_SEQ_NUM},
      '\0', "#", "Set Volume set sequence number" , ONE_DASH },
  { {"old-exclude", required_argument, NULL, 'x'},
      'x', "FILE", "Exclude file name(depreciated)" , ONE_DASH },
  { {"hard-disk-boot", no_argument, NULL, OPTION_HARD_DISK_BOOT},
      '\0', NULL, "Boot image is a hard disk image", ONE_DASH },
  { {"no-emul-boot", no_argument, NULL, OPTION_NO_EMUL_BOOT},
      '\0', NULL, "Boot image is 'no emulation' image", ONE_DASH },
  { {"no-boot", no_argument, NULL, OPTION_NO_BOOT},
      '\0', NULL, "Boot image is not bootable", ONE_DASH },
  { {"boot-load-seg", required_argument, NULL, OPTION_BOOT_LOAD_ADDR},
      '\0', "#", "Set load segment for boot image" , ONE_DASH },
  { {"boot-load-size", required_argument, NULL, OPTION_BOOT_LOAD_SIZE},
      '\0', "#", "Set numbers of load sectors", ONE_DASH },
  { {"boot-info-table", no_argument, NULL, OPTION_BOOT_INFO_TABLE},
      '\0', NULL, "Patch boot image with info table", ONE_DASH },
#ifdef ERIC_neverdef
  { {"transparent-compression", no_argument, NULL, 'z'},
      'z', NULL, "Enable transparent compression of files", ONE_DASH },
#endif
};

#define OPTION_COUNT (sizeof ld_options / sizeof ld_options[0])

#if defined(ultrix) || defined(_AUX_SOURCE)
char *strdup(s)
char *s;{char *c;if(c=(char *)malloc(strlen(s)+1))strcpy(c,s);return c;}
#endif

	void read_rcfile	__PR((char * appname));
	void usage		__PR((int excode));
static	void hide_reloc_dir	__PR((void));

void FDECL1(read_rcfile, char *, appname)
{
  FILE * rcfile = (FILE *)NULL;
  struct rcopts * rco;
  char * pnt, *pnt1;
  char linebuffer[256];
  static char rcfn[] = ".mkisofsrc";
  char filename[1000];
  int linum;

  strcpy(filename, rcfn);
  if (access(filename, R_OK) == 0)
	  rcfile = fopen(filename, "r");
  if (!rcfile && errno != ENOENT)
#ifdef	USE_LIBSCHILY
    errmsg("Cannot open '%s'.\n", filename);
#else
    perror(filename);
#endif

  if (!rcfile)
    {
      pnt = getenv("MKISOFSRC");
      if (pnt && strlen(pnt) <= sizeof(filename))
	{
	  strcpy(filename, pnt);
	  if (access(filename, R_OK) == 0)
		  rcfile = fopen(filename, "r");
	  if (!rcfile && errno != ENOENT)
#ifdef	USE_LIBSCHILY
	    errmsg("Cannot open '%s'.\n", filename);
#else
	    perror(filename);
#endif
	}
    }

  if (!rcfile)
    {
      pnt = getenv("HOME");
      if (pnt && strlen(pnt) + strlen(rcfn) + 2 <= sizeof(filename))
	{
	  strcpy(filename, pnt);
	  strcat(filename, "/");
	  strcat(filename, rcfn);
	  if (access(filename, R_OK) == 0)
		  rcfile = fopen(filename, "r");
	  if (!rcfile && errno != ENOENT)
#ifdef	USE_LIBSCHILY
	    errmsg("Cannot open '%s'.\n", filename);
#else
	    perror(filename);
#endif
	}
    }
  if (!rcfile && strlen(appname)+sizeof(rcfn)+2 <= sizeof(filename))
    {
      strcpy(filename, appname);
      pnt = strrchr(filename, '/');
      if (pnt)
	{
	  strcpy(pnt + 1, rcfn);
	  if (access(filename, R_OK) == 0)
		  rcfile = fopen(filename, "r");
	  if (!rcfile && errno != ENOENT)
#ifdef	USE_LIBSCHILY
	    errmsg("Cannot open '%s'.\n", filename);
#else
	    perror(filename);
#endif
	}
    }
  if (!rcfile)
    return;
  if ( verbose > 0 )
    {
      fprintf(stderr, "Using \"%s\"\n", filename);
    }

  /* OK, we got it.  Now read in the lines and parse them */
  linum = 0;
  while (fgets(linebuffer, sizeof(linebuffer), rcfile))
    {
      char *name;
      char *name_end;
      ++linum;
      /* skip any leading white space */
	pnt = linebuffer;
      while (*pnt == ' ' || *pnt == '\t')
	++pnt;
      /* If we are looking at a # character, this line is a comment. */
	if (*pnt == '#')
	  continue;
      /* The name should begin in the left margin.  Make sure it is in
	 upper case.  Stop when we see white space or a comment. */
	name = pnt;
      while (*pnt && isalpha((unsigned char)*pnt))
	{
	  if(islower((unsigned char)*pnt))
	    *pnt = toupper((unsigned char)*pnt);
	  pnt++;
	}
      if (name == pnt)
	{
	  fprintf(stderr, "%s:%d: name required\n", filename, linum);
	  continue;
	}
      name_end = pnt;
      /* Skip past white space after the name */
      while (*pnt == ' ' || *pnt == '\t')
	pnt++;
      /* silently ignore errors in the rc file. */
      if (*pnt != '=')
	{
	  fprintf(stderr, "%s:%d: equals sign required\n", filename, linum);
	  continue;
	}
      /* Skip pas the = sign, and any white space following it */
      pnt++; /* Skip past '=' sign */
      while (*pnt == ' ' || *pnt == '\t')
	pnt++;

      /* now it is safe to NUL terminate the name */

	*name_end = 0;

      /* Now get rid of trailing newline */

      pnt1 = pnt;
      while (*pnt1)
	{
	  if (*pnt1 == '\n')
	    {
	      *pnt1 = 0;
	      break;
	    }
	  pnt1++;
	};
      /* OK, now figure out which option we have */
      for(rco = rcopt; rco->tag; rco++) {
	if(strcmp(rco->tag, name) == 0)
	  {
	    *rco->variable = strdup(pnt);
	    break;
	  };
      }
      if (rco->tag == NULL)
	{
	  fprintf(stderr, "%s:%d: field name \"%s\" unknown\n", filename, linum,
		  name);
	}
     }
  if (ferror(rcfile))
#ifdef	USE_LIBSCHILY
    errmsg("Read error on '%s'.\n", filename);
#else
    perror(filename);
#endif
  fclose(rcfile);
}

char * path_table_l = NULL;
char * path_table_m = NULL;

char * jpath_table_l = NULL;
char * jpath_table_m = NULL;

int goof = 0;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

void usage(excode)
	int	excode;
{
  const char * program_name = "mkisofs";
#if 0
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,
"mkisofs [-o outfile] [-R] [-V volid] [-v] [-a] \
[-T]\n [-l] [-d] [-V] [-D] [-L] [-p preparer]"
"[-P publisher] [ -A app_id ] [-z] \n \
[-b boot_image_name] [-c boot_catalog-name] \
[-x path -x path ...] path\n");
#endif

  int i;
/*  const char **targets, **pp;*/

  fprintf (stderr, "Usage: %s [options] file...\n", program_name);

  fprintf (stderr, "Options:\n");
  for (i = 0; i < OPTION_COUNT; i++)
    {
      if (ld_options[i].doc != NULL)
	{
	  int comma;
	  int len;
	  int j;

	  fprintf (stderr, "  ");

	  comma = FALSE;
	  len = 2;

	  j = i;
	  do
	    {
	      if (ld_options[j].shortopt != '\0'
		  && ld_options[j].control != NO_HELP)
		{
		  fprintf (stderr, "%s-%c", comma ? ", " : "", ld_options[j].shortopt);
		  len += (comma ? 2 : 0) + 2;
		  if (ld_options[j].arg != NULL)
		    {
		      if (ld_options[j].opt.has_arg != optional_argument)
			{
			  fprintf (stderr, " ");
			  ++len;
			}
		      fprintf (stderr, "%s", ld_options[j].arg);
		      len += strlen (ld_options[j].arg);
		    }
		  comma = TRUE;
		}
	      ++j;
	    }
	  while (j < OPTION_COUNT && ld_options[j].doc == NULL);

	  j = i;
	  do
	    {
	      if (ld_options[j].opt.name != NULL
		  && ld_options[j].control != NO_HELP)
		{
		  fprintf (stderr, "%s-%s%s",
			  comma ? ", " : "",
			  ld_options[j].control == TWO_DASHES ? "-" : "",
			  ld_options[j].opt.name);
		  len += ((comma ? 2 : 0)
			  + 1
			  + (ld_options[j].control == TWO_DASHES ? 1 : 0)
			  + strlen (ld_options[j].opt.name));
		  if (ld_options[j].arg != NULL)
		    {
		      fprintf (stderr, " %s", ld_options[j].arg);
		      len += 1 + strlen (ld_options[j].arg);
		    }
		  comma = TRUE;
		}
	      ++j;
	    }
	  while (j < OPTION_COUNT && ld_options[j].doc == NULL);

	  if (len >= 30)
	    {
	      fprintf (stderr, "\n");
	      len = 0;
	    }

	  for (; len < 30; len++)
	    fputc (' ', stderr);

	  fprintf (stderr, "%s\n", ld_options[i].doc);
	}
    }
  exit(excode);
}


/* 
 * Fill in date in the iso9660 format 
 *
 * The standards  state that the timezone offset is in multiples of 15
 * minutes, and is what you add to GMT to get the localtime.  The U.S.
 * is always at a negative offset, from -5h to -8h (can vary a little
 * with DST,  I guess).  The Linux iso9660 filesystem has had the sign
 * of this wrong for ages (mkisofs had it wrong too for the longest time).
 */
int FDECL2(iso9660_date,char *, result, time_t, crtime){
  struct tm *local;
  local = localtime(&crtime);
  result[0] = local->tm_year;
  result[1] = local->tm_mon + 1;
  result[2] = local->tm_mday;
  result[3] = local->tm_hour;
  result[4] = local->tm_min;
  result[5] = local->tm_sec;

  /* 
   * Must recalculate proper timezone offset each time,
   * as some files use daylight savings time and some don't... 
   */
  result[6] = local->tm_yday;	/* save yday 'cause gmtime zaps it */
  local = gmtime(&crtime);
  local->tm_year -= result[0];
  local->tm_yday -= result[6];
  local->tm_hour -= result[3];
  local->tm_min -= result[4];
  if (local->tm_year < 0) 
    {
      local->tm_yday = -1;
    }
  else 
    {
      if (local->tm_year > 0) local->tm_yday = 1;
    }

  result[6] = -(local->tm_min + 60*(local->tm_hour + 24*local->tm_yday)) / 15;

  return 0;
}

/* hide "./rr_moved" if all its contents are hidden */
static void
hide_reloc_dir()
{
	struct directory_entry * s_entry;

	for (s_entry = reloc_dir->contents; s_entry; s_entry = s_entry->next) {
	    if(strcmp(s_entry->name,".")==0 || strcmp(s_entry->name,"..")==0)
		continue;

	    if((s_entry->de_flags & INHIBIT_ISO9660_ENTRY) == 0)
		return;
	}

	/* all entries are hidden, so hide this directory */
	reloc_dir->dir_flags |= INHIBIT_ISO9660_ENTRY;
	reloc_dir->self->de_flags |= INHIBIT_ISO9660_ENTRY;
}

/* get pathnames from the command line, and then from given file */
static char *
FDECL6(get_pnames, int, argc, char **, argv, int, opt, char *, pname, int, pnsize, FILE *, fp)
{
	if (opt < argc)
	    return (argv[opt]);

	if (fp == NULL)
	    return ((char *)0);

	if (fgets(pname, pnsize, fp)) {
		pname[strlen(pname)-1] = '\0';	/* Discard newline */
		return (pname);
	}
	return ((char *)0);
}

extern char * cdrecord_data;

int FDECL2(main, int, argc, char **, argv){
  struct directory_entry de;
#ifdef HAVE_SBRK
  unsigned long mem_start;
#endif
  struct stat statbuf;
  char * merge_image = NULL;
  struct iso_directory_record * mrootp = NULL;
  struct output_fragment * opnt;
  int longind;
  char shortopts[OPTION_COUNT * 3 + 2];
  struct option longopts[OPTION_COUNT + 1];
  int c;
  char *log_file = 0;
  char * node;
  char *pathnames = 0;
  FILE *pfp = NULL;
  char pname[1024], *arg;
  int no_path_names = 1;

#ifdef __EMX__ 
	/* This gives wildcard expansion with Non-Posix shells with EMX */ 
	_wildcard(&argc, &argv); 
#endif 
	save_args(argc, argv); 

  if (argc < 2) {
#ifdef	USE_LIBSCHILY
	errmsgno(EX_BAD, "Missing pathspec.\n");
#endif
	usage(1);
  }

  /* Get the defaults from the .mkisofsrc file */
  read_rcfile(argv[0]);

  outfile = NULL;

  /*
   * Copy long option initialization from GNU-ld.
   */
  /* Starting the short option string with '-' is for programs that
     expect options and other ARGV-elements in any order and that care about
     the ordering of the two.  We describe each non-option ARGV-element
     as if it were the argument of an option with character code 1.  */
  {
    int i, is, il;
    shortopts[0] = '-';
    is = 1;
    il = 0;
    for (i = 0; i < OPTION_COUNT; i++)
      {
	if (ld_options[i].shortopt != '\0')
	  {
	    shortopts[is] = ld_options[i].shortopt;
	    ++is;
	    if (ld_options[i].opt.has_arg == required_argument
		|| ld_options[i].opt.has_arg == optional_argument)
	      {
		shortopts[is] = ':';
		++is;
		if (ld_options[i].opt.has_arg == optional_argument)
		  {
		    shortopts[is] = ':';
		    ++is;
		  }
	      }
	  }
	if (ld_options[i].opt.name != NULL)
	  {
	    longopts[il] = ld_options[i].opt;
	    ++il;
	  }
      }
    shortopts[is] = '\0';
    longopts[il].name = NULL;
  }

  while ((c = getopt_long_only (argc, argv, shortopts, longopts, &longind)) != EOF)
    switch (c)
      {
      case 1:
	/*
	 * A filename that we take as input.
	 */
	optind--;
	goto parse_input_files;
      case 'C':
	/*
	 * This is a temporary hack until cdrecord gets the proper hooks in
	 * it.
	 */
	cdrecord_data = optarg;
	break;
      case OPTION_GUI:
	gui++;
	break;
      case 'i':
#ifdef	USE_LIBSCHILY
	comerrno(EX_BAD, "-i option no longer supported.\n");
#else
	fprintf(stderr, "-i option no longer supported.\n");
	exit(1);
#endif
	break;
      case 'J':
	use_Joliet++;
	break;
      case 'a':
	all_files++;
#ifdef	USE_LIBSCHILY
	errmsgno(EX_BAD, "Option -a is obsolete. All files are included by default.\n");
#else
	fprintf(stderr, "Option -a is obsolete. All files are included by default.\n");
#endif
	break;
      case OPTION_NOBAK:
	all_files = 0;
	break;
      case 'b':
	use_eltorito++;
	boot_image = optarg;  /* pathname of the boot image on disk */
	if (boot_image == NULL) {
#ifdef	USE_LIBSCHILY
	        comerrno(EX_BAD, "Required Eltorito boot image pathname missing\n");
#else
	        fprintf(stderr, "Required Eltorito boot image pathname missing\n");
		exit(1);
#endif
	}
	break;
      case 'B':
	use_sparcboot++;
	scan_sparc_boot(optarg); /* list of pathnames of boot images */
	break;
      case 'G':
	use_genboot++;
	genboot_image = optarg;  /* pathname of the boot image on disk */
	if (genboot_image == NULL) {
#ifdef	USE_LIBSCHILY
	        comerrno(EX_BAD, "Required generic boot image pathname missing\n");
#else
	        fprintf(stderr, "Required generic boot image pathname missing\n");
		exit(1);
#endif
	}
	break;
      case OPTION_SPARCLABEL:
	sparc_boot_label(optarg); /* Sun disk label string */
	break;
      case 'c':
	use_eltorito++;
	boot_catalog = optarg;  /* pathname of the boot image on cd */
	if (boot_catalog == NULL) {
#ifdef	USE_LIBSCHILY
	        comerrno(EX_BAD, "Required boot catalog pathname missing\n");
#else
	        fprintf(stderr, "Required boot catalog pathname missing\n");
		exit(1);
#endif
	}
	break;
      case OPTION_ABSTRACT:
	abstract = optarg;
	if(strlen(abstract) > 37) {
#ifdef	USE_LIBSCHILY
		comerrno(EX_BAD, "Abstract filename string too long\n");
#else
		fprintf(stderr, "Abstract filename string too long\n");
		exit(1);
#endif
	};
	break;
      case 'A':
	appid = optarg;
	if(strlen(appid) > 128) {
#ifdef	USE_LIBSCHILY
		comerrno(EX_BAD, "Application-id string too long\n");
#else
		fprintf(stderr, "Application-id string too long\n");
		exit(1);
#endif
	};
	break;
      case OPTION_BIBLIO:
	biblio = optarg;
	if(strlen(biblio) > 37) {
#ifdef	USE_LIBSCHILY
		comerrno(EX_BAD, "Bibliographic filename string too long\n");
#else
		fprintf(stderr, "Bibliographic filename string too long\n");
		exit(1);
#endif
	};
	break;
      case OPTION_COPYRIGHT:
	copyright = optarg;
	if(strlen(copyright) > 37) {
#ifdef	USE_LIBSCHILY
		comerrno(EX_BAD, "Copyright filename string too long\n");
#else
		fprintf(stderr, "Copyright filename string too long\n");
		exit(1);
#endif
	};
	break;
      case 'd':
	omit_period++;
	break;
      case 'D':
	RR_relocation_depth = 32767;
	break;
      case 'f':
	follow_links++;
	break;
      case 'l':
	full_iso9660_filenames++;
	break;
      case 'L':
        allow_leading_dots++;
        break;
     case OPTION_LOG_FILE:
	log_file = optarg;
	break;
      case 'M':
	merge_image = optarg;
	break;
      case 'N':
	omit_version_number++;
	break;
      case OPTION_NO_RR:
	no_rr++;
	break;
      case 'o':
	outfile = optarg;
	break;
      case OPTION_P_LIST:
	pathnames = optarg;
	break;
      case 'p':
	preparer = optarg;
	if(strlen(preparer) > 128) {
#ifdef	USE_LIBSCHILY
		comerrno(EX_BAD, "Preparer string too long\n");
#else
		fprintf(stderr, "Preparer string too long\n");
		exit(1);
#endif
	};
	break;
      case OPTION_PRINT_SIZE:
	print_size++;
	break;
      case 'P':
	publisher = optarg;
	if(strlen(publisher) > 128) {
#ifdef	USE_LIBSCHILY
		comerrno(EX_BAD, "Publisher string too long\n");
#else
		fprintf(stderr, "Publisher string too long\n");
		exit(1);
#endif
	};
	break;
      case OPTION_QUIET:
	verbose = 0;
	break;
      case 'R':
	use_RockRidge++;
	break;
      case 'r':
	rationalize++;
	use_RockRidge++;
	break;
      case OPTION_SPLIT_OUTPUT:
	split_output++;
	break;
      case OPTION_SYSID:
	system_id = optarg;
	if(strlen(system_id) > 32) {
#ifdef	USE_LIBSCHILY
		comerrno(EX_BAD, "System ID string too long\n");
#else
		fprintf(stderr, "System ID string too long\n");
		exit(1);
#endif
	};
	break;
      case OPTION_TRANS_TBL:
	trans_tbl = optarg;
	/* fall through */
      case 'T':
	generate_tables++;
	break; 
      case 'U': 
	allow_untranslated++; 
	omit_period++; 
	full_iso9660_filenames++; 
	allow_leading_dots++; 
	omit_version_number++; 
	break;
      case 'V':
	volume_id = optarg;
	if(strlen(volume_id) > 32) {
#ifdef	USE_LIBSCHILY
		comerrno(EX_BAD, "Volume ID string too long\n");
#else
		fprintf(stderr, "Volume ID string too long\n");
		exit(1);
#endif
	};
	break;
      case OPTION_VOLSET:
	volset_id = optarg;
	if(strlen(volset_id) > 128) {
#ifdef	USE_LIBSCHILY
		comerrno(EX_BAD, "Volume set ID string too long\n");
#else
		fprintf(stderr, "Volume set ID string too long\n");
		exit(1);
#endif
	};
	break;
      case OPTION_VOLSET_SIZE:
	volume_set_size = atoi(optarg);
	break;
      case OPTION_VOLSET_SEQ_NUM:
	volume_sequence_number = atoi(optarg);
	if (volume_sequence_number > volume_set_size) {
#ifdef	USE_LIBSCHILY
		comerrno(EX_BAD, "Volume set sequence number too big\n");
#else
		fprintf(stderr, "Volume set sequence number too big\n");
		exit(1);
#endif
	}
	break;
      case 'v':
	verbose++;
	break;
      case 'z':
#ifdef VMS
#ifdef	USE_LIBSCHILY
	comerrno(EX_BAD, "Transparent compression not supported with VMS\n");
#else
	fprintf(stderr, "Transparent compression not supported with VMS\n");
	exit(1);
#endif
#else
	transparent_compression++;
#endif
	break;
      case 'x':
      case 'm':
	/*
	 * Somehow two options to do basically the same thing got added somewhere along
	 * the way.  The 'match' code supports limited globbing, so this is the one
	 * that got selected.  Unfortunately the 'x' switch is probably more intuitive.
	 */
        add_match(optarg);
	break;
      case OPTION_X_LIST:
	add_list(optarg);
	break;
      case OPTION_I_HIDE:
	i_add_match(optarg);
	break;
      case OPTION_I_LIST:
	i_add_list(optarg);
	break;
      case OPTION_J_HIDE:
	j_add_match(optarg);
	break;
      case OPTION_J_LIST:
	j_add_list(optarg);
	break;
      case OPTION_HIDE_TRANS_TBL:
	jhide_trans_tbl++;
	break;
      case OPTION_HIDE_RR_MOVED:
	hide_rr_moved++;
	break;
      case OPTION_HELP:
	usage(0);
	break;
      case OPTION_PVERSION:
	printf("%s (%s-%s-%s)\n", version_string, HOST_CPU, HOST_VENDOR, HOST_OS);
	exit (0);
	break;
      case OPTION_NOSPLIT_SL_COMPONENT:
	split_SL_component = 0;
	break;
      case OPTION_NOSPLIT_SL_FIELD:
	split_SL_field = 0;
	break;
      case OPTION_HARD_DISK_BOOT:
	use_eltorito++;
	hard_disk_boot++;
	break;
      case OPTION_NO_EMUL_BOOT:
	use_eltorito++;
	no_emul_boot++;
	break;
      case OPTION_NO_BOOT:
	use_eltorito++;
	not_bootable++;
	break;
      case OPTION_BOOT_LOAD_ADDR:
	use_eltorito++;
	{
		long val;
		char *ptr;
		
		val = strtol(optarg, &ptr, 0);
		if(*ptr || val < 0 || val >= 0x10000) {
#ifdef	USE_LIBSCHILY
			comerrno(EX_BAD, "Boot image load address invalid.\n");
#else
			fprintf(stderr, "Boot image load address invalid.\n");
			exit(1);
#endif
		}
		load_addr = val;
	}
	break;
      case OPTION_BOOT_LOAD_SIZE:
	use_eltorito++;
	{
		long val;
		char *ptr;
		
		val = strtol(optarg, &ptr, 0);
		if(*ptr || val < 0 || val >= 0x10000) {
#ifdef	USE_LIBSCHILY
			comerrno(EX_BAD, "Boot image load size invalid.\n");
#else
			fprintf(stderr, "Boot image load size invalid.\n");
			exit(1);
#endif
		}
		load_size = val;
	}
	break;
      case OPTION_BOOT_INFO_TABLE:
	use_eltorito++;
	boot_info_table++;
	break;
      default:
	usage(1);
      }

parse_input_files:

  if( merge_image != NULL )
    {
      if (open_merge_image(merge_image) < 0)
	{
	  /*
	   * Complain and die.
	   */
#ifdef	USE_LIBSCHILY
	  comerr("Unable to open previous session image %s\n",
		  merge_image);
#else
	  fprintf(stderr,"Unable to open previous session image %s\n",
		  merge_image);
	  exit(1);
#endif
	}
    }
	/*
	 * We don't need root privilleges anymore.
	 */
#ifdef	HAVE_SETREUID
	if (setreuid(-1, getuid()) < 0)
#else
#ifdef	HAVE_SETEUID
	if (seteuid(getuid()) < 0)
#else
	if (setuid(getuid()) < 0)
#endif
#endif
#ifdef	USE_LIBSCHILY
		comerr("Panic cannot set back efective uid.\n");
#else
		{perror("Panic cannot set back efective uid.");exit(1);}
#endif


#ifdef	no_more_needed
#ifdef __NetBSD__
    {
	int resource;
    struct rlimit rlp;
	if (getrlimit(RLIMIT_DATA,&rlp) == -1) 
		perror("Warning: getrlimit");
	else {
		rlp.rlim_cur=33554432;
		if (setrlimit(RLIMIT_DATA,&rlp) == -1)
			perror("Warning: setrlimit");
		}
	}
#endif
#endif	/* no_more_needed */
#ifdef HAVE_SBRK
  mem_start = (unsigned long) sbrk(0);
#endif

  /* if the -hide-joliet option has been given, set the Joliet option */
  if (!use_Joliet && j_ishidden())
    use_Joliet++;

  if(verbose > 1) fprintf(stderr,"%s (%s-%s-%s)\n", version_string, HOST_CPU, HOST_VENDOR, HOST_OS);

  if(cdrecord_data == NULL && merge_image != NULL)
    {
#ifdef	USE_LIBSCHILY
      comerrno(EX_BAD, "Multisession usage bug: Must specify -C if -M is used.\n");
#else
      fprintf(stderr, "Multisession usage bug: Must specify -C if -M is used.\n");
      exit(1);
#endif
    }

  if(cdrecord_data != NULL && merge_image == NULL)
    {
#ifdef	USE_LIBSCHILY
      errmsgno(EX_BAD, "Warning: -C specified without -M: old session data will not be merged.\n");
#else
      fprintf(stderr, "Warning: -C specified without -M: old session data will not be merged.\n");
#endif
    }

  /*  The first step is to scan the directory tree, and take some notes */

  if(!argv[optind]) {
#ifdef	USE_LIBSCHILY
	errmsgno(EX_BAD, "Missing pathspec.\n");
#endif
	usage(1);
  }

  if(use_RockRidge){
#if 1
	extension_record = generate_rr_extension_record("RRIP_1991A",
				       "THE ROCK RIDGE INTERCHANGE PROTOCOL PROVIDES SUPPORT FOR POSIX FILE SYSTEM SEMANTICS",
				       "PLEASE CONTACT DISC PUBLISHER FOR SPECIFICATION SOURCE.  SEE PUBLISHER IDENTIFIER IN PRIMARY VOLUME DESCRIPTOR FOR CONTACT INFORMATION.", &extension_record_size);
#else
	extension_record = generate_rr_extension_record("IEEE_P1282",
				       "THE IEEE P1282 PROTOCOL PROVIDES SUPPORT FOR POSIX FILE SYSTEM SEMANTICS",
				       "PLEASE CONTACT THE IEEE STANDARDS DEPARTMENT, PISCATAWAY, NJ, USA FOR THE P1282 SPECIFICATION.", &extension_record_size);
#endif
  }

  if (log_file) {
    FILE *lfp;
    int i;

    /* open log file - test that we can open OK */
    if ((lfp = fopen(log_file, "w")) == NULL) {
#ifdef	USE_LIBSCHILY
      comerr("can't open logfile: %s\n", log_file);
#else
      fprintf(stderr,"can't open logfile: %s\n", log_file);
      exit (1);
#endif
    }
    fclose(lfp);

    /* redirect all stderr message to log_file */
    fprintf(stderr, "re-directing all messages to %s\n", log_file);
    fflush(stderr);

    /* associate stderr with the log file */
    if (freopen(log_file, "w", stderr) == NULL) {
#ifdef	USE_LIBSCHILY
      comerr("can't open logfile: %s\n", log_file);
#else
      fprintf(stderr,"can't open logfile: %s\n", log_file);
      exit (1);
#endif
    }
    if(verbose > 1) {
      for (i=0;i<argc;i++)
       fprintf(stderr,"%s ", argv[i]);

      fprintf(stderr,"\n%s (%s-%s-%s)\n", version_string, HOST_CPU, HOST_VENDOR, HOST_OS);
    }
  }

  /*
   * Find name of root directory.
   */
  node = strchr(argv[optind], '=');
  if ( node == NULL ) {
    node = argv[optind];
  } else {
    ++node;
  }

  /*
   * See if boot catalog file exists in root directory, if not
   * we will create it.
   */
  if (use_eltorito)
    init_boot_catalog(node);

  /*
   * Find the device and inode number of the root directory.
   * Record this in the hash table so we don't scan it more than
   * once.
   */
  stat_filter(node, &statbuf);
  add_directory_hash(statbuf.st_dev, STAT_INODE(statbuf));

  memset(&de, 0, sizeof(de));

  de.filedir = root;  /* We need this to bootstrap */

  if (cdrecord_data != NULL && merge_image == NULL) {
    /* in case we want to add a new session, but don't want to merge old one */
    get_session_start(NULL);
  }

  if( merge_image != NULL )
    {
      mrootp = merge_isofs(merge_image);
      if( mrootp == NULL )
	{
	  /*
	   * Complain and die.
	   */
#ifdef	USE_LIBSCHILY
	  comerr("Unable to find previous session image %s\n",
		  merge_image);
#else
	  fprintf(stderr,"Unable to find previous session image %s\n",
		  merge_image);
	  exit(1);
#endif
	}

      memcpy(de.isorec.extent, mrootp->extent, 8);      
    }

  /*
   * Create an empty root directory. If we ever scan it for real, we will fill in the
   * contents.
   */
  find_or_create_directory(NULL, "", &de, TRUE);

  /* see if we have a list of pathnames to process */
  if (pathnames) {
	/* "-" means take list from the standard input */
	if (strcmp(pathnames, "-")) {
	    if ((pfp = fopen(pathnames, "r")) == NULL) {
#ifdef	USE_LIBSCHILY
		comerr("Unable to open pathname list %s.\n", pathnames);
#else
		fprintf(stderr, "Unable to open pathname list %s.\n", pathnames);
		exit (1);
#endif
	    }
	}
	else
	    pfp = stdin;
  }

  /*
   * Scan the actual directory (and any we find below it)
   * for files to write out to the output image.  Note - we
   * take multiple source directories and keep merging them
   * onto the image.
   */
    while((arg = get_pnames(argc, argv, optind, pname, sizeof(pname), pfp)) != NULL)
    {
      struct directory * graft_dir;
      struct stat        st;
      char             * short_name;
      int                status;
      char   graft_point[1024];

      /*
       * We would like a syntax like:
       *
       * /tmp=/usr/tmp/xxx
       *
       * where the user can specify a place to graft each
       * component of the tree.  To do this, we may have to create
       * directories along the way, of course.
       * Secondly, I would like to allow the user to do something
       * like:
       *
       * /home/baz/RMAIL=/u3/users/baz/RMAIL
       *
       * so that normal files could also be injected into the tree
       * at an arbitrary point.
       *
       * The idea is that the last component of whatever is being
       * entered would take the name from the last component of
       * whatever the user specifies.
       *
       * The default will be that the file is injected at the
       * root of the image tree.
       */
      node = strchr(arg, '=');
      short_name = NULL;

      if( node != NULL )
	{
	  char * pnt;
	  char * xpnt;

	  *node = '\0';
	  strcpy(graft_point, arg);
	  *node = '=';
	  node++;

	  graft_dir = root;
	  xpnt = graft_point;
	  if( *xpnt == PATH_SEPARATOR )
	    {
	      xpnt++;
	    }

	  /*
	   * Loop down deeper and deeper until we
	   * find the correct insertion spot.
	   */
	  while(1==1)
	    {
	      pnt = strchr(xpnt, PATH_SEPARATOR);
	      if( pnt == NULL )
		{
		  if( *xpnt != '\0' )
		    {
		      short_name = xpnt;
		    }
		  break;
		}
	      *pnt = '\0';
	      graft_dir = find_or_create_directory(graft_dir, 
						   graft_point, 
						   NULL, TRUE);
	      *pnt = PATH_SEPARATOR;
	      xpnt = pnt + 1;
	    }
	}
      else
	{
	  graft_dir = root;
	  node = arg;
	}

      /*
       * Now see whether the user wants to add a regular file,
       * or a directory at this point.
       */
      status = stat_filter(node, &st);
      if( status != 0 )
	{
	  /*
	   * This is a fatal error - the user won't be getting what
	   * they want if we were to proceed.
	   */
#ifdef	USE_LIBSCHILY
	  comerr("Invalid node - %s\n", node);
#else
	  fprintf(stderr, "Invalid node - %s\n", node);
	  exit(1);
#endif
	}
      else
	{
	  if( S_ISDIR(st.st_mode) )
	    {
	      if (!scan_directory_tree(graft_dir, node, &de))
		{
		  exit(1);
		}
	    }
	  else
	    {
	      if( short_name == NULL )
		{
		  short_name = strrchr(node, PATH_SEPARATOR);
		  if( short_name == NULL || short_name < node )
		    {
		      short_name = node;
		    }
		  else
		    {
		      short_name++;
		    }
		}
	      if( !insert_file_entry(graft_dir, node, short_name) )
		{
		 exit(1);
		}
	    }
	}

      optind++;
      no_path_names = 0;
    }

  if (pfp && pfp != stdin)
    fclose(pfp);

  /*
   * exit if we don't have any pathnames to process - not going to happen
   * at the moment as we have to have at least one path on the command line
   */
  if (no_path_names) {
#ifdef	USE_LIBSCHILY
	errmsgno(EX_BAD, "No pathnames found.\n");
#endif
	usage(1);
  }

  /*
   * Now merge in any previous sessions.  This is driven on the source
   * side, since we may need to create some additional directories.
   */
  if( merge_image != NULL )
    {
      if (merge_previous_session(root, mrootp) < 0) {
#ifdef	USE_LIBSCHILY
	comerrno(EX_BAD, "Cannot merge previous session.\n");
#else
	fprintf(stderr, "Cannot merge previous session.\n");
	exit(1);
#endif
      }
    }

  /* hide "./rr_moved" if all its contents have been hidden */
  if (reloc_dir && i_ishidden())
    hide_reloc_dir();

  /* insert the boot catalog if required */
  if (use_eltorito)
    insert_boot_cat();

  /*
   * Sort the directories in the required order (by ISO9660).  Also,
   * choose the names for the 8.3 filesystem if required, and do
   * any other post-scan work.
   */
  goof += sort_tree(root);

  if (goof)
    {
#ifdef	USE_LIBSCHILY
      comerrno(EX_BAD, "ISO9660/Rock Ridge tree sort failed.\n");
#else
      fprintf(stderr, "ISO9660/Rock Ridge tree sort failed.\n");
      exit(1);
#endif
    }

  if( use_Joliet )
    {
      goof += joliet_sort_tree(root);
    }

  if (goof)
    {
#ifdef	USE_LIBSCHILY
      comerrno(EX_BAD, "Joliet tree sort failed.\n");
#else
      fprintf(stderr, "Joliet tree sort failed.\n");
      exit(1);
#endif
    }
  
  /*
   * Fix a couple of things in the root directory so that everything
   * is self consistent.
   */
  root->self = root->contents;  /* Fix this up so that the path 
				   tables get done right */

  /*
   * OK, ready to write the file.  Open it up, and generate the thing.
   */
  if (print_size){
	  discimage = fopen("/dev/null", "wb");
	  if (!discimage){
#ifdef	USE_LIBSCHILY
		  comerr("Unable to open /dev/null\n");
#else
		  fprintf(stderr,"Unable to open /dev/null\n");
		  exit(1);
#endif
	  }
  } else if (outfile){
	  discimage = fopen(outfile, "wb");
	  if (!discimage){
#ifdef	USE_LIBSCHILY
		  comerr("Unable to open disc image file\n");
#else
		  fprintf(stderr,"Unable to open disc image file\n");
		  exit(1);
#endif
	  };
  } else {
	  discimage =  stdout;

#if	defined(__CYGWIN32__) || defined(__EMX__)
	setmode(fileno(stdout), O_BINARY);
#endif
  }

  /* Now assign addresses on the disc for the path table. */

  path_blocks = (path_table_size + (SECTOR_SIZE - 1)) >> 11;
  if (path_blocks & 1) path_blocks++;

  jpath_blocks = (jpath_table_size + (SECTOR_SIZE - 1)) >> 11;
  if (jpath_blocks & 1) jpath_blocks++;

  /*
   * Start to set up the linked list that we use to track the
   * contents of the disc.
   */
  if (use_sparcboot)
	outputlist_insert(&sunlabel_desc);
  if (use_genboot)
	outputlist_insert(&genboot_desc);
  outputlist_insert(&padblock_desc);

  /*
   * PVD for disc.
   */
  outputlist_insert(&voldesc_desc);

  /*
   * SVD for El Torito. MUST be immediately after the PVD!
   */
  if( use_eltorito)
    {
      outputlist_insert(&torito_desc);
    }

  /*
   * SVD for Joliet.
   */
  if( use_Joliet)
    {
      outputlist_insert(&joliet_desc);
    }

  /*
   * Finally the last volume desctiptor.
   */
  outputlist_insert(&end_vol);


  outputlist_insert(&pathtable_desc);
  if( use_Joliet)
    {
      outputlist_insert(&jpathtable_desc);
    }

  outputlist_insert(&dirtree_desc);
  if( use_Joliet)
    {
      outputlist_insert(&jdirtree_desc);
    }

  outputlist_insert(&dirtree_clean);

  if(extension_record) 
    { 
      outputlist_insert(&extension_desc);
    }

  outputlist_insert(&files_desc);

  /*
   * Allow room for the various headers we will be writing.  There
   * will always be a primary and an end volume descriptor.
   */
  last_extent = session_start;
  
  /*
   * Calculate the size of all of the components of the disc, and assign
   * extent numbers.
   */
  for(opnt = out_list; opnt; opnt = opnt->of_next )
    {
      if( opnt->of_size != NULL )
	{
	  (*opnt->of_size)(last_extent);
	}
    }

  /*
   * Generate the contents of any of the sections that we want to generate.
   * Not all of the fragments will do anything here - most will generate the
   * data on the fly when we get to the write pass.
   */
  for(opnt = out_list; opnt; opnt = opnt->of_next )
    {
      if( opnt->of_generate != NULL )
	{
	  (*opnt->of_generate)();
	}
    }
  c = 0;
  if (use_sparcboot) {
	c = make_sun_label();
	outputlist_insert(&sunboot_desc);
  }

  if( in_image != NULL )
    {
      fclose(in_image);
    }

  if( print_size > 0 )
    {
      fprintf(stderr,"Total extents scheduled to be written = %d\n", 
	      (last_extent - session_start) + c);
	exit(0);
    }
  /*
   * Now go through the list of fragments and write the data that corresponds to
   * each one.
   */
  for(opnt = out_list; opnt; opnt = opnt->of_next )
    {
      if( opnt->of_write != NULL )
	{
	  (*opnt->of_write)(discimage);
	}
    }

  if( verbose > 0 )
    {
#ifdef HAVE_SBRK
      fprintf(stderr,"Max brk space used %x\n", 
	      (unsigned int)(((unsigned long)sbrk(0)) - mem_start));
#endif
      fprintf(stderr,"%d extents written (%d Mb)\n", last_extent, last_extent >> 9);
    }

#ifdef VMS
  return 1;
#else
  return 0;
#endif
}

void *
FDECL1(e_malloc, size_t, size)
{
void* pt = 0;
	if( (size > 0) && ((pt=malloc(size))==NULL) ) {
#ifdef	USE_LIBSCHILY
		comerr("Not enough memory\n");
#else
		fprintf(stderr, "Not enough memory\n");
		exit (1);
#endif
		}
return pt;
}
