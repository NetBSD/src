/*	$NetBSD: ex_put.c,v 1.8 2002/04/09 01:47:33 thorpej Exp $	*/

/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1992, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 */

#include "config.h"

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char sccsid[] = "@(#)ex_put.c	10.7 (Berkeley) 3/6/96";
#else
__RCSID("$NetBSD");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>

#include <bitstring.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../common/common.h"

/*
 * ex_put -- [line] pu[t] [buffer]
 *	Append a cut buffer into the file.
 *
 * PUBLIC: int ex_put __P((SCR *, EXCMD *));
 */
int
ex_put(sp, cmdp)
	SCR *sp;
	EXCMD *cmdp;
{
	MARK m;

	NEEDFILE(sp, cmdp);

	m.lno = sp->lno;
	m.cno = sp->cno;
	if (put(sp, NULL,
	    FL_ISSET(cmdp->iflags, E_C_BUFFER) ? &cmdp->buffer : NULL,
	    &cmdp->addr1, &m, 1))
		return (1);
	sp->lno = m.lno;
	sp->cno = m.cno;
	return (0);
}
