/* $NetBSD: multiproc.s,v 1.9.84.1 2007/07/11 19:57:22 mjf Exp $ */

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

__KERNEL_RCSID(5, "$NetBSD: multiproc.s,v 1.9.84.1 2007/07/11 19:57:22 mjf Exp $")

/*
 * Multiprocessor glue code.
 */

	.text
inc5:	.stabs	__FILE__,132,0,0,inc5; .loc	1 __LINE__

/*
 * cpu_spinup_trampoline:
 *
 * We come here via the secondary processor's console.  We simply
 * make the function call look right, and call cpu_hatch() to finish
 * starting up the processor.
 *
 * We are provided an argument in $27 (pv).  It will be a pointer to
 * our cpu_info.
 */
NESTED_NOPROFILE(cpu_spinup_trampoline,0,0,ra,0,0)
	mov	pv, s0			/* squirrel away argument */

	br	pv, 1f			/* compute new GP */
1:	LDGP(pv)

	/* Write new KGP. */
	mov	gp, a0
	call_pal PAL_OSF1_wrkgp

	/* Store our CPU info in SysValue. */
	mov	s0, a0
	call_pal PAL_OSF1_wrval

	/* Switch to this CPU's idle thread. */
	ldq	a0, CPU_INFO_IDLE_LWP(s0)
	stq	a0, CPU_INFO_CURLWP(s0)	/* set curlwp */
	ldq	a0, L_MD_PCBPADDR(a0)
	SWITCH_CONTEXT

	/* Invalidate TLB and I-stream. */
	ldiq	a0, -2			/* TBIA */
	call_pal PAL_OSF1_tbi
	call_pal PAL_imb

	/* Make sure the FPU is turned off. */
	mov	zero, a0
	call_pal PAL_OSF1_wrfen

	/* Restore argument and call cpu_hatch() */
	mov	s0, a0
	CALL(cpu_hatch)

	/* enable all interrupts */
	mov	zero, a0
	call_pal PAL_OSF1_swpipl
	/* Jump into the idle loop! */
	jmp	zero, idle_loop

	END(cpu_spinup_trampoline)
