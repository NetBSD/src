/* $NetBSD: mips_mcclock.c,v 1.10.8.1 2002/04/01 07:41:11 nathanw Exp $ */

/*
 * Copyright (c) 1997 Jonathan Stone (hereinafter referred to as the author)
 * All rights reserved.
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
 *      This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mips_mcclock.c,v 1.10.8.1 2002/04/01 07:41:11 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ic/mc146818reg.h>
#include <dev/dec/mcclockvar.h>
#include <dev/dec/mcclock_pad32.h>

#include <mips/cpu.h>			/* MIPS_HAS_CLOCK */
#include <mips/locore.h>		/* mips_cp0_cause_read() */
#include <mips/mips/mips_mcclock.h>


unsigned mips_mc_cpuspeed(void *, int, int (*)(void *, int));
int mips_mcclock_tickloop(void *, int);
unsigned mips_mcclock_to_mhz(unsigned iters);


/*
 * Compute MHz and DELAY() constants using the default
 * polling function.
 */
unsigned
mc_cpuspeed(mcclock_addr, cpuintmask)
	vaddr_t mcclock_addr;
    	int cpuintmask;
{
	return mips_mc_cpuspeed((void *)mcclock_addr, cpuintmask,
	           mips_mcclock_tickloop);
}


/*
 * Estimate CPU cycle speed by counting cycles (acutally executions of a
 * one-line loop) between two adjacent ticks of an mc146818 clock.
 * Return loop iteration count so hand-calibrated  MD  code can
 * estimate clock speed from cycles.
 *
 * Runs before CPU is attached (so we can print CPU speed)  which is
 * before the clock is attached, so we can't use the normal clock driver.
 */
unsigned
mips_mc_cpuspeed(mcclock_addr, clockmask, tickpollfn)
	void *mcclock_addr;
	int clockmask;
	int (*tickpollfn)(void *mcclock_addr, int clockmask);
{
	int s;
	int iters = 0;
	int saved_rega, saved_regb;
	volatile struct mcclock_pad32_clockdatum *clk = (void *)mcclock_addr;

	/*
	 * Block all interrupts, including clock ticks.
	 */
	s = splhigh();

	/*
	 * Enable periodic interrupst on the mc146818,
	 * and set it up for 256Hz (4ms) interrupts.
	 * Save any state we change so we can restore it on exit.
	 */
	saved_rega = clk[MC_REGA].datum;
	saved_regb = clk[MC_REGB].datum;

#if 0
	mcclock_addr->rega = (saved_rega & ~MC_BASE_RESET) | MC_RATE_256_Hz;
	mcclock_addr->regb = MC_REGB_BINARY|MC_REGB_24HR|MC_REGB_PIE;
#else
	clk[MC_REGA].datum = MC_BASE_32_KHz | MC_RATE_256_Hz;
	clk[MC_REGB].datum = MC_REGB_BINARY|MC_REGB_24HR|MC_REGB_PIE| MC_REGB_SQWE;
#endif
	/* count loop iterations between ticks */
	iters = (*tickpollfn)(mcclock_addr, clockmask);

	/* Restore mcclock registers */
	clk[MC_REGA].datum = saved_rega;
	clk[MC_REGB].datum = saved_regb;

	splx(s);

	/*
	 * Compute approximate CPU speed in MHz, and an
	 * appropriate base for  DELAY() and delay(), from
	 * the number of completed iterations.
	 */
	cpu_mhz = mips_mcclock_to_mhz(iters);

#if defined(DEBUG)
	printf("mcclock: iters %d computed MHz %d, instrs per usec=%d\n",
	       iters, cpu_mhz, cpuspeed);
#endif
	return (iters);
}


/*
 * Poll mcclock chip for the next tick interrupt and count
 * instructions until the subsequent tick.
 *
 * XXX Assumes the mcclock chip has exclusive use of a CPU interrupt line.
 * XXX Assumes bus access to clock registers is cheap (not a function call).
 *  MD alternatives must be used where this doesn't hold.
 */

