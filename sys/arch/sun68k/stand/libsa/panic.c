/*	$NetBSD: panic.c,v 1.4 2011/07/17 20:54:48 joerg Exp $	*/


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
