/*	$NetBSD: panic.c,v 1.5 1998/02/05 04:57:13 gwr Exp $	*/


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
