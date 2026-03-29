/*	$NetBSD: locore.s,v 1.54 2026/03/29 13:41:37 thorpej Exp $	*/

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
 * Copyright (c) 2001 Matthew Fredette
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
| used during process switch (two 2K pages ending at start)
ASGLOBAL(tmpstk)
ASGLOBAL(start)

| As opposed to the sun3, on the sun2 the kernel is linked low.  The
| boot loader loads us exactly where we are linked, so we don't have
| to worry about writing position independent code or moving the 
| kernel around.
	movw	#PSL_HIGHIPL,%sr	| no interrupts
	moveq	#FC_CONTROL,%d0		| make movs access "control"
	movc	%d0,%sfc		| space where the sun2 designers
	movc	%d0,%dfc		| put all the "useful" stuff

| Set context zero and stay there until pmap_bootstrap.
	moveq	#0,%d0
	movsb	%d0,CONTEXT_REG
	movsb	%d0,SCONTEXT_REG

| Jump around the g0 and g4 entry points.
	jra	L_high_code

| These entry points are here in pretty low memory, so that they
| can be reached from virtual address zero using the classic,
| old-school "g0" and "g4" commands from the monitor.  (I.e., 
| they need to be reachable using 16-bit displacements from PCs 
| 0 and 4).
ENTRY(g0_entry)
	jra	_C_LABEL(g0_handler)
ENTRY(g4_entry)
	jra	_C_LABEL(g4_handler)
	
L_high_code:
| We are now running in the correctly relocated kernel, so
| we are no longer restricted to position-independent code.

| Disable interrupts, and initialize the soft copy of the
| enable register.
	movsw	SYSTEM_ENAB, %d0	| read the enable register
	moveq	#ENA_INTS, %d1
	notw	%d1
	andw	%d1, %d0
	movsw	%d0, SYSTEM_ENAB	| disable all interrupts
	movw	%d0, _C_LABEL(enable_reg_soft)

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

/*
 * Set or clear bits in the enable register.  This mimics the
 * strange behavior in SunOS' locore.o, where they keep a soft
 * copy of what they think is in the enable register and loop
 * making a change until it sticks.  This is apparently to
 * be concurrent-safe without disabling interrupts.  Why you
 * can't just disable interrupts while mucking with the register
 * I dunno, but it may jive with sun3/intreg.c using the single-instruction
 * bit operations and the sun3 intreg being memory-addressable,
 * i.e., once the sun2 was designed they realized the enable
 * register had to be treated this way, so on the sun3 they made
 * it memory-addressable so you could just use the single-instructions.
 */
ENTRY(enable_reg_and)
	movc	%dfc,%a1		| save current dfc
	moveq	#FC_CONTROL, %d1
	movc	%d1, %dfc		| make movs access "control"
	movl	%sp@(4), %d1		| get our AND mask
	clrl	%d0
1:	andw	%d1, _C_LABEL(enable_reg_soft)	| do our AND
	movew	_C_LABEL(enable_reg_soft), %d0	| get the result
	movsw	%d0, SYSTEM_ENAB		| install the result
	cmpw	_C_LABEL(enable_reg_soft), %d0
	bne	1b				| install it again if the soft value changed
	movc	%a1,%dfc		| restore dfc
	rts

ENTRY(enable_reg_or)
	movc	%dfc,%a1		| save current dfc
	moveq	#FC_CONTROL, %d1
	movc	%d1, %dfc		| make movs access "control"
	movl	%sp@(4), %d1		| get our OR mask
	clrl	%d0
1:	orw	%d1, _C_LABEL(enable_reg_soft)	| do our OR
	movew	_C_LABEL(enable_reg_soft), %d0	| get the result
	movsw	%d0, SYSTEM_ENAB			| install the result
	cmpw	_C_LABEL(enable_reg_soft), %d0
	bne	1b				| install it again if the soft value changed
	movc	%a1,%dfc		| restore dfc
	rts

/*
 * Use common m68k 16-bit aligned copy routines.
 */
#include <m68k/m68k/w16copy.s>

| Define some addresses, mostly so DDB can print useful info.
| Not using _C_LABEL() here because these symbols are never
| referenced by any C code, and if the leading underscore
| ever goes away, these lines turn into syntax errors...
	.set	_KERNBASE,KERNBASE
	.set	_MONSTART,SUN2_MONSTART
	.set	_PROM_BASE,SUN2_PROM_BASE
	.set	_MONEND,SUN2_MONEND

|The end!
