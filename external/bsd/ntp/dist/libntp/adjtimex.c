/*	$NetBSD: adjtimex.c,v 1.1.1.1.26.1 2014/12/24 00:05:20 riz Exp $	*/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
_sccsid:.asciz  "11/19/91       ULTRIX  @(#)adjtime.c   6.1"
#endif not lint

#include "SYS.h"

SYSCALL(adjtimex)
        ret

