/*	$NetBSD: rnd.c,v 1.4 1997/10/19 16:59:39 christos Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: rnd.c,v 1.4 1997/10/19 16:59:39 christos Exp $");
#endif				/* not lint */

#include <stdlib.h>
#include "hack.h"
#include "extern.h"

#define RND(x)	((random()>>3) % x)

int
rn1(x, y)
	int             x, y;
{
	return (RND(x) + y);
}

int
rn2(x)
	int             x;
{
	return (RND(x));
}

int
rnd(x)
	int             x;
{
	return (RND(x) + 1);
}

int
d(n, x)
	int             n, x;
{
	int tmp = n;

	while (n--)
		tmp += RND(x);
	return (tmp);
}
