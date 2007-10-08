/*	$NetBSD: kern_softint.c,v 1.3 2007/10/08 20:06:19 ad Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Stub for code to be merged from the vmlocking CVS branch.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_softint.c,v 1.3 2007/10/08 20:06:19 ad Exp $");

#include <sys/param.h>
#include <sys/intr.h>

u_int	softint_timing;

/*
 * softint_init:
 *
 *	Initialize per-CPU data structures.  Called from mi_cpu_attach().
 */
void
softint_init(struct cpu_info *ci)
{

	/* nothing yet */
}

/*
 * softint_establish:
 *
 *	Register a software interrupt handler.
 */
void *
softint_establish(u_int flags, void (*func)(void *), void *arg)
{
	u_int level;

	level = (flags & SOFTINT_LVLMASK);
	KASSERT(level < SOFTINT_COUNT);

	switch (level) {
	case SOFTINT_CLOCK:
		level = IPL_SOFTCLOCK;
		break;
	case SOFTINT_NET:
	case SOFTINT_BIO:
		level = IPL_SOFTNET;
		break;
	case SOFTINT_SERIAL:
#ifdef IPL_SOFTSERIAL
		level = IPL_SOFTSERIAL;
#else
		level = IPL_SOFTNET;
#endif
		break;
	default:
		panic("softint_establish");
	}

	return softintr_establish(level, func, arg);
}

/*
 * softint_disestablish:
 *
 *	Unregister a software interrupt handler.
 */
void
softint_disestablish(void *arg)
{

	softintr_disestablish(arg);
}

/*
 * softint_schedule:
 *
 *	Trigger a software interrupt.  Must be called from a hardware
 *	interrupt handler, or with preemption disabled (since we are
 *	using the value of curcpu()).
 */
void
softint_schedule(void *arg)
{

	softintr_schedule(arg);
}

/*
 * softint_block:
 *
 *	Update statistics when the soft interrupt blocks.
 */
void
softint_block(lwp_t *l)
{

	/* nothing yet */
}
