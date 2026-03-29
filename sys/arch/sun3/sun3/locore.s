/*	$NetBSD: locore.s,v 1.127 2026/03/29 03:24:58 thorpej Exp $	*/

/*
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: Utah $Hdr: locore.s 1.66 92/12/22$
 *	@(#)locore.s	8.6 (Berkeley) 5/27/94
 */

/*
 * Copyright (c) 1994, 1995 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: Utah $Hdr: locore.s 1.66 92/12/22$
 *	@(#)locore.s	8.6 (Berkeley) 5/27/94
 */

#include "opt_compat_netbsd.h"
#include "opt_compat_sunos.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"

#include "assym.h"
#include <machine/asm.h>
#include <machine/trap.h>

| Remember this is a fun project!

| This is for kvm_mkdb, and should be the address of the beginning
| of the kernel text segment (not necessarily the same as kernbase).
	.text
GLOBAL(kernel_text)

| This is the entry point, as well as the end of the temporary stack
| used during process switch (one 8K page ending at start)
ASGLOBAL(tmpstk)
ASGLOBAL(start)

| First we need to set it up so we can access the sun MMU, and be otherwise
| undisturbed.  Until otherwise noted, all code must be position independent
| as the boot loader put us low in memory, but we are linked high.
	movw	#PSL_HIGHIPL,%sr	| no interrupts
	moveq	#FC_CONTROL,%d0		| make movs access "control"
	movc	%d0,%sfc		| space where the sun3 designers
	movc	%d0,%dfc		| put all the "useful" stuff

| Set context zero and stay there until pmap_bootstrap.
	moveq	#0,%d0
	movsb	%d0,CONTEXT_REG

| In order to "move" the kernel to high memory, we are going to copy the
| first 4 Mb of pmegs such that we will be mapped at the linked address.
| This is all done by copying in the segment map (top-level MMU table).
| We will unscramble which PMEGs we actually need later.

	movl	#(SEGMAP_BASE+0),%a0		| src
	movl	#(SEGMAP_BASE+KERNBASE3),%a1	| dst
	movl	#(0x400000/NBSG),%d0		| count

L_per_pmeg:
	movsb	%a0@,%d1		| copy segmap entry
	movsb	%d1,%a1@
	addl	#NBSG,%a0		| increment pointers
	addl	#NBSG,%a1
	subql	#1,%d0			| decrement count
	bgt	L_per_pmeg

| Kernel is now double mapped at zero and KERNBASE.
| Force a long jump to the relocated code (high VA).
	movl	#IC_CLEAR,%d0		| Flush the I-cache
	movc	%d0,%cacr
	jmp	L_high_code:l		| long jump

L_high_code:
| We are now running in the correctly relocated kernel, so
| we are no longer restricted to position-independent code.

| Do bootstrap stuff needed before main() gets called.
| Make sure the initial frame pointer is zero so that
| the backtrace algorithm used by KGDB terminates nicely.
|
| _bootstrap() returns lwp0 SP in %a0
	lea	_ASM_LABEL(tmpstk),%sp
	jsr	_C_LABEL(_bootstrap)	| See locore2.c
	movl	%a0,%sp			| now running on lwp0's stack
	movl	#0,%a6			| terminate the stack back trace

	jra	_C_LABEL(main)		| main() (never returns)

| That is all the assembly startup code we need on the sun3!
| The rest of this is like the hp300/locore.s where possible.

/*
 * Other exceptions only cause four and six word stack frame and require
 * no post-trap stack adjustment.
 */

/*
 * Interrupt handlers.  Most are auto-vectored,
 * and hard-wired the same way on all sun3 models.
 * Format in the stack is:
 *   %d0,%d1,%a0,%a1, sr, pc, vo
 */

/* clock: see clock.c */
#ifdef __ELF__
	.align	4
#else
	.align	2
#endif
GLOBAL(_isr_clock)
	INTERRUPT_SAVEREG
	jbsr	_C_LABEL(clock_intr)
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)

/*
 * Initialization is at the beginning of this file, because the
 * kernel entry point needs to be at zero for compatibility with
 * the Sun boot loader.  This works on Sun machines because the
 * interrupt vector table for reset is NOT at address zero.
 * (The MMU has a "boot" bit that forces access to the PROM)
 */

/* TBIA, TBIS, TBIAS, TBIAU */

/*
 * Invalidate instruction cache
 */
ENTRY(ICIA)
	movl	#IC_CLEAR,%d0
	movc	%d0,%cacr		| invalidate i-cache
	rts

/* DCIA, DCIS */

/*
 * Invalidate data cache.
 */
ENTRY(DCIU)
	rts

/* ICPL, ICPP, DCPL, DCPP, DCPA, DCFL, DCFP */
/* PCIA */

/* loadustp, ptest_addr */

/*
 * void set_segmap_allctx(vaddr_t va, int sme)
 */
ENTRY(set_segmap_allctx)
	linkw	%fp,#0
	moveml	#0x3000,%sp@-
	movl	8(%fp),%d3		| d3 = va
	andl	#0xffffffc,%d3
	bset	#29,%d3
	movl	%d3,%a1			| a1 = ctrladdr, d3 avail
	movl	12(%fp),%d1		| d1 = sme
	moveq	#FC_CONTROL,%d0
	movl	#CONTEXT_REG,%a0	| a0 = ctxreg
	movc	%sfc,%d3		| d3 = oldsfc
	movc	%d0,%sfc
	movsb	%a0@,%d2
	andi	#7,%d2			| d2 = oldctx
	movc	%d3,%sfc		| restore sfc, d3 avail
	movc	%dfc,%d3		| d3 = olddfc
	movc	%d0,%dfc
	movl	#(CONTEXT_NUM - 1),%d0	| d0 = ctx number
1:
	movsb	%d0,%a0@		| change to ctx
	movsb	%d1,%a1@		| set segmap
	dbf	%d0,1b			| loop setting each ctx
	movsb	%d2,%a0@		| restore ctx
	movc	%d3,%dfc		| restore dfc
	moveml	%sp@+,#0x000c
	unlk	%fp
	rts

| Define some addresses, mostly so DDB can print useful info.
| Not using _C_LABEL() here because these symbols are never
| referenced by any C code, and if the leading underscore
| ever goes away, these lines turn into syntax errors...
	.set	_KERNBASE3,KERNBASE3
	.set	_MONSTART,SUN3_MONSTART
	.set	_PROM_BASE,SUN3_PROM_BASE
	.set	_MONEND,SUN3_MONEND

|The end!
