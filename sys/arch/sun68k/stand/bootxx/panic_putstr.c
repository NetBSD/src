/*	$NetBSD: panic_putstr.c,v 1.1.2.1 2002/07/15 01:22:22 gehenna Exp $	*/


#include <machine/stdarg.h>
#include <stand.h>
#include "libsa.h"

__dead void
panic(const char *fmt, ...)
{

	putstr("panic: ");
	putstr(fmt);
	putchar('\n');

	breakpoint();
	exit(0);
}
