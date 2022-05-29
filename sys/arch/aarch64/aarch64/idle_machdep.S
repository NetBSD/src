/* $NetBSD: idle_machdep.S,v 1.12 2022/05/29 23:39:59 ryo Exp $ */

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

#include "opt_arm_intr_impl.h"
#include "opt_ddb.h"
#include "opt_gprof.h"
#include <aarch64/asm.h>
#include <aarch64/locore.h>
#include "assym.h"

RCSID("$NetBSD: idle_machdep.S,v 1.12 2022/05/29 23:39:59 ryo Exp $");

#ifdef ARM_INTR_IMPL
#include ARM_INTR_IMPL
#else
#error ARM_INTR_IMPL not defined
#endif

#ifndef ARM_IRQ_HANDLER
#error ARM_IRQ_HANDLER not defined
#endif


ENTRY(cpu_idle)
#ifdef LAZY_CPUIDLE

	/*
	 * hardware interrupt -> trap handler -> interrupt handler
	 */
	dsb	sy
	wfi

#else /* LAZY_CPUIDLE */

	/*
	 * hardware interrupt -> interrupt handler
	 */
	sub	sp, sp, #TF_SIZE		/* allocate trapframe */
	str	x28, [sp, #TF_X28]		/* save x28 */
	stp	x29, x30, [sp, #TF_X29]		/* save x29,x30 */
#ifdef DDB
	add	x29, sp, #TF_X29		/* link frame for backtrace */
	mov	x0, #-1
	str	x0, [sp, #TF_ESR]
	str	xzr, [sp, #TF_FAR]
#endif

	/* fill the minimum required trapframe */
	mov	x2, #SPSR_M_EL1H		/* what our spsr should be */
	str	x2, [sp, #TF_SPSR]
	adr	x0, 1f
	str	x0, [sp, #TF_PC]		/* CLKF_PC refer to tf_pc */

	/*
	 * "idle/N" lwp is allocated on a per-CPU basis,
	 * curcpu() always return the same, and there is no need to
	 * consider KPREEMPT. safe even with interrupt enabled.
	 */
	mrs	x1, tpidr_el1			/* get curlwp */
	ldr	x28, [x1, #L_CPU]		/* get curcpu */

	mov	w2, #1
	mov	x0, sp				/* get pointer to trapframe */

	DISABLE_INTERRUPT
	/*
	 * assert(ci->ci_intr_depth == 0),
	 * therefore, ci->ci_intr_depth++ would be definitely 1.
	 */
	str	w2, [x28, #CI_INTR_DEPTH]	/* ci->ci_intr_depth = 1 */

	dsb	sy
	wfi
	bl	ARM_IRQ_HANDLER			/* irqhandler(trapframe) */
1:
	/* x28 is curcpu() */
	str	wzr, [x28, #CI_INTR_DEPTH]	/* ci->ci_intr_depth = 0 */

#if defined(__HAVE_FAST_SOFTINTS) && !defined(__HAVE_PIC_FAST_SOFTINTS)
	ldr	w3, [x28, #CI_SOFTINTS]		/* Get pending softint mask */
	/* CPL should be 0 */
	ldr	w2, [x28, #CI_CPL]		/* Get current priority level */
	lsr	w3, w3, w2			/* shift mask by cpl */
	cbz	w3, 1f
	bl	_C_LABEL(dosoftints)		/* dosoftints() */
1:
#endif /* __HAVE_FAST_SOFTINTS && !__HAVE_PIC_FAST_SOFTINTS */

	ldr	x28, [sp, #TF_X28]		/* restore x28 */
	ldp	x29, x30, [sp, #TF_X29]		/* restore x29,x30 */
	add	sp, sp, #TF_SIZE		/* pop trapframe */

	ENABLE_INTERRUPT
#endif /* LAZY_CPUIDLE */

	ret
END(cpu_idle)
