/*	$NetBSD: argtest.c,v 1.1 1994/12/28 22:12:38 cgd Exp $	*/

/*
 * Test originally written by Chris Hopps.  Beefed up a bit, and converted
 * to test both stdarg.h and varargs.h by cgd.
 */

#if !defined(STDARG) && !defined(VARARGS)
#error	STDARG or VARARGS must be defined
#endif

#include <sys/cdefs.h>
#include <assert.h>
#include <stdio.h>
#ifdef STDARG
#include <stdarg.h>
#else
#include <varargs.h>
#endif

typedef struct {
	char c;
} Cstruct;

typedef struct {
	short s;
} Sstruct;

typedef struct {
	int i;
} Istruct;

typedef struct {
	int i;
	short s;
} Jstruct;

typedef struct {
	short s;
	int i;
} Lstruct;

#ifdef STDARG
int
tryit(const char *fmt, ...)
#else
int
tryit(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	int ctr = 0, i;
	va_list ap;

#ifdef STDARG
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	for (; *fmt; ++fmt)
		switch (*fmt) {
		case 'i':
			assert(va_arg(ap, int) == ++ctr);
			break;
		case 'd':
			assert(va_arg(ap, double) == ++ctr);
			break;
		case 'p':
			assert(va_arg(ap, char *)[0] == ++ctr);
			break;
		case 'C':
			assert(va_arg(ap, Cstruct).c == ++ctr);
			break;
		case 'S':
			assert(va_arg(ap, Sstruct).s == ++ctr);
			break;
		case 'I':
			assert(va_arg(ap, Istruct).i == ++ctr);
			break;
		case 'J':
			assert(va_arg(ap, Jstruct).i == ++ctr);
			break;
		case 'K':
			assert(va_arg(ap, Jstruct).s == ++ctr);
			break;
		case 'L':
			assert(va_arg(ap, Lstruct).s == ++ctr);
			break;
		case 'M':
			assert(va_arg(ap, Lstruct).i == ++ctr);
			break;
		}
	va_end(ap);
	return (ctr);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	Cstruct x = { 3 };
	Sstruct y = { 8 };
	Istruct z = { 11 };
	Jstruct j = { 4, 7 };
	Lstruct l = { 5, 9 };

	assert(tryit("iiCJLdKSMiI", '\1', 2, x, j, l, 6.0, j, y, l, 10, z)
	    == 11);
	assert(tryit("") == 0);
	assert(tryit("pdp", "\1", 2.0, "\3") == 3);
	exit(0);
}
