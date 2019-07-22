/*	$NetBSD: printf.c,v 1.50 2019/07/22 17:34:31 kre Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if !defined(BUILTIN) && !defined(SHELL)
__COPYRIGHT("@(#) Copyright (c) 1989, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif
#endif

#ifndef lint
#if 0
static char sccsid[] = "@(#)printf.c	8.2 (Berkeley) 3/22/95";
#else
__RCSID("$NetBSD: printf.c,v 1.50 2019/07/22 17:34:31 kre Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __GNUC__
#define ESCAPE '\e'
#else
#define ESCAPE 033
#endif

static void	 conv_escape_str(char *, void (*)(int), int);
static char	*conv_escape(char *, char *, int);
static char	*conv_expand(const char *);
static char	 getchr(void);
static double	 getdouble(void);
static int	 getwidth(void);
static intmax_t	 getintmax(void);
static char	*getstr(void);
static char	*mklong(const char *, char);
static void      check_conversion(const char *, const char *);
static void	 usage(void);

static void	b_count(int);
static void	b_output(int);
static size_t	b_length;
static char	*b_fmt;

static int	rval;
static char  **gargv;

#ifdef BUILTIN		/* csh builtin */
#define main progprintf
#endif

#ifdef SHELL		/* sh (aka ash) builtin */
#define main printfcmd
#include "../../bin/sh/bltin/bltin.h"
#endif /* SHELL */

#define PF(f, func) { \
	if (fieldwidth != -1) { \
		if (precision != -1) \
			error = printf(f, fieldwidth, precision, func); \
		else \
			error = printf(f, fieldwidth, func); \
	} else if (precision != -1) \
		error = printf(f, precision, func); \
	else \
		error = printf(f, func); \
}

#define APF(cpp, f, func) { \
	if (fieldwidth != -1) { \
		if (precision != -1) \
			error = asprintf(cpp, f, fieldwidth, precision, func); \
		else \
			error = asprintf(cpp, f, fieldwidth, func); \
	} else if (precision != -1) \
		error = asprintf(cpp, f, precision, func); \
	else \
		error = asprintf(cpp, f, func); \
}

#ifdef main
int main(int, char *[]);
#endif

