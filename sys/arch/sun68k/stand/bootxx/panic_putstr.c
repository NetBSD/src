/*	$NetBSD: panic_putstr.c,v 1.1 2002/05/15 04:07:42 lukem Exp $	*/


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
	exit();
}
