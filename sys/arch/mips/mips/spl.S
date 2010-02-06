/*	$NetBSD: spl.S,v 1.1.2.1 2010/02/06 14:43:15 matt Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Digital Equipment Corporation and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/loMem.s,
 *	v 1.1 89/07/11 17:55:04 nelson Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAsm.s,
 *	v 9.2 90/01/29 18:00:39 shirriff Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/vm/ds3100.md/vmPmaxAsm.s,
 *	v 1.1 89/07/10 14:27:41 nelson Exp  SPRITE (DECWRL)
 *
 *	@(#)locore.s	8.5 (Berkeley) 1/4/94
 */

#include "opt_cputype.h"	/* which mips CPU levels do we support? */

#include <sys/cdefs.h>

#include <machine/param.h>
#include <mips/asm.h>
#include <mips/cpuregs.h>

RCSID("$NetBSD: spl.S,v 1.1.2.1 2010/02/06 14:43:15 matt Exp $")

#include "assym.h"

/*
 * MIPS processor interrupt control
 *
 * Used as building blocks for spl(9) kernel interface.
 */
LEAF(_splraise)
XLEAF(_splraise_noprof)				# does not get mcount hooks
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	and	a0, a0, MIPS_INT_MASK		# extract INT bits
	nor	a0, zero, a0			# bitwise inverse of A0
	and	a0, a0, v0			# disable retaining other bits
	DYNAMIC_STATUS_MASK(a0,t0)		# machine dependent masking
	mtc0	a0, MIPS_COP_0_STATUS		# store back
	COP0_SYNC
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	j	ra
	nop
END(_splraise)

LEAF(_spllower)
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	li	v1, ~MIPS_INT_MASK
	and	v1, v0, v1			# turn off INT bit
	nor	a0, zero, a0			# bitwise inverse of A0
	and	a0, a0, MIPS_INT_MASK		# extract INT bits
	or	a0, a0, v1			# disable making other bits on
	DYNAMIC_STATUS_MASK(a0,t0)		# machine dependent masking
	mtc0	a0, MIPS_COP_0_STATUS		# store back
	COP0_SYNC
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	j	ra
	nop
END(_spllower)

LEAF(_splrestore)
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	and	a0, a0, MIPS_INT_MASK
	li	v1, ~MIPS_INT_MASK
	and	v1, v1, v0			# turn off every INT bit
	or	v1, v1, a0			# set old INT bits
	DYNAMIC_STATUS_MASK(v1,t0)		# machine dependent masking
	mtc0	v1, MIPS_COP_0_STATUS		# store back
	COP0_SYNC
	and	v0, v0, MIPS_INT_MASK
	j	ra
	nop
END(_splrestore)

LEAF(_splset)
XLEAF(_splset_noprof)				# does not get mcount hooks
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	and	a0, a0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	li	v1, ~(MIPS_INT_MASK | MIPS_SR_INT_IE)
	and	v1, v1, v0			# turn off every INT bit
	or	v1, v1, a0			# set old INT bits
	DYNAMIC_STATUS_MASK(v1,t0)		# machine dependent masking
	mtc0	v1, MIPS_COP_0_STATUS		# store back
	COP0_SYNC
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	j	ra
	nop
END(_splset)

LEAF(_splget)
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	j	ra
	nop
END(_splget)

LEAF(_setsoftintr)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
#if 0
	li	a3, ~MIPS_SR_INT_IE
	and	a3, v1
	mtc0	a3, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
#else
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
#endif
	COP0_SYNC
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE		# fetch cause register
	nop
	or	v0, v0, a0			# set soft intr. bits
	mtc0	v0, MIPS_COP_0_CAUSE		# store back
	COP0_SYNC
	mtc0	v1, MIPS_COP_0_STATUS		# enable interrupts
	COP0_SYNC
	j	ra
	nop
END(_setsoftintr)

LEAF(_clrsoftintr)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
#if 0
	li	a3, ~MIPS_SR_INT_IE
	and	a3, v1
	mtc0	a3, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
#else
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
#endif
	COP0_SYNC
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE		# fetch cause register
	nor	a0, zero, a0			# bitwise inverse of A0
	and	v0, v0, a0			# clear soft intr. bits
	mtc0	v0, MIPS_COP_0_CAUSE		# store back
	COP0_SYNC
	mtc0	v1, MIPS_COP_0_STATUS		# enable interrupts
	COP0_SYNC
	j	ra
	nop
END(_clrsoftintr)

LEAF(_splnone)
	mtc0	zero, MIPS_COP_0_CAUSE		# clear SOFT_INT bits
	COP0_SYNC
#ifdef _LP64
	li	v0, (MIPS_INT_MASK | MIPS_SR_INT_IE | MIPS_SR_KX)
#else
	li	v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
#endif
	DYNAMIC_STATUS_MASK(v0,t0)		# machine dependent masking
	mtc0	v0, MIPS_COP_0_STATUS		# enable all sources
	COP0_SYNC
	nop
	j	ra
	nop
END(_splnone)

	.rdata
	.globl _C_LABEL(stdsplsw)
_C_LABEL(splsw):
        PTR_WORD _C_LABEL(_splraise)
        PTR_WORD _C_LABEL(_splset)
        PTR_WORD _C_LABEL(_splget)
        PTR_WORD _C_LABEL(_spllower)
        PTR_WORD _C_LABEL(_splnone)
        PTR_WORD _C_LABEL(_splraise_noprof)
        PTR_WORD _C_LABEL(_splset_noprof)
	PTR_WORD _C_LABEL(_setsoftintr)
	PTR_WORD _C_LABEL(_clrsoftintr)
