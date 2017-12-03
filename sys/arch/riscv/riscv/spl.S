/* $NetBSD: spl.S,v 1.2.18.2 2017/12/03 11:36:39 jdolecek Exp $ */
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

__RCSID("$NetBSD: spl.S,v 1.2.18.2 2017/12/03 11:36:39 jdolecek Exp $")

	.data
	.globl	_C_LABEL(ipl_sr_map)
	.type	_C_LABEL(ipl_sr_map), @object
	.p2align INT_SCALESHIFT
_C_LABEL(ipl_sr_map):
	.word	0			/* IPL_NONE */
	.word	0			/* IPL_SOFTCLOCK */
	.word	0			/* IPL_SOFTBIO */
	.word	0			/* IPL_SOFTNET */
	.word	0			/* IPL_SOFTSERIAL */
	.word	SR_IM			/* IPL_VM */
	.word	SR_IM			/* IPL_SCHED */
	.word	SR_IM			/* IPL_DDB */
	.word	SR_IM			/* IPL_HIGH */

ENTRY_NP(splx)
	// a0 = new lower IPL
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	INT_L	t0, CI_CPL(a3)		// get current IPL
	beq	a0, t0, 2f
.L_splset:
	// a0 = new ipl
	PTR_LA	t0, ipl_sr_map
	slli	a1, a0, 2		// make integer index
	add	t0, t0, a1		// index into table
	INT_L	t0, (t0)		// get new mask bits to clear
	li	t2, SR_IM		// get mask bits
	xor	t0, t0, t2		// invert mask bits
	csrc	sstatus, t2		// block everything
	INT_S	a0, CI_CPL(a3)		// change IPL
	beqz	t0, 2f
	csrs	sstatus, t0		// unmask appropriate bits
2:	INT_L	t4, CI_SOFTINTS(a3)	// get softint mask
	srl	t4, t4, a0		// see what softints are pending.
	beqz	t4, 3f			// none, just return
	// there are softints that need to be delivered, so instead of
	// returning to the caller, we'll jump to softint_deliver and it
	// will do a tailcall back to splx and then we can return (if there
	// are no pending softints).
	tail	_C_LABEL(softint_deliver)
3:	ret				// return (or do softints)
END(splx)

#if IPL_NONE != 0
#error IPL_NONE is not 0
#endif
ENTRY_NP(spl0)
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	li	t0, SR_IM|SR_EI		// load SR_IM|EI
	csrci	sstatus, SR_EI		// disable interrupts
	INT_S	zero, CI_CPL(a3)	// set current IPL to IPL_NONE
	csrs	sstatus, t0		// unmask all & enable interrupts
	// spl0() is only called rarely so the overhead of always calling
	// softint_deliver is minimal.
	tail	_C_LABEL(softint_deliver)
END(spl0)

ENTRY_NP(splhigh)
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	INT_L	a0, CI_CPL(a3)		// get current IPL
	li	t1, SR_IM		// load SR_IM
	li	t0, IPL_HIGH		// 
	csrc	sstatus, t1		// mask all interrupts
	INT_S	t0, CI_CPL(a3)		// set it to IPL_HIGH
	ret
END(splhigh)

ENTRY_NP(splsoftclock)
	li	a0, IPL_SOFTCLOCK
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	j	.L_splset
END(splsoftclock)

ENTRY_NP(splsoftbio)
	li	a0, IPL_SOFTBIO
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	j	.L_splset
END(splsoftbio)

ENTRY_NP(splsoftnet)
	li	a0, IPL_SOFTNET
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	j	.L_splset
END(splsoftnet)

ENTRY_NP(splsoftserial)
	li	a0, IPL_SOFTSERIAL
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	j	.L_splset
END(splsoftserial)

ENTRY_NP(splvm)
	li	a0, IPL_VM
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	j	.L_splset
END(splvm)

ENTRY_NP(splsched)
	li	a0, IPL_SCHED
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	j	.L_splset
END(splsched)

ENTRY_NP(splddb)
	li	a0, IPL_DDB
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	j	.L_splset
END(splddb)

ENTRY_NP(splraise)
	mv	t0, a0			// need a0 for return value
	PTR_L	a3, L_CPU(tp)		// get curcpu()
	INT_L	a0, CI_CPL(a3)		// get current IPL
	bgt	t0, a0, .L_splset	// set if new IPL is higher
	ret
END(splraise)
