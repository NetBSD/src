/*	$NetBSD: lock_stubs.S,v 1.2 2020/08/13 07:14:04 skrll Exp $	*/

/*-
 * Copyright (c) 2014, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry, and by Andrew Doran.
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

#include "opt_lockdebug.h"

#include <aarch64/asm.h>

#include "assym.h"

RCSID("$NetBSD: lock_stubs.S,v 1.2 2020/08/13 07:14:04 skrll Exp $")

#ifndef LOCKDEBUG
/*
 * mutex_enter(): the compare-and-set must be atomic with respect to
 * interrupts and with respect to other CPUs.
 */
ENTRY(mutex_enter)
	mrs	x1, tpidr_el1		/* x1 = curlwp */
1:
	ldxr	x2, [x0]		/* load old value */
	cbnz	x2, 3f			/*   equals zero? */
	stxr	w3, x1, [x0]		/* store curlwp as new value */
	cbnz	w3, 2f			/*   succeed? nope, try again. */
	dmb	sy			/* membar_enter() */
	ret
2:
	b	1b
3:
	b	_C_LABEL(mutex_vector_enter)
END(mutex_enter)

/*
 * mutex_exit(): the compare-and-set need only be atomic with respect
 * to interrupts.  the cheapest way to achieve that may be to use a
 * restartable sequence, but the code do that would be quite involved,
 * so just use ldxr+stxr to achieve the same.
 */
ENTRY(mutex_exit)
	dmb	sy			/* membar_exit() */
	mrs	x1, tpidr_el1		/* x1 = curlwp */
1:
	ldxr	x2, [x0]		/* load old value */
	cmp	x1, x2			/*   equals curlwp? */
	b.ne	3f			/*     slow path if different */
	stxr	w3, xzr, [x0]		/* store zero as new value */
	cbnz	w3, 2f			/*   succeed? nope, try again. */
	ret
2:
	b	1b
3:
	b	_C_LABEL(mutex_vector_exit)
END(mutex_exit)
#endif	/* !LOCKDEBUG */
