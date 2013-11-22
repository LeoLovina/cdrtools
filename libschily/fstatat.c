/* @(#)fstatat.c	1.2 13/10/30 Copyright 2013 J. Schilling */
/*
 *	Emulate the behavior of fstatat(int fd, const char *name,
 *					struct stat *sbuf, int flag)
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
#include <schily/stat.h>
#include <schily/maxpath.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "at-defs.h"

#ifndef	HAVE_FSTATAT

#ifndef	HAVE_LSTAT
#define	lstat	stat
#endif

/* CSTYLED */
#define	PROTO_DECL	, struct stat *sbuf, int flag
#define	KR_DECL		struct stat *sbuf; int flag;
/* CSTYLED */
#define	KR_ARGS		, sbuf, flag
#define	FUNC_CALL(n)	(flag & AT_SYMLINK_NOFOLLOW ? \
				lstat(n, sbuf) : stat(n, sbuf))
#define	FLAG_CHECK()	if (flag & ~(AT_SYMLINK_NOFOLLOW|_AT_TRIGGER)) { \
				seterrno(EINVAL);			 \
				return (-1);				 \
			}
#define	FUNC_NAME	fstatat
#define	FUNC_RESULT	int

#include "at-base.c"

#endif	/* HAVE_FSTATAT */
