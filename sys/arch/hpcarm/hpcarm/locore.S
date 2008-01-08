/*	$NetBSD: locore.S,v 1.9.64.1 2008/01/08 22:09:47 bouyer Exp $	*/

/*
 * Copyright (C) 1994-1997 Mark Brinicombe
 * Copyright (C) 1994 Brini
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
 *	This product includes software developed by Brini.
 * 4. The name of Brini may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ipkdb.h"
#include "assym.h"
#include <sys/syscall.h>
#include <sys/errno.h>
#include <machine/asm.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/param.h>

/* What size should this really be ? It is only used by init_arm() */
#define INIT_ARM_STACK_SIZE	2048

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */

ENTRY_NP(kernel_text)

ASENTRY_NP(start)
	/* Put the processer in SVC mode */
	mov	r5, sp
	mrs	r4, cpsr_all
	bic	r4, r4, #31
	orr	r4, r4, #(PSR_SVC32_MODE)
	msr	cpsr_all, r4
	mov	sp, r5

	/* Disable PID virtual address mapping */
	mov	r4, #0
	mcr	15, 0, r4, c13, c0, 0

	mov	fp, #0x00000000		/* trace back starts here */
	bl	_C_LABEL(initarm)	/* Off we go */

	/* init arm will return the new stack pointer. */
	mov	sp, r0

	mov	fp, #0x00000000		/* trace back starts here */
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4

	/* Setup an initial trap frame for start_init to use */

	PUSHFRAME

	mov	r0, sp			/* parameter to main is trap frame */ 

	bl	_C_LABEL(main)		/* Lets light the flame and start her up */

	PULLFRAME			/* Pull the trap frame, now valid */

	movs	pc, lr			/* Exit to user process */

	/* Never gets here */

	b	.

	.text
	.align	0

.Lcpufuncs:	
	.word	_C_LABEL(cpufuncs)

ENTRY_NP(cpu_reset)
	mrs     r2, cpsr
	bic	r2, r2, #(PSR_MODE)
	orr     r2, r2, #(PSR_SVC32_MODE)
	orr	r2, r2, #(I32_bit | F32_bit)
	msr     cpsr_all, r2

	ldr	r4, .Lcpu_reset_address
	ldr	r4, [r4]

	ldr	r0, .Lcpufuncs
	mov	lr, pc
	ldr	pc, [r0, #CF_IDCACHE_WBINV_ALL]

	/*
 	 * MMU & IDC off, 32 bit program & data space
	 * Hurl ourselves into the ROM
	 */
	mov	r0, #(CPU_CONTROL_32BP_ENABLE | CPU_CONTROL_32BD_ENABLE)
	mcr     15, 0, r0, c1, c0, 0
	mcr     15, 0, r0, c8, c7, 0
	mov     pc, r4

	/*
	 * _cpu_reset_address contains the address to branch to, to complete
	 * the CPU reset after turning the MMU off
	 * This variable is provided by the hardware specific code
	 */

.Lcpu_reset_address:
	.word	_C_LABEL(cpu_reset_address)

#ifdef IPKDB
/*
 * Execute(inst, psr, args, sp)
 *
 * Execute INSTruction with PSR and ARGS[0] - ARGS[3] making
 * available stack at SP for next undefined instruction trap.
 *
 * Move the instruction onto the stack and jump to it.
 */
ENTRY_NP(Execute)
	mov	ip, sp
	stmfd	sp!, {r2, r4-r7, fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	ip, r3
	ldr	r7, .Lreturn
	stmfd	sp!, {r0, r7}
	adr	r7, .LExec
	mov	r5, r1
	mrs	r4, cpsr
	ldmia	r2, {r0-r3}
	mov	r6, sp
	mov	sp, ip
	msr	cpsr_all, r5
	mov	pc, r6
.LExec:
	mrs	r5, cpsr
/* XXX Cannot switch thus easily back from user mode */
	msr	cpsr_all, r4
	add	sp, r6, #8
	ldmfd	sp!, {r6}
	stmia	r6, {r0-r3}
	mov	r0, r5
	ldmdb	fp, {r4-r7, fp, sp, pc}
.Lreturn:
	mov	pc, r7
#endif

/*
 * setjump + longjmp
 */
ENTRY(setjmp)
	stmia	r0, {r4-r14}
	mov	r0, #0x00000000
	mov	pc, lr

ENTRY(longjmp)
	ldmia	r0, {r4-r14}
	mov	r0, #0x00000001
	mov	pc, lr

	.data
	.global _C_LABEL(esym)
_C_LABEL(esym):	.word	_C_LABEL(end)

ENTRY_NP(abort)
	b	_C_LABEL(abort)

/*
 * part of doing a system dump, we need to save registers and cpsr onto the
 * stack, then save the rest of the registers into the dumppcb
 */
ENTRY(dumpsys)
	/* push registers onto stack */
	stmfd	sp!, {r0-r7, lr}
	
	/* push the status bits onto the stack */
	mrs	r0, cpsr_all
	stmfd	sp!, {r0}

	/* fill in dumppcb */
	ldr	r0, .Ldumppcb

#ifndef __XSCALE__
        add     r2, r0, #(PCB_R8)
        stmia   r2, {r8-r13}
#else
        strd    r8, [r0, #(PCB_R8)]
        strd    r10, [r0, #(PCB_R10)]
        strd    r12, [r0, #(PCB_R12)]
#endif

	bl	_C_LABEL(dodumpsys)

	/* unwind the stack */
	ldmfd	sp!, {r0}
	nop
	ldmfd	sp!, {r0-r7, pc}

.Ldumppcb:
	.word	_C_LABEL(dumppcb)

/* End of locore.S */
