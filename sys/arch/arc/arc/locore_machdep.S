/*	$NetBSD: locore_machdep.S,v 1.7 2000/03/03 12:32:43 soda Exp $	*/
/*	$OpenBSD: locore.S,v 1.12 1997/04/19 17:19:43 pefo Exp $	*/

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
 *	from: @(#)locore.s	8.5 (Berkeley) 1/4/94
 */

/*
 * ARC-specific mips locore code
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>	/* XXX - misnomer? */
#include <machine/endian.h>

	.set	noreorder

/*
 * GCC2 seems to want to call __main in main() for some reason.
 */
LEAF(__main)
	j	ra
	nop
END(__main)


/*
 *	Block I/O routines mainly used by I/O drivers.
 *
 *	Args as:	a0 = port
 *			a1 = memory address
 *			a2 = count
 */
LEAF(insb)
	beq	a2, zero, 2f
	addu	a2, a1
1:
	lbu	v0, 0(a0)
	addiu	a1, 1
	bne	a1, a2, 1b
	sb	v0, -1(a1)
2:
	jr	ra
	nop
END(insb)

LEAF(insw)
	beq	a2, zero, 2f
	addu	a2, a2
	addu	a2, a1
1:
	lhu	v0, 0(a0)
	addiu	a1, 2
	bne	a1, a2, 1b
	sh	v0, -2(a1)
2:
	jr	ra
	nop
END(insw)

LEAF(insl)
	beq	a2, zero, 2f
	sll	a2, 2
	addu	a2, a1
1:
	lw	v0, 0(a0)
	addiu	a1, 4
	bne	a1, a2, 1b
	sw	v0, -4(a1)
2:
	jr	ra
	nop
END(insl)

LEAF(outsb)
	beq	a2, zero, 2f
	addu	a2, a1
1:
	lbu	v0, 0(a1)
	addiu	a1, 1
	bne	a1, a2, 1b
	sb	v0, 0(a0)
2:
	jr	ra
	nop
END(outsb)

LEAF(outsw)
	beq	a2, zero, 2f
	addu	a2, a2
	li	v0, 1
	and	v0, a1
	bne	v0, zero, 3f		# arghh, unaligned.
	addu	a2, a1
1:
	lhu	v0, 0(a1)
	addiu	a1, 2
	bne	a1, a2, 1b
	sh	v0, 0(a0)
2:
	jr	ra
	nop
3:
	LWHI	v0, 0(a1)
	LWLO	v0, 3(a1)
	addiu	a1, 2
	bne	a1, a2, 3b
	sh	v0, 0(a0)

	jr	ra
	nop
END(outsw)

LEAF(outsl)
	beq	a2, zero, 2f
	sll	a2, 2
	li	v0, 3
	and	v0, a1
	bne	v0, zero, 3f		# arghh, unaligned.
	addu	a2, a1
1:
	lw	v0, 0(a1)
	addiu	a1, 4
	bne	a1, a2, 1b
	sw	v0, 0(a0)
2:
	jr	ra
	nop
3:
	LWHI	v0, 0(a1)
	LWLO	v0, 3(a1)
	addiu	a1, 4
	bne	a1, a2, 3b
	sw	v0, 0(a0)

	jr	ra
	nop
END(outsl)

/*
 * fillw(pat, addr, count)
 */
LEAF(fillw)
1:
	addiu	a2, a2, -1
	sh	a0, 0(a1)
	bne	a2,zero, 1b
	addiu	a1, a1, 2

	jr	ra
	nop
END(fillw)

/*#ifdef DEBUG*/ /* for minidebug.c: fix trap() to use this */
#if 0
/*
 * Read a long and return it.
 * Note: addresses can be unaligned!
 *
 * long
L* mdbpeek(addr)
L*	caddt_t addr;
L* {
L*	return (*(long *)addr);
L* }
 */
LEAF(mdbpeek)
	li	v0, MDBERR
	sw	v0, UADDR+U_PCB_ONFAULT
	and	v0, a0, 3		# unaligned address?
	bne	v0, zero, 1f
	nop
	b	2f
	lw	v0, (a0)		# aligned access
1:
	LWHI	v0, 0(a0)		# get next 4 bytes (unaligned)
	LWLO	v0, 3(a0)
2:
	j	ra			# made it w/o errors
	sw	zero, UADDR+U_PCB_ONFAULT
mdberr:
	li	v0, 1			# trap sends us here
	sw	v0, mdbmkfault
	j	ra
	nop
END(mdbpeek)

/*
 * Write a long to 'addr'.
 * Note: addresses can be unaligned!
 *
L* void
L* mdbpoke(addr, value)
L*	caddt_t addr;
L*	long value;
L* {
L*	*(long *)addr = value;
L* }
 */
LEAF(mdbpoke)
	li	v0, MDBERR
	sw	v0, UADDR+U_PCB_ONFAULT
	and	v0, a0, 3		# unaligned address?
	bne	v0, zero, 1f
	nop
	b	2f
	sw	a1, (a0)		# aligned access
1:
	SWHI	a1, 0(a0)		# store next 4 bytes (unaligned)
	SWLO	a1, 3(a0)
	and	a0, a0, ~3		# align address for cache flush
2:
	sw	zero, UADDR+U_PCB_ONFAULT
	b	R4K_FlushICache		# flush instruction cache
	li	a1, 8
END(mdbpoke)

/*
 * Save registers and state so we can do a 'mdbreset' (like longjmp) later.
 * Always returns zero.
 *
L* int mdb_savearea[11];
L*
L* int
L* mdbsetexit()
L* {
L*	mdb_savearea[0] = 0;
L*	return (0);
L* }
 */
	.comm	mdb_savearea, (11 * 4)

LEAF(mdbsetexit)
	la	a0, mdb_savearea
	sw	s0, 0(a0)
	sw	s1, 4(a0)
	sw	s2, 8(a0)
	sw	s3, 12(a0)
	sw	s4, 16(a0)
	sw	s5, 20(a0)
	sw	s6, 24(a0)
	sw	s7, 28(a0)
	sw	sp, 32(a0)
	sw	s8, 36(a0)
	sw	ra, 40(a0)
	j	ra
	move	v0, zero
END(mdbsetexit)

/*
 * Restore registers and state (like longjmp) and return x.
 *
L* int
L* mdbreset(x)
L* {
L*	return (x);
L* }
 */
LEAF(mdbreset)
	la	v0, mdb_savearea
	lw	ra, 40(v0)
	lw	s0, 0(v0)
	lw	s1, 4(v0)
	lw	s2, 8(v0)
	lw	s3, 12(v0)
	lw	s4, 16(v0)
	lw	s5, 20(v0)
	lw	s6, 24(v0)
	lw	s7, 28(v0)
	lw	sp, 32(v0)
	lw	s8, 36(v0)
	j	ra
	move	v0, a0
END(mdbreset)

/*
 * Trap into the debugger.
 *
L* void
L* mdbpanic()
L* {
L* }
 */
LEAF(mdbpanic)
	break	BREAK_SOVER_VAL
	j	ra
	nop
END(mdbpanic)
#endif /* DEBUG */

	.set	mips3

/*--------------------------------------------------------------------------
 *
 * mips3_TLBWriteIndexedVPS --
 *
 *	Write the given entry into the TLB at the given index.
 *	Pass full r4000 tlb info including variable page size mask.
 *
 *	mips3_TLBWriteIndexed(index, tlb)
 *		unsigned index;
 *		tlb *tlb;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TLB entry set.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips3_TLBWriteIndexedVPS)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS_REG	# Disable interrupts
	nop
	lw	a2, 8(a1)
	lw	a3, 12(a1)
	dmfc0	t0, MIPS_COP_0_TLB_HI		# Save the current PID.

	dmtc0	a2, MIPS_COP_0_TLB_LO0		# Set up entry low0.
	dmtc0	a3, MIPS_COP_0_TLB_LO1		# Set up entry low1.
	nop
	lw	a2, 0(a1)
	lw	a3, 4(a1)
	nop
	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index.
	dmtc0	a2, MIPS_COP_0_TLB_PG_MASK	# Set up entry mask.
	dmtc0	a3, MIPS_COP_0_TLB_HI		# Set up entry high.
	nop
	tlbwi					# Write the TLB
	nop
	nop
	nop					# Delay for effect
	nop

	dmtc0	t0, MIPS_COP_0_TLB_HI		# Restore the PID.
	nop
	dmtc0	zero, MIPS_COP_0_TLB_PG_MASK	# Default mask value.
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS_REG	# Restore the status register
END(mips3_TLBWriteIndexedVPS)


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
	.asciiz	"fpu"
_C_LABEL(eintrnames):
	.align	3
_C_LABEL(intrcnt):
	.word	0,0,0
_C_LABEL(eintrcnt):
