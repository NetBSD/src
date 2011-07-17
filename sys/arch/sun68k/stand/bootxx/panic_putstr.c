/*	$NetBSD: panic_putstr.c,v 1.3 2011/07/17 20:54:48 joerg Exp $	*/


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
