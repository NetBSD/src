/*	$NetBSD: sigprocmask.S,v 1.3 1997/05/08 13:39:08 matthias Exp $	*/

/* 
 * Copyright (c) 1992 Helsinki University of Technology
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * HELSINKI UNIVERSITY OF TECHNOLOGY ALLOWS FREE USE OF THIS SOFTWARE IN
 * ITS "AS IS" CONDITION. HELSINKI UNIVERSITY OF TECHNOLOGY DISCLAIMS ANY
 * LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE
 * USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * 29-Apr-92  Johannes Helander (jvh) at Helsinki University of Technology
 *	Created.
 */

#include "SYS.h"

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: sigprocmask.S,v 1.3 1997/05/08 13:39:08 matthias Exp $")
#endif

ENTRY(sigprocmask)
	movd	S_ARG1,r1		/* fetch new sigset pointer */
	cmpqd	0,r1			/* check new sigset pointer */
	bne	0f			/* if not null, indirect */
/*	movqd	0,S_ARG1		/* null mask pointer: block empty set */
	movqd	1,S_ARG0		/* SIG_BLOCK */
	br	1f
0:	movd	0(r1),S_ARG1		/* fetch indirect to new mask arg */
1:	SYSTRAP(sigprocmask)
	bcs	_ASM_LABEL(cerror)
	movd	S_ARG2,r1		/* fetch old mask requested */
	cmpqd	0,r1			/* test if old mask requested */
	beq	2f
	movd	r0,0(r1)		/* store old mask */
2:	movqd	0,r0
	ret	0
