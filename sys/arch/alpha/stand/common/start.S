/* $NetBSD: start.S,v 1.9 1999/03/31 16:12:52 cgd Exp $ */

/*  
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include "include/asm.h"

/*
 * start --
 *	Entry point for boot/standalone programs.
 *
 * Arguments:
 *	a0				long (first free physical page)
 *
 * This is where the prom comes to.  Leaves all exception and interrupts
 * to prom, runs off prom's stack too.  No return values.
 */
	.text
	.set	noreorder		/* don't reorder instructions */

#define ENTRY_FRAME	32

NESTED(start, 1, ENTRY_FRAME, ra, 0, 0)
	br	pv,Lstartgp
Lstartgp:
	LDGP(pv)

#if !defined(PRIMARY_BOOTBLOCK) && !defined(UNIFIED_BOOTBLOCK)
	lda	sp,start		/* start stack below text */
	lda	sp,-ENTRY_FRAME(sp)

	or	a0,zero,s0
#endif /* !defined(PRIMARY_BOOTBLOCK) && !defined(UNIFIED_BOOTBLOCK) */
	lda	a0,_edata
	lda	a1,_end
	subq	a1,a0,a1
	CALL(bzero)
#if !defined(PRIMARY_BOOTBLOCK) && !defined(UNIFIED_BOOTBLOCK)
	or	s0,zero,a0
#endif /* !defined(PRIMARY_BOOTBLOCK) && !defined(UNIFIED_BOOTBLOCK) */

	CALL(main)			/* transfer to C */

XLEAF(_rtt, 0)
XLEAF(halt, 0)
	call_pal PAL_halt		/* halt if we ever return */
END(start)
