/* @(#)mkfifo.c	1.1 13/10/27 Copyright 2013 J. Schilling */
/*
 *	Emulate the behavior of mkfifo(const char *name, mode_t mode)
 *
 *	Copyright (c) 2013 J. Schilling
 */
/*@@C@@*/

#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/time.h>
#include <schily/fcntl.h>
#include <schily/stat.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifndef	HAVE_MKFIFO

#define	S_ALLPERM	(S_IRWXU|S_IRWXG|S_IRWXO)
#define	S_ALLFLAGS	(S_ISUID|S_ISGID|S_ISVTX)
#define	S_ALLMODES	(S_ALLFLAGS | S_ALLPERM)

#ifdef	PROTOTYPES
EXPORT int
mkfifo(const char *name, mode_t mode)
#else
EXPORT int
mkfifo(name, mode)
	const char		*name;
	mode_t			mode;
#endif
{
#ifdef	HAVE_MKNOD
	return (mknod(name, S_IFIFO | (mode & S_ALLMODES), 0));
#else
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
#endif
}

#endif	/* HAVE_MKFIFO */
