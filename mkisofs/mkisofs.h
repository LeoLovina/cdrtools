/* @(#)mkisofs.h	1.49 00/04/27 joerg */
/*
 * Header file mkisofs.h - assorted structure definitions and typecasts.

   Written by Eric Youngdale (1993).

   Copyright 1993 Yggdrasil Computing, Incorporated
   Copyright (c) 1999,2000 J. Schilling

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

/* APPLE_HYB James Pearson j.pearson@ge.ucl.ac.uk 23/2/2000 */

#include <stdio.h>
#include <prototyp.h>

/*
 * This symbol is used to indicate that we do not have things like
 * symlinks, devices, and so forth available.  Just files and dirs
 */
#ifdef VMS
#define NON_UNIXFS
#endif

#ifdef DJGPP
#define NON_UNIXFS
#endif

#ifdef VMS
#include <sys/dir.h>
#define dirent direct
#endif

#ifdef _WIN32
#define NON_UNIXFS
#endif	/* _WIN32 */

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <statdefs.h>

#ifndef	HAVE_LSTAT
#ifndef	VMS
#define	lstat	stat
#endif
#endif

#if defined(HAVE_DIRENT_H)
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if defined(HAVE_SYS_NDIR_H)
#include <sys/ndir.h>
#endif
#if defined(HAVE_SYS_DIR_H)
#include <sys/dir.h>
#endif
#if defined(HAVE_NDIR_H)
#include <ndir.h>
#endif
#endif

#if defined(HAVE_STRING_H)
#include <string.h>
#else
#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif
#endif

#ifdef ultrix
extern char    *strdup();

#endif

#ifdef __STDC__
#else
#define const
#endif


#ifdef __SVR4
#include <stdlib.h>
#else
extern int      optind;
extern char    *optarg;

/* extern int getopt (int __argc, char **__argv, char *__optstring); */
#endif

#include "iso9660.h"
#include "defaults.h"

#ifdef APPLE_HYB
#include "mactypes.h"
#include "hfs.h"

struct hfs_info {
	unsigned char	finderinfo[32];
	char		name[HFS_MAX_FLEN + 1];
	/* should have fields for dates here as well */
	char		*keyname;
	struct hfs_info *next;
};

#endif	/* APPLE_HYB */

struct directory_entry {
	struct directory_entry *next;
	struct directory_entry *jnext;
	struct iso_directory_record isorec;
	unsigned int    starting_block;
	unsigned int    size;
	unsigned short  priority;
	unsigned char   jreclen;	/* Joliet record len */
	char           *name;
	char           *table;
	char           *whole_name;
	struct directory *filedir;
	struct directory_entry *parent_rec;
	unsigned int    de_flags;
	ino_t           inode;		/* Used in the hash table */
	dev_t           dev;		/* Used in the hash table */
	unsigned char  *rr_attributes;
	unsigned int    rr_attr_size;
	unsigned int    total_rr_attr_size;
	unsigned int    got_rr_name;
#ifdef APPLE_HYB
	struct directory_entry *assoc;	/* entry has a resource fork */
	hfsdirent      *hfs_ent;	/* HFS parameters */
	unsigned int    hfs_off;	/* offset to real start of fork */
	unsigned int    hfs_type;	/* type of HFS Unix file */
#endif	/* APPLE_HYB */
#ifdef SORTING
	int		sort;		/* sort weight for entry */
#endif /* SORTING */
};

struct file_hash {
	struct file_hash *next;
	ino_t           inode;		/* Used in the hash table */
	dev_t           dev;		/* Used in the hash table */
	unsigned int    starting_block;
	unsigned int    size;
#ifdef SORTING
	struct directory_entry *de;
#endif /* SORTING */
};


/*
 * This structure is used to control the output of fragments to the cdrom
 * image.  Everything that will be written to the output image will eventually
 * go through this structure.   There are two pieces - first is the sizing where
 * we establish extent numbers for everything, and the second is when we actually
 * generate the contents and write it to the output image.
 *
 * This makes it trivial to extend mkisofs to write special things in the image.
 * All you need to do is hook an additional structure in the list, and the rest
 * works like magic.
 *
 * The three passes each do the following:
 *
 * The 'size' pass determines the size of each component and assigns the extent number
 * for that component.
 *
 * The 'generate' pass will adjust the contents and pointers as required now that extent
 * numbers are assigned.   In some cases, the contents of the record are also generated.
 *
 * The 'write' pass actually writes the data to the disc.
 */
