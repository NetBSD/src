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
 * 11-May-92  Johannes Helander (jvh) at Helsinki University of Technology
 *	Created.
 *
 *	$Id: brk.s,v 1.1 1993/10/07 00:20:51 cgd Exp $
 */

#include <sys/syscall.h>
#include "SYS.h"

	.globl	_curbrk
	.globl	_minbrk

ENTRY(_brk)
	br	ok

ENTRY(brk)
	cmpd	S_ARG0, _minbrk(pc)
	bge	ok
	movd	_minbrk(pc), S_ARG0
ok:
	movd	SYS_break,r0
	SVC
	bcs	cerror
	movd	S_ARG0, _curbrk(pc)
	movqd	0, r0
	ret	0
