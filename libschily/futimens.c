/* @(#)futimens.c	1.2 13/10/30 Copyright 2013 J. Schilling */
/*
 *	Emulate the behavior of futimens(int fd,
 *					const struct timespec times[2])
 *
 *	Copyright (c) 2013 J. Schilling
 */
/*@@C@@*/

#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/time.h>
#include <schily/utime.h>
#include <schily/fcntl.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifndef	HAVE_FUTIMENS

EXPORT int
futimens(fd, times)
	int			fd;
	const struct timespec	times[2];
{
#ifdef	HAVE_UTIMENSAT
	return (utimensat(fd, NULL, times, 0));
#else
#ifdef	HAVE_FUTIMESAT
	struct timeval tv[2];

	if (times == NULL)
		return (futimesat(fd, NULL, NULL));
	tv[0].tv_sec  = times[0].tv_sec;
	tv[0].tv_usec = times[0].tv_nsec/1000;
	tv[1].tv_sec  = times[1].tv_sec;
	tv[1].tv_usec = times[1].tv_nsec/1000;
	return (futimesat(fd, NULL, tv));
#else
#ifdef	HAVE_FUTIMES
	struct timeval tv[2];

	if (times == NULL)
		return (futimes(fd, NULL));
	tv[0].tv_sec  = times[0].tv_sec;
	tv[0].tv_usec = times[0].tv_nsec/1000;
	tv[1].tv_sec  = times[1].tv_sec;
	tv[1].tv_usec = times[1].tv_nsec/1000;
	return (futimes(fd, tv));

#else
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
#endif
#endif
#endif
}

#endif	/* HAVE_UTIMENS */