struct output_fragment {
	struct output_fragment *of_next;
	int             (*of_size)	__PR((int));
	int             (*of_generate)	__PR((void));
	int             (*of_write)	__PR((FILE *));
};

extern struct output_fragment *out_list;
extern struct output_fragment *out_tail;

extern struct output_fragment padblock_desc;
extern struct output_fragment voldesc_desc;
extern struct output_fragment joliet_desc;
extern struct output_fragment torito_desc;
extern struct output_fragment end_vol;
extern struct output_fragment version_desc;
extern struct output_fragment pathtable_desc;
extern struct output_fragment jpathtable_desc;
extern struct output_fragment dirtree_desc;
extern struct output_fragment dirtree_clean;
extern struct output_fragment jdirtree_desc;
extern struct output_fragment extension_desc;
extern struct output_fragment files_desc;
extern struct output_fragment padend_desc;
extern struct output_fragment sunboot_desc;
extern struct output_fragment sunlabel_desc;
extern struct output_fragment genboot_desc;

#ifdef APPLE_HYB
extern struct output_fragment hfs_desc;

#endif	/* APPLE_HYB */

/*
 * This structure describes one complete directory.  It has pointers
 * to other directories in the overall tree so that it is clear where
 * this directory lives in the tree, and it also must contain pointers
 * to the contents of the directory.  Note that subdirectories of this
 * directory exist twice in this stucture.  Once in the subdir chain,
 * and again in the contents chain.
 */
struct directory {
	struct directory *next;		/* Next directory at same level as this one */
	struct directory *subdir;	/* First subdirectory in this directory */
	struct directory *parent;
	struct directory_entry *contents;
	struct directory_entry *jcontents;
	struct directory_entry *self;
	char           *whole_name;	/* Entire path */
	char           *de_name;	/* Entire path */
	unsigned int    ce_bytes;	/* Number of bytes of CE entries read
					   for this dir */
	unsigned int    depth;
	unsigned int    size;
	unsigned int    extent;
	unsigned int    jsize;
	unsigned int    jextent;
	unsigned short  path_index;
	unsigned short  jpath_index;
	unsigned short  dir_flags;
	unsigned short  dir_nlink;
#ifdef APPLE_HYB
	hfsdirent      *hfs_ent;	/* HFS parameters */
	struct hfs_info *hfs_info;	/* list of info for all entries in dir */
#endif	/* APPLE_HYB */
#ifdef SORTING
	int		sort;		/* sort weight for child files */
#endif /* SORTING */
};

struct deferred_write {
	struct deferred_write *next;
	char           *table;
	unsigned int    extent;
	unsigned int    size;
	char           *name;
	struct directory_entry *s_entry;
	unsigned int    pad;
	unsigned int    off;
};

extern int      goof;
extern struct directory *root;
extern struct directory *reloc_dir;
extern unsigned int next_extent;
extern unsigned int last_extent;
extern unsigned int last_extent_written;
extern unsigned int session_start;

extern unsigned int path_table_size;
extern unsigned int path_table[4];
extern unsigned int path_blocks;
extern char    *path_table_l;
extern char    *path_table_m;

extern unsigned int jpath_table_size;
extern unsigned int jpath_table[4];
extern unsigned int jpath_blocks;
extern char    *jpath_table_l;
extern char    *jpath_table_m;

extern struct iso_directory_record root_record;
extern struct iso_directory_record jroot_record;

extern int      check_oldnames;
extern int      use_eltorito;
extern int      hard_disk_boot;
extern int      not_bootable;
extern int      no_emul_boot;
extern int      load_addr;
extern int      load_size;
extern int      boot_info_table;
extern int      use_RockRidge;
extern int      use_Joliet;
extern int      rationalize;
extern int      follow_links;
extern int      verbose;
extern int      gui;
extern int      all_files;
extern int      generate_tables;
extern int      print_size;
extern int      split_output;
extern int      jhide_trans_tbl;
extern int      hide_rr_moved;
extern int      omit_period;
extern int      omit_version_number;
extern int      no_rr;
extern int      transparent_compression;
extern int      RR_relocation_depth;
extern int	iso9660_level;
extern int	iso9660_namelen;
extern int      full_iso9660_filenames;
extern int	relaxed_filenames;
extern int	allow_lowercase;
extern int	allow_multidot;
extern int	iso_translate;
extern int	allow_leading_dots;
extern int	use_fileversion;
extern int      split_SL_component;
extern int      split_SL_field;
extern char    *trans_tbl;

