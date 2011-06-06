/* $NetBSD: au_timer.c,v 1.9.26.1 2011/06/06 09:06:02 jruoho Exp $ */

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: au_timer.c,v 1.9.26.1 2011/06/06 09:06:02 jruoho Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <mips/locore.h>

#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>

/*
 * Set a programmable clock register.
 * If "wait" is non-zero, wait for that bit to become 0 in the
 * counter control register before and after writing to the
 * specified clock register.
 */
#define	SET_PC_REG(reg, wait, val)					\
do {									\
	if (wait)							\
		while (bus_space_read_4(st, sh, PC_COUNTER_CONTROL)	\
		    & (wait))						\
			/* nothing */;					\
	bus_space_write_4(st, sh, (reg), (val));			\
	if (wait)							\
		while (bus_space_read_4(st, sh, (reg)) & (wait))	\
			/* nothing */;					\
} while (0)

void
au_cal_timers(bus_space_tag_t st, bus_space_handle_t sh)
{
	struct cpu_info * const ci = curcpu();
	uint32_t ctrdiff[4], startctr, endctr;
	uint32_t ctl, ctr, octr;
	int i;

	/* Enable the programmable counter 1. */
	ctl = bus_space_read_4(st, sh, PC_COUNTER_CONTROL);
	if ((ctl & (CC_EO | CC_EN1)) != (CC_EO | CC_EN1))
		SET_PC_REG(PC_COUNTER_CONTROL, 0, ctl | CC_EO | CC_EN1);

	/* Initialize for 16Hz. */
	SET_PC_REG(PC_TRIM1, CC_T1S, PC_RATE / 16 - 1);

	/* Run the loop an extra time to prime the cache. */
	for (i = 0; i < 4; i++) {
		/* Reset the counter. */
		SET_PC_REG(PC_COUNTER_WRITE1, CC_C1S, 0);

		/* Wait for 1/16th of a second. */
		//startctr = mips3_cp0_count_read();

		/* Wait for the PC to tick over. */
		ctr = bus_space_read_4(st, sh, PC_COUNTER_READ_1);
		do {
			octr = bus_space_read_4(st, sh, PC_COUNTER_READ_1);
		} while (ctr == octr);

		startctr = mips3_cp0_count_read();
		do {
			ctr = bus_space_read_4(st, sh, PC_COUNTER_READ_1);
		} while (ctr == octr);	// while (ctr <= octr + 1);
		endctr = mips3_cp0_count_read();
		ctrdiff[i] = endctr - startctr;
	}

	/* Disable the counter (if it wasn't enabled already). */
	if ((ctl & (CC_EO | CC_EN1)) != (CC_EO | CC_EN1))
		SET_PC_REG(PC_COUNTER_CONTROL, 0, ctl);

	/* Compute the number of cycles per second. */
	ci->ci_cpu_freq = ((ctrdiff[2] + ctrdiff[3]) / 2) * 16;
	ci->ci_cctr_freq = ci->ci_cpu_freq;

	/* Compute the number of ticks for hz. */
	ci->ci_cycles_per_hz = (ci->ci_cpu_freq + hz / 2) / hz;

	/* Compute the delay divisor. */
	ci->ci_divisor_delay = (ci->ci_cpu_freq + 500000) / 1000000;

	/*
	 * Get correct cpu frequency if the CPU runs at twice the
	 * external/cp0-count frequency.
	 */
	if (mips_options.mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		ci->ci_cpu_freq *= 2;

#ifdef DEBUG
	printf("Timer calibration: %lu cycles/sec [(%u, %u) * 16]\n",
	    ci->ci_cpu_freq, ctrdiff[2], ctrdiff[3]);
#endif
}
