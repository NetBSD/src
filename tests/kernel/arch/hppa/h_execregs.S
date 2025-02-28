/*	$NetBSD: h_execregs.S,v 1.2 2025/02/28 16:08:19 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#define	_LOCORE

#include <sys/syscall.h>

#include <machine/asm.h>
#include <machine/vmparam.h>

#include "execregs.h"

_ENTRY(execregs_start)
	.callinfo frame=(NEXECREGS*4), calls
	.entry

	ldo	(NEXECREGS*4)(%sp), %sp		/* space for NEXECREGS */
	stw	%t1, (4*(0 - NEXECREGS))(%sp)	/* order matches execregs.h */
	stw	%t2, (4*(1 - NEXECREGS))(%sp)
	/* sp: stack pointer */
	stw	%t3, (4*(2 - NEXECREGS))(%sp)
	/* cr17/iisq_head: privileged */
	/* cr17/iisq_tail: privileged */
	/* cr18/iioq_head: privileged */
	/* cr18/iioq_tail: privileged */
	/* cr15/eiem: privileged */
	/* cr22/ipsw: privileged */
	/* sr3: privileged(?) */
	/* cr8/pidr1: privileged */
	/* cr20/isr: privileged */
	/* cr21/ior: privileged */
	/* cr19/iir: privileged */
	/* flags: N/A(?) */
	stw	%sar, (4*(3 - NEXECREGS))(%sp)
	stw	%r1, (4*(4 - NEXECREGS))(%sp)
	stw	%rp, (4*(5 - NEXECREGS))(%sp)
	/* r3: frame pointer (set to initial stack pointer) */
	stw	%r4, (4*(6 - NEXECREGS))(%sp)
	stw	%r5, (4*(7 - NEXECREGS))(%sp)
	stw	%r6, (4*(8 - NEXECREGS))(%sp)
	stw	%r7, (4*(9 - NEXECREGS))(%sp)
	stw	%r8, (4*(10 - NEXECREGS))(%sp)
	stw	%r9, (4*(11 - NEXECREGS))(%sp)
	stw	%r10, (4*(12 - NEXECREGS))(%sp)
	stw	%r11, (4*(13 - NEXECREGS))(%sp)
	stw	%r12, (4*(14 - NEXECREGS))(%sp)
	stw	%r13, (4*(15 - NEXECREGS))(%sp)
	stw	%r14, (4*(16 - NEXECREGS))(%sp)
	stw	%r15, (4*(17 - NEXECREGS))(%sp)
	stw	%r16, (4*(18 - NEXECREGS))(%sp)
	stw	%r17, (4*(19 - NEXECREGS))(%sp)
	stw	%r18, (4*(20 - NEXECREGS))(%sp)
	stw	%t4, (4*(21 - NEXECREGS))(%sp)
	stw	%arg3, (4*(22 - NEXECREGS))(%sp)
	stw	%arg2, (4*(23 - NEXECREGS))(%sp)
	stw	%arg1, (4*(24 - NEXECREGS))(%sp)
	/* arg0: ps_strings */
	stw	%dp, (4*(25 - NEXECREGS))(%sp)
	stw	%ret0, (4*(26 - NEXECREGS))(%sp)
	stw	%ret1, (4*(27 - NEXECREGS))(%sp)
	stw	%r31, (4*(28 - NEXECREGS))(%sp)
	/* sr0-sr7: space registers initialized by kernel */
	/* cr9/pidr2: privileged */
	/* cr12/pidr3: privileged */
	/* cr13/pidr4: privileged */
	/* cr0/rctr: privileged */
	/* cr10/ccr: privileged */
	/* cr23/eirr: privileged */
	/* cr24: privileged */
	/* cr25/vtop: privileged */
	/* cr26: ??? */
	stw	%cr27, (4*(29 - NEXECREGS))(%sp)
	stw	%cr28, (4*(30 - NEXECREGS))(%sp)
	/* cr30/fpregs: privileged */
	/* cr31: privileged */

	addc	%t1, %r0, %r0	/* t1 := PSW[C/B]{0} */
	zdep	%t1, 23, 8, %t1	/* t1 := PSW */
	stw	%t1, (4*(31 - NEXECREGS))(%sp)

	/* store the fp registers */
	ldo	(4*(32 - NEXECREGS))(%sp), %t1
	fstd,ma	%fr0, 8(%t1)
	fstd,ma	%fr1, 8(%t1)
	fstd,ma	%fr2, 8(%t1)
	fstd,ma	%fr3, 8(%t1)
	fstd,ma	%fr4, 8(%t1)
	fstd,ma	%fr5, 8(%t1)
	fstd,ma	%fr6, 8(%t1)
	fstd,ma	%fr7, 8(%t1)
	fstd,ma	%fr8, 8(%t1)
	fstd,ma	%fr9, 8(%t1)
	fstd,ma	%fr10, 8(%t1)
	fstd,ma	%fr11, 8(%t1)
	fstd,ma	%fr12, 8(%t1)
	fstd,ma	%fr13, 8(%t1)
	fstd,ma	%fr14, 8(%t1)
	fstd,ma	%fr15, 8(%t1)
	fstd,ma	%fr16, 8(%t1)
	fstd,ma	%fr17, 8(%t1)
	fstd,ma	%fr18, 8(%t1)
	fstd,ma	%fr19, 8(%t1)
	fstd,ma	%fr20, 8(%t1)
	fstd,ma	%fr21, 8(%t1)
	fstd,ma	%fr22, 8(%t1)
	fstd,ma	%fr23, 8(%t1)
	fstd,ma	%fr24, 8(%t1)
	fstd,ma	%fr25, 8(%t1)
	fstd,ma	%fr26, 8(%t1)
	fstd,ma	%fr27, 8(%t1)
	fstd,ma	%fr28, 8(%t1)
	fstd,ma	%fr29, 8(%t1)
	fstd,ma	%fr30, 8(%t1)
	fstd	%fr31, 0(%t1)

	/* call write(STDOUT_FILENO, regs, sizeof(regs)) */
	ldi	1, %arg0			/* arg0 := STDOUT_FILENO */
	ldo	-(4*NEXECREGS)(%sp), %arg1	/* arg1 := regs */
	ldi	(4*NEXECREGS), %arg2		/* arg2 := sizeof(regs) */
	ldil	L%SYSCALLGATE, %r1
	ble	4(%sr2, %r1)
	 ldi	SYS_write, %t1

	comb,<>,n	%r0, %t1, 2f		/* bail if write failed */
	ldi		(4*NEXECREGS), %t1	/* bail if wrong # bytes */
	comb,<>,n	%ret0, %t1, 2f

	/* call exit(0) */
	ldi	0, %arg0
1:	ldil	L%SYSCALLGATE, %r1
	ble	4(%sr2, %r1)
	 ldi	SYS_exit, %t1
	break	0, 0				/* paranoia */

2:	/* call exit(127) */
	b	1b
	 ldi	127, %arg0
EXIT(execregs_start)

/* main stub to simplify linking */
LEAF_ENTRY(main)
	break	0, 0				/* paranoia */
EXIT(main)
