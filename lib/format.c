/* @(#)format.c	1.18 96/02/04 Copyright 1985 J. Schilling */
/*
 *	format
 *	common code for printf fprintf & sprintf
 *
 *	allows recursive printf with "%r", used in:
 *	error, comerr, comerrno, errmsg, errmsgno and the like
 *
 *	Copyright (c) 1985 J. Schilling
 */
/* This program is free software; you can redistribute it and/or modify
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

#define	format	__nothing__	/* prototype in standard.h may be wrong */
#include <standard.h>
#undef format

#ifdef	HAVE_STDARG_H
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif
#ifdef	HAVE_STRING_H
#include <string.h>
#endif
#ifdef	HAVE_STDLIB_H
#include <stdlib.h>
#else
extern	char	*gcvt __PR((double, int, char *));
#endif

#ifdef	DO_MASK
#define	CHARMASK	(0xFFL)
#define	SHORTMASK	(0xFFFFL)
#define	INTMASK		(0xFFFFFFFFL)
#endif

#ifdef	DIVLBYS
extern	long	divlbys();
extern	long	modlbys();
#else
#define	divlbys(val, base)	((val)/(base))
#define	modlbys(val, base)	((val)%(base))
#endif

/*
 *	We use macros here to avoid the need to link to the international
 *	character routines.
 *	We don't need internationalization for our purpose.
 */
#define	is_dig(c)	(((c) >= '0') && ((c) <= '9'))
#define	is_cap(c)	((c) >= 'A' && (c) <= 'Z')
#define	to_cap(c)	(is_cap(c) ? c : c - 'a' + 'A')
#define	cap_ty(c)	(is_cap(c) ? 'L' : 'I')

typedef unsigned int	uint;
typedef unsigned short	ushort;
typedef unsigned long	ulong;

typedef struct f_args {
	int	(*outf) __PR((char, long));
	long	farg;
	int	minusflag;
	int	fldwidth;
	int	signific;
	char	*buf;
	char	*bufp;
	char	fillc;
	char	*prefix;
	int	prefixlen;
} f_args;

LOCAL	void	prmod  __PR((ulong, unsigned, f_args *));
LOCAL	void	prdmod __PR((ulong, f_args *));
LOCAL	void	promod __PR((ulong, f_args *));
LOCAL	void	prxmod __PR((ulong, f_args *));
LOCAL	void	prXmod __PR((ulong, f_args *));
LOCAL	int	prbuf  __PR((const char *, f_args *));
LOCAL	int	prc    __PR((char, f_args *));
LOCAL	int	prstring __PR((const char *, f_args *));


#ifdef	PROTOTYPES
EXPORT int format(	int (*fun)(char, long),
			long farg,
			const char *fmt,
			va_list args)
#else
EXPORT int format(fun, farg, fmt, args)
	register int	(*fun)();
	register long	farg;
	register char	*fmt;
	va_list		args;
