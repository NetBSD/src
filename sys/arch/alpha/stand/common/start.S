/* $NetBSD: start.S,v 1.13 2003/08/29 12:58:39 dsl Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
 * Unified and Primary Bootstrap arguments:
 *	None.
 *
 * Secondary Bootstrap arguments:
 *	a0		first free physical page
 *
 * Standalone program arguments:
 *
 *	a0		first free physical page
 *	a1		page table base register (PTBR)
 *	a2		bootinfo magic number
 *	a3		pointer to the bootinfo structure
 *	a4		bootinfo version number
 *
 * All bootstrap and standalone programs leave exception and interrupt
 * handling to the firmware, and run from the firmware-provided stack.
 * To return to the firmware, these programs simply invoke the 'halt'
 * PALcode operation.
 *
 * Bootstrap programs have to clear their own BSS.  Standalone programs
 * have their BSS cleared by the bootstraps.
 */
	.text
	.set	noreorder		/* don't reorder instructions */

#define ENTRY_FRAME	32

NESTED(start, 1, ENTRY_FRAME, ra, 0, 0)
	br	pv,Lstartgp
Lstartgp:
	LDGP(pv)

#if defined(STANDALONE_PROGRAM)

	/*
	 * save the arguments, invoke init_prom_calls() to set up console
	 * callbacks and output, then restore the arguments.
	 */
	mov	a0, s0
	mov	a1, s1
	mov	a2, s2
	mov	a3, s3
	mov	a4, s4
	CALL(init_prom_calls)
	mov	s0, a0
	mov	s1, a1
	mov	s2, a2
	mov	s3, a3
	mov	s4, a4

#else /* defined(STANDALONE_PROGRAM) */

#if !defined(PRIMARY_BOOTBLOCK) && !defined(UNIFIED_BOOTBLOCK)
	lda	sp,start		/* start stack below text */
	lda	sp,-ENTRY_FRAME(sp)

	or	a0,zero,s0
#endif /* !defined(PRIMARY_BOOTBLOCK) && !defined(UNIFIED_BOOTBLOCK) */
	lda	a0,_edata
	xor	a1,a1,a1
	lda	a2,_end
	subq	a2,a0,a2
	CALL(memset)
#if !defined(PRIMARY_BOOTBLOCK) && !defined(UNIFIED_BOOTBLOCK)
	or	s0,zero,a0
#endif /* !defined(PRIMARY_BOOTBLOCK) && !defined(UNIFIED_BOOTBLOCK) */

#endif /* defined(STANDALONE_PROGRAM) */

	CALL(main)			/* transfer to C */

XLEAF(_rtt, 0)
XLEAF(halt, 0)
	call_pal PAL_halt		/* halt if we ever return */
END(start)