int
mips_mcclock_tickloop(mcclock_addr, clockmask)
	void *mcclock_addr;
	int clockmask;
{
	int iters = 0;
	volatile int junk;
	volatile struct mcclock_pad32_clockdatum *clk = mcclock_addr;

	/* clear any old pending interrupts */
	junk = clk[MC_REGC].datum;
	junk++;	junk++;	junk++;	junk++;

	/* Poll clock interrupt, waiting for next tick to happen. */
	while ((mips_cp0_cause_read() & clockmask) == 0)
		;

	/* Ack the mc146818 interrupt caused by starting tick. */
	junk = clk[MC_REGC].datum;

	junk++;	junk++;	junk++;	junk++;

	/* Count loops until next tick-interrupt request occurs (4ms). */
	if (MIPS_HAS_CLOCK) {
		while ((mips_cp0_cause_read() & clockmask) == 0) {
			__asm __volatile ("nop; nop; nop; nop");
			iters++;
		}
	} else {
		while ((mips_cp0_cause_read() & clockmask) == 0) {
			__asm __volatile ("nop; nop;");
			iters++;
		}
	}

	/* Ack the  interrupt from the just-gone-off tick */
	junk = clk[MC_REGC].datum;

	return (iters);
}


/*
 * mips_mcclock_to_mhz(iters) -- convert an mcclock cycles-per-tick count
 * 	to a CPU speed in MHz.
 *
 * Side Effects:
 * 	set the global variables "cpuspeed", used by DELAY() and delay()
 *	as an instructions-per-microsecond multiplier, to an value appropriate
 *	for the estimated clock speed.
 */
unsigned
mips_mcclock_to_mhz(unsigned iters)
{
	unsigned mhz = 0;

	/* XXX KN01? */

	/*
	 * Measured thresholds for Digital systems from Sean Davidson.
	 *
	 * r3000-core DECstations values fit to:
	 *     iters per 4ms tick = 425 * MHz)
	 *     instructions per mhz = kHz * 575
	 * with about 2 Mhz slop to allow for variation.
	 */

#ifdef MIPS3_PLUS
	if (CPUISMIPS3) {
		if (iters < 18100) {
			/* error */
			printf("mcclock loop count %d too low for r4000\n",
			       iters);
			mhz = 45;
			cpuspeed = 20;	/* XXX */
		} else if (iters < 21000) {
			mhz = 50;
			cpuspeed = 25;	/* XXX */
		} else if (iters < 25315) {
			mhz = 60;
			cpuspeed = 27;	/* XXX */
		} else if (iters < 28497) {
			mhz = 67;
			cpuspeed = 33;	/* XXX */
		} else if (iters < 31500) {
			mhz = 75;
			cpuspeed = 38;	/* XXX */
		}
	}
#endif /* MIPS3_PLUS */

#ifdef MIPS1
	if (!CPUISMIPS3) {
		if (iters < 5100) {
			/* assume memory-bound DS 2100 */
			mhz = 12;	/* 12.5 MHz? */
			cpuspeed = 8;
		} else if (iters < 6700) {
			/* assume memory-bound DS3100 */
			mhz = 15;
			cpuspeed = 8;
		} else if (iters < 8800) {
			mhz = 20;
			cpuspeed = 11;
		} else if (iters < 11300) {
			mhz = 25;
			cpuspeed = 13;
		} else if (iters < 14000) {
			mhz = 33;
			cpuspeed = 19;
		} else if (iters < 15000) {
			mhz = 36;
			cpuspeed = 21;
		} else if (iters < 16000) {
			mhz = 40;
			cpuspeed = 23;
		} else if (iters < 18800) {
			mhz = 45;
			cpuspeed = 25;
		} else if (iters < 21000) {
			mhz = 50;
			cpuspeed = 29;
		} else if (iters < 26000) {
			mhz = 60;
			cpuspeed = 35;
		} else {
			/* XXX */
			mhz = 70;
			cpuspeed = 40;
		}
	}
#endif	/* MIPS1 */

	return (mhz);
}
