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
 *
 *	$Id: sbrk.S,v 1.1 1993/10/07 00:21:01 cgd Exp $
 */

#include <sys/syscall.h>
#include "SYS.h"

	.globl	_end
	.globl	_curbrk
	.globl	_minbrk

	.data
_curbrk:	.long	_end
_minbrk:	.long	_end
	.text

ENTRY(sbrk)
	addd	_curbrk(pc), S_ARG0
	movd	SYS_break,r0
	SVC
	bcs	cerror
	movd	_curbrk(pc), r0
	movd	S_ARG0,_curbrk(pc)		/* XXX check this b*/
	ret	0
