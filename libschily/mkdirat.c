/* @(#)mkdirat.c	1.2 13/10/30 Copyright 2013 J. Schilling */
/*
 *	Emulate the behavior of mkdirat(int fd, const char *name, mode_t mode)
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

#ifndef	HAVE_MKDIRAT

/* CSTYLED */
#define	PROTO_DECL	, mode_t mode
#define	KR_DECL		mode_t mode;
/* CSTYLED */
#define	KR_ARGS		, mode
#define	FUNC_CALL(n)	mkdir(n, mode)
#define	FLAG_CHECK()
#define	FUNC_NAME	mkdirat
#define	FUNC_RESULT	int

#include "at-base.c"

#endif	/* HAVE_MKDIRAT */
