/*	$NetBSD: clock.c,v 1.1.4.2 2002/06/23 17:39:34 jdolecek Exp $	*/
/*      $OpenBSD: clock.c,v 1.3 1997/10/13 13:42:53 pefo Exp $  */

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>
#include <machine/intrpriv.h>

u_long ticks_per_sec;
static u_long ns_per_tick;
static long ticks_per_intr;
static volatile u_long lasttb;

void decr_intr(struct clockframe *); /* Called from trap_subr.S */
static inline u_quad_t mftb(void);

void
decr_intr(struct clockframe *frame)
{
	u_long tb;
	long tick;
	int nticks;
	int pri, msr;

#if 1
	{
#define XXSHFT 7
		extern void setleds(int);
		int csr, cer;
		csr = in32(0xff500880);
		cer = in32(0xff500888);
		/*x = in32(0xff500898);*/
		setleds(((cer >> XXSHFT) & 3) | (((csr >> XXSHFT) & 3) << 2));
	}
#endif
	/*
	 * Check whether we are initialized.
	 */
	if (!ticks_per_intr)
		return;

	/*
	 * Based on the actual time delay since the last decrementer reload,
	 * we arrange for earlier interrupt next time.
	 */
	asm ("mfdec %0" : "=r"(tick));
	for (nticks = 0; tick < 0; nticks++)
		tick += ticks_per_intr;
	asm volatile ("mtdec %0" :: "r"(tick));

	uvmexp.intrs++;
	intrcnt[CNT_CLOCK]++;

	pri = splclock();
	if (pri & SPL_CLOCK) {
		tickspending += nticks;
	} else {
		nticks += tickspending;
		tickspending = 0;

		/*
		 * lasttb is used during microtime. Set it to the virtual
		 * start of this tick interval.
		 */
		asm ("mftb %0" : "=r"(tb));
		lasttb = tb + tick - ticks_per_intr;

		/*
		 * Reenable interrupts
		 */
		asm volatile ("mfmsr %0; ori %0, %0, %1; mtmsr %0"
			      : "=r"(msr) : "K"(PSL_EE));
		
		/*
		 * Do standard timer interrupt stuff.
		 * Do softclock stuff only on the last iteration.
		 */
		frame->pri = pri | SINT_CLOCK;
		while (--nticks > 0)
			hardclock(frame);
		frame->pri = pri;
		hardclock(frame);
	}
	splx(pri);
}

void
cpu_initclocks(void)
{
	ticks_per_intr = ticks_per_sec / hz;
	ns_per_tick = 1000000000 / ticks_per_sec;
	asm volatile ("mftb %0" : "=r"(lasttb));
	asm volatile ("mtdec %0" :: "r"(ticks_per_intr));
}

static __inline u_quad_t
mftb(void)
{
	u_long scratch;
	u_quad_t tb;
	
	asm ("1: mftbu %0; mftb %0+1; mftbu %1; cmpw 0,%0,%1; bne 1b"
	    : "=r"(tb), "=r"(scratch));
	return tb;
}

/*
 * Fill in *tvp with current time with microsecond resolution.
 */
void
microtime(struct timeval *tvp)
{
	u_long tb;
	u_long ticks;
	int msr, scratch;
	
	asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
		      : "=r"(msr), "=r"(scratch) : "K"((u_short)~PSL_EE));
	asm ("mftb %0" : "=r"(tb));
	ticks = (tb - lasttb) * ns_per_tick;
	*tvp = time;
	asm volatile ("mtmsr %0" :: "r"(msr));
	ticks /= 1000;
	tvp->tv_usec += ticks;
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}
}

/*
 * Wait for about n microseconds (at least!).
 */
void
delay(unsigned int n)
{
	u_quad_t tb;
	u_long tbh, tbl, scratch;
	
	tb = mftb();
	tb += (n * 1000 + ns_per_tick - 1) / ns_per_tick;
	tbh = tb >> 32;
	tbl = tb;
	asm volatile ("1: mftbu %0; cmplw %0,%1; blt 1b; bgt 2f;"
		      "mftb %0; cmplw %0,%2; blt 1b; 2:"
		      : "=r"(scratch) : "r"(tbh), "r"(tbl));
}

/*
 * Nothing to do.
 */
void
setstatclockrate(int arg)
{
	/* Do nothing */
}