#endif
{
	char buf[512];
	const char *sfmt;
	register int unsflag;
	register long val;
	register char type;
	register char mode;
	register char c;
	int count;
	int i;
	short sh;
	const char *str;
	double dval;
	ulong res;
	char *rfmt;
	va_list rargs;
	f_args	fa;

	fa.outf = fun;
	fa.farg = farg;
	count = 0;
	/*
	 * Main loop over the format string.
	 * Increment and check for end of string is made here.
	 */
	for(; *fmt != '\0'; fmt++) {
		c = *fmt;
		while (c != '%') {
			if (c == '\0')
				return (count);
			(*fun)(c, farg);
			c = *(++fmt);
			count++;
		}

		/*
		 * We reached a '%' sign.
		 */
		buf[0] = '\0';
		fa.buf = fa.bufp = buf;
		fa.minusflag = 0;
		fa.fldwidth = 0;
		fa.signific = -1;
		fa.fillc = ' ';
		fa.prefixlen = 0;
		sfmt = fmt;
		unsflag = FALSE;
		if (*(++fmt) == '+') {
			fmt++;
/*			fa.plusflag++;*/ /* XXX to be implemented */
		}
		if (*fmt == '-') {
			fmt++;
			fa.minusflag++;
		}
		if (*fmt == '0') {
			fmt++;
			fa.fillc = '0';
		}
		if (*fmt == '*') {
			fmt++;
			fa.fldwidth = va_arg(args, int);
			if (fa.fldwidth < 0) {
				fa.fldwidth = -fa.fldwidth;
				fa.minusflag ^= 1;
			}
		} else while (c = *fmt, is_dig(c)) {
			fa.fldwidth *= 10;
			fa.fldwidth += c - '0';
			fmt++;
		}
		if (*fmt == '.') {
			fmt++;
			fa.signific = 0;
			if (*fmt == '*') {
				fmt++;
				fa.signific = va_arg(args, int);
				if (fa.signific < 0)
					fa.signific = 0;
			} else while (c = *fmt, is_dig(c)) {
				fa.signific *= 10;
				fa.signific += c - '0';
				fmt++;
			}
		}
		if (strchr("UCSIL", *fmt)) {
			/*
			 * Enhancements to K&R and ANSI:
			 *
			 * got a type specifyer
			 */
			if (fa.signific == -1)
				fa.signific = 0;
			if (*fmt == 'U') {
				fmt++;
				unsflag = TRUE;
			}
			if (!strchr("CSILZODX", *fmt)) {
				/*
				 * Got only 'U'nsigned specifyer,
				 * use default type and mode.
				 */
				type = 'I';
				mode = 'D';
				fmt--;
			} else if (!strchr("CSIL", *fmt)) {
				/*
				 * no type, use default
				 */
				type = 'I';
				mode = *fmt;
			} else {
				/*
				 * got CSIL
				 */
				type = *fmt++;
				if (!strchr("ZODX", mode = *fmt)) {
					fmt--;
					mode = 'D';/* default mode */
				}
			}
		} else switch(*fmt) {

		case 'l':
			if (fa.signific == -1)
				fa.signific = 0;
			type = 'L';		/* convert to type */

			if (!strchr("udiox", *(++fmt))) {
				fmt--;
				mode = 'D';
			} else {
				if ((mode = to_cap(*fmt)) == 'U')
					unsflag = TRUE;
				if (mode == 'I')	/*XXX */
					mode = 'D';
			}
			break;
		case 'u':
			unsflag = TRUE;
		case 'o': case 'O':
		case 'd': case 'D':
		case 'i': case 'I':
		case 'x': case 'X':
		case 'z': case 'Z':
			if (fa.signific == -1)
				fa.signific = 0;
			mode = to_cap(*fmt);
			type = cap_ty(*fmt);
			if (mode == 'I')	/*XXX kann entfallen*/
				mode = 'D';	/*wenn besseres uflg*/
			break;

		case '%':
			if (fa.signific == -1)
				fa.signific = 0;
			count += prc('%', &fa);
			continue;
		case ' ':
			if (fa.signific == -1)
				fa.signific = 0;
			count += prbuf("", &fa);
			continue;
		case 'c':
			if (fa.signific == -1)
				fa.signific = 0;
			c = va_arg(args, int);
			count += prc(c, &fa);
			continue;
		case 's':
			if (fa.signific == -1)
				fa.signific = 0;
			str = va_arg(args, char *);
			count += prstring(str, &fa);
			continue;
		case 'b':
			str = va_arg(args, char *);
			fa.signific = va_arg(args, int);
			count += prstring(str, &fa);
			continue;

#ifndef	NO_FLOATINGPOINT
		case 'e':
			if (fa.signific == -1)
				fa.signific = 6;
			dval = va_arg(args, double);
			ftoes(buf, dval, 0, fa.signific);
			count += prbuf(buf, &fa);
			continue;
		case 'f':
			if (fa.signific == -1)
				fa.signific = 6;
			dval = va_arg(args, double);
			ftofs(buf, dval, 0, fa.signific);
			count += prbuf(buf, &fa);
			continue;
		case 'g':
			if (fa.signific == -1)
				fa.signific = 6;
			if (fa.signific == 0)
				fa.signific = 1;
			dval = va_arg(args, double);
			gcvt(dval, fa.signific, buf);
			count += prbuf(buf, &fa);
			continue;
#endif

		case 'r':			/* recursive printf */
			rfmt  = va_arg(args, char *);
			rargs = va_arg(args, va_list);
			format(fun, farg, rfmt, rargs);
			continue;

		default:
			count += fmt - sfmt;
			while (sfmt < fmt)
				(*fun)(*(sfmt++), farg);
			if (*fmt == '\0') {
				fmt--;
				continue;
			} else {
				(*fun)(*fmt, farg);
				count++;
				continue;
			}
		}
		/*
		 * print numbers:
		 * first prepare type 'C'har, 'S'hort, 'I'nt, or 'L'ong
		 */
		switch(type) {

		case 'C':
			c = va_arg(args, int);
			val = c;		/* extend sign here */
			if (unsflag || mode != 'D')
#ifdef	DO_MASK
				val &= CHARMASK;
#else
				val = (unsigned char)val;
#endif
			break;
		case 'S':
			sh = va_arg(args, int);
			val = sh;		/* extend sign here */
			if (unsflag || mode != 'D')
#ifdef	DO_MASK
				val &= SHORTMASK;
#else
				val = (unsigned short)val;
#endif
			break;
		case 'I':
			i = va_arg(args, int);
			val = i;		/* extend sign here */
			if (unsflag || mode != 'D')
#ifdef	DO_MASK
				val &= INTMASK;
#else
				val = (unsigned int)val;
#endif
			break;
		case 'L':
			val = va_arg(args, long);
			break;
		}

		/*
		 * Final print out, take care of mode:
		 * mode is one of: 'O'ctal, 'D'ecimal, or he'X'
		 * oder 'Z'weierdarstellung.
		 */
		if (val == 0) {
			count += prbuf("0", &fa);
			continue;
		} else switch(mode) {

		case 'U':
		case 'D':
			if (!unsflag && val < 0) {
				fa.prefix = "-";
				fa.prefixlen = 1;
				val = -val;
			}

			/* output a long unsigned decimal number */
			if (res = divlbys(((ulong)val)>>1, (uint)5))
				prdmod(res, &fa);
			val = modlbys(val, (uint)10);
			prdmod(val < 0 ?-val:val, &fa);
			break;
		case 'O':
			/* output a long octal number */
			if (res = (val>>3) & 0x1FFFFFFFL)
				promod(res, &fa);
			promod(val & 07, &fa);
			break;
		case 'x':
			/* output a hex long */
			if (res = (val>>4) & 0xFFFFFFFL)
				prxmod(res, &fa);
			prxmod(val & 0xF, &fa);
			break ;
		case 'X':
			/* output a hex long */
			if (res = (val>>4) & 0xFFFFFFFL)
				prXmod(res, &fa);
			prXmod(val & 0xF, &fa);
			break ;
		case 'Z':
			/* output a binary long */
			if (res = (val>>1) & 0x7FFFFFFFL)
				prmod(res, 2, &fa);
			prmod(val & 0x1, 2, &fa);
		}
		*fa.bufp = '\0';
		count += prbuf(fa.buf, &fa);
	}
	return (count);
}

