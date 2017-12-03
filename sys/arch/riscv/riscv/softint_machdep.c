/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>

__RCSID("$NetBSD: softint_machdep.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $");

#define __INTR_PRIVATE

#include <sys/param.h>
#include <sys/intr.h>

#include <riscv/locore.h>

#define	SOFTINT2IPLMAP \
	(((IPL_SOFTSERIAL - IPL_SOFTCLOCK) << (SOFTINT_SERIAL * 4)) | \
	 ((IPL_SOFTNET    - IPL_SOFTCLOCK) << (SOFTINT_NET    * 4)) | \
	 ((IPL_SOFTBIO    - IPL_SOFTCLOCK) << (SOFTINT_BIO    * 4)) | \
	 ((IPL_SOFTCLOCK  - IPL_SOFTCLOCK) << (SOFTINT_CLOCK  * 4)))
#define	SOFTINT2IPL(l)	((SOFTINT2IPLMAP >> ((l) * 4)) & 0x0f)

/*
 * This returns a mask of softint IPLs that be dispatch at <ipl>
 * SOFTIPLMASK(IPL_NONE)	= 0x0000000f
 * SOFTIPLMASK(IPL_SOFTCLOCK)	= 0x0000000e
 * SOFTIPLMASK(IPL_SOFTBIO)	= 0x0000000c
 * SOFTIPLMASK(IPL_SOFTNET)	= 0x00000008
 * SOFTIPLMASK(IPL_SOFTSERIAL)	= 0x00000000
 */
#define	SOFTIPLMASK(ipl) ((0x0f << (ipl)) & 0x0f)

void softint_switch(lwp_t *, int);

void
softint_trigger(uintptr_t mask)
{
	curcpu()->ci_softints |= mask;
}

void
softint_init_md(lwp_t *l, u_int level, uintptr_t *machdep)
{
	lwp_t ** lp = &l->l_cpu->ci_softlwps[level];
	KASSERT(*lp == NULL || *lp == l);
	*lp = l;
	*machdep = 1 << SOFTINT2IPL(level);
	KASSERT(level != SOFTINT_CLOCK || *machdep == (1 << (IPL_SOFTCLOCK - IPL_SOFTCLOCK)));
	KASSERT(level != SOFTINT_BIO || *machdep == (1 << (IPL_SOFTBIO - IPL_SOFTCLOCK)));
	KASSERT(level != SOFTINT_NET || *machdep == (1 << (IPL_SOFTNET - IPL_SOFTCLOCK)));
	KASSERT(level != SOFTINT_SERIAL || *machdep == (1 << (IPL_SOFTSERIAL - IPL_SOFTCLOCK)));
}

void
softint_deliver(void)
{
	struct cpu_info * const ci = curcpu();
	const int opl = ci->ci_cpl;
	const uint32_t softiplmask = SOFTIPLMASK(opl);

	splhigh();
	for (;;) {
		u_int softints = ci->ci_softints & softiplmask;
		KASSERT((softints != 0) == ((ci->ci_softints >> opl) != 0));
		KASSERT(opl == IPL_NONE || (softints & (1 << (opl - IPL_SOFTCLOCK))) == 0);
		if (softints == 0) {
			splx(opl);
			return;
		}
#define	DOSOFTINT(n) \
		if (ci->ci_softints & (1 << (IPL_SOFT ## n - IPL_SOFTCLOCK))) { \
			ci->ci_softints &= \
			    ~(1 << (IPL_SOFT ## n - IPL_SOFTCLOCK)); \
			cpu_fast_switchto(ci->ci_softlwps[SOFTINT_ ## n], \
			    IPL_SOFT ## n); \
			continue; \
		}
		DOSOFTINT(SERIAL);
		DOSOFTINT(NET);
		DOSOFTINT(BIO);
		DOSOFTINT(CLOCK);
		panic("dosoftints wtf (softints=%u?, ipl=%d)", softints, opl);
	}
}

