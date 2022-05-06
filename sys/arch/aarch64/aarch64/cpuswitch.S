/* $NetBSD: cpuswitch.S,v 1.34 2022/05/06 06:09:50 ryo Exp $ */

/*-
 * Copyright (c) 2014, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <aarch64/asm.h>
#include <aarch64/locore.h>

#include "assym.h"

#include "opt_cpuoptions.h"
#include "opt_ddb.h"
#include "opt_kasan.h"

RCSID("$NetBSD: cpuswitch.S,v 1.34 2022/05/06 06:09:50 ryo Exp $")

	ARMV8_DEFINE_OPTIONS

/*
 * At IPL_SCHED:
 *	x0 = oldlwp
 *	x1 = newlwp
 *	x2 = returning
 * returns x0-x2 unchanged
 */
ENTRY_NP(cpu_switchto)
	/*
	 * Store the callee saved register on the stack.
	 */
	sub	sp, sp, #TF_SIZE		/* make switchframe */
	stp	x19, x20, [sp, #TF_X19]
	stp	x21, x22, [sp, #TF_X21]
	stp	x23, x24, [sp, #TF_X23]
	stp	x25, x26, [sp, #TF_X25]
	stp	x27, x28, [sp, #TF_X27]
	stp	x29, x30, [sp, #TF_X29]
#ifdef DDB
	str	lr, [sp, #TF_PC]		/* for backtrace */
	str	xzr, [sp, #TF_SP]		/* mark as switchframe */
#endif

	/*
	 * Save the current stack pointer and the CPACR and save them in
	 * old lwp md area.
	 */
	mov	x4, sp
	mrs	x5, cpacr_el1
	str	x5, [x0, #L_MD_CPACR]
	ldr	x6, [x0, #L_PCB]	/* x6 = lwp_getpcb(oldlwp) */
	str	x4, [x6, #PCB_TF]

	/* We are done with the old lwp */
	ldr	x6, [x1, #L_PCB]	/* x6 = lwp_getpcb(newlwp) */
	ldr	x4, [x6, #PCB_TF]	/* get trapframe ptr (aka SP) */
#ifdef DDB
	str	xzr, [x6, #PCB_TF]	/* clear l->l_addr->pcb_tf */
#endif
	ldr	x5, [x1, #L_MD_CPACR]	/* get cpacr_el1 */

	DISABLE_INTERRUPT
	mov	sp, x4			/* restore stack pointer */
	msr	cpacr_el1, x5		/* restore cpacr_el1 */
	isb

#ifdef ARMV83_PAC
	/* Switch the PAC key. */
	adrl	x4, _C_LABEL(aarch64_pac_enabled)
	ldr	w4, [x4]
	cbz	w4, 1f
	ldp	x5, x6, [x1, #L_MD_IA_KERN]
	msr	APIAKeyLo_EL1, x5
	msr	APIAKeyHi_EL1, x6

	/* Other keys only need to be updated when switching to user process */
	ldr	w5, [x1, #L_FLAG]
	and	w5, w5, #LW_SYSTEM	/* (lwp->l_flag & LW_SYSTEM) ? */
	cbnz	w5, 1f

	ldp	x5, x6, [x1, #L_MD_IB_USER]
	msr	APIBKeyLo_EL1, x5
	msr	APIBKeyHi_EL1, x6
	ldp	x5, x6, [x1, #L_MD_DA_USER]
	msr	APDAKeyLo_EL1, x5
	msr	APDAKeyHi_EL1, x6
	ldp	x5, x6, [x1, #L_MD_DB_USER]
	msr	APDBKeyLo_EL1, x5
	msr	APDBKeyHi_EL1, x6
	ldp	x5, x6, [x1, #L_MD_GA_USER]
	msr	APGAKeyLo_EL1, x5
	msr	APGAKeyHi_EL1, x6
1:
	isb
#endif

	msr	tpidr_el1, x1		/* switch curlwp to new lwp */
	ldr	x3, [x1, #L_CPU]
	str	x1, [x3, #CI_CURLWP]	/* switch curlwp to new lwp */
	dmb	ishst			/* see comments in kern_mutex.c */
	ENABLE_INTERRUPT

	/*
	 * Restore callee save registers.
	 */
	ldp	x19, x20, [sp, #TF_X19]
	ldp	x21, x22, [sp, #TF_X21]
	ldp	x23, x24, [sp, #TF_X23]
	ldp	x25, x26, [sp, #TF_X25]
	ldp	x27, x28, [sp, #TF_X27]
	ldp	x29, lr, [sp, #TF_X29]
	add	sp, sp, #TF_SIZE	/* unwind switchframe */

	ret
END(cpu_switchto)

/*
 * void
 * cpu_switchto_softint(struct lwp *softlwp, int ipl)
 * {
 *	build a switchframe on kernel stack.
 *	craft TF_X30 to have softint_cleanup.
 *	pinned_lwp = curlwp
 *	switch to softlwp context.
 *	call softint_dispatch(pinned_lwp, ipl);
 *	switch back to pinned_lwp context.
 *	unwind switchframe made on kernel stack.
 *	return to caller this time.
 * }
 */
ENTRY_NP(cpu_switchto_softint)
	stp	x19, x20, [sp, #-16]!	/* save */
	sub	sp, sp, #TF_SIZE	/* make switchframe */
	adr	x2, softint_cleanup	/* return address for cpu_switchto() */
	mov	x20, lr			/* x20 := original lr */
	mrs	x19, daif		/* x19 := original interrupt mask */
	stp	x19, x20, [sp, #TF_X19]
	stp	x21, x22, [sp, #TF_X21]
	stp	x23, x24, [sp, #TF_X23]
	stp	x25, x26, [sp, #TF_X25]
	stp	x27, x28, [sp, #TF_X27]
	stp	x29, x2, [sp, #TF_X29]	/* tf->tf_lr = softint_cleanup; */

	mrs	x19, tpidr_el1		/* x19 := curlwp */
	mov	x4, sp

	mrs	x5, cpacr_el1
	ldr	x6, [x19, #L_PCB]	/* x6 = lwp_getpcb(curlwp) */
	str	x4, [x6, #PCB_TF]
	str	x5, [x19, #L_MD_CPACR]

#ifdef KASAN
	/* clear the new stack */
	stp	x0, x1, [sp, #-16]!
	bl	_C_LABEL(kasan_softint)
	ldp	x0, x1, [sp], #16
#endif

	ldr	x4, [x0, #L_MD_UTF]

	DISABLE_INTERRUPT
	ldr	x20, [x19, #L_CPU]	/* x20 := curlwp->l_cpu */

	/* onto new stack */
	sub	sp, x4, #TF_SIZE	/* new sp := softlwp->l_md_utf - 1 */
	msr	tpidr_el1, x0		/* curlwp = softlwp; */
	str	x0, [x20, #CI_CURLWP]	/* curcpu()->ci_curlwp = softlwp; */
					/* no need for memory barrier here */

	mov	x5, #CPACR_FPEN_NONE
	msr	cpacr_el1, x5		/* cpacr_el1 = CPACR_FPEN_NONE */
	isb

#ifdef ARMV83_PAC
	/* Switch the PAC key. */
	adrl	x4, _C_LABEL(aarch64_pac_enabled)
	ldr	w4, [x4]
	cbz	w4, 1f
	ldp	x5, x6, [x0, #L_MD_IA_KERN]
	msr	APIAKeyLo_EL1, x5
	msr	APIAKeyHi_EL1, x6
	isb
1:
#endif
	ENABLE_INTERRUPT

	/* softint_dispatch(pinned_lwp, ipl) */
	mov	x0, x19			/* x0 := pinned_lwp */
	bl	_C_LABEL(softint_dispatch)

	ldr	x6, [x19, #L_PCB]	/* x6 = lwp_getpcb(curlwp) */
	ldr	x4, [x6, #PCB_TF]	/* x4 := pinned_lwp->l_addr->pcb_tf */
#ifdef DDB
	str	xzr, [x6, #PCB_TF]	/* clear l->l_addr->pcb_tf */
#endif
	ldr	x5, [x19, #L_MD_CPACR]	/* x5 := pinned_lwp->l_md_cpacr */

	DISABLE_INTERRUPT
	msr	tpidr_el1, x19		/* curlwp = pinned_lwp */
	ldr	x3, [x19, #L_CPU]	/* x3 = curlwp->l_cpu */
	str	x19, [x3, #CI_CURLWP]	/* curlwp->l_cpu->ci_curlwp := x19 */
	dmb	ishst			/* see comments in kern_mutex.c */

	mov	sp, x4			/* restore pinned_lwp sp */
	msr	cpacr_el1, x5		/* restore pinned_lwp cpacr */
	isb

#ifdef ARMV83_PAC
	/* Restore the PAC key. */
	adrl	x4, _C_LABEL(aarch64_pac_enabled)
	ldr	w4, [x4]
	cbz	w4, 1f
	ldp	x5, x6, [x19, #L_MD_IA_KERN]
	msr	APIAKeyLo_EL1, x5
	msr	APIAKeyHi_EL1, x6
	isb
1:
#endif

	ldp	x19, x20, [sp, #TF_X19]
	msr	daif, x19		/* restore interrupt mask */
	mov	lr, x20			/* restore pinned_lwp lr */

	add	sp, sp, #TF_SIZE	/* unwind switchframe */
	ldp	x19, x20, [sp], #16	/* restore */
	ret
END(cpu_switchto_softint)

/*
 * void
 * softint_cleanup(struct lwp *softlwp)
 * {
 *	cpu_switchto() bottom half arranges to start this when softlwp.
 *	kernel thread is to yield CPU for the pinned_lwp in the above.
 *	curcpu()->ci_mtx_count += 1;
 *	this returns as if cpu_switchto_softint finished normally.
 * }
 */
ENTRY_NP(softint_cleanup)
	mov	lr, x20			/* restore original lr */

	mrs	x20, tpidr_el1		/* curlwp */
	ldr	x3, [x20, #L_CPU]	/* curcpu */
	ldr	w2, [x3, #CI_MTX_COUNT]	/* ->ci_mtx_count */
	add	w2, w2, #1
	str	w2, [x3, #CI_MTX_COUNT]

	msr	daif, x19		/* restore interrupt mask */
	ldp	x19, x20, [sp], #16	/* restore */
	ret
END(softint_cleanup)

/*
 * Called at IPL_SCHED:
 *	x0 = old lwp (from cpu_switchto)
 *	x1 = new lwp (from cpu_switchto)
 *	x27 = func
 *	x28 = arg
 */
ENTRY_NP(lwp_trampoline)
	stp	x0, x1, [sp, #-16]!	/* save x0, x1 */
	bl	_C_LABEL(lwp_startup)
	ldp	x0, x1, [sp], #16	/* restore x0, x1 */

#ifdef ARMV83_PAC
	/* Generate a new PAC key. */
	adrl	x4, _C_LABEL(aarch64_pac_enabled)
	ldr	w4, [x4]
	cbz	w4, 1f
	mov	x26, x1

	bl	_C_LABEL(cprng_strong64)
	mov	x25, x0
	bl	_C_LABEL(cprng_strong64)
	stp	x25, x0, [x26, #L_MD_IA_KERN]
	msr	APIAKeyLo_EL1, x25
	msr	APIAKeyHi_EL1, x0
	isb
1:
#endif

	/*
	 * When the x27 function returns, it will jump to el0_trap_exit.
	 */
	adr	lr, el0_trap_exit	/* tail call via lr */
	mov	x0, x28			/* mov arg into place */
	mov	x16, x27		/* use x16 as jump register, for BTI */
	br	x16			/* call function with arg */
END(lwp_trampoline)


#ifdef DDB
ENTRY_NP(cpu_Debugger)
	brk	#0xffff
	ret
END(cpu_Debugger)
#endif /* DDB */

/*
 * int cpu_set_onfault(struct faultbuf *fb)
 */
ENTRY_NP(cpu_set_onfault)
	mrs	x2, tpidr_el1		/* x2 = curlwp */
	str	x0, [x2, #L_MD_ONFAULT] /* l_md.md_onfault = fb */

	stp	x19, x20, [x0, #(FB_X19 * 8)]
	stp	x21, x22, [x0, #(FB_X21 * 8)]
	stp	x23, x24, [x0, #(FB_X23 * 8)]
	stp	x25, x26, [x0, #(FB_X25 * 8)]
	stp	x27, x28, [x0, #(FB_X27 * 8)]
	stp	x29, x30, [x0, #(FB_X29 * 8)]
	mov	x1, sp
	str	x1, [x0, #(FB_SP * 8)]
	mov	x0, #0
	ret
END(cpu_set_onfault)

/*
 * setjmp(9)
 * int setjmp(label_t *label);
 * void longjmp(label_t *label);
 */
ENTRY_NP(setjmp)
	stp	x19, x20, [x0, #(LBL_X19 * 8)]
	stp	x21, x22, [x0, #(LBL_X21 * 8)]
	stp	x23, x24, [x0, #(LBL_X23 * 8)]
	stp	x25, x26, [x0, #(LBL_X25 * 8)]
	stp	x27, x28, [x0, #(LBL_X27 * 8)]
	stp	x29, x30, [x0, #(LBL_X29 * 8)]
	mov	x1, sp
	str	x1, [x0, #(LBL_SP * 8)]
	mov	x0, #0
	ret
END(setjmp)

ENTRY_NP(longjmp)
	ldp	x19, x20, [x0, #(LBL_X19 * 8)]
	ldp	x21, x22, [x0, #(LBL_X21 * 8)]
	ldp	x23, x24, [x0, #(LBL_X23 * 8)]
	ldp	x25, x26, [x0, #(LBL_X25 * 8)]
	ldp	x27, x28, [x0, #(LBL_X27 * 8)]
	ldp	x29, x30, [x0, #(LBL_X29 * 8)]
	ldr	x1, [x0, #(LBL_SP * 8)]
	mov	sp, x1
	mov	x0, #1
	ret
END(longjmp)

ENTRY_NP(load_fpregs)
	ldp	q0, q1, [x0, #FPREG_Q0]
	ldp	q2, q3, [x0, #FPREG_Q2]
	ldp	q4, q5, [x0, #FPREG_Q4]
	ldp	q6, q7, [x0, #FPREG_Q6]
	ldp	q8, q9, [x0, #FPREG_Q8]
	ldp	q10, q11, [x0, #FPREG_Q10]
	ldp	q12, q13, [x0, #FPREG_Q12]
	ldp	q14, q15, [x0, #FPREG_Q14]
	ldp	q16, q17, [x0, #FPREG_Q16]
	ldp	q18, q19, [x0, #FPREG_Q18]
	ldp	q20, q21, [x0, #FPREG_Q20]
	ldp	q22, q23, [x0, #FPREG_Q22]
	ldp	q24, q25, [x0, #FPREG_Q24]
	ldp	q26, q27, [x0, #FPREG_Q26]
	ldp	q28, q29, [x0, #FPREG_Q28]
	ldp	q30, q31, [x0, #FPREG_Q30]
	ldr	w8, [x0, #FPREG_FPCR]
	ldr	w9, [x0, #FPREG_FPSR]
	msr	fpcr, x8
	msr	fpsr, x9
	ret
END(load_fpregs)

ENTRY_NP(save_fpregs)
	stp	q0, q1, [x0, #FPREG_Q0]
	stp	q2, q3, [x0, #FPREG_Q2]
	stp	q4, q5, [x0, #FPREG_Q4]
	stp	q6, q7, [x0, #FPREG_Q6]
	stp	q8, q9, [x0, #FPREG_Q8]
	stp	q10, q11, [x0, #FPREG_Q10]
	stp	q12, q13, [x0, #FPREG_Q12]
	stp	q14, q15, [x0, #FPREG_Q14]
	stp	q16, q17, [x0, #FPREG_Q16]
	stp	q18, q19, [x0, #FPREG_Q18]
	stp	q20, q21, [x0, #FPREG_Q20]
	stp	q22, q23, [x0, #FPREG_Q22]
	stp	q24, q25, [x0, #FPREG_Q24]
	stp	q26, q27, [x0, #FPREG_Q26]
	stp	q28, q29, [x0, #FPREG_Q28]
	stp	q30, q31, [x0, #FPREG_Q30]
	mrs	x8, fpcr
	mrs	x9, fpsr
	str	w8, [x0, #FPREG_FPCR]
	str	w9, [x0, #FPREG_FPSR]
	ret
END(save_fpregs)
