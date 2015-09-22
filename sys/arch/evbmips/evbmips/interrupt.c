/*	$NetBSD: interrupt.c,v 1.20.2.2 2015/09/22 12:05:41 skrll Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.20.2.2 2015/09/22 12:05:41 skrll Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>

#include <mips/locore.h>
#include <mips/mips3_clock.h>

struct clockframe cf;

void
intr_init(void)
{

	evbmips_intr_init();	/* board specific stuff */
}

void
cpu_intr(int ppl, vaddr_t pc, uint32_t status)
{
	struct cpu_info * const ci = curcpu();
	uint32_t pending;
	int ipl;
#ifdef DIAGNOSTIC
	const int mtx_count = ci->ci_mtx_count;
	const u_int biglock_count = ci->ci_biglock_count;
	const u_int blcnt = curlwp->l_blcnt;
#endif
	KASSERT(ci->ci_cpl == IPL_HIGH);
	KDASSERT(mips_cp0_status_read() & MIPS_SR_INT_IE);

	ci->ci_data.cpu_nintr++;

	while (ppl < (ipl = splintr(&pending))) {
		KDASSERT(mips_cp0_status_read() & MIPS_SR_INT_IE);
		splx(ipl);	/* lower to interrupt level */
		KDASSERT(mips_cp0_status_read() & MIPS_SR_INT_IE);

		KASSERTMSG(ci->ci_cpl == ipl,
		    "%s: cpl (%d) != ipl (%d)", __func__, ci->ci_cpl, ipl);
		KASSERT(pending != 0);

		cf.pc = pc;
		cf.sr = status;
		cf.intr = (ci->ci_idepth > 1);

#ifdef MIPS3_ENABLE_CLOCK_INTR
		if (pending & MIPS_INT_MASK_5) {
			KASSERTMSG(ipl == IPL_SCHED,
			    "%s: ipl (%d) != IPL_SCHED (%d)",
			     __func__, ipl, IPL_SCHED);
			/* call the common MIPS3 clock interrupt handler */
			mips3_clockintr(&cf);
			pending ^= MIPS_INT_MASK_5;
		}
#endif

		if (pending != 0) {
			/* Process I/O and error interrupts. */
			evbmips_iointr(ipl, pc, pending);
		}
		KASSERT(biglock_count == ci->ci_biglock_count);
		KASSERT(blcnt == curlwp->l_blcnt);
		KASSERT(mtx_count == ci->ci_mtx_count);

		/*
		 * If even our spl is higher now (due to interrupting while
		 * spin-lock is held and higher IPL spin-lock is locked, it
		 * can no longer be locked so it's safe to lower IPL back
		 * to ppl.
		 */
		(void) splhigh();	/* disable interrupts */
	}

	KASSERT(ci->ci_cpl == IPL_HIGH);
	KDASSERT(mips_cp0_status_read() & MIPS_SR_INT_IE);
}
