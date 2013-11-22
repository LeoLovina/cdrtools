/* @(#)renameat.c	1.1 13/10/30 Copyright 2011-2013 J. Schilling */
/*
 *	Emulate the behavior of renameat(int fd1, const char *name1,
 *					int fd2, const char *name2)
 *
 *	Note that emulation methods that do not use the /proc filesystem are
 *	not MT safe. In the non-MT-safe case, we do:
 *
 *		savewd()/fchdir()/open(name)/restorewd()
 *
 *	Errors may force us to abort the program as our caller is not expected
 *	to know that we do more than a simple open() here and that the
 *	working directory may be changed by us.
 *
 *	Copyright (c) 2013 J. Schilling
 */
/*@@C@@*/

#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/fcntl.h>
#include <schily/maxpath.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "at-defs.h"

#ifndef	HAVE_RENAMEAT

#define	KR_DECL
#define	KR_ARGS
#define	FUNC_CALL(n1, n2)	rename(n2, n2)
#define	FLAG_CHECK()
#define	FUNC_NAME		renameat
#define	FUNC_RESULT		int

#include "at-base2.c"

#endif	/* HAVE_RENAMEAT */
