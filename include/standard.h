/* @(#)standard.h	1.18 98/02/15 Copyright 1985 J. Schilling */
/*
 *	standard definitions
 *
 *	This file should be included past:
 *
 *	mconfig.h / config.h
 *	stdio.h
 *	stdlib.h
 *	unistd.h
 *	string.h
 *	sys/types.h
 *
 *	Copyright (c) 1985 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _STANDARD_H
#define _STANDARD_H

#ifdef	M68000
#	ifndef	tos
#		define	JOS	1
#	endif
#endif
#include <prototyp.h>

/*
 *	fundamental constants
 */
#ifndef	NULL
#	define	NULL		(char *)0
#endif
#ifndef	TRUE
#	define	TRUE		1
#	define	FALSE		0
#endif
#define	YES			1
#define	NO			0

/*
 *	Program exit codes
 */
#define	EX_BAD			(-1)

/*
 *	standard storage class definitions
 */
#define	GLOBAL	extern
#define	IMPORT	extern
#define	EXPORT
#define	INTERN	static
#define	LOCAL	static
#define	FAST	register
#if defined(_JOS) || defined(JOS)
#	define	global	extern
#	define	import	extern
#	define	export
#	define	intern	static
#	define	local	static
#	define	fast	register
#endif
#ifndef	PROTOTYPES
#	ifndef	const
#		define	const
#	endif
#	ifndef	signed
#		define	signed
#	endif
#	ifndef	volatile
#		define	volatile
#	endif
#endif	/* PROTOTYPES */

/*
 *	standard type definitions
 */
typedef int BOOL;
typedef int bool;
#ifdef	JOS
#	define	NO_VOID
#endif
#ifdef	NO_VOID
	typedef	int	VOID;
#	ifndef	lint
		typedef int void;
#	endif
#else
	typedef	void	VOID;
#endif

#if	defined(_SIZE_T)     || defined(_T_SIZE_) || defined(_T_SIZE) || \
	defined(__SIZE_T)    || defined(_SIZE_T_) || \
	defined(_GCC_SIZE_T) || defined(_SIZET_)  || \
	defined(__sys_stdtypes_h) || defined(___int_size_t_h)

#ifndef	HAVE_SIZE_T
#	define	HAVE_SIZE_T
#endif

#endif

#ifdef	EOF	/* stdio.h has been included */
extern	int	_cvmod __PR((const char *, int *, int *));
extern	FILE	*_fcons __PR((FILE *, int, int));
extern	FILE	*fdup __PR((FILE *));
extern	int	fdown __PR((FILE *));
extern	int	fexecl __PR((const char *, FILE *, FILE *, FILE *,
							const char *, ...));
extern	int	fexecle __PR((const char *, FILE *, FILE *, FILE *,
							const char *, ...));
		/* 6th arg not const, fexecv forces av[ac] = NULL */
extern	int	fexecv __PR((const char *, FILE *, FILE *, FILE *, int,
							char **));
extern	int	fexecve __PR((const char *, FILE *, FILE *, FILE *,
					char * const *, char * const *));
extern	int	fgetline __PR((FILE *, char *, int));
extern	int	fgetstr __PR((FILE *, char *, int));
extern	void	file_raise __PR((FILE *, int));
extern	int	fileclose __PR((FILE *));
extern	FILE	*fileluopen __PR((int, const char *));
extern	FILE	*fileopen __PR((const char *, const char *));
extern	long	filepos __PR((FILE *));
extern	int	fileread __PR((FILE *, void *, int));
extern	int	ffileread __PR((FILE *, void *, int));
extern	FILE	*filereopen __PR((const char *, const char *, FILE *));
extern	long	fileseek __PR((FILE *, long));
extern	long	filesize __PR((FILE *));
#ifdef	S_IFMT
extern	int	filestat __PR((FILE *, struct stat *));
#endif
extern	int	filewrite __PR((FILE *, void *, int));
extern	int	ffilewrite __PR((FILE *, void *, int));
extern	int	flush __PR((void));
extern	int	fpipe __PR((FILE **));
extern	int	fprintf __PR((FILE *, const char *, ...));
extern	int	getbroken __PR((FILE *, char *, char, char **, int));
extern	int	ofindline __PR((FILE *, char, const char *, int,
							char **, int));
extern	int	peekc __PR((FILE *));

extern	int	spawnv __PR((FILE *, FILE *, FILE *, int, char * const *));
extern	int	spawnl __PR((FILE *, FILE *, FILE *,
					const char *, const char *, ...));
extern	int	spawnv_nowait __PR((FILE *, FILE *, FILE *,
					const char *, int, char *const*));
#endif	/* EOF */

extern	int	_niread __PR((int, void *, int));
extern	int	_openfd __PR((const char *, int));
extern	void	comerr __PR((const char *, ...));
extern	void	comerrno __PR((int, const char *, ...));
extern	int	errmsg __PR((const char *, ...));
extern	int	errmsgno __PR((int, const char *, ...));
extern	char	*errmsgstr __PR((int));
extern	int	error __PR((const char *, ...));
extern	char	*fillbytes __PR((void *, int, char));
extern	int	findline __PR((const char *, char, const char *,
							int, char **, int));
extern	int	getline __PR((char *, int));
extern	int	getstr __PR((char *, int));
extern	int	breakline __PR((char *, char, char **, int));
extern	int	getallargs __PR((int *, char * const**, const char *, ...));
extern	int	getargs __PR((int *, char * const**, const char *, ...));
extern	int	getfiles __PR((int *, char * const**, const char *));
extern	char	*astoi __PR((const char *, int *));
extern	char	*astol __PR((const char *, long *));

/*extern	void	handlecond __PR((const char *, SIGBLK *, int(*)(const char *, long, long), long));*/
extern	void	unhandlecond __PR((void));

extern	int		patcompile __PR((const unsigned char *, int, int *));
extern	unsigned char	*patmatch __PR((const unsigned char *, const int *,
					const unsigned char *, int, int, int));

extern	int	printf __PR((const char *, ...));
extern	char	*movebytes __PR((const void *, void *, int));

extern	void	save_args __PR((int, char**));
extern	int	saved_ac __PR((void));
extern	char	**saved_av __PR((void));
extern	char	*saved_av0 __PR((void));
#ifndef	seterrno
extern	int	seterrno __PR((int));
#endif
extern	void	set_progname __PR((const char *));
extern	char	*get_progname __PR((void));

extern	void	setfp __PR((void * const *));
extern	int	wait_chld __PR((int));
extern	int	geterrno __PR((void));
extern	void	raisecond __PR((const char *, long));
#ifdef	HAVE_SIZE_T
extern	int	snprintf __PR((char *, size_t, const char *, ...));
#endif
/*extern	int	sprintf __PR((char *, const char *, ...)); ist woanders falsch deklariert !!!*/
extern	char	*strcatl __PR((char *, ...));
extern	int	streql __PR((const char *, const char *));
#ifdef	va_arg
extern	int	format __PR((void (*)(char, long), long, const char *, va_list));
#else
extern	int	format __PR((void (*)(char, long), long, const char *, void *));
#endif

extern	int	ftoes __PR((char *, double, int, int));
extern	int	ftofs __PR((char *, double, int, int));

extern	void	swabbytes __PR((void *, int));
extern	char	*getav0 __PR((void));
extern	char	**getavp __PR((void));
extern	void	**getfp __PR((void));
extern	int	flush_reg_windows __PR((int));
extern	int	cmpbytes __PR((const void *, const void *, int));

#if defined(_JOS) || defined(JOS)
#	include <jos_io.h>
#endif

#endif	/* _STANDARD_H */
