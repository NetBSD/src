/*	$NetBSD: locore.s,v 1.24 1995/01/24 06:01:55 gwr Exp $	*/

/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * All rights reserved.
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "assym.s"
#include <machine/trap.h>
#include <machine/asm.h>
#include <sys/syscall.h>

| remember this is a fun project :)

| Define some addresses, mostly so DDB can print useful info.
.set	_kernbase,KERNBASE
.globl	_kernbase
.set	_dvma_base,DVMA_SPACE_START
.globl	_dvma_base

.set	_prom_start,MONSTART
.globl	_prom_start
.set	_prom_base,PROM_BASE
.globl	_prom_base


| This is where the UPAGES get mapped.
.set	_kstack,UADDR
.globl _kstack

| This is for kvm_mkdb, and should be the address of the beginning
| of the kernel text segment (not necessarily the same as kernbase).
	.text
	.globl	_kernel_text
_kernel_text:

| This is the entry point, as well as the end of the temporary stack
| used during process switch (one 8K page ending at start)
	.globl tmpstk
tmpstk:
	.globl start
	.globl _start
start: _start:
/*
 * First we need to set it up so we can access the sun MMU, and be otherwise
 * undisturbed.  Until otherwise noted, all code must be position independent
 * as the boot loader put us low in memory, but we are linked high.
 */
	movw #PSL_HIGHIPL, sr		| no interrupts
	moveq #FC_CONTROL, d0		| make movs get us to the control
	movc d0, dfc			| space where the sun3 designers
	movc d0, sfc			| put all the "useful" stuff
	moveq #CONTEXT_0, d0
	movsb d0, CONTEXT_REG		| now in context 0

/*
 * In order to "move" the kernel to high memory, we are going to copy the
 * first 4 Mb of pmegs such that we will be mapped at the linked address.
 * This is all done by playing with the segment map, and then propagating
 * it to the other contexts.
 * We will unscramble which pmegs we actually need later.
 *
 */

percontext:					|loop among the contexts
	clrl d1					
	movl #(SEGMAP_BASE+KERNBASE), a0	| base index into seg map

perpmeg:
	movsb d1, a0@				| establish mapping
	addql #1, d1			
	addl #NBSG, a0
	cmpl #(0x400000 / NBSG), d1		| up to 4MB yet?
	bne perpmeg

	addql #1, d0				| next context ....
	cmpl #CONTEXT_NUM, d0
	bne percontext

	clrl d0					
	movsb d0, CONTEXT_REG			| back to context 0
	
	jmp mapped_right:l			| things are now mapped right:)



mapped_right:

	movl #_edata, a0
	movl #_end, a1
bsszero: clrl a0@
	addql #4, a0
	cmpl a0, a1
	bne bsszero

fpu_setup:
	movsb SYSTEM_ENAB, d0			| read enable register
	orb #SYSTEM_ENAB_FPP, d0		| set fpu bit
	movsb d0, SYSTEM_ENAB

final_before_main:

	lea tmpstk, sp			| switch to tmpstack
	jsr _sun3_bootstrap		| init everything but calling main()
	lea _kstack, a1			| proc0 kernel stack (low end)
	lea a1@(UPAGES*NBPG-4),sp	| set stack pointer to last word
	pea _kstack_fall_off		| push something to fall back on :)
	movl #USRSTACK-4, a2		
	movl a2, usp			| set user stack
	movl	_proc0paddr,a1		| get proc0 pcb addr
	movl	a1,_curpcb		| proc0 is running
	clrw	a1@(PCB_FLAGS)		| clear flags
#ifdef FPCOPROC
	clrl	a1@(PCB_FPCTX)		| ensure null FP context
	movl	a1,sp@-
	jbsr	_m68881_restore		| restore it (does not kill a1)
	addql	#4,sp
#endif
/* Interrupts remain disabled until configure is nearly done. */
/*
 * Final preparation for calling main:
 *
 * Create a fake exception frame that returns to user mode,
 * make space for the rest of a fake saved register set, and
 * pass the first available RAM and a pointer to the register
 * set to "main()".  "main()" will call "icode()", which fakes
 * an "execve()" system call, which is why we need to do that
 * ("main()" sets "u.u_ar0" to point to the register set).
 * When "main()" returns, we're running in process 1 and have
 * successfully faked the "execve()".  We load up the registers from
 * that set; the "rte" loads the PC and PSR, which jumps to "init".
 */
  	clrw	sp@-			| vector offset/frame type
	clrl	sp@-			| PC - filled in by "execve"
  	movw	#PSL_USER,sp@-		| in user mode
	clrl	sp@-			| stack adjust count and padding
	lea	sp@(-64),sp		| construct space for D0-D7/A0-A7
	pea	sp@			| addr of space for D0
	jbsr	_main			| main(firstaddr, r0)
	addql	#4,sp			| pop args
	movl	sp@(FR_SP),a0		| grab and load
	movl	a0,usp			|   user SP
	moveml	sp@+,#0x7FFF		| load most registers (all but SSP)
	addql	#8,sp			| pop SSP and align word
  	rte

#include "lib.s"
#include "m68k.s"
#include "signal.s"
#include "process.s"
#include "softint.s"
#include "interrupt.s"
#include "trap.s"
#include "delay.s"