int
main(int argc, char *argv[])
{
	char *fmt, *start;
	int fieldwidth, precision;
	char nextch;
	char *format;
	char ch;
	int error;

#if !defined(SHELL) && !defined(BUILTIN)
	(void)setlocale (LC_ALL, "");
#endif

	rval = 0;	/* clear for builtin versions (avoid holdover) */

	/*
	 * printf does not comply with Posix XBD 12.2 - there are no opts,
	 * not even the -- end of options marker.   Do not run getoot().
	 */
	if (argc > 2 && strchr(argv[1], '%') == NULL) {
		int o;

		/*
		 * except that if there are multiple args and
		 * the first (the nominal format) contains no '%'
		 * conversions (which we will approximate as no '%'
		 * characters at all, conversions or not) then the
		 * results are unspecified, and we can do what we
		 * like.   So in that case, for some backward compat
		 * to scripts which (stupidly) do:
		 *	printf -- format args
		 * process this case the old way.
		 */

		while ((o = getopt(argc, argv, "")) != -1) {
			switch (o) {
			case '?':
			default:
				usage();
				return 1;
			}
		}
		argc -= optind;
		argv += optind;
	} else {
		argc -= 1;	/* drop argv[0] (the program name) */
		argv += 1;
	}

	if (argc < 1) {
		usage();
		return 1;
	}

	format = *argv;
	gargv = ++argv;

#define SKIP1	"#-+ 0'"
#define SKIP2	"0123456789"
	do {
		/*
		 * Basic algorithm is to scan the format string for conversion
		 * specifications -- once one is found, find out if the field
		 * width or precision is a '*'; if it is, gather up value.
		 * Note, format strings are reused as necessary to use up the
		 * provided arguments, arguments of zero/null string are
		 * provided to use up the format string.
		 */

		/* find next format specification */
		for (fmt = format; (ch = *fmt++) != '\0';) {
			if (ch == '\\') {
				char c_ch;
				fmt = conv_escape(fmt, &c_ch, 0);
				putchar(c_ch);
				continue;
			}
			if (ch != '%' || (*fmt == '%' && ++fmt)) {
				(void)putchar(ch);
				continue;
			}

			/*
			 * Ok - we've found a format specification,
			 * Save its address for a later printf().
			 */
			start = fmt - 1;

			/* skip to field width */
			fmt += strspn(fmt, SKIP1);
			if (*fmt == '*') {
				fmt++;
				fieldwidth = getwidth();
			} else {
				fieldwidth = -1;

				/* skip to possible '.' for precision */
				fmt += strspn(fmt, SKIP2);
			}

			if (*fmt == '.') {
				 /* get following precision */
				fmt++;
				if (*fmt == '*') {
					fmt++;
					precision = getwidth();
				} else {
					precision = -1;
					fmt += strspn(fmt, SKIP2);
				}
			} else
				precision = -1;

			ch = *fmt;
			if (!ch) {
				warnx("%s: missing format character", start);
				return 1;
			}

			/*
			 * null terminate format string to we can use it
			 * as an argument to printf.
			 */
			nextch = fmt[1];
			fmt[1] = 0;

			switch (ch) {

			case 'B': {
				const char *p = conv_expand(getstr());

				if (p == NULL)
					goto out;
				*fmt = 's';
				PF(start, p);
				if (error < 0)
					goto out;
				break;
			}
			case 'b': {
				/*
				 * There has to be a better way to do this,
				 * but the string we generate might have
				 * embedded nulls
				 */
				static char *a, *t;
				char *cp = getstr();

				/* Free on entry in case shell longjumped out */
				if (a != NULL)
					free(a);
				a = NULL;
				if (t != NULL)
					free(t);
				t = NULL;

				/* Count number of bytes we want to output */
				b_length = 0;
				conv_escape_str(cp, b_count, 0);
				t = malloc(b_length + 1);
				if (t == NULL)
					goto out;
				(void)memset(t, 'x', b_length);
				t[b_length] = 0;

				/* Get printf to calculate the lengths */
				*fmt = 's';
				APF(&a, start, t);
				if (error == -1)
					goto out;
				b_fmt = a;

				/* Output leading spaces and data bytes */
				conv_escape_str(cp, b_output, 1);

				/* Add any trailing spaces */
				printf("%s", b_fmt);
				break;
			}
			case 'c': {
				char p = getchr();

				PF(start, p);
				if (error < 0)
					goto out;
				break;
			}
			case 's': {
				char *p = getstr();

				PF(start, p);
				if (error < 0)
					goto out;
				break;
			}
			case 'd':
			case 'i': {
				intmax_t p = getintmax();
				char *f = mklong(start, ch);

				PF(f, p);
				if (error < 0)
					goto out;
				break;
			}
			case 'o':
			case 'u':
			case 'x':
			case 'X': {
				uintmax_t p = (uintmax_t)getintmax();
				char *f = mklong(start, ch);

				PF(f, p);
				if (error < 0)
					goto out;
				break;
			}
			case 'a':
			case 'A':
			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G': {
				double p = getdouble();

				PF(start, p);
				if (error < 0)
					goto out;
				break;
			}
			case '%':
				/* Don't ask, but this is useful ... */
				if (fieldwidth == 'N' && precision == 'B')
					return 0;
				/* FALLTHROUGH */
			default:
				warnx("%s: invalid directive", start);
				return 1;
			}
			*fmt++ = ch;
			*fmt = nextch;
			/* escape if a \c was encountered */
			if (rval & 0x100)
				return rval & ~0x100;
		}
	} while (gargv != argv && *gargv);

	return rval & ~0x100;
  out:
	warn("print failed");
	return 1;
}

/* helper functions for conv_escape_str */

static void
/*ARGSUSED*/
b_count(int ch)
{
	b_length++;
}

/* Output one converted character for every 'x' in the 'format' */

