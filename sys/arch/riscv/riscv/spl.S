/* $NetBSD: spl.S,v 1.5 2020/11/10 06:58:46 skrll Exp $ */

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

#include <machine/asm.h>
#include "assym.h"

__RCSID("$NetBSD: spl.S,v 1.5 2020/11/10 06:58:46 skrll Exp $")

#if notyet
	.data
	.globl	_C_LABEL(ipl_si_map)
	.type	_C_LABEL(ipl_si_map), @object
	.p2align INT_SCALESHIFT
_C_LABEL(ipl_si_map):
	.word	0					/* IPL_NONE */
	.word	0					/* IPL_SOFTCLOCK */
	.word	0					/* IPL_SOFTBIO */
	.word	0					/* IPL_SOFTNET */
	.word	0					/* IPL_SOFTSERIAL */
	.word	SI_EXTERAL				/* IPL_VM */
	.word	SI_EXTERNAL | SI_TIMER			/* IPL_SCHED */
	.word	SI_EXTERNAL | SI_SOFTWARE | SI_TIMER	/* IPL_HIGH */
#endif

ENTRY_NP(splx)
	// a0 = new lower IPL
	csrci	sstatus, SR_SIE		// disable interrupts
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	INT_L	t0, CI_CPL(a3)		// get current IPL
	beq	a0, t0, 2f

	// a0 = new ipl
	INT_S	a0, CI_CPL(a3)		// change IPL
	beqz	t0, 2f

	//call riscv_do_pending_irqs
2:
	csrsi	sstatus, SR_SIE		// enable interrupts

	INT_L	t4, CI_SOFTINTS(a3)	// get softint mask
	srl	t4, t4, a0		// see what softints are pending.
	beqz	t4, 3f			// none, just return
	// there are softints that need to be delivered, so instead of
	// returning to the caller, we'll jump to softint_deliver and it
	// will do a tailcall back to splx and then we can return (if there
	// are no pending softints).
	tail	_C_LABEL(softint_deliver)
3:
	ret				// return (or do softints)
END(splx)

#if IPL_NONE != 0
#error IPL_NONE is not 0
#endif
ENTRY_NP(spl0)
	csrci	sstatus, SR_SIE		// disable interrupts
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	INT_S	zero, CI_CPL(a3)	// set current IPL to IPL_NONE

	//call riscv_do_pending_irqs

	csrsi	sstatus, SR_SIE		// enable interrupts
	// spl0() is only called rarely so the overhead of always calling
	// softint_deliver is minimal.
	tail	_C_LABEL(softint_deliver)
END(spl0)

ENTRY_NP(splhigh)
	csrci	sstatus, SR_SIE		// disable interrupts
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	INT_L	a0, CI_CPL(a3)		// get current IPL for return value
	li	t0, IPL_HIGH		//
	INT_S	t0, CI_CPL(a3)		// set it to IPL_HIGH
					// interrupts remain disabled???
	ret
END(splhigh)


ENTRY_NP(splsoftclock)
	li	t0, IPL_SOFTCLOCK
	j	_splraise
END(splsoftclock)

ENTRY_NP(splsoftbio)
	li	t0, IPL_SOFTBIO
	j	_splraise
END(splsoftbio)

ENTRY_NP(splsoftnet)
	li	t0, IPL_SOFTNET
	j	_splraise
END(splsoftnet)

ENTRY_NP(splsoftserial)
	li	t0, IPL_SOFTSERIAL
	j	_splraise
END(splsoftserial)

ENTRY_NP(splvm)
	li	t0, IPL_VM
	j	_splraise
END(splvm)

ENTRY_NP(splsched)
	li	t0, IPL_SCHED
	j	_splraise
END(splsched)

ENTRY_NP(splddb)
	li	t0, IPL_DDB
	j	_splraise
END(splddb)

ENTRY_NP(splraise)
	// a0 = new higher IPL
	mv	t0, a0			// need a0 for return value
_splraise:
	csrci	sstatus, SR_SIE		// disable interrupts
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	INT_L	a0, CI_CPL(a3)		// get current IPL
	bge	a0, t0, 2f		// already at same or higher?
	INT_S	t0, CI_CPL(a3)		// change IPL
2:
	csrsi	sstatus, SR_SIE		// enable interrupts
	ret
END(splraise)
