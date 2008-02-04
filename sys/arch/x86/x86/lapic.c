/* $NetBSD: lapic.c,v 1.12.2.9 2008/02/04 09:22:51 yamt Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lapic.c,v 1.12.2.9 2008/02/04 09:22:51 yamt Exp $");

#include "opt_ddb.h"
#include "opt_mpbios.h"		/* for MPDEBUG */
#include "opt_multiprocessor.h"
#include "opt_ntp.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/timetc.h>

#include <uvm/uvm_extern.h>

#include <dev/ic/i8253reg.h>

#include <machine/cpu.h>
#include <machine/cpu_counter.h>
#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/pmap.h>
#include <machine/vmparam.h>
#include <machine/mpbiosvar.h>
#include <machine/pcb.h>
#include <machine/specialreg.h>
#include <machine/segments.h>
#include <x86/x86/tsc.h>
#include <x86/i82093var.h>

#include <machine/apicvar.h>
#include <machine/i82489reg.h>
#include <machine/i82489var.h>

/* Referenced from vector.S */
void		lapic_clockintr(void *, struct intrframe *);

static void	lapic_delay(unsigned int);
static uint32_t lapic_gettick(void);
static void 	lapic_map(paddr_t);

static void lapic_hwmask(struct pic *, int);
static void lapic_hwunmask(struct pic *, int);
static void lapic_setup(struct pic *, struct cpu_info *, int, int, int);

struct pic local_pic = {
	.pic_dev = {
		.dv_xname = "lapic",
	},
	.pic_type = PIC_LAPIC,
	.pic_lock = __SIMPLELOCK_UNLOCKED,
	.pic_hwmask = lapic_hwmask,
	.pic_hwunmask = lapic_hwunmask,
	.pic_addroute =lapic_setup,
	.pic_delroute = lapic_setup,
};

static void
lapic_map(paddr_t lapic_base)
{
	int s;
	pt_entry_t *pte;
	vaddr_t va = (vaddr_t)&local_apic;

	x86_disable_intr();
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
	x86_enable_intr();
}

/*
 * enable local apic
 */
void
lapic_enable(void)
{
	i82489_writereg(LAPIC_SVR, LAPIC_SVR_ENABLE | LAPIC_SPURIOUS_VECTOR);
}

void
lapic_suspend(void)
{
}

