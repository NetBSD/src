/*	$NetBSD: panic_putstr.c,v 1.2 2002/06/03 00:18:26 fredette Exp $	*/


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
