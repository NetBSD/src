/*	$NetBSD: nap.c,v 1.6 2012/06/19 05:30:43 dholland Exp $	*/

/* nap.c		 Larn is copyrighted 1986 by Noah Morgan. */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: nap.c,v 1.6 2012/06/19 05:30:43 dholland Exp $");
#endif				/* not lint */

#include <unistd.h>
#include "header.h"
#include "extern.h"

/*
 *	routine to take a nap for n milliseconds
 */
void
nap(int x)
{
	if (x <= 0)
		return;		/* eliminate chance for infinite loop */
	lflush();
	usleep(x * 1000);
}