void
lapic_set_lvt(void)
{
	struct cpu_info *ci = curcpu();
	int i;
	struct mp_intr_map *mpi;
	uint32_t lint0, lint1;

#ifdef MULTIPROCESSOR
	if (mp_verbose) {
		apic_format_redir (ci->ci_dev->dv_xname, "prelint", 0, 0,
		    i82489_readreg(LAPIC_LVINT0));
		apic_format_redir (ci->ci_dev->dv_xname, "prelint", 1, 0,
		    i82489_readreg(LAPIC_LVINT1));
	}
#endif

	/*
	 * If an I/O APIC has been attached, assume that it is used instead of
	 * the 8259A for interrupt delivery.  Otherwise request the LAPIC to
	 * get external interrupts via LINT0 for the primary CPU.
	 */
	lint0 = LAPIC_DLMODE_EXTINT;
	if (nioapics > 0 || !CPU_IS_PRIMARY(curcpu()))
		lint0 |= LAPIC_LVT_MASKED;
	i82489_writereg(LAPIC_LVINT0, lint0);

	/*
	 * Non Maskable Interrupts are to be delivered to the primary CPU.
	 */
	lint1 = LAPIC_DLMODE_NMI;
	if (!CPU_IS_PRIMARY(curcpu()))
		lint1 |= LAPIC_LVT_MASKED;
	i82489_writereg(LAPIC_LVINT1, lint1);

	for (i = 0; i < mp_nintr; i++) {
		mpi = &mp_intrs[i];
		if (mpi->ioapic == NULL && (mpi->cpu_id == MPS_ALL_APICS
					    || mpi->cpu_id == ci->ci_apicid)) {
#ifdef DIAGNOSTIC
			if (mpi->ioapic_pin > 1)
				panic("lapic_set_lvt: bad pin value %d",
				    mpi->ioapic_pin);
#endif
			if (mpi->ioapic_pin == 0)
				i82489_writereg(LAPIC_LVINT0, mpi->redir);
			else
				i82489_writereg(LAPIC_LVINT1, mpi->redir);
		}
	}

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
lapic_boot_init(paddr_t lapic_base)
{
	lapic_map(lapic_base);

#ifdef MULTIPROCESSOR
	idt_vec_reserve(LAPIC_IPI_VECTOR);
	idt_vec_set(LAPIC_IPI_VECTOR, Xintr_lapic_ipi);
	idt_vec_reserve(LAPIC_TLB_MCAST_VECTOR);
	idt_vec_set(LAPIC_TLB_MCAST_VECTOR, Xintr_lapic_tlb_mcast);
	idt_vec_reserve(LAPIC_TLB_BCAST_VECTOR);
	idt_vec_set(LAPIC_TLB_BCAST_VECTOR, Xintr_lapic_tlb_bcast);
#endif
	idt_vec_reserve(LAPIC_SPURIOUS_VECTOR);
	idt_vec_set(LAPIC_SPURIOUS_VECTOR, Xintrspurious);

	idt_vec_reserve(LAPIC_TIMER_VECTOR);
	idt_vec_set(LAPIC_TIMER_VECTOR, Xintr_lapic_ltimer);
}

static uint32_t
lapic_gettick(void)
{
	return i82489_readreg(LAPIC_CCR_TIMER);
}

#include <sys/kernel.h>		/* for hz */

int lapic_timer = 0;
uint32_t lapic_tval;

/*
 * this gets us up to a 4GHz busclock....
 */
uint32_t lapic_per_second;
uint32_t lapic_frac_usec_per_cycle;
uint64_t lapic_frac_cycle_per_usec;
uint32_t lapic_delaytab[26];

static u_int
lapic_get_timecount(struct timecounter *tc)
{
	struct cpu_info *ci;
	uint32_t cur_timer;
	int s;

	s = splhigh();
	ci = curcpu();

	/*
	 * Check for a race against the clockinterrupt.
	 * The update of ci_lapic_counter is blocked by splhigh() and
	 * the check for a pending clockinterrupt compensates for that.
	 *
	 * If the current tick is almost the Initial Counter, explicitly
	 * check for the pending interrupt bit as the interrupt delivery
	 * could be asynchronious and compensate as well.
	 *
	 * This can't be done without splhigh() as the calling code might
	 * have masked the clockinterrupt already.
	 *
	 * This code assumes that clockinterrupts are not missed.
	 */
	cur_timer = lapic_gettick();
	if (cur_timer >= lapic_tval - 1) {
		uint16_t reg = LAPIC_IRR + LAPIC_TIMER_VECTOR / 32 * 16;

		if (i82489_readreg(reg) & (1 << (LAPIC_TIMER_VECTOR % 32))) {
			cur_timer -= lapic_tval;
		}
	} else if (ci->ci_istate.ipending & (1 << LIR_TIMER))
		cur_timer = lapic_gettick() - lapic_tval;
	cur_timer = ci->ci_lapic_counter - cur_timer;
	splx(s);

	return cur_timer;
}

static struct timecounter lapic_timecounter = {
	lapic_get_timecount,
	NULL,
	~0u,
	0,
	"lapic",
#ifndef MULTIPROCESSOR
	2100,
#else
	-100, /* per CPU state */
#endif
	NULL,
	NULL,
};

extern u_int i8254_get_timecount(struct timecounter *);

void
lapic_clockintr(void *arg, struct intrframe *frame)
{
#if defined(TIMECOUNTER_DEBUG)
	static u_int last_count[X86_MAXPROCS],
		     last_delta[X86_MAXPROCS],
		     last_tsc[X86_MAXPROCS],
		     last_tscdelta[X86_MAXPROCS],
	             last_factor[X86_MAXPROCS];
#endif /* TIMECOUNTER_DEBUG */
	struct cpu_info *ci = curcpu();

	ci->ci_lapic_counter += lapic_tval;
	ci->ci_isources[LIR_TIMER]->is_evcnt.ev_count++;

#if defined(TIMECOUNTER_DEBUG)
	{
		int cid = ci->ci_cpuid;
		u_int c_count = i8254_get_timecount(NULL);
		u_int c_tsc = cpu_counter32();
		u_int delta, ddelta, tsc_delta, factor = 0;
		int idelta;

		if (c_count > last_count[cid])
			delta = c_count - last_count[cid];
		else
			delta = 0x100000000ULL - last_count[cid] + c_count;

		if (delta > last_delta[cid])
			ddelta = delta - last_delta[cid];
		else
			ddelta = last_delta[cid] - delta;

		if (c_tsc > last_tsc[cid])
			tsc_delta = c_tsc - last_tsc[cid];
		else
			tsc_delta = 0x100000000ULL - last_tsc[cid] + c_tsc;

		idelta = tsc_delta - last_tscdelta[cid];
		if (idelta < 0)
			idelta = -idelta;

		if (delta) {
			int fdelta = tsc_delta / delta - last_factor[cid];
			if (fdelta < 0)
				fdelta = -fdelta;

			if (fdelta > last_factor[cid] / 10) {
				printf("cpu%d: freq skew exceeds 10%%: delta %u, "
				    "factor %u, last %u\n", cid, fdelta,
				    tsc_delta / delta, last_factor[cid]);
			}
			factor = tsc_delta / delta;
		}

		if (ddelta > last_delta[cid] / 10) {
			printf("cpu%d: tick delta exceeds 10%%: delta %u, "
			    "last %u, tick %u, last %u, factor %u, last %u\n",
			    cid, ddelta, last_delta[cid], c_count,
			    last_count[cid], factor, last_factor[cid]);
		}

		if (last_count[cid] > c_count) {
			printf("cpu%d: tick wrapped/lost: delta %u, tick %u, "
			    "last %u\n", cid, last_count[cid] - c_count,
			    c_count, last_count[cid]);
		}

		if (idelta > last_tscdelta[cid] / 10) {
			printf("cpu%d: TSC delta exceeds 10%%: delta %u, "
			    "last %u, tsc %u, factor %u, last %u\n", cid, idelta,
			    last_tscdelta[cid], last_tsc[cid],
			    factor, last_factor[cid]);
		}
 
		last_factor[cid]   = factor;
		last_delta[cid]    = delta;
		last_count[cid]    = c_count;
		last_tsc[cid]      = c_tsc;
		last_tscdelta[cid] = tsc_delta;
	}
#endif /* TIMECOUNTER_DEBUG */

	hardclock((struct clockframe *)frame);
}

void
lapic_initclocks(void)
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

extern unsigned int gettick(void);	/* XXX put in header file */
extern int rtclock_tval; /* XXX put in header file */
extern void (*initclock_func)(void); /* XXX put in header file */

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
lapic_calibrate_timer(struct cpu_info *ci)
{
	unsigned int seen, delta, initial_i8254, initial_lapic;
	unsigned int cur_i8254, cur_lapic;
	uint64_t tmp;
	int i;
	char tbuf[9];

	aprint_verbose("%s: calibrating local timer\n", ci->ci_dev->dv_xname);

	/*
	 * Configure timer to one-shot, interrupt masked,
	 * large positive number.
	 */
	i82489_writereg (LAPIC_LVTT, LAPIC_LVTT_M);
	i82489_writereg (LAPIC_DCR_TIMER, LAPIC_DCRT_DIV1);
	i82489_writereg (LAPIC_ICR_TIMER, 0x80000000);

	x86_disable_intr();

	initial_lapic = lapic_gettick();
	initial_i8254 = gettick();

	for (seen = 0; seen < TIMER_FREQ / 100; seen += delta) {
		cur_i8254 = gettick();
		if (cur_i8254 > initial_i8254)
			delta = rtclock_tval - (cur_i8254 - initial_i8254);
		else
			delta = initial_i8254 - cur_i8254;
		initial_i8254 = cur_i8254;
	}
	cur_lapic = lapic_gettick();

	x86_enable_intr();

	tmp = initial_lapic - cur_lapic;
	lapic_per_second = (tmp * TIMER_FREQ + seen / 2) / seen;

	humanize_number(tbuf, sizeof(tbuf), lapic_per_second, "Hz", 1000);

	aprint_verbose("%s: apic clock running at %s\n",
	    ci->ci_dev->dv_xname, tbuf);

	if (lapic_per_second != 0) {
		/*
		 * reprogram the apic timer to run in periodic mode.
		 * XXX need to program timer on other CPUs, too.
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
		 * in lapic_delay.
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
		initrtclock(0);

		if (lapic_timecounter.tc_frequency == 0) {
			/*
			 * Hook up time counter.
			 * This assume that all LAPICs have the same frequency.
			 */
			lapic_timecounter.tc_frequency = lapic_per_second;
			tc_init(&lapic_timecounter);
		}
	}
}

