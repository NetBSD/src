/*	$NetBSD: interrupt.c,v 1.5 2003/05/25 14:08:20 tsutsui Exp $	*/

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

#include <sys/param.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/locore.h>

#include <evbmips/evbmips/clockvar.h>

struct evcnt mips_int5_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 5 (clock)");

uint32_t last_cp0_count;	/* used by microtime() */
uint32_t next_cp0_clk_intr;	/* used to schedule hard clock interrupts */

void
intr_init(void)
{

	evcnt_attach_static(&mips_int5_evcnt);
	evbmips_intr_init();	/* board specific stuff */

	softintr_init();
}

void
cpu_intr(u_int32_t status, u_int32_t cause, u_int32_t pc, u_int32_t ipending)
{
	struct clockframe cf;
	uint32_t new_cnt;

	uvmexp.intrs++;

	if (ipending & MIPS_INT_MASK_5) {
		last_cp0_count = next_cp0_clk_intr;
		next_cp0_clk_intr += curcpu()->ci_cycles_per_hz;
		mips3_cp0_compare_write(next_cp0_clk_intr);

		/* Check for lost clock interrupts */
		new_cnt = mips3_cp0_count_read();

		/* 
		 * Missed one or more clock interrupts, so let's start 
		 * counting again from the current value.
		 */
		if ((next_cp0_clk_intr - new_cnt) & 0x80000000) {
#if 0	/* XXX - should add an event counter for this */
			missed_clk_intrs++;
#endif

			next_cp0_clk_intr = new_cnt +
			    curcpu()->ci_cycles_per_hz;
			mips3_cp0_compare_write(next_cp0_clk_intr);
		}

		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);

		mips_int5_evcnt.ev_count++;

		/* Re-enable clock interrupts. */
		cause &= ~MIPS_INT_MASK_5;
		_splset(MIPS_SR_INT_IE |
		    ((status & ~cause) & MIPS_HARD_INT_MASK));
	}

	if (ipending & (MIPS_INT_MASK_0|MIPS_INT_MASK_1|MIPS_INT_MASK_2|
			MIPS_INT_MASK_3|MIPS_INT_MASK_4)) {
		/* Process I/O and error interrupts. */
		evbmips_iointr(status, cause, pc, ipending);
	}

	ipending &= (MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0);
	if (ipending == 0)
		return;

	_clrsoftintr(ipending);

	softintr_dispatch(ipending);
}
