/*	$NetBSD: nullcom.c,v 1.2.8.2 2009/05/04 08:11:00 yamt Exp $	*/

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