#ifdef APPLE_HYB
extern int      apple_hyb;	/* create HFS hybrid */
extern int      apple_ext;	/* use Apple extensions */
extern int      apple_both;	/* common flag (for above) */
extern int      hfs_extra;	/* extra ISO extents (hfs_ce_size) */
extern hce_mem *hce;		/* libhfs/mkisofs extras */
extern int      use_mac_name;	/* use Mac name for ISO9660/Joliet/RR */
extern int      create_dt;	/* create the Desktp files */
extern char    *hfs_boot_file;	/* name of HFS boot file */
extern char    *magic_file;	/* magic file for CREATOR/TYPE matching */
extern int      hfs_last;	/* order in which to process map/magic files */
extern char    *deftype;	/* default Apple TYPE */
extern char    *defcreator;	/* default Apple CREATOR */
extern int      gen_pt;		/* generate HFS partition table */
extern char    *autoname;	/* Autostart filename */
extern int      afe_size;	/* Apple File Exchange block size */
extern char    *hfs_volume_id;	/* HFS volume ID */
extern int      icon_pos;	/* Keep Icon position */

#define MAP_LAST	1	/* process magic then map file */
#define MAG_LAST	2	/* process map then magic file */

#ifndef PREP_BOOT
#define PREP_BOOT
#endif	/* PREP_BOOT */

#ifdef PREP_BOOT
extern char    *prep_boot_image[4];
extern int      use_prep_boot;

#endif	/* PREP_BOOT */
#endif	/* APPLE_HYB */

#ifdef SORTING
extern int      do_sort;
#endif /* SORTING */

/* tree.c */
extern int stat_filter __PR((char *, struct stat *));
extern int lstat_filter __PR((char *, struct stat *));
extern int sort_tree __PR((struct directory *));
extern struct directory *
                find_or_create_directory __PR((struct directory *,
		                const char *,
		                struct directory_entry * self, int));
extern void finish_cl_pl_entries __PR((void));
extern int scan_directory_tree __PR((struct directory * this_dir,
		                char *path,
		                struct directory_entry * self));

#ifdef APPLE_HYB
extern int insert_file_entry __PR((struct directory *, char *,
		                char *, int));

#else
extern int insert_file_entry __PR((struct directory *, char *,
		                char *));

#endif	/* APPLE_HYB */

extern void generate_iso9660_directories __PR((struct directory *, FILE *));
extern void dump_tree __PR((struct directory * node));
extern struct directory_entry *search_tree_file __PR((struct
		                directory * node, char *filename));
extern void update_nlink_field __PR((struct directory * node));
extern void init_fstatbuf __PR((void));
extern struct stat root_statbuf;
extern struct stat fstatbuf;

/* eltorito.c */
extern void init_boot_catalog __PR((const char *path));
extern void get_torito_desc __PR((struct eltorito_boot_descriptor * path));
extern void insert_boot_cat __PR((void));

/* boot.c */
extern void sparc_boot_label __PR((char *label));
extern void scan_sparc_boot __PR((char *files));
extern int make_sun_label __PR((void));

/* write.c */
extern int get_731 __PR((char *));
extern int get_732 __PR((char *));
extern int get_733 __PR((char *));
extern int isonum_733 __PR((unsigned char *));
extern void set_723 __PR((char *, unsigned int));
extern void set_731 __PR((char *, unsigned int));
extern void set_721 __PR((char *, unsigned int));
extern void set_733 __PR((char *, unsigned int));
extern int sort_directory __PR((struct directory_entry **, int));
extern void generate_one_directory __PR((struct directory *, FILE *));
extern void memcpy_max __PR((char *, char *, int));
extern int oneblock_size __PR((int starting_extent));
extern struct iso_primary_descriptor vol_desc;
extern void xfwrite __PR((void *buffer, int count, int size, FILE * file));
extern void set_732 __PR((char *pnt, unsigned int i));
extern void set_722 __PR((char *pnt, unsigned int i));
extern void outputlist_insert __PR((struct output_fragment * frag));

#ifdef APPLE_HYB
extern int get_adj_size __PR((int Csize));
extern int adj_size __PR((int Csize, int start_extent, int extra));
extern void adj_size_other __PR((struct directory * dpnt));
extern int insert_padding_file __PR((int size));
extern int gen_mac_label __PR((struct deferred_write *));

