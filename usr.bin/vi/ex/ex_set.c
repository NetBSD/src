/*	$NetBSD: ex_set.c,v 1.8 2002/04/09 01:47:33 thorpej Exp $	*/

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
static const char sccsid[] = "@(#)ex_set.c	10.7 (Berkeley) 3/6/96";
#else
__RCSID("$NetBSD");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>

#include <bitstring.h>
#include <limits.h>
#include <stdio.h>

#include "../common/common.h"

/*
 * ex_set -- :set
 *	Ex set option.
 *
 * PUBLIC: int ex_set __P((SCR *, EXCMD *));
 */
int
ex_set(sp, cmdp)
	SCR *sp;
	EXCMD *cmdp;
{
	switch(cmdp->argc) {
	case 0:
		opts_dump(sp, CHANGED_DISPLAY);
		break;
	default:
		if (opts_set(sp, cmdp->argv, cmdp->cmd->usage))
			return (1);
		break;
	}
	return (0);
}
