/*	$NetBSD: nullcom.c,v 1.2.2.2 2009/01/17 13:27:59 mjf Exp $	*/

/* 
 * nulldev-like console stubs for quiet gzboot
 */
#include <lib/libsa/stand.h>
#include "board.h"

void
cons_init(void)
{
}

int
getchar(void)
{
	return 0;	/* XXX */
}

void
putchar(int c)
{
}
