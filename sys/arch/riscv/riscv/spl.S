/* $NetBSD: spl.S,v 1.8 2023/06/12 19:04:14 skrll Exp $ */

/*-
 * Copyright (c) 2014,2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry, and Nick Hudson.
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

__RCSID("$NetBSD: spl.S,v 1.8 2023/06/12 19:04:14 skrll Exp $")

#define SZINT	(1 << INT_SCALESHIFT)

	.data
	.globl	_C_LABEL(ipl_sie_map)
	.type	_C_LABEL(ipl_sie_map), @object
	.p2align INT_SCALESHIFT
_C_LABEL(ipl_sie_map):
	.word	0					/* IPL_NONE */
	.word	0					/* IPL_SOFTCLOCK */
	.word	0					/* IPL_SOFTBIO */
	.word	0					/* IPL_SOFTNET */
	.word	0					/* IPL_SOFTSERIAL */
	.word	SIE_SEIE				/* IPL_VM */
	.word	SIE_SEIE | SIE_STIE			/* IPL_SCHED */
	.word	SIE_SEIE | SIE_STIE | SIE_SSIE		/* IPL_HIGH */


ENTRY_NP(splx)
	// a0 = new IPL
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	INT_L	t0, CI_CPL(a3)		// get current IPL
	bge	a0, t0, 2f

	sll	t2, a0, INT_SCALESHIFT
	PTR_LA	a1, _C_LABEL(ipl_sie_map)
	add	a1, a1, t2
	INT_L	a1, 0(a1)

	li	t2, (SIE_SEIE | SIE_STIE | SIE_SSIE)
	xor	a1, a1, t2

	// a0 = new ipl
	INT_S	a0, CI_CPL(a3)		// change IPL
	csrs	sie, a1

#ifdef __HAVE_FAST_SOFTINTS
	INT_L	t4, CI_SOFTINTS(a3)	// get softint mask
	srl	t4, t4, a0		// see what softints are pending.
	beqz	t4, 3f			// none, just return
	// there are softints that need to be delivered, so instead of
	// returning to the caller, we'll jump to softint_deliver and it
	// will do a tailcall back to splx and then we can return (if there
	// are no pending softints).
	tail	_C_LABEL(softint_deliver)
3:
#endif /* __HAVE_FAST_SOFTINTS */
2:
	ret				// return (or do softints)
END(splx)


#if IPL_NONE != 0
#error IPL_NONE is not 0
#endif
ENTRY_NP(spl0)
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	INT_S	zero, CI_CPL(a3)	// set current IPL to IPL_NONE
	li	t2, (SIE_SEIE | SIE_STIE | SIE_SSIE)
	csrs	sie, t2

	csrsi	sstatus, SR_SIE		// enable interrupts
#ifdef __HAVE_FAST_SOFTINTS
	// spl0() is only called rarely so the overhead of always calling
	// softint_deliver is minimal.
	tail	_C_LABEL(softint_deliver)
#else /* __HAVE_FAST_SOFTINTS */
	ret
#endif /* __HAVE_FAST_SOFTINTS */
END(spl0)


ENTRY(splintr)
	csrr	 t0, sip

	li	 t1, IPL_NONE
	andi	 t0, t0, (SIP_SEIP | SIP_STIP | SIP_SSIP)
	beq	 t0, zero, 1f	// If nothing is pending return IPL_NONE

	PTR_LA	 t3, _C_LABEL(ipl_sie_map)

	li	 t1, IPL_VM
	li	 t2, IPL_HIGH + 1
2:
	INT_L	 t5, IPL_VM * (1 << INT_SCALESHIFT)(t3)
	PTR_ADDI t3, t3, 1 << INT_SCALESHIFT
	not	 t5, t5
	and	 t5, t5, t0
	beq	 t5, zero, 1f
	addi	 t1, t1, 1
	bne	 t1, t2, 2b

1:
	LONG_S	 t0, 0(a0)
	mv	 a0, t1
	jr	 ra
END(splintr)


ENTRY_NP(splsoftclock)
	li	t1, IPL_SOFTCLOCK
	INT_L	t0, _C_LABEL(ipl_sie_map) + SZINT * IPL_SOFTCLOCK
	j	_splraise
END(splsoftclock)


ENTRY_NP(splsoftbio)
	li	t1, IPL_SOFTBIO
	INT_L	t0, _C_LABEL(ipl_sie_map) + SZINT * IPL_SOFTBIO
	j	_splraise
END(splsoftbio)


ENTRY_NP(splsoftnet)
	li	t1, IPL_SOFTNET
	INT_L	t0, _C_LABEL(ipl_sie_map) + SZINT * IPL_SOFTNET
	j	_splraise
END(splsoftnet)


ENTRY_NP(splsoftserial)
	li	t1, IPL_SOFTSERIAL
	INT_L	t0, _C_LABEL(ipl_sie_map) + SZINT * IPL_SOFTSERIAL
	j	_splraise
END(splsoftserial)


ENTRY_NP(splvm)
	li	t1, IPL_VM
	INT_L	t0, _C_LABEL(ipl_sie_map) + SZINT * IPL_VM
	j	_splraise
END(splvm)


ENTRY_NP(splsched)
	li	t1, IPL_SCHED
	INT_L	t0, _C_LABEL(ipl_sie_map) + SZINT * IPL_SCHED
	j	_splraise
END(splsched)

ENTRY_NP(splhigh)
	li	t1, IPL_HIGH
	INT_L	t0, _C_LABEL(ipl_sie_map) + SZINT * IPL_HIGH
	j	_splraise
END(splhigh)


ENTRY_NP(splraise)
	// a0 = new higher IPL
	mv	t1, a0
	sll	t2, a0, INT_SCALESHIFT
	PTR_LA	a1, _C_LABEL(ipl_sie_map)
	add	a1, a1, t2
	INT_L	t0, 0(a1)

_splraise:
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	INT_L	a0, CI_CPL(a3)		// get current IPL
	bge	a0, t1, 2f		// already at same or higher?
	csrc	sie, t0			//
	INT_S	t1, CI_CPL(a3)		// change IPL
2:
	ret
END(splraise)
