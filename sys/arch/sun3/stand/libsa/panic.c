/*	$NetBSD: panic.c,v 1.6 2001/02/22 07:11:10 chs Exp $	*/


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
