/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1989 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "from: @(#)printf.c	5.9 (Berkeley) 6/1/90";*/
static char rcsid[] = "$Id: printf.c,v 1.4 1993/11/05 20:12:40 jtc Exp $";
#endif /* not lint */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <locale.h>
#include <err.h>

void	do_printf();
void	print_escape_str();
int	print_escape();

int	getchr();
int	getint();
long	getlong();
double	getdouble();
char   *getstr();
     
int	rval;
char  **gargv;

#define isodigit(c)	((c) >= '0' && (c) <= '7')
#define octtobin(c)	((c) - '0')
#define hextobin(c)	((c) >= 'A' && 'c' <= 'F' ? c - 'A' + 10 : (c) >= 'a' && (c) <= 'f' ? c - 'a' + 10 : c - '0')

#define PF(f, func) { \
	if (fieldwidth) \
		if (precision) \
			(void)printf(f, fieldwidth, precision, func); \
		else \
			(void)printf(f, fieldwidth, func); \
	else if (precision) \
		(void)printf(f, precision, func); \
	else \
		(void)printf(f, func); \
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	char *format;

	setlocale (LC_ALL, "");

	if (argc < 2) {
		fprintf(stderr, "usage: printf format [arg ...]\n");
		exit(1);
	}

	format = argv[1];
	gargv  = argv + 2;

	do {
		do_printf(format);
	} while (*gargv);

	exit (rval);
}

void
do_printf(format)
	char *format;
{
	static char *skip1, *skip2;
	register char *fmt, *start;
	register int fieldwidth, precision;
	char convch, nextch, *mklong();

	/*
	 * Basic algorithm is to scan the format string for conversion
	 * specifications -- once one is found, find out if the field
	 * width or precision is a '*'; if it is, gather up value.  Note,
	 * format strings are reused as necessary to use up the provided
	 * arguments, arguments of zero/null string are provided to use
	 * up the format string.
	 */
	skip1 = "#-+ 0";
	skip2 = "*0123456789";

	/* find next format specification */
	for (fmt = format; *fmt; fmt++) {
		switch (*fmt) {
		case '%':
			start = fmt++;

			if (*fmt == '%') {
				putchar ('%');
				break;
			} else if (*fmt == 'b') {
				char *p = getstr();
				print_escape_str(p);
				break;
			}

			/* skip to field width */
			for (; index(skip1, *fmt); ++fmt);
			fieldwidth = *fmt == '*' ? getint() : 0;

			/* skip to possible '.', get following precision */
			for (; index(skip2, *fmt); ++fmt);
			if (*fmt == '.')
				++fmt;
			precision = *fmt == '*' ? getint() : 0;

			for (; index(skip2, *fmt); ++fmt);
			if (!*fmt) {
			        warnx ("missing format character");
				rval = 1;
				return;
			}

			convch = *fmt;
			nextch = *(fmt + 1);
			*(fmt + 1) = '\0';
			switch(convch) {
			case 'c': {
				char p = getchr();
				PF(start, p);
				break;
			}
			case 's': {
				char *p = getstr();
				PF(start, p);
				break;
			}
			case 'd': case 'i': case 'o': case 'u': case 'x': case 'X': {
				char *f = mklong(start, convch);
				long p = getlong();
				PF(f, p);
				free(f);
				break;
			}
			case 'e': case 'E': case 'f': case 'g': case 'G': {
				double p = getdouble();
				PF(start, p);
				break;
			}
			default:
			        warnx ("%s: invalid directive", start);
			        rval = 1;
			}
			*(fmt + 1) = nextch;
			break;

		case '\\':
			fmt += print_escape(fmt);
			break;

		default:
			putchar (*fmt);
			break;
		}
	}
}


/*
 * Print SysV echo(1) style escape string 
 */
