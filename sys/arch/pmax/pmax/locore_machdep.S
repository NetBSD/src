/*	$NetBSD: locore_machdep.S,v 1.8.2.3 1999/01/28 06:46:46 nisimura Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * DECstation specific locore routine collection.
 */
#include <mips/asm.h>
#include <mips/cpuregs.h>

#include "opt_dec_3100.h"
#include "opt_dec_5100.h"
#include "opt_dec_5400.h"
#include "opt_dec_3max.h"

	.set	noreorder
LEAF(_splraise)
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	and	a0, a0, MIPS_INT_MASK		# extract INT bits
	nor	a0, zero, a0			# bitwise inverse of A0
	and	a0, a0, v0			# disable retaining other bits
	mtc0	a0, MIPS_COP_0_STATUS		# store back
	nop
	j	ra
	and	v0, v0, MIPS_INT_MASK
END(_splraise)

LEAF(_spllower)
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	li	v1, ~MIPS_INT_MASK
	and	v1, v0, v1			# turn off INT bit
	nor	a0, zero, a0			# bitwise inverse of A0
	and	a0, a0, MIPS_INT_MASK		# extract INT bits
	or	a0, a0, v1			# disable making other bits on
	mtc0	a0, MIPS_COP_0_STATUS		# store back
	nop
	j	ra
	and	v0, v0, MIPS_INT_MASK
END(_spllower)

LEAF(_splset)
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	and	a0, a0, MIPS_INT_MASK		# extract INT bits
	li	v1, ~MIPS_INT_MASK
	and	v1, v1, v0			# turn off every INT bit
	or	v1, v1, a0			# set old INT bits
	mtc0	v1, MIPS_COP_0_STATUS		# store back
	nop
	j	ra
	and	v0, v0, MIPS_INT_MASK
END(_splset)

LEAF(_splget)
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	nop
	j	ra
	and	v0, v0, MIPS_INT_MASK
END(_splget)

LEAF(_setsoftintr)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
	nop; nop
	mfc0	v0, MIPS_COP_0_CAUSE		# fetch cause register
	nop
	or	v0, v0, a0			# set soft intr. bits
	mtc0	v0, MIPS_COP_0_CAUSE		# store back
	mtc0	v1, MIPS_COP_0_STATUS		# enable interrupts
	j	ra
	nop
END(_setsoftintr)

LEAF(_clrsoftintr)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
	nop; nop
	mfc0	v0, MIPS_COP_0_CAUSE		# fetch cause register
	nor	a0, zero, a0			# bitwise inverse of A0
	and	v0, v0, a0			# clear soft intr. bits
	mtc0	v0, MIPS_COP_0_CAUSE		# store back
	mtc0	v1, MIPS_COP_0_STATUS		# enable interrupts
	j	ra
	nop
END(_clrsoftintr)


#if defined(DEC_3100) || defined(DEC_5100)
/*
 * Copy data to a DMA buffer padded with 16 bits of data, 16
 * bits of padding per 32bit word (e.g., for pmin/pmax  sii (and lance) DMA).
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
	subu	a2, a2, 2
	addu	a0, a0, 2
	addu	a1, a1, 4
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
	addu	a0, a0, 4		# keep buffer pointer word aligned
	addu	a1, a1, 2
	subu	a2, a2, 2
	sb	t0, -2(a1)
	srl	t0, t0, 8
	bge	a2, 2, 1b
	sb	t0, -1(a1)
3:
	blt	a2, 2, 7f		# at least 2 bytes to copy?
	nop
6:
	lhu	t0, 0(a0)		# read 2 bytes of data from buffer
	addu	a0, a0, 4		# keep buffer pointer word aligned
	addu	a1, a1, 2
	subu	a2, a2, 2
	bge	a2, 2, 6b
	sh	t0, -2(a1)
7:
	ble	a2, zero, 9f		# done?
	nop
	lhu	t0, 0(a0)		# copy one more byte
	nop
	sb	t0, 0(a1)
9:
	j	ra
	nop
END(CopyFromBuffer)
#endif /* DEC_3100 || DEC_5100 */


#if defined(DEC_3100) || defined(DEC_3MAX)
/*
 * Processor's CU0 (coprocessor 0 usability) bit wired with the writebuffer
 * circuit; This indicates write operations are pending or not.
 */
LEAF(cop0_busy)
ALIAS(kn01_wbflush)
ALIAS(kn02_wbflush)
1:					# spin
	bc0f	1b			# until write buffer is drained
	nop				# -delay slot-
	j	ra
	nop
END(cop0_busy)
#endif


#if defined(DEC_5100) || defined(DEC_5400)
/*
 * Processor's CU3 (coprocessor 3 usability) bit wired with the writebuffer
 * circuit; This indicates write operations are pending or not.
 */
LEAF(cop3_busy)
ALIAS(kn210_wbflush)
ALIAS(kn230_wbflush)
        mfc0	v0, MIPS_COP_0_STATUS	# save original SR in v0
	li	t0, 0x80000000		# set CU3 bit
	or	v1, v0 ,t0		# v1 = v0 | 0x80000000
	nop
	mtc0	v1, MIPS_COP_0_STATUS	
	nop				# make sure new SR takes the effect
 	nop

1:					# spin 
	bc3f    1b			# until write buffer drained
        nop                     	# -delay slot-

        mtc0    v0, MIPS_COP_0_STATUS	# restore SR on exit
        nop
        j       ra
        nop
END(cop3_busy)
#endif

/*
 * Interrupt counters for vmstat.
 */
	.data
	.globl _C_LABEL(intrcnt)
	.globl _C_LABEL(eintrcnt)
	.globl _C_LABEL(intrnames)
	.globl _C_LABEL(eintrnames)
_C_LABEL(intrnames):
	.asciiz	"softclock"
	.asciiz	"softnet"
	.asciiz	"serial0"
	.asciiz	"serial1"
	.asciiz	"serial2"
	.asciiz	"ether"
	.asciiz	"scsi"
	.asciiz	"memory"
	.asciiz	"clock"
	.asciiz	"fpu"
	.asciiz	"tcslot0"
	.asciiz	"tcslot1"
	.asciiz	"tcslot2"
	.asciiz "dtop"
	.asciiz "isdn"
	.asciiz	"floppy"
	.asciiz	"stray"
	.asciiz	"nmi"
	.asciiz	"lostclock"
_C_LABEL(eintrnames):
	.align	2
_C_LABEL(intrcnt):
	.word	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0
_C_LABEL(eintrcnt):
	.word	0	# This shouldn't be needed but with 4.4bsd's as, the eintrcnt
			#  label ends end up in a different section otherwise.
