/*	$NetBSD: cpuswitch.S,v 1.55 2008/01/19 16:49:42 chris Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpuswitch.S
 *
 * cpu switching functions
 *
 * Created      : 15/10/94
 */

#include "opt_armfpe.h"
#include "opt_arm32_pmap.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"

#include "assym.h"
#include <arm/arm32/pte.h>
#include <machine/param.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/asm.h>

/* LINTSTUB: include <sys/param.h> */
	
#undef IRQdisable
#undef IRQenable

/*
 * New experimental definitions of IRQdisable and IRQenable
 * These keep FIQ's enabled since FIQ's are special.
 */

#define IRQdisable \
	mrs	r14, cpsr ; \
	orr	r14, r14, #(I32_bit) ; \
	msr	cpsr_c, r14 ; \

#define IRQenable \
	mrs	r14, cpsr ; \
	bic	r14, r14, #(I32_bit) ; \
	msr	cpsr_c, r14 ; \

	.text

#ifdef MULTIPROCESSOR
.Lcpu_info_store:
	.word	_C_LABEL(cpu_info_store)
.Lcurlwp:
	/* FIXME: This is bogus in the general case. */
	.word	_C_LABEL(cpu_info_store) + CI_CURLWP

.Lcurpcb:
	.word	_C_LABEL(cpu_info_store) + CI_CURPCB
#else
.Lcurlwp:
	.word	_C_LABEL(curlwp)

.Lcurpcb:
	.word	_C_LABEL(curpcb)
#endif

#ifndef MULTIPROCESSOR
	.data
	.global	_C_LABEL(curpcb)
_C_LABEL(curpcb):
	.word	0x00000000
	.text
#endif

.Lblock_userspace_access:
	.word	_C_LABEL(block_userspace_access)


/*
 * struct lwp *
 * cpu_switchto(struct lwp *current, struct lwp *next)
 *
 * Switch to the specified next LWP
 * Arguments:
 *
 *	r0	'struct lwp *' of the current LWP
 *	r1	'struct lwp *' of the LWP to switch to
 */
ENTRY(cpu_switchto)
	mov	ip, sp
	stmfd	sp!, {r4-r7, ip, lr}

	/* move lwps and new pcb into caller saved registers */
	mov	r6, r1
	mov	r4, r0
	ldr	r7, [r6, #(L_ADDR)]

	/* rem: r4 = old lwp */
	/* rem: r6 = new lwp */
	/* rem: r7 = new pcb */
	
	IRQdisable

#ifdef MULTIPROCESSOR
	/* XXX use curcpu() */
	ldr	r0, .Lcpu_info_store
	str	r0, [r6, #(L_CPU)]
#else
	/* l->l_cpu initialized in fork1() for single-processor */
#endif

	/* We have a new curlwp and pcb so make a note of them */
	ldr	r0, .Lcurlwp
	ldr	r1, .Lcurpcb

	str	r6, [r0]
	str	r7, [r1]

	/* At this point we can allow IRQ's again. */
	IRQenable

	/* rem: r4 = old lwp */
	/* rem: r6 = new lwp */
	/* rem: r7 = new pcb */
	/* rem: interrupts are enabled */

	/*
	 * If the old lwp on entry to cpu_switchto was zero then the
	 * process that called it was exiting. This means that we do
	 * not need to save the current context. Instead we can jump
	 * straight to restoring the context for the new process.
	 */
	teq	r4, #0x00000000
	beq	.Ldo_switch

	/* rem: r4 = old lwp */
	/* rem: r6 = new lwp */
	/* rem: r7 = new pcb */
	/* rem: interrupts are enabled */

	/* Save old context */

	/* Get the user structure for the old lwp. */
	ldr	r5, [r4, #(L_ADDR)]

	/* Save all the registers in the old lwp's pcb */
#ifndef __XSCALE__
	add	r0, r5, #(PCB_R8)
	stmia	r0, {r8-r13}
#else
	strd	r8, [r5, #(PCB_R8)]
	strd	r10, [r5, #(PCB_R10)]
	strd	r12, [r5, #(PCB_R12)]
#endif

	/*
	 * NOTE: We can now use r8-r13 until it is time to restore
	 * them for the new process.
	 */

	/* rem: r4 = old lwp */
	/* rem: r5 = old pcb */
	/* rem: r6 = new lwp */
	/* rem: r7 = new pcb */
	/* rem: interrupts are enabled */

	/* What else needs to be saved? Only FPA stuff when that is supported */

	/* Restore saved context */

.Ldo_switch:
	mov	r0, r4
	mov	r1, r6
	bl	_C_LABEL(pmap_switch)

	/* rem: r4 = old lwp */
	/* rem: r6 = new lwp */
	/* rem: r7 = new pcb */
	/* rem: interrupts are enabled */

	ldr	r5, [r6, #(L_PROC)]	/* fetch the proc for below */

	/* Restore all the saved registers */
#ifndef __XSCALE__
	add	r0, r7, #PCB_R8
	ldmia	r0, {r8-r13}
#else
	ldr	r8, [r7, #(PCB_R8)]
	ldr	r9, [r7, #(PCB_R9)]
	ldr	r10, [r7, #(PCB_R10)]
	ldr	r11, [r7, #(PCB_R11)]
	ldr	r12, [r7, #(PCB_R12)]
	ldr	r13, [r7, #(PCB_SP)]
#endif

	/* rem: r4 = old lwp */
	/* rem: r5 = new lwp's proc */
	/* rem: r6 = new lwp */
	/* rem: r7 = new pcb */

#ifdef ARMFPE
	add	r0, r7, #(USER_SIZE) & 0x00ff
	add	r0, r0, #(USER_SIZE) & 0xff00 
	bl	_C_LABEL(arm_fpe_core_changecontext)
#endif

	/* rem: r4 = old lwp */
	/* rem: r5 = new lwp's proc */
	/* rem: r6 = new lwp */
	/* rem: r7 = new PCB */

	/* 
	 * Check for restartable atomic sequences (RAS).
	 */

	ldr	r2, [r5, #(P_RASLIST)]
	ldr	r1, [r7, #(PCB_TF)]	/* r1 = trapframe (used below) */
	teq	r2, #0			/* p->p_nras == 0? */
	bne	.Lswitch_do_ras		/* no, check for one */

.Lswitch_return:
	/* cpu_switchto returns the old lwp */
	mov	r0, r4
	/* lwp_trampoline expects new lwp as it's second argument */
	mov	r1, r6

	/*
	 * Pull the registers that got pushed when cpu_switchto() was called,
	 * and return.
	 */
	ldmfd	sp, {r4-r7, sp, pc}

.Lswitch_do_ras:
	ldr	r1, [r1, #(TF_PC)]	/* second ras_lookup() arg */
	mov	r0, r5			/* first ras_lookup() arg */
	bl	_C_LABEL(ras_lookup)
	cmn	r0, #1			/* -1 means "not in a RAS" */
	ldrne	r1, [r7, #(PCB_TF)]
	strne	r0, [r1, #(TF_PC)]
	b	.Lswitch_return

ENTRY(lwp_trampoline)
	/*
	 * cpu_switchto gives us:
	 *
	 * arg0(r0) = old lwp
	 * arg1(r1) = new lwp
	 */
	bl	_C_LABEL(lwp_startup)

	mov	r0, r5
	mov	r1, sp
	mov	lr, pc
	mov	pc, r4

	/* Kill irq's */
        mrs     r0, cpsr
        orr     r0, r0, #(I32_bit)
        msr     cpsr_c, r0

	PULLFRAME

	movs	pc, lr			/* Exit */