/*
 * delay for N usec.
 */

static void
lapic_delay(unsigned int usec)
{
	int32_t xtick, otick;
	int64_t deltat;		/* XXX may want to be 64bit */

	otick = lapic_gettick();

	if (usec <= 0)
		return;
	if (usec <= 25)
		deltat = lapic_delaytab[usec];
	else
		deltat = (lapic_frac_cycle_per_usec * usec) >> 32;

	while (deltat > 0) {
		xtick = lapic_gettick();
		if (xtick > otick)
			deltat -= lapic_tval - (xtick - otick);
		else
			deltat -= otick - xtick;
		otick = xtick;

		x86_pause();
	}
}

/*
 * XXX the following belong mostly or partly elsewhere..
 */

static void
i82489_icr_wait(void)
{
#ifdef DIAGNOSTIC
	unsigned j = 100000;
#endif /* DIAGNOSTIC */

	while ((i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) != 0) {
		x86_pause();
#ifdef DIAGNOSTIC
		j--;
		if (j == 0)
			panic("i82489_icr_wait: busy");
#endif /* DIAGNOSTIC */
	}
}

int
x86_ipi_init(int target)
{

	if ((target&LAPIC_DEST_MASK)==0) {
		i82489_writereg(LAPIC_ICRHI, target<<LAPIC_ID_SHIFT);
	}

	i82489_writereg(LAPIC_ICRLO, (target & LAPIC_DEST_MASK) |
	    LAPIC_DLMODE_INIT | LAPIC_LEVEL_ASSERT );

	i82489_icr_wait();

	i8254_delay(10000);

	i82489_writereg(LAPIC_ICRLO, (target & LAPIC_DEST_MASK) |
	     LAPIC_DLMODE_INIT | LAPIC_TRIGGER_LEVEL | LAPIC_LEVEL_DEASSERT);

	i82489_icr_wait();

	return (i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY)?EBUSY:0;
}