/*
 * Routines to print (not negative) numbers in an arbitrary base
 */
LOCAL	unsigned char	dtab[]  = "0123456789abcdef";
LOCAL	unsigned char	udtab[] = "0123456789ABCDEF";

LOCAL void prmod(val, base, fa)
	ulong val;
	unsigned base;
	f_args *fa;
{
	if (val >= base)
		prmod(divlbys(val, base), base, fa);
	*fa->bufp++ = dtab[modlbys(val, base)];
}

LOCAL void prdmod(val, fa)
	ulong val;
	f_args *fa;
{
	if (val >= (unsigned)10)
		prdmod(divlbys(val, (unsigned)10), fa);
	*fa->bufp++ = dtab[modlbys(val, (unsigned)10)];
}

LOCAL void promod(val, fa)
	ulong val;
	f_args *fa;
{
	if (val >= (unsigned)8)
		promod(val>>3, fa);
	*fa->bufp++ = dtab[val & 7];
}

LOCAL void prxmod(val, fa)
	ulong val;
	f_args *fa;
{
	if (val >= (unsigned)16)
		prxmod(val>>4, fa);
	*fa->bufp++ = dtab[val & 15];
}

LOCAL void prXmod(val, fa)
	ulong val;
	f_args *fa;
{
	if (val >= (unsigned)16)
		prXmod(val>>4, fa);
	*fa->bufp++ = udtab[val & 15];
}

/*
 * Final buffer print out routine.
 */
LOCAL int prbuf(s, fa)
	register const char *s;
	f_args *fa;
{
	register int diff;
	register int rfillc;
	register long arg			= fa->farg;
	register int (*fun) __PR((char, long))	= fa->outf;
	register int count;

	count = strlen(s);
	diff = fa->fldwidth - count - fa->prefixlen;
	if (diff > 0)
		count += diff;

	if (fa->prefixlen && fa->fillc != ' ') {
		while (*fa->prefix != '\0')
			(*fun)(*fa->prefix++, arg);
	}
	if (!fa->minusflag) {
		rfillc = fa->fillc;
		while (--diff >= 0)
			(*fun)(rfillc, arg);
	}
	if (fa->prefixlen && fa->fillc == ' ') {
		while (*fa->prefix != '\0')
			(*fun)(*fa->prefix++, arg);
	}
	while (*s != '\0')
		(*fun)(*s++, arg);
	if (fa->minusflag) {
		rfillc = ' ';
		while (--diff >= 0)
			(*fun)(rfillc, arg);
	}
	return (count);
}

/*
 * Print out one char, allowing prc('\0')
 * Similar to prbuf()
 */
#ifdef	PROTOTYPES

LOCAL int prc(char c, f_args *fa)

#else
LOCAL int prc(c, fa)
	char	c;
	f_args *fa;
#endif
{
	register int diff;
	register int rfillc;
	register long arg			= fa->farg;
	register int (*fun) __PR((char, long))	= fa->outf;
	register int count;

	count = 1;
	diff = fa->fldwidth - 1;
	if (diff > 0)
		count += diff;

	if (!fa->minusflag) {
		rfillc = fa->fillc;
		while (--diff >= 0)
			(*fun)(rfillc, arg);
	}
	(*fun)(c, arg);
	if (fa->minusflag) {
		rfillc = ' ';
		while (--diff >= 0)
			(*fun)(rfillc, arg);
	}
	return (count);
}

/*
 * String output routine.
 * If fa->signific is != 0, it uses only fa->signific chars.
 */
LOCAL int prstring(s, fa)
	register const char	*s;
	f_args *fa;
{
	register char	*bp;

	if (s == NULL)
		return (prbuf("(NULL POINTER)", fa));

	if (fa->signific == 0)
		return (prbuf(s, fa));

	bp = fa->buf;

	while (--fa->signific >= 0 && *s != '\0')
		*bp++ = *s++;
	*bp = '\0';

	return (prbuf(fa->buf, fa));
}