static void
b_output(int ch)
{
	for (;;) {
		switch (*b_fmt++) {
		case 0:
			b_fmt--;
			return;
		case ' ':
			putchar(' ');
			break;
		default:
			putchar(ch);
			return;
		}
	}
}


/*
 * Print SysV echo(1) style escape string
 *	Halts processing string if a \c escape is encountered.
 */
static void
conv_escape_str(char *str, void (*do_putchar)(int), int quiet)
{
	int value;
	int ch;
	char c;

	while ((ch = *str++) != '\0') {
		if (ch != '\\') {
			do_putchar(ch);
			continue;
		}

		ch = *str++;
		if (ch == 'c') {
			/* \c as in SYSV echo - abort all processing.... */
			rval |= 0x100;
			break;
		}

		/*
		 * %b string octal constants are not like those in C.
		 * They start with a \0, and are followed by 0, 1, 2,
		 * or 3 octal digits.
		 */
		if (ch == '0') {
			int octnum = 0, i;
			for (i = 0; i < 3; i++) {
				if (!isdigit((unsigned char)*str) || *str > '7')
					break;
				octnum = (octnum << 3) | (*str++ - '0');
			}
			do_putchar(octnum);
			continue;
		}

		/* \[M][^|-]C as defined by vis(3) */
		if (ch == 'M' && *str == '-') {
			do_putchar(0200 | str[1]);
			str += 2;
			continue;
		}
		if (ch == 'M' && *str == '^') {
			str++;
			value = 0200;
			ch = '^';
		} else
			value = 0;
		if (ch == '^') {
			ch = *str++;
			if (ch == '?')
				value |= 0177;
			else
				value |= ch & 037;
			do_putchar(value);
			continue;
		}

		/* Finally test for sequences valid in the format string */
		str = conv_escape(str - 1, &c, quiet);
		do_putchar(c);
	}
}

/*
 * Print "standard" escape characters
 */
static char *
conv_escape(char *str, char *conv_ch, int quiet)
{
	char value;
	char ch;
	char num_buf[4], *num_end;

	ch = *str++;

	switch (ch) {
	case '\0':
		if (!quiet)
			warnx("incomplete escape sequence");
		rval = 1;
		value = '\\';
		--str;
		break;

	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
		num_buf[0] = ch;
		ch = str[0];
		num_buf[1] = ch;
		num_buf[2] = (char)(ch != '\0' ? str[1] : '\0');
		num_buf[3] = '\0';
		value = (char)strtoul(num_buf, &num_end, 8);
		str += num_end  - (num_buf + 1);
		break;

	case 'x':
		/*
		 * Hexadecimal character constants are not required to be
		 * supported (by SuS v1) because there is no consistent
		 * way to detect the end of the constant.
		 * Supporting 2 byte constants is a compromise.
		 */
		ch = str[0];
		num_buf[0] = ch;
		num_buf[1] = (char)(ch != '\0' ? str[1] : '\0');
		num_buf[2] = '\0';
		value = (char)strtoul(num_buf, &num_end, 16);
		str += num_end - num_buf;
		break;

	case '\\':	value = '\\';	break;	/* backslash */
	case '\'':	value = '\'';	break;	/* single quote */
	case '"':	value = '"';	break;	/* double quote */
	case 'a':	value = '\a';	break;	/* alert */
	case 'b':	value = '\b';	break;	/* backspace */
	case 'e':	value = ESCAPE;	break;	/* escape */
	case 'E':	value = ESCAPE;	break;	/* escape */
	case 'f':	value = '\f';	break;	/* form-feed */
	case 'n':	value = '\n';	break;	/* newline */
	case 'r':	value = '\r';	break;	/* carriage-return */
	case 't':	value = '\t';	break;	/* tab */
	case 'v':	value = '\v';	break;	/* vertical-tab */

	default:
		if (!quiet)
			warnx("unknown escape sequence `\\%c'", ch);
		rval = 1;
		value = ch;
		break;
	}

	*conv_ch = value;
	return str;
}

/* expand a string so that everything is printable */

