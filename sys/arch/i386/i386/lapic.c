/* $NetBSD: lapic.c,v 1.1.2.15 2002/04/30 14:09:07 sommerfeld Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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


#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_mpbios.h"		/* for MPDEBUG */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/pmap.h>
#include <machine/vmparam.h>
#include <machine/mpbiosvar.h>
#include <machine/pcb.h>
#include <machine/specialreg.h>
#include <machine/segments.h>

#include <machine/apicvar.h>
#include <machine/i82489reg.h>
#include <machine/i82489var.h>

#include <i386/isa/timerreg.h>	/* XXX for TIMER_FREQ */

void		lapic_delay __P((int));
void		lapic_microtime __P((struct timeval *));
static u_int32_t lapic_gettick __P((void));
void		lapic_clockintr __P((void *));
static void 	lapic_map __P((paddr_t));

static void
lapic_map(lapic_base)
	paddr_t lapic_base;
{
	int s;
	pt_entry_t *pte;
	vaddr_t va = (vaddr_t)&local_apic;

	disable_intr();
	s = lapic_tpr;

	/*
	 * Map local apic.  If we have a local apic, it's safe to assume
	 * we're on a 486 or better and can use invlpg and non-cacheable PTE's
	 *
	 * Whap the PTE "by hand" rather than calling pmap_kenter_pa because
	 * the latter will attempt to invoke TLB shootdown code just as we
	 * might have changed the value of cpu_number()..
	 */

	pte = kvtopte(va);
	*pte = lapic_base | PG_RW | PG_V | PG_N | pmap_pg_g;
	invlpg(va);

#ifdef MULTIPROCESSOR
	cpu_init_first();	/* catch up to changed cpu_number() */
#endif

	lapic_tpr = s;
	enable_intr();
}

/*
 * enable local apic
 */
void
lapic_enable()
{
	i82489_writereg(LAPIC_SVR, LAPIC_SVR_ENABLE | LAPIC_SPURIOUS_VECTOR);
}

extern struct mp_intr_map *lapic_ints[]; /* XXX header file? */

void
lapic_set_lvt ()
{
#ifdef MULTIPROCESSOR
	struct cpu_info *ci = curcpu();

	if (mp_verbose) {
		apic_format_redir (ci->ci_dev->dv_xname, "prelint", 0, 0,
		    i82489_readreg(LAPIC_LVINT0));
		apic_format_redir (ci->ci_dev->dv_xname, "prelint", 1, 0,
		    i82489_readreg(LAPIC_LVINT1));
	}
#endif
	if (lapic_ints[0])
		i82489_writereg(LAPIC_LVINT0, lapic_ints[0]->redir);
	if (lapic_ints[1])
		i82489_writereg(LAPIC_LVINT1, lapic_ints[1]->redir);

#ifdef MULTIPROCESSOR
	if (mp_verbose) {
		apic_format_redir (ci->ci_dev->dv_xname, "timer", 0, 0,
		    i82489_readreg(LAPIC_LVTT));
		apic_format_redir (ci->ci_dev->dv_xname, "pcint", 0, 0,
		    i82489_readreg(LAPIC_PCINT));
		apic_format_redir (ci->ci_dev->dv_xname, "lint", 0, 0,
		    i82489_readreg(LAPIC_LVINT0));
		apic_format_redir (ci->ci_dev->dv_xname, "lint", 1, 0,
		    i82489_readreg(LAPIC_LVINT1));
		apic_format_redir (ci->ci_dev->dv_xname, "err", 0, 0,
		    i82489_readreg(LAPIC_LVERR));
	}
#endif
}

/*
 * Initialize fixed idt vectors for use by local apic.
 */
void
lapic_boot_init(lapic_base)
	paddr_t lapic_base;
{
	lapic_map(lapic_base);

#ifdef MULTIPROCESSOR
	idt_vec_set(LAPIC_IPI_VECTOR, Xintripi);
#endif
	idt_vec_set(LAPIC_SPURIOUS_VECTOR, Xintrspurious);
	idt_vec_set(LAPIC_TIMER_VECTOR, Xintrltimer);
}

