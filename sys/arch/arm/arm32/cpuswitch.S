/*	cpuswitch.S,v 1.48.12.4 2007/11/07 06:24:51 matt Exp	*/

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

/*
 * These are used for switching the translation table/DACR.
 * Since the vector page can be invalid for a short time, we must
 * disable both regular IRQs *and* FIQs.
 *
 * XXX: This is not necessary if the vector table is relocated.
 */
#ifdef _ARM_ARCH_6

#define	IRQdisableALL	cpsid	if
#define	IRQenableALL	cpsie	if

#else

#define IRQdisableALL \
	mrs	r14, cpsr ; \
	orr	r14, r14, #(I32_bit | F32_bit) ; \
	msr	cpsr_c, r14

#define IRQenableALL \
	mrs	r14, cpsr ; \
	bic	r14, r14, #(I32_bit | F32_bit) ; \
	msr	cpsr_c, r14

#endif

	.text

#if !defined(PROCESS_ID_IS_CURLWP) && !defined(PROCESS_ID_IS_CURCPU)
.Lcpu_info_store:
	.word	_C_LABEL(cpu_info_store)
#endif

.Lcpufuncs:	
	.word	_C_LABEL(cpufuncs)
	
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
	stmfd	sp!, {r4-r7, lr}

#ifdef DIAGNOSTIC
	cmp	r1, #0
	bne	1f	
	adr	r0, .Lnewlwp_panicstr
	bl	_C_LABEL(panic)
.Lnewlwp_panicstr:
	.asciz	"cpu_switchto: new lwp is NULL!"
	.p2align 2
#endif
1:	mov	r4, r0		/* save old lwp, it's the return value */
	mov	r6, r1		/* save new lwp */

	IRQdisable

#ifdef PROCESS_ID_CURCPU
	GET_CURCPU(r7)