int
x86_ipi(int vec, int target, int dl)
{
	int result, s;

	s = splclock();

	i82489_icr_wait();

	if ((target & LAPIC_DEST_MASK) == 0)
		i82489_writereg(LAPIC_ICRHI, target << LAPIC_ID_SHIFT);

	i82489_writereg(LAPIC_ICRLO,
	    (target & LAPIC_DEST_MASK) | vec | dl | LAPIC_LEVEL_ASSERT);

#ifdef DIAGNOSTIC
	i82489_icr_wait();
	result = (i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) ? EBUSY : 0;
#else
	/* Don't wait - if it doesn't go, we're in big trouble anyway. */
        result = 0;
#endif
	splx(s);

	return result;
}


/*
 * Using 'pin numbers' as:
 * 0 - timer
 * 1 - unused
 * 2 - PCINT
 * 3 - LVINT0
 * 4 - LVINT1
 * 5 - LVERR
 */

static void
lapic_hwmask(struct pic *pic, int pin)
{
	int reg;
	u_int32_t val;

	reg = LAPIC_LVTT + (pin << 4);
	val = i82489_readreg(reg);
	val |= LAPIC_LVT_MASKED;
	i82489_writereg(reg, val);
}

static void
lapic_hwunmask(struct pic *pic, int pin)
{
	int reg;
	u_int32_t val;

	reg = LAPIC_LVTT + (pin << 4);
	val = i82489_readreg(reg);
	val &= ~LAPIC_LVT_MASKED;
	i82489_writereg(reg, val);
}

static void
lapic_setup(struct pic *pic, struct cpu_info *ci,
    int pin, int idtvec, int type)
{
}
