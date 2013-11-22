/* @(#)linkat.c	1.2 13/10/30 Copyright 2011-2013 J. Schilling */
/*
 *	Emulate the behavior of linkat(int fd1, const char *name1, int fd2,
 *						const char *name2, int flag)
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

#ifndef	HAVE_LINKAT

EXPORT int	linkfollow	__PR((const char *old, const char *new));
EXPORT int
linkfollow(old, new)
	const char	*old;
	const char	*new;
{
	char	buf[max(8192, PATH_MAX+1)];

	buf[0] = '\0';
	if (resolvepath(old, buf, sizeof (buf)) < 0)
		return (-1);
	return (link(buf, new));
}

#define	KR_DECL		int flag;
/* CSTYLED */
#define	KR_ARGS		, flag
#define	FUNC_CALL(n1, n2)	(flag & AT_SYMLINK_FOLLOW ? \
					linkfollow(n1, n2) : link(n1, n2))
#define	FLAG_CHECK()	if (flag & ~(AT_SYMLINK_FOLLOW)) { \
				seterrno(EINVAL);			 \
				return (-1);				 \
			}
#define	FUNC_NAME	linkat
#define	FUNC_RESULT	int

#include "at-base2.c"

#endif	/* HAVE_LINKAT */
