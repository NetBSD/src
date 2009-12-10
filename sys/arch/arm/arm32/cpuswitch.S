/*	$NetBSD: cpuswitch.S,v 1.60 2009/12/10 05:10:01 rmind Exp $	*/

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
#include "opt_cpuoptions.h"
#include "opt_lockdebug.h"

#include "assym.h"
#include <arm/arm32/pte.h>
#include <machine/param.h>
#include <machine/frame.h>
#include <machine/asm.h>
#include <machine/cpu.h>

	RCSID("$NetBSD: cpuswitch.S,v 1.60 2009/12/10 05:10:01 rmind Exp $")

/* LINTSTUB: include <sys/param.h> */
	
#undef IRQdisable
#undef IRQenable

/*
 * New experimental definitions of IRQdisable and IRQenable
 * These keep FIQ's enabled since FIQ's are special.
 */

#ifdef _ARM_ARCH_6
#define	IRQdisable	cpsid	i
#define	IRQenable	cpsie	i
#else
#define IRQdisable \
	mrs	r14, cpsr ; \
	orr	r14, r14, #(I32_bit) ; \
	msr	cpsr_c, r14

#define IRQenable \
	mrs	r14, cpsr ; \
	bic	r14, r14, #(I32_bit) ; \
	msr	cpsr_c, r14

#endif

	.text
.Lpmap_previous_active_lwp:
	.word	_C_LABEL(pmap_previous_active_lwp)

/*
 * struct lwp *
 * cpu_switchto(struct lwp *current, struct lwp *next)
 *
 * Switch to the specified next LWP
 * Arguments:
 *
 *	r0	'struct lwp *' of the current LWP (or NULL if exiting)
 *	r1	'struct lwp *' of the LWP to switch to
 *	r2	returning
 */
ENTRY(cpu_switchto)
	mov	ip, sp
	stmfd	sp!, {r4-r7, ip, lr}

	/* move lwps into caller saved registers */
	mov	r6, r1
	mov	r4, r0

#ifdef PROCESS_ID_CURCPU
	GET_CURCPU(r7)
