/*	$NetBSD: vr_idle.S,v 1.6.10.1 2002/06/24 22:04:46 nathanw Exp $	*/

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
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <hpcmips/vr/vr_asm.h>

#include "opt_lockdebug.h"

#include "assym.h"

	.set	noreorder

/*
 * When no processes are on the runq, cpu_switch branches to idle
 * to wait for something to come ready.
 * Note: this is really a part of cpu_switch() but defined here for kernel
 * profiling.
 */
LEAF(vr_idle)
	li	t0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	mtc0	t0, MIPS_COP_0_STATUS		# enable all interrupts
	nop
	sw	zero, _C_LABEL(curlwp)		# set curlwp NULL for stats
#if defined(LOCKDEBUG)
	jal	_C_LABEL(sched_unlock_idle)	# release sched_lock
	nop
#endif

	/* Try to zero some free pages. */
	lw	t0, _C_LABEL(uvm) + UVM_PAGE_IDLE_ZERO
	nop
	beq	t0, zero, 1f
	nop
	jal	_C_LABEL(uvm_pageidlezero)
	nop
1:
	lw	t0, _C_LABEL(sched_whichqs)	# look for non-empty queue
	bne	t0, zero, 1f
	nop
	.word	VR_OPCODE_STANDBY
	nop
	nop
	nop
	nop
	beq	t0, zero, 1b
	nop
1:
#if defined(LOCKDEBUG)
	mtc0	zero, MIPS_COP_0_STATUS		# disable all interrupts
	nop
	nop
	nop
	nop
	jal	_C_LABEL(sched_lock_idle)	# acquire sched_lock
	nop
	la	ra, cpu_switch_queuescan	# rathole to cpu_switch()
	j	ra
	nop
#else
	mtc0	zero, MIPS_COP_0_STATUS		# disable all interrupts
	nop
	nop
	nop
	la	ra, cpu_switch_queuescan	# rathole to cpu_switch()
	j	ra
	nop
#endif
END(vr_idle)
