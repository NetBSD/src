/*	$NetBSD: panic.c,v 1.1 2001/06/14 12:57:15 fredette Exp $	*/


#include <machine/stdarg.h>
#include <stand.h>
#include "libsa.h"

__dead void
panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
	breakpoint();
	exit();
}
