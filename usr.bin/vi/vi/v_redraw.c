/*	$NetBSD: v_redraw.c,v 1.8 2002/04/09 01:47:35 thorpej Exp $	*/

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
static const char sccsid[] = "@(#)v_redraw.c	10.6 (Berkeley) 3/6/96";
#else
__RCSID("$NetBSD: v_redraw.c,v 1.8 2002/04/09 01:47:35 thorpej Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <bitstring.h>
#include <limits.h>
#include <stdio.h>

#include "../common/common.h"
#include "vi.h"

/*
 * v_redraw -- ^L, ^R
 *	Redraw the screen.
 *
 * PUBLIC: int v_redraw __P((SCR *, VICMD *));
 */
int
v_redraw(sp, vp)
	SCR *sp;
	VICMD *vp;
{
	return (sp->gp->scr_refresh(sp, 1));
}
