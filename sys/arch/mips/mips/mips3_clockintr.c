/*	$NetBSD: mips3_clockintr.c,v 1.9.6.1 2011/06/06 09:06:07 jruoho Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah Hdr: clock.c 1.18 91/01/21
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mips3_clockintr.c,v 1.9.6.1 2011/06/06 09:06:07 jruoho Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/evcnt.h>
#include <sys/intr.h>

#include <mips/mips3_clock.h>

#include <machine/locore.h>

/*
 * Handling to be done upon receipt of a MIPS 3 clock interrupt.  This
 * routine is to be called from the master interrupt routine
 * (e.g. cpu_intr), if MIPS INT5 is pending.  The caller is
 * responsible for blocking and renabling the interrupt in the
 * cpu_intr() routine.
 */
void
mips3_clockintr(struct clockframe *cfp)
{
	struct cpu_info * const ci = curcpu();
	uint32_t new_cnt;

	ci->ci_ev_count_compare.ev_count++;

	KASSERT((ci->ci_cycles_per_hz & ~(0xffffffff)) == 0);
	ci->ci_next_cp0_clk_intr += (uint32_t)(ci->ci_cycles_per_hz & 0xffffffff);
	mips3_cp0_compare_write(ci->ci_next_cp0_clk_intr);

	/* Check for lost clock interrupts */
	new_cnt = mips3_cp0_count_read();

	/* 
	 * Missed one or more clock interrupts, so let's start 
	 * counting again from the current value.
	 */
	if ((ci->ci_next_cp0_clk_intr - new_cnt) & 0x80000000) {

		ci->ci_next_cp0_clk_intr = new_cnt + curcpu()->ci_cycles_per_hz;
		mips3_cp0_compare_write(ci->ci_next_cp0_clk_intr);
		curcpu()->ci_ev_count_compare_missed.ev_count++;
	}

	/*
	 * Since hardclock is at the end, we can invoke it by a tailcall.
	 */

	hardclock(cfp);

	/* caller should renable clock interrupts */
}

/*
 * Start the real-time and statistics clocks. Leave stathz 0 since there
 * are no other timers available.
 */
void
mips3_initclocks(void)
{
	struct cpu_info * const ci = curcpu();

	ci->ci_next_cp0_clk_intr = mips3_cp0_count_read() + ci->ci_cycles_per_hz;
	mips3_cp0_compare_write(ci->ci_next_cp0_clk_intr);

	mips3_init_tc();

	/*
	 * Now we can enable all interrupts including hardclock(9)
	 * by CPU INT5.
	 */
	spl0();
}

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
mips3_setstatclockrate(int newhz)
{

	/* nothing we can do */
}

__weak_alias(setstatclockrate, mips3_setstatclockrate);
__weak_alias(cpu_initclocks, mips3_initclocks);
