/*	$NetBSD: panic.c,v 1.4.2.1 1998/01/27 02:35:34 gwr Exp $	*/


#include <stdarg.h>
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