static inline u_int32_t lapic_gettick()
{
	return i82489_readreg(LAPIC_CCR_TIMER);
}

#include <sys/kernel.h>		/* for hz */

int lapic_timer = 0;
u_int32_t lapic_tval;

/*
 * this gets us up to a 4GHz busclock....
 */
u_int32_t lapic_per_second;
u_int32_t lapic_frac_usec_per_cycle;
u_int64_t lapic_frac_cycle_per_usec;
u_int32_t lapic_delaytab[26];

void
lapic_clockintr (arg)
	void *arg;
{
	struct clockframe *frame = arg;
#if defined(I586_CPU) || defined(I686_CPU)
	static int microset_iter; /* call tsc_microset once/sec */
	struct cpu_info *ci = curcpu();
	extern struct timeval tsc_time;

	/*
	 * If we have a cycle counter, do the microset thing.
	 */
	if (ci->ci_feature_flags & CPUID_TSC) {
		if (
#if defined(MULTIPROCESSOR)
		    CPU_IS_PRIMARY(ci) &&
#endif
		    (microset_iter--) == 0) {
			microset_iter = hz-1;
			tsc_time = time;
#if defined(MULTIPROCESSOR)
			i386_broadcast_ipi(I386_IPI_MICROSET);
#endif
			tsc_microset(ci);
		}
	}
#endif

	hardclock(frame);
}

void
lapic_initclocks ()
{
	/*
	 * Start local apic countdown timer running, in repeated mode.
	 *
	 * Mask the clock interrupt and set mode,
	 * then set divisor,
	 * then unmask and set the vector.
	 */
	i82489_writereg (LAPIC_LVTT, LAPIC_LVTT_TM|LAPIC_LVTT_M);
	i82489_writereg (LAPIC_DCR_TIMER, LAPIC_DCRT_DIV1);
	i82489_writereg (LAPIC_ICR_TIMER, lapic_tval);
	i82489_writereg (LAPIC_LVTT, LAPIC_LVTT_TM|LAPIC_TIMER_VECTOR);
}

extern int gettick __P((void));	/* XXX put in header file */
extern int rtclock_tval; /* XXX put in header file */
extern void (*initclock_func) __P((void)); /* XXX put in header file */

/*
 * Calibrate the local apic count-down timer (which is running at
 * bus-clock speed) vs. the i8254 counter/timer (which is running at
 * a fixed rate).
 *
 * The Intel MP spec says: "An MP operating system may use the IRQ8
 * real-time clock as a reference to determine the actual APIC timer clock
 * speed."
 *
 * We're actually using the IRQ0 timer.  Hmm.
 */
void
lapic_calibrate_timer(ci)
	struct cpu_info *ci;
{
	unsigned int starttick, tick1, tick2, endtick;
	unsigned int startapic, apic1, apic2, endapic;
	u_int64_t dtick, dapic, tmp;
	int i;
	char tbuf[9];

	printf("%s: calibrating local timer\n", ci->ci_dev->dv_xname);

	/*
	 * Configure timer to one-shot, interrupt masked,
	 * large positive number.
	 */
	i82489_writereg (LAPIC_LVTT, LAPIC_LVTT_M);
	i82489_writereg (LAPIC_DCR_TIMER, LAPIC_DCRT_DIV1);
	i82489_writereg (LAPIC_ICR_TIMER, 0x80000000);

	starttick = gettick();
	startapic = lapic_gettick();

	for (i=0; i<hz; i++) {
		DELAY(2);
		do {
			tick1 = gettick();
			apic1 = lapic_gettick();
		} while (tick1 < starttick);
		DELAY(2);
		do {
			tick2 = gettick();
			apic2 = lapic_gettick();
		} while (tick2 > starttick);
	}

	endtick = gettick();
	endapic = lapic_gettick();

	dtick = hz * rtclock_tval + (starttick-endtick);
	dapic = startapic-endapic;

	/*
	 * there are TIMER_FREQ ticks per second.
	 * in dtick ticks, there are dapic bus clocks.
	 */
	tmp = (TIMER_FREQ * dapic) / dtick;