#ifdef PREP_BOOT
extern void gen_prepboot_label __PR((unsigned char *));

#endif	/* PREP_BOOT */
#endif	/* APPLE_HYB */

/* multi.c */

extern FILE    *in_image;
extern int open_merge_image __PR((char *path));
extern int close_merge_image __PR((void));
extern struct iso_directory_record *
	                merge_isofs __PR((char *path));
extern int free_mdinfo __PR((struct directory_entry **, int len));
extern unsigned char *parse_xa __PR((unsigned char *pnt, int *lenp,
		                struct directory_entry * dpnt));
extern struct directory_entry **
	                read_merging_directory __PR((struct iso_directory_record *, int *));
extern void merge_remaining_entries __PR((struct directory *,
		                struct directory_entry **, int));
extern int merge_previous_session __PR((struct directory *,
		                struct iso_directory_record *));
extern int get_session_start __PR((int *));

/* joliet.c */
int joliet_sort_tree __PR((struct directory * node));

/* match.c */
extern int matches __PR((char *));
extern int add_match __PR((char *));

/* files.c */
struct dirent  *readdir_add_files __PR((char **, char *, DIR *));

/* name.c */

extern void iso9660_check	__PR((struct iso_directory_record *idr, struct directory_entry *ndr)); 
extern int iso9660_file_length __PR((const char *name,
		                struct directory_entry * sresult, int flag));

/* various */
extern int iso9660_date __PR((char *, time_t));
extern void add_hash __PR((struct directory_entry *));
extern struct file_hash *find_hash __PR((dev_t, ino_t));

extern void flush_hash __PR((void));
extern void add_directory_hash __PR((dev_t, ino_t));
extern struct file_hash *find_directory_hash __PR((dev_t, ino_t));
extern void flush_file_hash __PR((void));
extern int delete_file_hash __PR((struct directory_entry *));
extern struct directory_entry *find_file_hash __PR((char *));
extern void add_file_hash __PR((struct directory_entry *));

extern int generate_rock_ridge_attributes __PR((char *, char *,
		                struct directory_entry *,
		                struct stat *, struct stat *,
		                int deep_flag));
extern char    *generate_rr_extension_record __PR((char *id,
		                char *descriptor,
		                char *source, int *size));

extern int check_prev_session __PR((struct directory_entry **, int len,
		                struct directory_entry *,
		                struct stat *,
		                struct stat *,
		                struct directory_entry **));

extern void match_cl_re_entries __PR((void));
extern void finish_cl_pl_for_prev_session __PR((void));
extern char    *find_rr_attribute __PR((unsigned char *pnt, int len, char *attr_type));

#ifdef	USE_SCG
/* scsi.c */
extern int readsecs __PR((int startsecno, void *buffer, int sectorcount));
extern int scsidev_open __PR((char *path));
extern int scsidev_close __PR((void));

#endif

#ifdef APPLE_HYB
/* volume.c */
extern int make_mac_volume __PR((struct directory * dpnt, int start_extent));
extern int write_fork __PR((hfsfile * hfp, long tot));

/* apple.c */

extern void del_hfs_info __PR((struct hfs_info *));
extern int get_hfs_dir __PR((char *, char *, struct directory_entry *));
extern int get_hfs_info __PR((char *, char *, struct directory_entry *));
extern int get_hfs_rname __PR((char *, char *, char *));
extern int hfs_exclude __PR((char *));
extern void print_hfs_info __PR((struct directory_entry *));
extern void hfs_init __PR((char *, unsigned short, unsigned int));
extern void delete_rsrc_ent __PR((struct directory_entry *));
extern void clean_hfs __PR((void));
extern void perr __PR((char *));
extern void set_root_info __PR((char *));

/* desktop.c */

extern int make_desktop __PR((hfsvol *, int));

/* mac_label.c */

#ifdef	_MAC_LABEL_H
#ifdef PREP_BOOT
extern void gen_prepboot_label __PR((MacLabel * mac_label));
#endif
extern int gen_mac_label __PR((defer *));
#endif
extern int autostart __PR((void));

/* libfile */

extern char    *get_magic_match __PR((const char *));
extern void clean_magic __PR((void));

#endif	/* APPLE_HYB */

extern char    *extension_record;
extern int      extension_record_extent;
extern int      n_data_extents;

/*
 * These are a few goodies that can be specified on the command line, and  are
 * filled into the root record
 */