static char *
conv_expand(const char *str)
{
	static char *conv_str;
	char *cp;
	char ch;

	if (conv_str)
		free(conv_str);
	/* get a buffer that is definitely large enough.... */
	conv_str = malloc(4 * strlen(str) + 1);
	if (!conv_str)
		return NULL;
	cp = conv_str;

	while ((ch = *(const char *)str++) != '\0') {
		switch (ch) {
		/* Use C escapes for expected control characters */
		case '\\':	ch = '\\';	break;	/* backslash */
		case '\'':	ch = '\'';	break;	/* single quote */
		case '"':	ch = '"';	break;	/* double quote */
		case '\a':	ch = 'a';	break;	/* alert */
		case '\b':	ch = 'b';	break;	/* backspace */
		case ESCAPE:	ch = 'e';	break;	/* escape */
		case '\f':	ch = 'f';	break;	/* form-feed */
		case '\n':	ch = 'n';	break;	/* newline */
		case '\r':	ch = 'r';	break;	/* carriage-return */
		case '\t':	ch = 't';	break;	/* tab */
		case '\v':	ch = 'v';	break;	/* vertical-tab */
		default:
			/* Copy anything printable */
			if (isprint((unsigned char)ch)) {
				*cp++ = ch;
				continue;
			}
			/* Use vis(3) encodings for the rest */
			*cp++ = '\\';
			if (ch & 0200) {
				*cp++ = 'M';
				ch &= (char)~0200;
			}
			if (ch == 0177) {
				*cp++ = '^';
				*cp++ = '?';
				continue;
			}
			if (ch < 040) {
				*cp++ = '^';
				*cp++ = ch | 0100;
				continue;
			}
			*cp++ = '-';
			*cp++ = ch;
			continue;
		}
		*cp++ = '\\';
		*cp++ = ch;
	}

	*cp = 0;
	return conv_str;
}

static char *
mklong(const char *str, char ch)
{
	static char copy[64];
	size_t len;	

	len = strlen(str) + 2;
	if (len > sizeof copy) {
		warnx("format %s too complex", str);
		len = 4;
	}
	(void)memmove(copy, str, len - 3);
	copy[len - 3] = 'j';
	copy[len - 2] = ch;
	copy[len - 1] = '\0';
	return copy;	
}

static char
getchr(void)
{
	if (!*gargv)
		return 0;
	return **gargv++;
}

static char *
getstr(void)
{
	static char empty[] = "";
	if (!*gargv)
		return empty;
	return *gargv++;
}

static int
getwidth(void)
{
	unsigned long val;
	char *s, *ep;

	s = *gargv;
	if (s == NULL)
		return 0;
	gargv++;

	errno = 0;
	val = strtoul(s, &ep, 0);
	check_conversion(s, ep);

	/* Arbitrarily 'restrict' field widths to 1Mbyte */
	if (val > 1 << 20) {
		warnx("%s: invalid field width", s);
		return 0;
	}

	return (int)val;
}

static intmax_t
getintmax(void)
{
	intmax_t val;
	char *cp, *ep;

	cp = *gargv;
	if (cp == NULL)
		return 0;
	gargv++;

	if (*cp == '\"' || *cp == '\'')
		return *(cp + 1);

	errno = 0;
	val = strtoimax(cp, &ep, 0);
	check_conversion(cp, ep);
	return val;
}

static double
getdouble(void)
{
	double val;
	char *ep;

	if (!*gargv)
		return 0.0;

	if (**gargv == '\"' || **gargv == '\'')
		return (double) *((*gargv++)+1);

	errno = 0;
	val = strtod(*gargv, &ep);
	check_conversion(*gargv++, ep);
	return val;
}

static void
check_conversion(const char *s, const char *ep)
{
	if (*ep) {
		if (ep == s)
			warnx("%s: expected numeric value", s);
		else
			warnx("%s: not completely converted", s);
		rval = 1;
	} else if (errno == ERANGE) {
		warnx("%s: %s", s, strerror(ERANGE));
		rval = 1;
	}
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s format [arg ...]\n", getprogname());
}