	lapic_per_second = tmp;

	humanize_number(tbuf, sizeof(tbuf), tmp, "Hz", 1000);

	printf("%s: apic clock running at %s\n", ci->ci_dev->dv_xname, tbuf);

	if (lapic_per_second != 0) {
		/*
		 * reprogram the apic timer to run in periodic mode.
		 * XXX need to program timer on other cpu's, too.
		 */
		lapic_tval = (lapic_per_second * 2) / hz;
		lapic_tval = (lapic_tval / 2) + (lapic_tval & 0x1);

		i82489_writereg (LAPIC_LVTT, LAPIC_LVTT_TM|LAPIC_LVTT_M
		    |LAPIC_TIMER_VECTOR);
		i82489_writereg (LAPIC_DCR_TIMER, LAPIC_DCRT_DIV1);
		i82489_writereg (LAPIC_ICR_TIMER, lapic_tval);

		/*
		 * Compute fixed-point ratios between cycles and
		 * microseconds to avoid having to do any division
		 * in lapic_delay and lapic_microtime.
		 */

		tmp = (1000000 * (u_int64_t)1<<32) / lapic_per_second;
		lapic_frac_usec_per_cycle = tmp;

		tmp = (lapic_per_second * (u_int64_t)1<<32) / 1000000;

		lapic_frac_cycle_per_usec = tmp;

		/*
		 * Compute delay in cycles for likely short delays in usec.
		 */
		for (i=0; i<26; i++)
			lapic_delaytab[i] = (lapic_frac_cycle_per_usec * i) >>
			    32;

		/*
		 * Now that the timer's calibrated, use the apic timer routines
		 * for all our timing needs..
		 */
		delay_func = lapic_delay;
		initclock_func = lapic_initclocks;
	}
}

/*
 * delay for N usec.
 */

void lapic_delay(usec)
	int usec;
{
	int32_t tick, otick;
	int64_t deltat;		/* XXX may want to be 64bit */

	otick = lapic_gettick();

	if (usec <= 0)
		return;
	if (usec <= 25)
		deltat = lapic_delaytab[usec];
	else
		deltat = (lapic_frac_cycle_per_usec * usec) >> 32;

	while (deltat > 0) {
		tick = lapic_gettick();
		if (tick > otick)
			deltat -= lapic_tval - (tick - otick);
		else
			deltat -= otick - tick;
		otick = tick;
	}
}

/*
 * XXX the following belong mostly or partly elsewhere..
 */

int
i386_ipi_init(target)
	int target;
{
	unsigned j;

	if ((target&LAPIC_DEST_MASK)==0) {
		i82489_writereg(LAPIC_ICRHI, target<<LAPIC_ID_SHIFT);
	}

	i82489_writereg(LAPIC_ICRLO, (target & LAPIC_DEST_MASK) |
	    LAPIC_DLMODE_INIT | LAPIC_LVL_ASSERT );

	for (j=100000; j > 0; j--)
		if ((i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) == 0)
			break;

	delay(10000);

	i82489_writereg(LAPIC_ICRLO, (target & LAPIC_DEST_MASK) |
	     LAPIC_DLMODE_INIT | LAPIC_LVL_TRIG | LAPIC_LVL_DEASSERT);

	for (j=100000; j > 0; j--)
		if ((i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) == 0)
			break;

	return (i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY)?EBUSY:0;
}

int
i386_ipi(vec,target,dl)
	int vec,target,dl;
{
	unsigned j;
	int result;

	for (j=100000;
	     j > 0 && (i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY);
	     j--)
		;

	if ((target & LAPIC_DEST_MASK) == 0)
		i82489_writereg(LAPIC_ICRHI, target << LAPIC_ID_SHIFT);

	i82489_writereg(LAPIC_ICRLO,
	    (target & LAPIC_DEST_MASK) | vec | dl | LAPIC_LVL_ASSERT);

	for (j=100000;
	     j > 0 && (i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY);
	     j--)
		;

	result = (i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) ? EBUSY : 0;

	return result;
}