void
print_escape_str(str)
	register char *str;
{
	int value;
	int c;

	while (*str) {
		if (*str == '\\') {
			str++;
			/* 
			 * %b string octal constants are not like those in C.
			 * They start with a \0, and are followed by 0, 1, 2, 
			 * or 3 octal digits. 
			 */
			if (*str == '0') {
				str++;
				for (c = 3, value = 0; c-- && isodigit(*str); str++) {
					value <<= 3;
					value += octtobin(*str);
				}
				putchar (value);
				str--;
			} else if (*str == 'c') {
				exit (rval);
			} else {
				str--;			
				str += print_escape(str);
			}
		} else {
			putchar (*str);
		}
		str++;
	}
}

/*
 * Print "standard" escape characters 
 */
int
print_escape(str)
	register char *str;
{
	char *start = str;
	int c;
	int value;

	str++;

	switch (*str) {
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
		for (c = 3, value = 0; c-- && isodigit(*str); str++) {
			value <<= 3;
			value += octtobin(*str);
		}
		putchar(value);
		return str - start - 1;
		/* NOTREACHED */

	case 'x':
		str++;
		for (value = 0; isxdigit(*str); str++) {
			value <<= 4;
			value += hextobin(*str);
		}
		if (value > UCHAR_MAX) {
			warnx ("escape sequence out of range for character");
			rval = 1;
		}
		putchar (value);
		return str - start - 1;
		/* NOTREACHED */

	case '\\':			/* backslash */
		putchar('\\');
		break;

	case '\'':			/* single quote */
		putchar('\'');
		break;

	case '"':			/* double quote */
		putchar('"');
		break;

	case 'a':			/* alert */
#ifdef __STDC__
		putchar('\a');
#else
		putchar(007);
#endif
		break;

	case 'b':			/* backspace */
		putchar('\b');
		break;

	case 'e':			/* escape */
#ifdef __GNUC__
		putchar('\e');
#else
		putchar(033);
#endif
		break;

	case 'f':			/* form-feed */
		putchar('\f');
		break;

	case 'n':			/* newline */
		putchar('\n');
		break;

	case 'r':			/* carriage-return */
		putchar('\r');
		break;

	case 't':			/* tab */
		putchar('\t');
		break;

	case 'v':			/* vertical-tab */
		putchar('\v');
		break;

	default:
		putchar(*str);
		warnx("unknown escape sequence `\\%c'", *str);
		rval = 1;
	}

	return 1;
}


char *
mklong(str, ch)
	char *str, ch;
{
	int len;
	char *copy;

	len = strlen(str) + 2;
	if (!(copy = malloc((unsigned int)len))) {
	        err(1, NULL);
		/* NOTREACHED */
	}
	bcopy(str, copy, len - 3);
	copy[len - 3] = 'l';
	copy[len - 2] = ch;
	copy[len - 1] = '\0';
	return(copy);
}

int
getchr()
{
	if (!*gargv)
		return((int)'\0');
	return((int)**gargv++);
}

char *
getstr()
{
	if (!*gargv)
		return("");
	return(*gargv++);
}

static char *number = "+-.0123456789";
int
getint()
{
	if (!*gargv)
		return(0);

	if (index(number, **gargv))
		return(atoi(*gargv++));

	return 0;
}

long
getlong()
{
	long val;
	char *ep;

	if (!*gargv)
		return(0L);

	if (**gargv == '\"' || **gargv == '\'') {
		val = gargv[0][1];
		gargv++;
		return val;
	}

	val = strtol (*gargv, &ep, 0);
	if (*ep) {
		warnx ("incompletely converted argument: %s", *gargv);
		rval = 1;
	}

	gargv++;
	return val;
}

double
getdouble()
{
	double val;
	char *ep;

	if (!*gargv)
		return(0.0);

	if (**gargv == '\"' || **gargv == '\'') {
		val = gargv[0][1];
		gargv++;
		return val;
	}

	val = strtod (*gargv, &ep);
	if (*ep) {
		warnx ("incompletely converted argument: %s", *gargv);
		rval = 1;
	}

	gargv++;
	return val;
}