#elif defined(PROCESS_ID_IS_CURLWP)
	ldr	r7, [r4, #(L_CPU)]		/* get cpu from old lwp */
	mcr	p15, 0, r6, c13, c0, 4		/* save new lwp */
#else
	/* XXX use curcpu() if MULTIPROCESSOR */
	ldr	r7, .Lcpu_info_store
#endif
#ifdef MULTIPROCESSOR
	str	r7, [r6, #(L_CPU)]
#else
	/* l->l_cpu initialized in fork1() for single-processor */
#endif

#if !defined(PROCESS_ID_IS_CURLWP)
	/* We have a new curlwp now so make a note it */
	str	r6, [r7, #(CI_CURLWP)]
#endif

	/* Hook in a new pcb */
	ldr	r0, [r6, #(L_ADDR)]
	str	r0, [r7, #(CI_CURPCB)]

	/* At this point we can allow IRQ's again. */
	IRQenable

	/* rem: r4 = old lwp */
	/* rem: r6 = new lwp */
	/* rem: interrupts are enabled */

	/*
	 * If the old lwp on entry to cpu_switchto was zero then the
	 * process that called it was exiting. This means that we do
	 * not need to save the current context. Instead we can jump
	 * straight to restoring the context for the new process.
	 */
	teq	r4, #0x00000000
	ldreq	r9, [r6, #(L_ADDR)]
	beq	.Ldo_switch

	/* rem: r4 = old lwp */
	/* rem: r6 = new lwp */
	/* rem: interrupts are enabled */

	/* Save old context */

	/* Get the user structure for the old lwp. */
	ldr	r1, [r4, #(L_ADDR)]

	/* Save all the registers in the old lwp's pcb */
#ifndef __XSCALE__
	add	r7, r1, #(PCB_R8)
	stmia	r7, {r8-r13}
#else
	strd	r8, [r1, #(PCB_R8)]
	strd	r10, [r1, #(PCB_R10)]
	strd	r12, [r1, #(PCB_R12)]
#endif

#ifdef _ARM_ARCH_6
	/*
	 * Save user read/write thread/process id register
	 */
	mrc	p15, 0, r7, c13, c0, 2
	str	r7, [r1, #(PCB_USER_PID_RW)]
#endif
	/*
	 * NOTE: We can now use r8-r13 until it is time to restore
	 * them for the new process.
	 */

	/* rem: r1 = old lwp PCB */
	/* rem: r4 = old lwp */
	/* rem: r6 = new lwp */
	/* rem: interrupts are enabled */

	/* Remember the old PCB. */
	mov	r8, r1

	/* r1 now free! */

	/* Get the user structure for the new process in r9 */
	ldr	r9, [r6, #(L_ADDR)]

#ifdef _ARM_ARCH_6
	/*
	 * Restore user thread/process id registers
	 */
	ldr	r7, [r9, #(PCB_USER_PID_RW)]
	mcr	p15, 0, r7, c13, c0, 2
	ldr	r7, [r9, #(PCB_USER_PID_RO)]
	mcr	p15, 0, r7, c13, c0, 3
#endif
	/*
	 * This can be optimised... We know we want to go from SVC32
	 * mode to UND32 mode
	 */
        mrs	r3, cpsr
#ifdef _ARM_ARCH_6
	cpsie	i, #PSR_UND32_MODE
#else
	bic	r2, r3, #(PSR_MODE)
	orr	r2, r2, #(PSR_UND32_MODE | I32_bit)
        msr	cpsr_c, r2
#endif

	str	sp, [r8, #(PCB_UND_SP)]

        msr	cpsr_c, r3		/* Restore the old mode */

	/* What else needs to be saved? Only FPA stuff when that is supported */

	/* Restore saved context */

	/* rem: r4 = old lwp */
	/* rem: r6 = new lwp */
	/* rem: r9 = new PCB */
	/* rem: interrupts are enabled */

.Ldo_switch:
	mov	r0, r4
	mov	r1, r6
	bl	_C_LABEL(pmap_switch)

	/* rem: r4 = old lwp */
	/* rem: r6 = new lwp */
	/* rem: r9 = new PCB */

	/*
	 * This can be optimised... We know we want to go from SVC32
	 * mode to UND32 mode
	 */
        mrs	r3, cpsr
	bic	r2, r3, #(PSR_MODE)
	orr	r2, r2, #(PSR_UND32_MODE)
        msr	cpsr_c, r2

	ldr	sp, [r9, #(PCB_UND_SP)]

        msr	cpsr_c, r3		/* Restore the old mode */

	/* Restore all the save registers */
#ifndef __XSCALE__
	add	r7, r9, #PCB_R8
	ldmia	r7, {r8-r13}

	sub	r7, r7, #PCB_R8		/* restore PCB pointer */
#else
	mov	r7, r9
	ldr	r8, [r7, #(PCB_R8)]
	ldr	r9, [r7, #(PCB_R9)]
	ldr	r10, [r7, #(PCB_R10)]
	ldr	r11, [r7, #(PCB_R11)]
	ldr	r12, [r7, #(PCB_R12)]
	ldr	r13, [r7, #(PCB_SP)]
#endif

	ldr	r5, [r6, #(L_PROC)]	/* fetch the proc for below */

	/* rem: r4 = old lwp */
	/* rem: r5 = new lwp's proc */
	/* rem: r6 = new lwp */
	/* rem: r7 = new pcb */

#ifdef ARMFPE
	add	r0, r7, #(USER_SIZE) & 0x00ff
	add	r0, r0, #(USER_SIZE) & 0xff00 
	bl	_C_LABEL(arm_fpe_core_changecontext)
#endif

	/* We can enable interrupts again */
	IRQenableALL

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
	 * Pull the registers that got pushed when either savectx() or
	 * cpu_switchto() was called and return.
	 */
	ldmfd	sp!, {r4-r7, pc}

.Lswitch_do_ras:
	ldr	r1, [r1, #(TF_PC)]	/* second ras_lookup() arg */
	mov	r0, r5			/* first ras_lookup() arg */
	bl	_C_LABEL(ras_lookup)
	cmn	r0, #1			/* -1 means "not in a RAS" */
	ldrne	r1, [r7, #(PCB_TF)]
	strne	r0, [r1, #(TF_PC)]
	b	.Lswitch_return


/* LINTSTUB: Func: void savectx(struct pcb *pcb) */
ENTRY(savectx)
	/*
	 * r0 = pcb
	 */

	/* Push registers.*/
	stmfd	sp!, {r4-r7, lr}

	/* Store all the registers in the process's pcb */
#ifndef __XSCALE__
	add	r2, r0, #(PCB_R8)
	stmia	r2, {r8-r13}
#else
	strd	r8, [r0, #(PCB_R8)]
	strd	r10, [r0, #(PCB_R10)]
	strd	r12, [r0, #(PCB_R12)]
#endif

	/* Pull the regs of the stack */
	ldmfd	sp!, {r4-r7, pc}

ENTRY(lwp_trampoline)
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

#ifdef __HAVE_FAST_SOFTINTS
/*
 *	r0 = new lwp
 *	r1 = level
 */
ENTRY_NP(softint_switch)
	stmfd	sp!, {r4, r6, r7, lr}

#ifdef PROCESS_ID_CURCPU
	GET_CURCPU(r7)
	ldr	r4, [r7, #(CI_CURLWP)]		/* get old lwp */
#elif defined(PROCESS_ID_IS_CURLWP)
	mrc	p15, 0, r4, c13, c0, 4		/* get old lwp */
	ldr	r7, [r4, #(L_CPU)]		/* get cpu from old lwp */
#else
	/* XXX use curcpu() if MULTIPROCESSOR */
	ldr	r7, .Lcpu_info_store
	ldr	r4, [r7, #(CI_CURLWP)]		/* get old lwp */
#endif
	mrs	r6, cpsr			/* we need to save this */

	/*
	 * If the soft lwp blocks, it needs to return to softint_tramp
	 */
	adr	ip, softint_tramp
	stmfd	sp!, {r4-r7, ip}

	mov	r5, r0				/* save new lwp */

	ldr	r2, [r4, #(L_ADDR)]		/* get old lwp's pcb */

	/* Save all the registers into the old lwp's pcb */
#ifndef __XSCALE__
	add	r3, r2, #(PCB_R8)
	stmia	r3, {r8-r13}
#else
	strd	r8, [r2, #(PCB_R8)]
	strd	r10, [r2, #(PCB_R10)]
	strd	r12, [r2, #(PCB_R12)]
#endif

	IRQdisable
	/*
	 * We're switching to a bound LWP so its l_cpu is already correct.
	 */
#if !defined(PROCESS_ID_IS_CURLWP)
	str	r5, [r7, #(CI_CURLWP)]		/* save new lwp */
#else
	mcr	p15, 0, r5, c13, c0, 4		/* save new lwp */
#endif

	/* Hook in a new pcb */
	ldr	r2, [r5, #(L_ADDR)]
	str	r2, [r7, #(CI_CURPCB)]

	/*
	 * Normally, we'd get {r8-r13} but since this is a softint lwp
	 * it's existing state doesn't matter except for the stack pointer.
	 */
	ldr	r2, [r5, #(L_ADDR)]	/* get new lwp's pcb */
	ldr	sp, [r2, #(PCB_SP)]	/* get new lwp's stack ptr */

	/* At this point we can allow IRQ's again. */
	IRQenable

					/* r1 still has level */
	mov	r0, r4			/* r0 has pinned (old) lwp */
	bl	_C_LABEL(softint_dispatch)
	/*
	 * If we've returned, we need to change everything back and return.
	 */
	ldr	r2, [r4, #(L_ADDR)]	/* get pinned lwp's pcb */

	IRQdisable
	/*
	 * We don't need to restore all the registers since another lwp was
	 * never executed.  But we do need the SP from the formerly pinned lwp.
	 */
	ldr	sp, [r2, #(PCB_SP)]	/* now running on the old stack. */

#if defined(PROCESS_ID_IS_CURLWP)
	mcr	p15, 0, r4, c13, c0, 4		/* restore pinned lwp */
#else
	str	r4, [r7, #(CI_CURLWP)]		/* restore pinned lwp */
#endif
	str	r2, [r7, #(CI_CURPCB)]		/* restore the curpcb */

	/* At this point we can allow IRQ's again. */
	msr	cpsr_c, r6

	/*
	 * Grab the registers that got pushed at the start and return.
	 */
	ldmfd	sp!, {r4-r7, lr}	/* eat switch frame */
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
