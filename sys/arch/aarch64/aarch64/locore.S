/* $NetBSD: locore.S,v 1.1.4.3 2017/12/03 11:35:43 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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
#include "assym.h"

#include "opt_ddb.h"

RCSID("$NetBSD: locore.S,v 1.1.4.3 2017/12/03 11:35:43 jdolecek Exp $")

// XXX:AARCH64
lr	.req	x30
	.macro DISABLE_INTERRUPT
	msr	daifset, #DAIF_I|DAIF_F		/* daif'set */
	.endm
	.macro ENABLE_INTERRUPT
	msr	daifclr, #DAIF_I|DAIF_F		/* daif'clr */
	.endm

/*
 * At IPL_SCHED:
 *	x0 = oldlwp (maybe be NULL)
 *	x1 = newlwp
 *	x2 = returning
 * returns x0-x2 unchanged
 */
ENTRY_NP(cpu_switchto)
	cbz	x0, .Lrestore_lwp

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

	/*
	 * Save the previous trapframe pointer and EL0 thread ID in the
	 * switchframe.
	 */
	ldr	x5, [x0, #L_MD_KTF]
	mrs	x4, tpidr_el0
#if TF_TPIDR + 8 == TF_CHAIN
	stp	x4, x5, [sp, #TF_TPIDR]
#else
	str	x4, [sp, #TF_TPIDR]
	str	x5, [sp, #TF_CHAIN]
#endif

	/*
	 * Save the current stack pointer and the CPACR and save them in
	 * old lwp md area.
	 */
	mov	x4, sp 
	mrs	x5, cpacr_el1
#if L_MD_KTF + 8 == L_MD_CPACR
	stp	x4, x5, [x0, #L_MD_KTF]
#else
	str	x4, [x0, #L_MD_KTF]
	str	x5, [x0, #L_MD_CPACR]
#endif

	/* We are done with the old lwp */

.Lrestore_lwp:
#if L_MD_KTF + 8 == L_MD_CPACR
	ldp	x4, x5, [x1, #L_MD_KTF]	/* get trapframe ptr and cpacr_el1 */
#else
	ldr	x4, [x1, #L_MD_KTF]	/* get trapframe ptr (aka SP) */
	ldr	x5, [x1, #L_MD_CPACR]	/* get cpacr_el1 */
#endif
	mov	sp, x4			/* restore stack pointer */
	msr	cpacr_el1, x5		/* restore cpacr_el1 */

	ldr	x4, [sp, #TF_TPIDR]
	msr	tpidr_el0, x4		/* restore EL0 thread ID */

	mrs	x3, tpidr_el1
	str	x1, [x3, #CI_CURLWP]	/* switch curlwp to new lwp */

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
	sub	sp, sp, #TF_SIZE	/* make switchframe */
	adr	x2, softint_cleanup
	stp	x19, x20, [sp, #TF_X19]
	stp	x21, x22, [sp, #TF_X21]
	stp	x23, x24, [sp, #TF_X23]
	stp	x25, x26, [sp, #TF_X25]
	stp	x27, x28, [sp, #TF_X27]
	stp	x29, x2, [sp, #TF_X29]	/* tf->lr = softint_cleanup; */

	mrs	x3, tpidr_el1
	ldr	x2, [x3, #CI_CURLWP]	/* x2 := curcpu()->ci_curlwp */
	mov	x4, sp			/* x4 := sp */
	DISABLE_INTERRUPT
	str	x4, [x2, #L_MD_KTF]	/* curlwp->l_md_ktf := sp */
	str	x0, [x3, #CI_CURLWP]	/* curcpu()->ci_curlwp = softlwp; */
	ldr	x4, [x0, #L_MD_KTF]	/* switch to softlwp stack */
	mov	sp, x4			/* new sp := softlwp->l_md_ktf */
	ENABLE_INTERRUPT
	mov	x19, x2			/* x19 := pinned_lwp */
	mov	x20, lr			/* x20 := original lr */

	/* softint_dispatch(pinned_lwp, ipl) */
	mov	x0, x19
	bl	_C_LABEL(softint_dispatch)

	mrs	x3, tpidr_el1
	DISABLE_INTERRUPT
	str	x19, [x3, #CI_CURLWP]	/* curcpu()->ci_curlwp := x19 */
	ldr	x4, [x19, #L_MD_KTF]	/* x4 := curlwp->l_md_ktf */
	mov	sp, x4			/* restore pinned_lwp sp */
	ENABLE_INTERRUPT
	mov	lr, x20			/* restore pinned_lwp lr */
	ldp	x19, x20, [sp, #TF_X19]	/* restore x19 and x20 */
	add	sp, sp, #TF_SIZE	/* unwind switchframe */
	ret
END(cpu_switchto_softint)

/*
 * void
 * softint_cleanup(struct lwp *softlwp)
 * {
 *	cpu_switchto() bottom half arranges to start this when softlwp.
 *	kernel thread is to yield CPU for the pinned_lwp in the above.
 *	curcpu()->ci_mtx_count += 1;
 *	softlwp->l_ctxswtch = 0;
 *	this returns as if cpu_switchto_softint finished normally.
 * }
 */
ENTRY_NP(softint_cleanup)
	mrs	x3, tpidr_el1		/* curcpu() */
	ldr	w2, [x3, #CI_MTX_COUNT]	/* ->ci_mtx_count */
	add	w2, w2, #1
	str	w2, [x3, #CI_MTX_COUNT]
	str	wzr, [x0, #L_CTXSWTCH]	/* softlwp->l_ctxswtch = 0 */
	add	sp, sp, #TF_SIZE	/* unwind switchframe */
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
#if defined(MULTIPROCESSOR)
	mov	x19, x0
	mov	x20, x1
	bl	_C_LABEL(proc_trampoline_mp)
	mov	x1, x20
	mov	x0, x19
#endif
	bl	_C_LABEL(lwp_startup)

	/*
	 * If the function returns, have it return to the exception trap return
	 * handler which will restore all user state before returning to EL0.
	 */
	adr	x30, exception_trap_exit	// set function return address
	mov	x0, x28				// mov arg into place
	br	x27				// call function with arg
END(lwp_trampoline)

/*
 * Return from exception.  There's a trap return, an intr return, and
 * a syscall return.
 */
ENTRY_NP(exception_trap_exit)
	/* XXX critial section guarded by SR.EXL if it was MIPS XXX */
	ldp	x0, x1, [sp, #TF_X0]
	ldp	x2, x3, [sp, #TF_X2]
	ldp	x4, x5, [sp, #TF_X4]
	ldp	x6, x7, [sp, #TF_X6]
	ldp	x8, x9, [sp, #TF_X8]
	ldp	x10, x11, [sp, #TF_X10]
	ldp	x12, x13, [sp, #TF_X12]
	ldp	x14, x15, [sp, #TF_X14]
	ldp	x16, x17, [sp, #TF_X16]
	ldr	x18, [sp, #TF_X18]

	ldr	x20, [sp, #TF_PC]
	ldr	x21, [sp, #TF_SPSR]
	msr	elr_el1, x20		/* exception pc */
	msr	spsr_el1, x21		/* exception pstate */

	and	x21, x21, #1
	cbz	x21, .Lkernelexception
	ldr	x22, [sp, #TF_SP]
	msr	sp_el0, x22		/* restore EL0 stack */

	ldp	x19, x20, [sp, #TF_X19]
	ldp	x21, x22, [sp, #TF_X21]
	ldp	x23, x24, [sp, #TF_X23]
	ldp	x25, x26, [sp, #TF_X25]
	ldp	x27, x28, [sp, #TF_X27]
	ldp	x29, x30, [sp, #TF_X29]
	/* EL1 sp stays at l_md_utf */
	eret
 .Lkernelexception:
	ldp	x19, x20, [sp, #TF_X19]
	ldp	x21, x22, [sp, #TF_X21]
	ldp	x23, x24, [sp, #TF_X23]
	ldp	x25, x26, [sp, #TF_X25]
	ldp	x27, x28, [sp, #TF_X27]
	ldp	x29, x30, [sp, #TF_X29]
	add	sp, sp, #TF_SIZE	/* unwind trapframe on stack */
	eret
END(exception_trap_exit)

#ifdef DDB
ENTRY(cpu_Debugger)
	brk	#0xffff
	ret
END(cpu_Debugger)
#endif /* DDB */

#ifdef MULTIPROCESSOR
/*
 * void
 * cpu_spinup_trampoline(int cpu_index)
 * {
 *      ci := tp == cpu_info[cpu_index]
 *      ci->ci_curlwp = ci->ci_data.ci_idlelwp;
 *      sp := ci->ci_curlwp->l_addr + USPACE - sizeof(struct trapframe)
 *      cpu_hatch(ci);
 *      jump to idle_loop() to join the cpu pool.
 * }
 */
ENTRY(cpu_spinup_trampoline)
	bl	_C_LABEL(cpu_hatch)
	b	_C_LABEL(cpu_idle)
END(cpu_spinup_trampoline)
#endif

/*
 * int cpu_set_onfault(struct faultbuf *fb, register_t retval)
 */
ENTRY(cpu_set_onfault)
	mov	x9, sp
	stp	x19, x20, [x0, #0]
	stp	x21, x22, [x0, #16]
	stp	x23, x24, [x0, #32]
	stp	x25, x26, [x0, #48]
	stp	x27, x28, [x0, #64]
	stp	x29, lr, [x0, #80]
	stp	x9, x1, [x0, #96]
	mrs	x3, tpidr_el1
	ldr	x2, [x3, #CI_CURLWP]	/* curlwp = curcpu()->ci_curlwp */
	str	x0, [x2, #L_MD_ONFAULT] /* l_md.md_onfault = fb */
	mov	x0, #0
END(cpu_set_onfault)

/*
 * setjmp(9)
 * int setjmp(label_t *label);
 * void longjmp(label_t *label);
 */
ENTRY(setjmp)
	stp	x19, x20, [x0, #0]
	stp	x21, x22, [x0, #16]
	stp	x23, x24, [x0, #32]
	stp	x25, x26, [x0, #48]
	stp	x27, x28, [x0, #64]
	stp	x29, x30, [x0, #80]
	mov	x1, sp
	str	x1, [x0, #96]
	mov	x0, #0
	ret
END(setjmp)

ENTRY(longjmp)
	ldp	x19, x20, [x0, #0]
	ldp	x21, x22, [x0, #16]
	ldp	x23, x24, [x0, #32]
	ldp	x25, x26, [x0, #48]
	ldp	x27, x28, [x0, #64]
	ldp	x29, x30, [x0, #80]
	ldr	x1, [x0, #96]
	mov	sp, x1
	mov	x0, #1
	ret
END(longjmp)
