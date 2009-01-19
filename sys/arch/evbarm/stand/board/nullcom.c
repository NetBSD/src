/*	$NetBSD: nullcom.c,v 1.2.4.2 2009/01/19 13:16:07 skrll Exp $	*/

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
