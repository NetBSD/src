/*-
 * Copyright (c) 2009, 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mips_softint.c,v 1.1.2.2 2010/02/06 00:39:47 matt Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/user.h>
#include <sys/intr.h>
#include <sys/atomic.h>

#include <uvm/uvm_extern.h>

#include <mips/locore.h>

#ifdef __HAVE_FAST_SOFTINTS

#define	SOFTINT_BIO_MASK	(1 << SOFTINT_BIO)
#define	SOFTINT_CLOCK_MASK	(1 << SOFTINT_CLOCK)
#define	SOFTINT_NET_MASK	(1 << SOFTINT_NET)
#define	SOFTINT_SERIAL_MASK	(1 << SOFTINT_SERIAL)

/*
 * This is more complex than usual since we want the fast softint threads
 * to have stacks that are direct-mapped and avoid the TLB.  This means we
 * can avoid changing the TLB entry that maps the current lwp's kernel stack.
 *
 * This is a very big win so it's worth going through this effort.
 */
void
softint_init_md(lwp_t *l, u_int si_level, uintptr_t *machdep)
{
	struct cpu_info * const ci = l->l_cpu;

	cpu_uarea_remap(l);	/* switch to direct mapped stack */

	*machdep = si_level;
	ci->ci_softlwps[si_level] = l;
}

void
softint_trigger(uintptr_t si)
{
	/*
	 * Set the appropriate cause bit.  serial & net are 1 bit higher than
	 * clock & bio.  This avoid a branch and is fast.
	 */
	const uint32_t int_mask = MIPS_SOFT_INT_MASK_0
	    << (((SOFTINT_NET_MASK | SOFTINT_SERIAL_MASK) >> si) & 1);

	/*
	 * Use atomic_or since it's faster than splhigh/splx
	 */
	atomic_or_uint(&curcpu()->ci_softints, 1 << si);

	/*
	 * Now update cause.
	 */
	_setsoftintr(int_mask);
}

#define	SOFTINT_MASK_1	(SOFTINT_SERIAL_MASK | SOFTINT_NET_MASK)
#define	SOFTINT_MASK_0	(SOFTINT_CLOCK_MASK  | SOFTINT_BIO_MASK)

/*
 * Helper macro.
 *
 * Dispatch a softint and then restart the loop so that higher
 * priority softints are always done first.
 */
#define	DOSOFTINT(level) \
	if (softints & SOFTINT_##level## _MASK) { \
		ci->ci_softints ^= SOFTINT_##level##_MASK; \
		softint_fast_dispatch(ci->ci_softlwps[SOFTINT_##level], \
		    IPL_SOFT##level); \
		continue; \
	}

void
softint_process(uint32_t ipending)
{
	struct cpu_info * const ci = curcpu();
	u_int mask;
	int s;

	KASSERT((ipending & MIPS_SOFT_INT_MASK) != 0);
	KASSERT((ipending & ~MIPS_SOFT_INT_MASK) == 0);

	if (ipending & MIPS_SOFT_INT_MASK_0) {
		/*
		 * Since we run at splhigh, 
		 */
		mask = SOFTINT_MASK_1 | SOFTINT_MASK_0;
		ipending |= MIPS_SOFT_INT_MASK_1;
	} else {
		KASSERT(ipending & MIPS_SOFT_INT_MASK_1);
		mask = SOFTINT_MASK_1;
	}

	s = splhigh();

	for (;;) {
		u_int softints = ci->ci_softints & mask;
		if (softints == 0)
			break;

		DOSOFTINT(SERIAL);
		DOSOFTINT(NET);
		DOSOFTINT(BIO);
		DOSOFTINT(CLOCK);
	}

	_clrsoftintr(ipending);
	splx(s);
}

#endif /* __HAVE_FAST_SOFTINTS */
