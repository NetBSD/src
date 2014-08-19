/*	$NetBSD: ex_put.c,v 1.3.8.2 2014/08/19 23:51:52 tls Exp $	*/
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
#if 0
#ifndef lint
static const char sccsid[] = "Id: ex_put.c,v 10.8 2001/06/25 15:19:18 skimo Exp  (Berkeley) Date: 2001/06/25 15:19:18 ";
#endif /* not lint */
#else
__RCSID("$NetBSD: ex_put.c,v 1.3.8.2 2014/08/19 23:51:52 tls Exp $");
#endif

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
ex_put(SCR *sp, EXCMD *cmdp)
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
