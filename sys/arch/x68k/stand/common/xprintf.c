/*
 *	minimal printf for Human68k DOS
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 *	$NetBSD: xprintf.c,v 1.1.168.1 2009/05/13 17:18:43 jym Exp $
 */

#include <sys/types.h>
#ifdef __STDC__
# include <stdarg.h>
#else
# include <varargs.h>
#endif

#include <dos.h>
#include <dos_errno.h>

#include "xprintf.h"

/*
 * From ISO/IEC 9899:1990
 * 7.9.6.1 The fprintf function
 * ...
 * Environment limit
 *    The minimum value for the maximum number of characters
 * produced by any single conversion shall be 509.
 *
 * so the following value shall not be smaller than 510
 * if you want to conform ANSI C (it is only a guideline
 * and maybe no sense on this code, of course :-).
 */
#define PRINTF_BUFSZ	4096

/*
 * Shift-JIS kanji support
 * (No special handling needed for EUC)
 */
#define SJIS

#ifdef SJIS
#define UC(c)		((unsigned char) (c))
#define IS_SJIS1(c)	((UC(c) > 0x80 && UC(c) < 0xa0) ||	\
				(UC(c) >= 0xe0 && UC(c) <= 0xfc))
#define IS_SJIS2(c)	(UC(c) >= 0x40 && UC(c) <= 0xfc && UC(c) != 0x7f)
#endif

#if !defined(__STDC__) && !defined(const)
#define const
#endif

extern const char *const __progname;

static char * numstr(char *buf, long val, int base, int sign);

/*
 * convert number to string
 * buf must have enough space
 */
static char *
numstr(char *buf, long val, int base, int sign)
{
	unsigned long v;
	char rev[32];
	char *r = rev, *b = buf;

	/* negative? */
	if (sign && val < 0) {
		v = -val;
		*b++ = '-';
	} else {
		v = val;
	}

	/* inverse order */
	do {
		*r++ = "0123456789abcdef"[v % base];
		v /= base;
	} while (v);

	/* reverse string */
	while (r > rev)
		*b++ = *--r;

	*b = '\0';
	return buf;
}

/*
 * supported format: %x, %p, %s, %c, %d, %u, %o
 * \n is converted to \r\n
 *
 * XXX argument/parameter types are not strictly handled
 */
size_t
xvsnprintf(char *buf, size_t len, const char *fmt, va_list ap)
{
	char *b = buf;
	const char *s;
	char numbuf[32];

	while (*fmt && len > 1) {
		if (*fmt != '%') {
#ifdef SJIS
			if (IS_SJIS1(*fmt) && IS_SJIS2(fmt[1])) {
				if (len <= 2)
					break;	/* not enough space */
				*b++ = *fmt++;
				len--;
			}
#endif
			if (*fmt == '\n' && (b == buf || b[-1] != '\r')) {
				if (len <= 2)
					break;
				*b++ = '\r';
				len--;
			}
			*b++ = *fmt++;
			len--;
			continue;
		}

		/* %? */
		fmt++;
		switch (*fmt++) {
		case '%':	/* "%%" -> literal % */
			*b++ = '%';
			len--;
			break;

		case 'd':
			s = numstr(numbuf, va_arg(ap, long), 10, 1);
		copy_string:
			for ( ; *s && len > 1; len--)
				*b++ = *s++;
			break;

		case 'u':
			s = numstr(numbuf, va_arg(ap, long), 10, 0);
			goto copy_string;

		case 'p':
			*b++ = '0';
			len--;
			if (len > 1) {
				*b++ = 'x';
				len--;
			}
			/* FALLTHROUGH */
		case 'x':
			s = numstr(numbuf, va_arg(ap, long), 16, 0);
			goto copy_string;

		case 'o':
			s = numstr(numbuf, va_arg(ap, long), 8, 0);
			goto copy_string;

		case 's':
			s = va_arg(ap, char *);
			while (*s && len > 1) {
#ifdef SJIS
				if (IS_SJIS1(*s) && IS_SJIS2(s[1])) {
					if (len <= 2)
						goto break_loop;
					*b++ = *s++;
					len--;
				}
#endif
				if (*s == '\n' && (b == buf || b[-1] != '\r')) {
					if (len <= 2)
						goto break_loop;
					*b++ = '\r';
					len--;
				}
				*b++ = *s++;
				len--;
			}
			break;

		case 'c':
			*b++ = va_arg(ap, int);
			len--;
			break;
		}
	}
break_loop:

	*b = '\0';
	return (char *)b - buf;
}

#ifdef __STDC__
#define VA_START(a, v)	va_start(a, v)
#else
#define VA_START(a, v)	va_start(a)
#endif

#ifdef __STDC__
size_t
xsnprintf(char *buf, size_t len, const char *fmt, ...)
#else
size_t
xsnprintf(buf, len, fmt, va_alist)
	char *buf;
	size_t len;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	size_t ret;

	VA_START(ap, fmt);
	ret = xvsnprintf(buf, len, fmt, ap);
	va_end(ap);

	return ret;
}

size_t
xvfdprintf(int fd, const char *fmt, va_list ap)
{
	char buf[PRINTF_BUFSZ];
	size_t ret;

	ret = xvsnprintf(buf, sizeof buf, fmt, ap);
	if (ret)
		ret = DOS_WRITE(fd, buf, ret);

	return ret;
}

#ifdef __STDC__
size_t
xprintf(const char *fmt, ...)
#else
size_t
xprintf(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	size_t ret;

	VA_START(ap, fmt);
	ret = xvfdprintf(1, fmt, ap);
	va_end(ap);

	return ret;
}

#ifdef __STDC__
size_t
xerrprintf(const char *fmt, ...)
#else
size_t
xerrprintf(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	size_t ret;

	VA_START(ap, fmt);
	ret = xvfdprintf(2, fmt, ap);
	va_end(ap);

	return ret;
}

__dead void
#ifdef __STDC__
xerr(int eval, const char *fmt, ...)
#else
xerr(eval, fmt, va_alist)
	int eval;
	const char *fmt;
	va_dcl
#endif
{
	int e = dos_errno;
	va_list ap;

	xerrprintf("%s: ", __progname);
	if (fmt) {
		VA_START(ap, fmt);
		xvfdprintf(2, fmt, ap);
		va_end(ap);
		xerrprintf(": ");
	}
	xerrprintf("%s\n", dos_strerror(e));
	DOS_EXIT2(eval);
}

__dead void
#ifdef __STDC__
xerrx(int eval, const char *fmt, ...)
#else
xerrx(eval, fmt, va_alist)
	int eval;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;

	xerrprintf("%s: ", __progname);
	if (fmt) {
		VA_START(ap, fmt);
		xvfdprintf(2, fmt, ap);
		va_end(ap);
	}
	xerrprintf("\n");
	DOS_EXIT2(eval);
}

void
#ifdef __STDC__
xwarn(const char *fmt, ...)
#else
xwarn(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	int e = dos_errno;
	va_list ap;

	xerrprintf("%s: ", __progname);
	if (fmt) {
		VA_START(ap, fmt);
		xvfdprintf(2, fmt, ap);
		va_end(ap);
		xerrprintf(": ");
	}
	xerrprintf("%s\n", dos_strerror(e));
}

void
#ifdef __STDC__
xwarnx(const char *fmt, ...)
#else
xwarnx(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;

	xerrprintf("%s: ", __progname);
	if (fmt) {
		VA_START(ap, fmt);
		xvfdprintf(2, fmt, ap);
		va_end(ap);
	}
	xerrprintf("\n");
}
