/*	$NetBSD: locore_machdep.S,v 1.25 2011/03/28 14:40:00 tsutsui Exp $	*/

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

/*
 * DECstation-specific mips locore code.
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>	/* XXX - misnomer? */

#include "assym.h"
#include "sii.h"
#include "opt_dec_5100.h"
#include "opt_dec_5400.h"

	.set	noreorder

#if NSII > 0
/*
 * Copy data to a DMA buffer padded with 16 bits of data, 16
 * bits of padding per 32bit word (e.g., for pmin/pmax sii DMA).
 *
 * The DMA bufffer can only be written one short at a time
 * (and takes ~14 cycles).
 *
 *	CopyToBuffer(src, dst, length)
 *		u_short *src;	NOTE: must be short aligned
 *		u_short *dst;
 *		int length;
 */
LEAF(CopyToBuffer)
	blez	a2, 2f
	nop
1:
	lhu	t0, 0(a0)		# read 2 bytes of data
	PTR_SUBU a2, 2
	PTR_ADDU a0, 2
	PTR_ADDU a1, 4
	bgtz	a2, 1b
	sh	t0, -4(a1)		# write 2 bytes of data to buffer
2:
	j	ra
	nop
END(CopyToBuffer)

/*
 * Copy data from the DMA buffer.
 * The DMA bufffer can only be read one short at a time
 * (and takes ~12 cycles).
 *
 *	CopyFromBuffer(src, dst, length)
 *		u_short *src;
 *		char *dst;
 *		int length;
 */
LEAF(CopyFromBuffer)
	and	t0, a1, 1		# test for aligned dst
	beq	t0, zero, 3f
	nop
	blt	a2, 2, 7f		# at least 2 bytes to copy?
	nop
1:
	lhu	t0, 0(a0)		# read 2 bytes of data from buffer
	PTR_ADDU a0, 4			# keep buffer pointer word aligned
	PTR_ADDU a1, 2
	PTR_SUBU a2, 2
	sb	t0, -2(a1)
	srl	t0, 8
	bge	a2, 2, 1b
	sb	t0, -1(a1)
3:
	blt	a2, 2, 7f		# at least 2 bytes to copy?
	nop
6:
	lhu	t0, 0(a0)		# read 2 bytes of data from buffer
	PTR_ADDU a0, 4			# keep buffer pointer word aligned
	PTR_ADDU a1, 2
	PTR_SUBU a2, 2
	bge	a2, 2, 6b
	sh	t0, -2(a1)
7:
	blez	a2, 9f			# done?
	nop
	lhu	t0, 0(a0)		# copy one more byte
	nop
	sb	t0, 0(a1)
9:
	j	ra
	nop
END(CopyFromBuffer)
#endif	/* NSII > 0 */

/*
 * Write-buffer flush for writebuffer hardware on DEC mips r2000a
 */
#if defined(DEC_5100) || defined(DEC_5400)
LEAF(dec_mips1_wbflush)
ALEAF(kn230_wbflush)
ALEAF(kn210_wbflush)
        mfc0	v0, MIPS_COP_0_STATUS		# save original SR in v0
	li	t0, 0x80000000			# set CU3 bit
	or	v1, v0 ,t0			# v1 = v0 | 0x80000000
	nop
	mtc0	v1, MIPS_COP_0_STATUS
	nop				/* two  more cycles to complete */
 	nop

1:						# spin
	bc3f    1b				# until write buffer drained
        nop                     		# (branch delay slot)

        mtc0    v0, MIPS_COP_0_STATUS		# restore SR on exit
        nop
        j       ra
        nop
END(dec_mips1_wbflush)
#endif	/* defined(DEC_5100) || defined(DEC_5400) */

#ifdef __mips_o32
NESTED(promcall, CALLFRAME_SIZ, ra)
	PTR_SUBU sp, CALLFRAME_SIZ
	REG_S	ra, CALLFRAME_RA(sp)
	REG_S	s0, CALLFRAME_SP(sp)

	move	s0, t8			/* save MIPS_CURLWP */

	sll	t9, a0, 0
	sll	a0, a1, 0
	sll	a1, a2, 0
	sll	a2, a3, 0

	jalr	t9
	 nop

	move	t8, s0			/* restore MIPS_CURLWP */

	REG_L	ra, CALLFRAME_RA(sp)
	REG_L	s0, CALLFRAME_SP(sp)

	jr	ra
	 PTR_ADDU sp, CALLFRAME_SIZ
END(promcall)
#else
NESTED(promcall, TF_SIZ, ra)
	PTR_SUBU sp, TF_SIZ
	REG_S	ra, TF_REG_RA(sp)
	REG_S	s0, TF_REG_S0(sp)
	REG_S	s1, TF_REG_S1(sp)
	REG_S	s2, TF_REG_S2(sp)
	REG_S	s3, TF_REG_S3(sp)
	REG_S	s4, TF_REG_S4(sp)
	REG_S	s5, TF_REG_S5(sp)
	REG_S	s6, TF_REG_S6(sp)
	REG_S	s7, TF_REG_S7(sp)
	REG_S	s8, TF_REG_S8(sp)
	REG_S	t8, TF_REG_T8(sp)
	REG_S	gp, TF_REG_GP(sp)

	sll	t9, a0, 0
	sll	a0, a1, 0
	sll	a1, a2, 0
	sll	a2, a3, 0

	jalr	t9
	 nop

	sll	v0, v0, 0

	REG_L	ra, TF_REG_RA(sp)
	REG_L	s0, TF_REG_S0(sp)
	REG_L	s1, TF_REG_S1(sp)
	REG_L	s2, TF_REG_S2(sp)
	REG_L	s3, TF_REG_S3(sp)
	REG_L	s4, TF_REG_S4(sp)
	REG_L	s5, TF_REG_S5(sp)
	REG_L	s6, TF_REG_S6(sp)
	REG_L	s7, TF_REG_S7(sp)
	REG_L	s8, TF_REG_S8(sp)
	REG_L	t8, TF_REG_T8(sp)
	REG_L	gp, TF_REG_GP(sp)

	PTR_ADDU sp, TF_SIZ
	j	ra
	nop
END(promcall)
#endif
