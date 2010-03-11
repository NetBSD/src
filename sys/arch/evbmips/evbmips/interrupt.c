/*	$NetBSD: interrupt.c,v 1.11.18.4 2010/03/11 08:20:18 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.11.18.4 2010/03/11 08:20:18 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <mips/mips3_clock.h>
#include <machine/locore.h>

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

	KASSERT(ci->ci_cpl == IPL_HIGH);

	uvmexp.intrs++;

	while (ppl < (ipl = splintr(&pending))) {
		splx(ipl);	/* lower to interrupt level */

		KASSERT(pending != 0);

		if (pending & MIPS_INT_MASK_5) {
			struct clockframe cf;
			KASSERT(ipl == IPL_SCHED);
			/* call the common MIPS3 clock interrupt handler */ 
			cf.pc = pc;
			cf.sr = status;
			cf.intr = (ci->ci_idepth > 1);
			mips3_clockintr(&cf);
			pending ^= MIPS_INT_MASK_5;
		}

		if (pending != 0) {
			/* Process I/O and error interrupts. */
			evbmips_iointr(ipl, pc, pending);
		}
		(void)splhigh();	/* disable interrupts */
	}

	KASSERT(ci->ci_cpl == IPL_HIGH);
}
