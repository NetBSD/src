/*	$NetBSD: nullcom.c,v 1.2 2009/01/12 07:45:24 tsutsui Exp $	*/

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