#elif defined(PROCESS_ID_IS_CURLWP)
	mcr	p15, 0, r0, c13, c0, 4		/* get old lwp (r4 maybe 0) */
	ldr	r7, [r0, #(L_CPU)]		/* get cpu from old lwp */
#elif !defined(MULTIPROCESSOR) 
	ldr	r7, [r6, #L_CPU]		/* get cpu from new lwp */
#else
#error curcpu() method not defined
#endif

	/* rem: r4 = old lwp */
	/* rem: r6 = new lwp */
	/* rem: r7 = curcpu() */
	
#ifndef __HAVE_UNNESTED_INTRS
	IRQdisable
#endif

#ifdef MULTIPROCESSOR
	str	r7, [r6, #(L_CPU)]
#else
	/* l->l_cpu initialized in fork1() for single-processor */
#endif

#if defined(PROCESS_ID_IS_CURLWP)
	mcr	p15, 0, r6, c13, c0, 4		/* set current lwp */
#endif
#if !defined(PROCESS_ID_IS_CURLWP) || defined(MULTIPROCESSOR)
	/* We have a new curlwp now so make a note it */
	str	r6, [r7, #(CI_CURLWP)]
#endif

	/* Hook in a new pcb */
	ldr	r0, [r6, #(L_PCB)]
	str	r0, [r7, #(CI_CURPCB)]
	mov	r7, r0

	/* At this point we can allow IRQ's again. */
#ifndef __HAVE_UNNESTED_INTRS
	IRQenable
#endif

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
	teq	r4, #0
	beq	.Ldo_switch

	/* rem: r4 = old lwp */
	/* rem: r6 = new lwp */
	/* rem: r7 = new pcb */
	/* rem: interrupts are enabled */

	/* Save old context */

	/* Get the user structure for the old lwp. */
	ldr	r5, [r4, #(L_PCB)]

	/* Save all the registers in the old lwp's pcb */
#if defined(__XSCALE__) || defined(_ARM_ARCH_6)
	strd	r8, [r5, #(PCB_R8)]
	strd	r10, [r5, #(PCB_R10)]
	strd	r12, [r5, #(PCB_R12)]
#else
	add	r0, r5, #(PCB_R8)
	stmia	r0, {r8-r13}
#endif

#ifdef _ARM_ARCH_6
	/*
	 * Save user read/write thread/process id register
	 */
	mrc	p15, 0, r0, c13, c0, 2
	str	r0, [r5, #(PCB_USER_PID_RW)]
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

#ifdef FPU_VFP
	/*
	 * Now's a good time to 'save' the VFP context.  Note that we
	 * don't really force a save here, which can save time if we
	 * end up restarting the same context.
	 */
	bl	_C_LABEL(vfp_savecontext)
#endif

	/* Restore saved context */

.Ldo_switch:
	/* rem: r4 = old lwp */
	/* rem: r6 = new lwp */
	/* rem: r7 = new pcb */
	/* rem: interrupts are enabled */

#ifdef _ARM_ARCH_6
	/*
	 * Restore user thread/process id registers
	 */
	ldr	r0, [r7, #(PCB_USER_PID_RW)]
	mcr	p15, 0, r0, c13, c0, 2
	ldr	r0, [r7, #(PCB_USER_PID_RO)]
	mcr	p15, 0, r0, c13, c0, 3
#endif

	ldr	r5, [r6, #(L_PROC)]	/* fetch the proc for below */

	/* Restore all the saved registers */
#ifdef __XSCALE__
	ldr	r8, [r7, #(PCB_R8)]
	ldr	r9, [r7, #(PCB_R9)]
	ldr	r10, [r7, #(PCB_R10)]
	ldr	r11, [r7, #(PCB_R11)]
	ldr	r12, [r7, #(PCB_R12)]
	ldr	r13, [r7, #(PCB_SP)]
#elif defined(_ARM_ARCH_6)
	ldrd	r8, [r7, #(PCB_R8)]
	ldrd	r10, [r7, #(PCB_R10)]
	ldrd	r12, [r7, #(PCB_R12)]
#else
	add	r0, r7, #PCB_R8
	ldmia	r0, {r8-r13}
#endif

	/* Record the old lwp for pmap_activate()'s benefit */
	ldr	r1, .Lpmap_previous_active_lwp
	str	r4, [r1]

	/* rem: r4 = old lwp */
	/* rem: r5 = new lwp's proc */
	/* rem: r6 = new lwp */
	/* rem: r7 = new pcb */

#ifdef FPU_VFP
	mov	r0, r6
	bl	_C_LABEL(vfp_loadcontext)
#endif
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
        orr     r0, r0, #(IF32_bits)
        msr     cpsr_c, r0

	PULLFRAME

	movs	pc, lr			/* Exit */

#ifdef __HAVE_FAST_SOFTINTS
/*
 *	Called at IPL_HIGH
 *	r0 = new lwp
 *	r1 = ipl for softint_dispatch
 */
ENTRY_NP(softint_switch)
	stmfd	sp!, {r4, r6, r7, lr}

	ldr	r7, [r0, #L_CPU]		/* get curcpu */
#if defined(PROCESS_ID_IS_CURLWP)
	mrc	p15, 0, r4, c13, c0, 4		/* get old lwp */
#else
	ldr	r4, [r7, #(CI_CURLWP)]		/* get old lwp */
#endif
	mrs	r6, cpsr			/* we need to save this */

	/*
	 * If the soft lwp blocks, it needs to return to softint_tramp
	 */
	mov	r2, sp				/* think ip */
	adr	r3, softint_tramp		/* think lr */
	stmfd	sp!, {r2-r3}
	stmfd	sp!, {r4-r7}

	mov	r5, r0				/* save new lwp */

	ldr	r2, [r4, #(L_PCB)]		/* get old lwp's pcb */

	/* Save all the registers into the old lwp's pcb */
#if defined(__XSCALE__) || defined(_ARM_ARCH_6)
	strd	r8, [r2, #(PCB_R8)]
	strd	r10, [r2, #(PCB_R10)]
	strd	r12, [r2, #(PCB_R12)]
#else
	add	r3, r2, #(PCB_R8)
	stmia	r3, {r8-r13}
#endif

	/* this is an invariant so load before disabling intrs */
	ldr	r2, [r5, #(L_PCB)]	/* get new lwp's pcb */

#ifndef __HAVE_UNNESTED_INTRS
	IRQdisable
#endif
	/*
	 * We're switching to a bound LWP so its l_cpu is already correct.
	 */
#if defined(PROCESS_ID_IS_CURLWP)
	mcr	p15, 0, r5, c13, c0, 4		/* save new lwp */
#endif
#if !defined(PROCESS_ID_IS_CURLWP) || defined(MULTIPROCESSOR)
	str	r5, [r7, #(CI_CURLWP)]		/* save new lwp */
#endif

	/* Hook in a new pcb */
	str	r2, [r7, #(CI_CURPCB)]

	/*
	 * Normally, we'd get {r8-r13} but since this is a softint lwp
	 * it's existing state doesn't matter.  We start the stack just
	 * below the trapframe.
	 */
	ldr	sp, [r2, #(PCB_TF)]	/* get new lwp's stack ptr */

	/* At this point we can allow IRQ's again. */
#ifndef __HAVE_UNNESTED_INTRS
	IRQenable
#endif

					/* r1 still has ipl */
	mov	r0, r4			/* r0 has pinned (old) lwp */
	bl	_C_LABEL(softint_dispatch)
	/*
	 * If we've returned, we need to change everything back and return.
	 */
	ldr	r2, [r4, #(L_PCB)]	/* get pinned lwp's pcb */

#ifndef __HAVE_UNNESTED_INTRS
	IRQdisable
#endif
	/*
	 * We don't need to restore all the registers since another lwp was
	 * never executed.  But we do need the SP from the formerly pinned lwp.
	 */

#if defined(PROCESS_ID_IS_CURLWP)
	mcr	p15, 0, r4, c13, c0, 4		/* restore pinned lwp */
#endif
#if !defined(PROCESS_ID_IS_CURLWP) || defined(MULTIPROCESSOR)
	str	r4, [r7, #(CI_CURLWP)]		/* restore pinned lwp */
#endif
	str	r2, [r7, #(CI_CURPCB)]		/* restore the curpcb */
	ldr	sp, [r2, #(PCB_SP)]	/* now running on the old stack. */

	/* At this point we can allow IRQ's again. */
	msr	cpsr_c, r6

	/*
	 * Grab the registers that got pushed at the start and return.
	 */
	ldmfd	sp!, {r4-r7, ip, lr}	/* eat switch frame */
	ldmfd	sp!, {r4, r6, r7, pc}	/* pop stack and return */

END(softint_switch)

/*
 * r0 = previous LWP (the soft lwp)
 * r4 = original LWP (the current lwp)
 * r6 = original CPSR
 * r7 = curcpu()
 */
ENTRY_NP(softint_tramp)
	ldr	r3, [r7, #(CI_MTX_COUNT)]	/* readust after mi_switch */
	add	r3, r3, #1
	str	r3, [r7, #(CI_MTX_COUNT)]

	mov	r3, #0				/* tell softint_dispatch */
	str	r3, [r0, #(L_CTXSWTCH)]		/*    the soft lwp blocked */

	msr	cpsr_c, r6			/* restore interrupts */
	ldmfd	sp!, {r4, r6, r7, pc}		/* pop stack and return */
END(softint_tramp)
#endif /* __HAVE_FAST_SOFTINTS */
