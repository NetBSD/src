/*	$NetBSD: nap.c,v 1.5.68.1 2012/10/30 18:58:24 yamt Exp $	*/

/* nap.c		 Larn is copyrighted 1986 by Noah Morgan. */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: nap.c,v 1.5.68.1 2012/10/30 18:58:24 yamt Exp $");
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
