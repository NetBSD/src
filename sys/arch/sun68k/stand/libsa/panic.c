/*	$NetBSD: panic.c,v 1.3 2002/06/03 00:18:27 fredette Exp $	*/


#include <machine/stdarg.h>
#include <stand.h>
#include "libsa.h"

__dead void
panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	putchar('\n');
	va_end(ap);
	breakpoint();
	exit(0);
}
