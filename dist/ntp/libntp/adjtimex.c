/*	$NetBSD: adjtimex.c,v 1.2 2002/05/16 19:53:37 wiz Exp $	*/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
_sccsid:.asciz  "11/19/91       ULTRIX  @(#)adjtime.c   6.1"
#endif /* not lint */

#include "SYS.h"

SYSCALL(adjtimex)
        ret

