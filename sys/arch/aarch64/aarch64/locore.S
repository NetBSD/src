/* $NetBSD: locore.S,v 1.1 2014/08/10 05:47:37 matt Exp $ */

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

RCSID("$NetBSD: locore.S,v 1.1 2014/08/10 05:47:37 matt Exp $")

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
	 * Store the callee saved register on the stack in a trapframe
	 */
	sub	sp, sp, #TF_SIZE
	stp	x19, x20, [sp, #TF_X19]
	stp	x21, x22, [sp, #TF_X21]
	stp	x23, x24, [sp, #TF_X23]
	stp	x25, x26, [sp, #TF_X25]
	stp	x27, x28, [sp, #TF_X27]
	stp	x29, x30, [sp, #TF_X29]

	/*
	 * Get the previous trapframe pointer and the user writeable Thread ID
	 * register and save them in the trap frame.
	 */
	ldr	x5, [x0, #L_MD_KTF]
	mrs	x4, tpidr_el0
#if TF_TPIDR + 8 == TF_CHAIN
	str	x4, x5, [sp, #TF_TPIDR]
#else
	str	x4, [sp, #TF_TPIDR]
	str	x5, [sp, #TF_CHAIN]
#endif

	/*
	 * Get the current stack pointer and the CPACR and save them in
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
	ldp	x4, x5, [x1, #L_MD_KTF]	// get trapframe ptr and cpacr_el1
#else
	ldr	x4, [x0, #L_MD_KTF]	// get trapframe ptr (aka SP)
	ldr	x5, [x0, #L_MD_CPACR]	// get cpacr_el1
#endif
	mov	sp, x4			// restore stack pointer
	msr	cpacr_el1, x5		// restore cpacr_el1

	ldr	x4, [sp, #TF_TPIDR]	// load user writeable thread ip reg
	msr	tpidr_el0, x4		// restore it

	mrs	x3, tpidr_el1		// get curcpu
	str	x1, [x3, #CI_CURLWP]	// show as curlwp

	/*
	 * Restore callee save registers
	 */
	ldp	x19, x20, [sp, #TF_X19]
	ldp	x21, x22, [sp, #TF_X21]
	ldp	x23, x24, [sp, #TF_X23]
	ldp	x25, x26, [sp, #TF_X25]
	ldp	x27, x28, [sp, #TF_X27]
	ldp	x29, x30, [sp, #TF_X29]
	add	sp, sp, #TF_SIZE	/* pop trapframe from stack */

	ret
END(cpu_switchto)

/*
 * Called at IPL_SCHED
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
	ldp	x0, x1, [sp, #TF_X0]
	ldp	x2, x3, [sp, #TF_X2]
	ldp	x4, x5, [sp, #TF_X4]
	ldp	x6, x7, [sp, #TF_X6]
	ldp	x8, x9, [sp, #TF_X8]
	ldp	x10, x11, [sp, #TF_X10]
	ldp	x12, x13, [sp, #TF_X12]
	ldp	x14, x15, [sp, #TF_X14]
exception_syscall_exit:
	ldp	x16, x17, [sp, #TF_X16]
	ldr	x18, [sp, #TF_X18]

#if TF_SP + 8 == TF_PC
	ldp	x20, x21, [sp, #TF_SP]
#else
	ldr	x20, [sp, #TF_SP]
	ldr	x21, [sp, #TF_PC]
#endif
	ldr	x22, [sp, #TF_SPSR]
	msr	sp_el0, x20
	msr	elr_el1, x21
	msr	spsr_el1, x22

	ldp	x19, x20, [sp, #TF_X19]
	ldp	x21, x22, [sp, #TF_X21]
	ldp	x23, x24, [sp, #TF_X23]
	ldp	x25, x26, [sp, #TF_X25]
	ldp	x27, x28, [sp, #TF_X27]
	ldp	x29, x30, [sp, #TF_X29]

	/*
	 * Don't adjust the stack for the trapframe since we would
	 * just add subtract it again upon exception entry.
	 */
	eret
END(exception_trap_exit)

#ifdef DDB
ENTRY(cpu_Debugger)
	brk	#0xffff
	ret
END(cpu_Debugger)
#endif /* DDB */

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
