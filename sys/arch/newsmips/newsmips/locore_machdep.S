/*	$NetBSD: locore_machdep.S,v 1.5 1999/06/20 05:42:09 tsubai Exp $	*/

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
 * NEWS-specific mips locore code.
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <machine/machConst.h>

	.set	noreorder

LEAF(to_monitor)
	li	v0, MIPS_SR_BOOT_EXC_VEC	# no interrupt and
	mtc0	v0, MIPS_COP_0_STATUS_REG	# boot strap exception vector
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

/*
 * Set/clear software interrupt routines.
 */

LEAF(setsoftclock)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# save status register
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable interrupts (2 cycles)
	nop
	mfc0	v0, MIPS_COP_0_CAUSE_REG	# read cause register
	nop
	or	v0, v0, MIPS_SOFT_INT_MASK_0	# set soft clock interrupt
	mtc0	v0, MIPS_COP_0_CAUSE_REG	# save it
	mtc0	v1, MIPS_COP_0_STATUS_REG
	j	ra
	nop
END(setsoftclock)

LEAF(clearsoftclock)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# save status register
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable interrupts (2 cycles)
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE_REG	# read cause register
	nop
	and	v0, v0, ~MIPS_SOFT_INT_MASK_0	# clear soft clock interrupt
	mtc0	v0, MIPS_COP_0_CAUSE_REG	# save it
	mtc0	v1, MIPS_COP_0_STATUS_REG
	j	ra
	nop
END(clearsoftclock)

LEAF(setsoftnet)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# save status register
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable interrupts (2 cycles)
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE_REG	# read cause register
	nop
	or	v0, v0, MIPS_SOFT_INT_MASK_1	# set soft net interrupt
	mtc0	v0, MIPS_COP_0_CAUSE_REG	# save it
	mtc0	v1, MIPS_COP_0_STATUS_REG
	j	ra
	nop
END(setsoftnet)

LEAF(clearsoftnet)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# save status register
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable interrupts (2 cycles)
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE_REG	# read cause register
	nop
	and	v0, v0, ~MIPS_SOFT_INT_MASK_1	# clear soft net interrupt
	mtc0	v0, MIPS_COP_0_CAUSE_REG	# save it
	mtc0	v1, MIPS_COP_0_STATUS_REG
	j	ra
	nop
END(clearsoftnet)

/*
 * Set/change interrupt priority routines.
 */

LEAF(spl0)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	nop
	or	t0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	mtc0	t0, MIPS_COP_0_STATUS_REG	# enable all interrupts
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(spl0)

LEAF(splsoftclock)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~MIPS_SOFT_INT_MASK_0	# disable soft clock
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(splsoftclock)

LEAF(splsoftnet)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(splsoftnet)

/*
 * nesting interrupt masks.
 */
#define MIPS_INT_MASK_SPL_SOFT0	MIPS_SOFT_INT_MASK_0
#define MIPS_INT_MASK_SPL_SOFT1	(MIPS_SOFT_INT_MASK_1|MIPS_INT_MASK_SPL_SOFT0)
#define MIPS_INT_MASK_SPL0	(MIPS_INT_MASK_0|MIPS_INT_MASK_SPL_SOFT1)
#define MIPS_INT_MASK_SPL1	(MIPS_INT_MASK_1|MIPS_INT_MASK_SPL0)
#define MIPS_INT_MASK_SPL2	(MIPS_INT_MASK_2|MIPS_INT_MASK_SPL1)
#define MIPS_INT_MASK_SPL3	(MIPS_INT_MASK_3|MIPS_INT_MASK_SPL2)
#define MIPS_INT_MASK_SPL4	(MIPS_INT_MASK_4|MIPS_INT_MASK_SPL3)
#define MIPS_INT_MASK_SPL5	(MIPS_INT_MASK_5|MIPS_INT_MASK_SPL4)

/*
 * hardware-level spls for hardware where the device interrupt priorites
 * are ordered, and map onto mips interrupt pins in increasing priority.
 * This maps directly onto BSD spl levels.
 */

/*
 * Block out int2 (hardware interrupt 0) and lower mips levels.
 */
LEAF(cpu_spl0)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl0)

/*
 * Block out Int3 (hardware interrupt 1) and lower mips levels.
 */
LEAF(cpu_spl1)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL1)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl1)

LEAF(cpu_spl2)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL2)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl2)

LEAF(cpu_spl3)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL3)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl3)

LEAF(cpu_spl4)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL4)

	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl4)

LEAF(cpu_spl5)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL5)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl5)

/*
 * hardware-level spls for hardware where the interrupt priorites
 * DO NOT map onto levels.  
 */
LEAF(Mach_spl0)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_0|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl0)

LEAF(Mach_spl1)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_1|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl1)

LEAF(Mach_spl2)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_2|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl2)

LEAF(Mach_spl3)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_3|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl3)

LEAF(Mach_spl4)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_4|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl4)

LEAF(Mach_spl5)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_5|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl5)

/*
 * We define an alternate entry point after mcount is called so it
 * can be used in mcount without causeing a recursive loop.
 */
LEAF(splhigh)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~MIPS_SR_INT_IE	# disable all interrupts
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(splhigh)

/*
 * Restore saved interrupt mask.
 */
LEAF(splx)
	mfc0	v0, MIPS_COP_0_STATUS_REG
	li	t0, ~(MIPS_INT_MASK | MIPS_SR_INT_IE)
	and	t0, t0, v0
	or	t0, t0, a0
	mtc0	t0, MIPS_COP_0_STATUS_REG
	nop					# 3 ins to disable
	j	ra
	nop
	.globl	_C_LABEL(splx_end)
_C_LABEL(splx_end):
END(splx)

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
	.word	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0
_C_LABEL(eintrcnt):
	.word	0	# This shouldn't be needed but with 4.4bsd's as, the eintrcnt
			#  label ends end up in a different section otherwise.
