/*	$NetBSD: locore_machdep.S,v 1.14.96.1 2010/01/14 00:37:27 matt Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Digital Equipment Corporation, Ralph Campbell, Sony Corp and
 * Kazumasa Utashiro of Software Research Associates, Inc.
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
 * NEWS-specific mips locore code.
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <machine/machConst.h>
#include <machine/param.h>

#include "assym.h"

	.set	noreorder

LEAF(to_monitor)
	li	v0, MIPS_SR_BEV			# no interrupt and
	mtc0	v0, MIPS_COP_0_STATUS		# boot strap exception vector
	nop
	nop
	nop
	nop
	li	a1, MACH_MONARG_ADDR|MIPS_KSEG1_START
	sw	a0, (a1)			# pass argument(howto)
	move	a0, zero			# syscall(#0)
	syscall
	nop
END(to_monitor)

#ifdef MIPS3
	.set	mips3
	.set	noat
	.globl	checkromwork
checkromwork:
	dmfc0	k0, MIPS_COP_0_BAD_VADDR

	li	k1, 0xfff00000			# NEWS5000 ROM Work: 0xFFF00000~0xFFFFFFFF (1Mbyte)
	sltu	k1, k0, k1
	bne	k1, zero, outofworld
	nop

	li	k1, 0x000fe000			# XXX (depend on 4k page)
	and	k1, k0, k1			# k1 = (VADDR - 0xFFF00000) & 0xffffe000

	la	k0, _C_LABEL(physmem)
	lw	k0, 0(k0)			# k0 = page of physmem (tail of maxmemory - 0x00100000)
	sll	k0, k0, PGSHIFT			# k0 = memory tail
	add	k0, k0, k1			# 0xfff00000+? -> memory tail + ?

	srl	k0, k0, 6			# MIPS3_PG_SHIFT
	li	k1, 0x02|0x18|0x01		# MIPS3_PG_V|MIPS3_PG_CACHED|MIPS3_PG_G
	or	k0, k0, k1

	li	k1, 64				# set ODD page
	add	k1, k0, k1

	dsll	k0, k0, 34			# get rid of "wired" bit
	dsrl	k0, k0, 34
	dmtc0	k0, MIPS_COP_0_TLB_LO0		# load PTE entry
	dsll	k1, k1, 34
	dsrl	k1, k1, 34
	dmtc0	k1, MIPS_COP_0_TLB_LO1		# load PTE entry
	nop
	nop					# required for QED5230
	tlbwr					# write TLB
	nop
	nop
	nop
	nop
	nop
	eret

outofworld:
	/* Ensure we have a valid sp so panic has a chance */
	move	a1, sp
	la	sp, start			# set sp to a valid place
	PANIC("TLB out of universe: ksp was %p")

	.set	at
#endif /* MIPS3 */

/*
 * Interrupt counters for vmstat.
 */
	.data
	.globl _C_LABEL(intrcnt)
	.globl _C_LABEL(eintrcnt)
	.globl _C_LABEL(intrnames)
	.globl _C_LABEL(eintrnames)
_C_LABEL(intrnames):
	.asciiz	"serial0"
	.asciiz	"serial1"
	.asciiz	"serial2"
	.asciiz	"ether"
	.asciiz	"scsi"
	.asciiz	"memory"
	.asciiz	"clock"
	.asciiz	"fpu"
	.asciiz	"slot1"
	.asciiz	"slot2"
	.asciiz	"slot3"
	.asciiz	"floppy"
	.asciiz	"stray"
	.asciiz	"nmi"
	.asciiz	"lostclock"
_C_LABEL(eintrnames):
	.align	2
_C_LABEL(intrcnt):
	.word	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0
_C_LABEL(eintrcnt):
	.word	0	# This shouldn't be needed but with 4.4bsd's as, the eintrcnt
			#  label ends end up in a different section otherwise.
