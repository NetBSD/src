/*	$NetBSD: panic.c,v 1.5.14.1 2001/03/12 13:29:39 bouyer Exp $	*/


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
