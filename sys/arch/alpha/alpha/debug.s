/* $NetBSD: debug.s,v 1.13 2020/09/18 00:04:58 thorpej Exp $ */

/*-
 * Copyright (c) 1999, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

__KERNEL_RCSID(6, "$NetBSD: debug.s,v 1.13 2020/09/18 00:04:58 thorpej Exp $")

#include "opt_multiprocessor.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"

/*
 * Debugger glue.
 */

	.text
inc6:	.stabs	__FILE__,132,0,0,inc6; .loc	1 __LINE__

/*
 * Debugger stack.
 */
#define	DEBUG_STACK_SIZE	8192
BSS(debug_stack_bottom, DEBUG_STACK_SIZE)

#define debug_stack_top		(debug_stack_bottom + DEBUG_STACK_SIZE)

/*
 * alpha_debug:
 *
 *	Single debugger entry point, handling the housekeeping
 *	chores we need to deal with.
 *
 *	Arguments are:
 *
 *		a0	a0 from trap
 *		a1	a1 from trap
 *		a2	a2 from trap
 *		a3	kernel trap entry point
 *		a4	frame pointer
 */
NESTED_NOPROFILE(alpha_debug, 5, 64, ra,
    IM_RA|IM_S0|IM_S1|IM_S2|IM_S3|IM_S4|IM_S5, 0)
	br	pv, 1f
1:	LDGP(pv)
	lda	t0, FRAME_SIZE*8(a4)	/* what would sp have been? */
	stq	t0, FRAME_SP*8(a4)	/* belatedly save sp for ddb view */
	lda	sp, -64(sp)		/* set up stack frame */
	stq	s0, (0*8)(sp)		/* save s0 ... */
	stq	s1, (1*8)(sp)
	stq	s2, (2*8)(sp)
	stq	s3, (3*8)(sp)
	stq	s4, (4*8)(sp)
	stq	s5, (5*8)(sp)		/* ... through s5 */
	stq	ra, (6*8)(sp)		/* save ra */

	/* Remember our current stack pointer. */
	mov	sp, s5

	/* Save off our arguments. */
	mov	a0, s0
	mov	a1, s1
	mov	a2, s2
	mov	a3, s3
	mov	a4, s4

#if defined(MULTIPROCESSOR)
	/* Pause all other CPUs. */
	ldiq	a0, 1
	CALL(cpu_pause_resume_all)
#endif

	/*
	 * Switch to the debug stack if we're not on it already.
	 */
	lda	t0, debug_stack_bottom
	cmpule	sp, t0, t1		/* sp <= debug_stack_bottom */
	bne	t1, 2f			/* yes, switch now */

	lda	t0, debug_stack_top
	cmpule	t0, sp, t1		/* debug_stack_top <= sp? */
	bne	t1, 3f			/* yes, we're on the debug stack */

2:	lda	sp, debug_stack_top	/* sp <- debug_stack_top */

3:	/* Dispatch to the debugger. */
#if defined(KGDB)
	mov	s3, a0			/* a0 == entry (trap type) */
	mov	s4, a1			/* a1 == frame pointer */
	CALL(kgdb_trap)
	br	9f
#endif
#if defined(DDB)
	mov	s0, a1			/* same arguments as the call */
	mov	s1, a1			/* to alpha_debug() */
	mov	s2, a2			/* (these may have been clobbered */
	mov	s3, a3			/* when pausing other CPUs.) */
	mov	s4, a4
	CALL(ddb_trap)
	br	9f
#endif
9:	/* Debugger return value in v0; switch back to our previous stack. */
	mov	s5, sp

#if defined(MULTIPROCESSOR)
	mov	v0, s0

	/* Resume all other CPUs. */
	mov	zero, a0
	CALL(cpu_pause_resume_all)

	mov	s0, v0
#endif

	ldq	s0, (0*8)(sp)		/* restore s0 ... */
	ldq	s1, (1*8)(sp)
	ldq	s2, (2*8)(sp)
	ldq	s3, (3*8)(sp)
	ldq	s4, (4*8)(sp)
	ldq	s5, (5*8)(sp)		/* ... through s5 */
	ldq	ra, (6*8)(sp)		/* restore ra */
	lda	sp, 64(sp)		/* pop stack frame */
	RET
	END(alpha_debug)
