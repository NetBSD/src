/*	$NetBSD: hack.version.c,v 1.4 1997/10/19 16:59:25 christos Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: hack.version.c,v 1.4 1997/10/19 16:59:25 christos Exp $");
#endif				/* not lint */

#include "date.h"
#include "hack.h"
#include "extern.h"

int
doversion()
{
	pline("%s 1.0.3 - last edit %s.", (
#ifdef QUEST
					   "Quest"
#else
					   "Hack"
#endif	/* QUEST */
					   ), datestring);
	return (0);
}
