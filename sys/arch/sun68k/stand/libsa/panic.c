/*	$NetBSD: panic.c,v 1.1.8.2 2002/06/20 03:42:00 nathanw Exp $	*/


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
