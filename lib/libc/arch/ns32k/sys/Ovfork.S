/*	$NetBSD: Ovfork.S,v 1.3 1997/05/08 13:39:01 matthias Exp $	*/

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
	RCSID("$NetBSD: Ovfork.S,v 1.3 1997/05/08 13:39:01 matthias Exp $")
#endif

/* 
 * r0 = pid of child in parent / pid of parent in child
 * r1 = 0 in parent, 1 in child
 *
 * trickery here, due to keith sklower, uses ret to clear the stack,
 * and then returns with a jump indirect, since only one person can return
 * with a ret off this stack... we do the ret before we vfork!
 * (This is not really true for NetBSD. NetBSD's vfork creates a new
 * vm context for the child).
 */
ENTRY(vfork)
	movd	tos,r2
	SYSTRAP(vfork)
	bcc	0f
	movd	r2,tos
	br	_ASM_LABEL(cerror)
0:	cmpqd	0,r1
	beq	0f
	movqd	0,r0
0:	jump	0(r2)