extern char    *preparer;
extern char    *publisher;
extern char    *copyright;
extern char    *biblio;
extern char    *abstract;
extern char    *appid;
extern char    *volset_id;
extern char    *system_id;
extern char    *volume_id;
extern char    *boot_catalog;
extern char    *boot_image;
extern char    *genboot_image;
extern int	ucs_level;
extern int      volume_set_size;
extern int      volume_sequence_number;

extern void    *e_malloc __PR((size_t));


#define SECTOR_SIZE	(2048)
#define ISO_ROUND_UP(X)	((X + (SECTOR_SIZE - 1)) & ~(SECTOR_SIZE - 1))
#define ROUND_UP(X,Y)	(((X + (Y - 1)) / Y) * Y)

#ifdef APPLE_HYB
#define HFS_ROUND_UP(X)	ISO_ROUND_UP(((X)*HFS_BLOCKSZ))
/*
 * ISO blocks == 2048, HFS blocks == 512
 */
#define HFS_BLK_CONV	(SECTOR_SIZE/HFS_BLOCKSZ)

#define USE_MAC_NAME(E)	(use_mac_name && ((E)->hfs_ent != NULL) && (E)->hfs_type)
#endif	/* APPLE_HYB */

#define NEED_RE		1
#define NEED_PL		2
#define NEED_CL		4
#define NEED_CE		8
#define NEED_SP		16

#define PREV_SESS_DEV	(sizeof(dev_t) >= 4 ? 0x7ffffffd : 0x7ffd)
#define TABLE_INODE	(sizeof(ino_t) >= 4 ? 0x7ffffffe : 0x7ffe)
#define UNCACHED_INODE	(sizeof(ino_t) >= 4 ? 0x7fffffff : 0x7fff)
#define UNCACHED_DEVICE	(sizeof(dev_t) >= 4 ? 0x7fffffff : 0x7fff)

#ifdef VMS
#define STAT_INODE(X)	(X.st_ino[0])
#define PATH_SEPARATOR	']'
#define SPATH_SEPARATOR	""
#else
#define STAT_INODE(X)	(X.st_ino)
#define PATH_SEPARATOR	'/'
#define SPATH_SEPARATOR	"/"
#endif

/*
 * When using multi-session, indicates that we can reuse the
 * TRANS.TBL information for this directory entry.  If this flag
 * is set for all entries in a directory, it means we can just
 * reuse the TRANS.TBL and not generate a new one.
 */
#define SAFE_TO_REUSE_TABLE_ENTRY  0x01
#define DIR_HAS_DOT		   0x02
#define DIR_HAS_DOTDOT		   0x04
#define INHIBIT_JOLIET_ENTRY	   0x08
#define INHIBIT_RR_ENTRY	   0x10
#define RELOCATED_DIRECTORY	   0x20
#define INHIBIT_ISO9660_ENTRY	   0x40
#define MEMORY_FILE		   0x80
#define HIDDEN_FILE		   0x100

/*
 * Volume sequence number to use in all of the iso directory records.
 */
#define DEF_VSN		1

/*
 * Make sure we have a definition for this.  If not, take a very conservative
 * guess.
 * POSIX requires the max pathname component lenght to be defined in limits.h
 * If variable, it may be undefined. If undefined, there should be
 * a definition for _POSIX_NAME_MAX in limits.h or in unistd.h
 * As _POSIX_NAME_MAX is defined to 14, we cannot use it.
 * XXX Eric's wrong comment:
 * XXX From what I can tell SunOS is the only one with this trouble.
 */
#ifdef	HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef NAME_MAX
#ifdef FILENAME_MAX
#define NAME_MAX	FILENAME_MAX
#else
#define NAME_MAX	128
#endif
#endif

#ifndef PATH_MAX
#ifdef FILENAME_MAX
#define PATH_MAX	FILENAME_MAX
#else
#define PATH_MAX	1024
#endif
#endif

/*
 * XXX JS: Some structures have odd lengths!
 * Some compilers (e.g. on Sun3/mc68020) padd the structures to even length.
 * For this reason, we cannot use sizeof (struct iso_path_table) or
 * sizeof (struct iso_directory_record) to compute on disk sizes.
 * Instead, we use offsetof(..., name) and add the name size.
 * See iso9660.h
 */
#ifndef	offsetof
#define	offsetof(TYPE, MEMBER)	((size_t) &((TYPE *)0)->MEMBER)
#endif
