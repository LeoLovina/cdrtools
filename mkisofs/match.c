/*
 * 27-Mar-96: Jan-Piet Mens <jpm@mens.de>
 * added 'match' option (-m) to specify regular expressions NOT to be included
 * in the CD image.
 */

static char rcsid[] ="$Id: match.c,v 1.3 1999/03/02 03:41:25 eric Exp $";

#include "config.h"
#include <prototyp.h>
#include <stdio.h>
#ifndef VMS

#ifdef ORIG_BUT_DOES_NOT_WORK
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#else
#include <stdxlib.h>
#endif

#endif
#include <string.h>
#include "match.h"
#ifdef	USE_LIBSCHILY
#include <standard.h>
#endif

#define MAXMATCH 1000
static char *mat[MAXMATCH];

int
add_match(fn)
char * fn;
{
  register int i;

  for (i=0; mat[i] && i<MAXMATCH; i++);
  if (i == MAXMATCH) {
    fprintf(stderr,"Can't exclude RE '%s' - too many entries in table\n",fn);
    return 0;
  }

 
  mat[i] = (char *) malloc(strlen(fn)+1);
  if (mat[i] == NULL) {
#ifdef	USE_LIBSCHILY
    errmsg("Can't allocate memory for excluded filename\n");
#else
    fprintf(stderr,"Can't allocate memory for excluded filename\n");
#endif
    return 0;
  }

  strcpy(mat[i],fn);
  return 1;
}

int matches(fn)
char * fn;
{
  /* very dumb search method ... */
  register int i;

  for (i=0; mat[i] && i<MAXMATCH; i++) {
    if (fnmatch(mat[i], fn, FNM_FILE_NAME) != FNM_NOMATCH) {
      return 1; /* found -> excluded filename */
    }
  }
  return 0; /* not found -> not excluded */
}

/* ISO9660/RR hide */

static char *i_mat[MAXMATCH];

int
i_add_match(fn)
char * fn;
{
  register int i;

  for (i=0; i_mat[i] && i<MAXMATCH; i++);
  if (i == MAXMATCH) {
    fprintf(stderr,"Can't exclude RE '%s' - too many entries in table\n",fn);
    return 0;
  }

 
  i_mat[i] = (char *) malloc(strlen(fn)+1);
  if (i_mat[i] == NULL) {
#ifdef	USE_LIBSCHILY
    errmsg("Can't allocate memory for excluded filename\n");
#else
    fprintf(stderr,"Can't allocate memory for excluded filename\n");
#endif
    return 0;
  }

  strcpy(i_mat[i],fn);
  return 1;
}

int i_matches(fn)
char * fn;
{
  /* very dumb search method ... */
  register int i;

  for (i=0; i_mat[i] && i<MAXMATCH; i++) {
    if (fnmatch(i_mat[i], fn, FNM_FILE_NAME) != FNM_NOMATCH) {
      return 1; /* found -> excluded filename */
    }
  }
  return 0; /* not found -> not excluded */
}

int i_ishidden()
{
  return((int)(i_mat[0] != 0));
}

/* Joliet hide */

static char *j_mat[MAXMATCH];

int
j_add_match(fn)
char * fn;
{
  register int i;

  for (i=0; j_mat[i] && i<MAXMATCH; i++);
  if (i == MAXMATCH) {
    fprintf(stderr,"Can't exclude RE '%s' - too many entries in table\n",fn);
    return 0;
  }

 
  j_mat[i] = (char *) malloc(strlen(fn)+1);
  if (j_mat[i] == NULL) {
#ifdef	USE_LIBSCHILY
    errmsg("Can't allocate memory for excluded filename\n");
#else
    fprintf(stderr,"Can't allocate memory for excluded filename\n");
#endif
    return 0;
  }

  strcpy(j_mat[i],fn);
  return 1;
}

int j_matches(fn)
char * fn;
{
  /* very dumb search method ... */
  register int i;

  for (i=0; j_mat[i] && i<MAXMATCH; i++) {
    if (fnmatch(j_mat[i], fn, FNM_FILE_NAME) != FNM_NOMATCH) {
      return 1; /* found -> excluded filename */
    }
  }
  return 0; /* not found -> not excluded */
}

int j_ishidden()
{
  return((int)(j_mat[0] != 0));
}

void
add_list(file)
	char	*file;
{
  FILE *fp;
  char name[1024];

  if ((fp = fopen(file, "r")) == NULL) {
#ifdef	USE_LIBSCHILY
    comerr("Can't open exclude file list %s\n", file);
#else
    fprintf(stderr,"Can't open exclude file list %s\n", file);
    exit (1);
#endif
  }

  while (fscanf(fp, "%s", name) != EOF) {
    if (!add_match(name)) {
      fclose(fp);
      return;
    }
  }

  fclose(fp);
}

void
i_add_list(file)
	char	*file;
{
  FILE *fp;
  char name[1024];

  if ((fp = fopen(file, "r")) == NULL) {
#ifdef	USE_LIBSCHILY
    comerr("Can't open hidden file list %s\n", file);
#else
    fprintf(stderr,"Can't open hidden file list %s\n", file);
    exit (1);
#endif
  }

  while (fscanf(fp, "%s", name) != EOF) {
    if (!i_add_match(name)) {
      fclose(fp);
      return;
    }
  }

  fclose(fp);
}

void
j_add_list(file)
	char	*file;
{
  FILE *fp;
  char name[1024];

  if ((fp = fopen(file, "r")) == NULL) {
#ifdef	USE_LIBSCHILY
    comerr("Can't open hidden Joliet file list %s\n", file);
#else
    fprintf(stderr,"Can't open hidden Joliet file list %s\n", file);
    exit (1);
#endif
  }

  while (fscanf(fp, "%s", name) != EOF) {
    if (!j_add_match(name)) {
      fclose(fp);
      return;
    }
  }

  fclose(fp);
}
