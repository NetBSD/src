/*	$NetBSD: clock.c,v 1.35 2006/10/11 02:31:19 uwe Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.35 2006/10/11 02:31:19 uwe Exp $");

#include "opt_pclock.h"
#include "opt_hz.h"
#include "wdog.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/timetc.h>

#include <dev/clock_subr.h>

#include <sh3/clock.h>
#include <sh3/exception.h>
#include <sh3/rtcreg.h>
#include <sh3/tmureg.h>
#include <sh3/wdogvar.h>
#include <sh3/wdtreg.h>

#include <machine/intr.h>

#ifndef HZ
#define	HZ		64
#endif
#define	SH_RTC_CLOCK	16384	/* Hz */

/*
 * NetBSD/sh3 clock module
 *  + default 64Hz
 *  + use TMU channel 0 as clock interrupt source.
 *  + use TMU channel 1 as emulated software interrupt soruce.
 *  + use TMU channel 2 as freerunning counter for timecounter.
 *  + If RTC module is active, TMU channel 0 input source is RTC output.
 *    (16.384kHz)
 */
struct {
	/* Hard clock */
	uint32_t hz_cnt;	/* clock interrupt interval count */
	uint32_t cpucycle_1us;	/* calibrated loop variable (1 us) */
	uint32_t tmuclk;	/* source clock of TMU0 (Hz) */

	uint32_t pclock;	/* PCLOCK */
	uint32_t cpuclock;	/* CPU clock */
	int flags;

	struct timecounter tc;
} sh_clock = {
#ifdef PCLOCK
	.pclock = PCLOCK,
#endif
};

uint32_t maxwdog;

struct evcnt sh_hardclock_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "tmu0", "hardclock");

/* TMU */
/* interrupt handler is timing critical. prepared for each. */
int sh3_clock_intr(void *);
int sh4_clock_intr(void *);
u_int sh_timecounter_get(struct timecounter *);

/*
 * Estimate CPU and Peripheral clock.
 */
#define	TMU_START(x)							\
do {									\
	_reg_bclr_1(SH_(TSTR), TSTR_STR##x);				\
	_reg_write_4(SH_(TCNT ## x), 0xffffffff);			\
	_reg_bset_1(SH_(TSTR), TSTR_STR##x);				\
} while (/*CONSTCOND*/0)

#define	TMU_ELAPSED(x)							\
	(0xffffffff - _reg_read_4(SH_(TCNT ## x)))

void
sh_clock_init(int flags)
{
	uint32_t s, t0;

	sh_clock.flags = flags;

	/* Initialize TMU */
	_reg_write_2(SH_(TCR0), 0);
	_reg_write_2(SH_(TCR1), 0);
	_reg_write_2(SH_(TCR2), 0);

	/* Reset RTC alarm and interrupt */
	_reg_write_1(SH_(RCR1), 0);

	/* Stop all counter */
	_reg_write_1(SH_(TSTR), 0);

	/*
	 * Estimate CPU clock.
	 */
	if (sh_clock.flags & SH_CLOCK_NORTC) {
		/* Set TMU channel 0 source to PCLOCK / 16 */
		_reg_write_2(SH_(TCR0), TCR_TPSC_P16);
		sh_clock.tmuclk = sh_clock.pclock / 16;
	} else {
		/* Set TMU channel 0 source to RTC counter clock (16.384kHz) */
		_reg_write_2(SH_(TCR0),
		    CPU_IS_SH3 ? SH3_TCR_TPSC_RTC : SH4_TCR_TPSC_RTC);
		sh_clock.tmuclk = SH_RTC_CLOCK;

		/* Make sure RTC oscillator is enabled */
		_reg_write_1(SH_(RCR2), SH_RCR2_ENABLE);
	}

	s = _cpu_exception_suspend();
	_cpu_spin(1);	/* load function on cache. */
	TMU_START(0);
	_cpu_spin(10000000);
	t0 = TMU_ELAPSED(0);
	_cpu_exception_resume(s);

	sh_clock.cpuclock
	    = ((uint64_t)sh_clock.tmuclk * 10000000 * 10 + t0/2) / t0;
	sh_clock.cpucycle_1us = (sh_clock.tmuclk * 10) / t0;

	if (CPU_IS_SH4)
		sh_clock.cpuclock >>= 1;	/* two-issue */

	/*
	 * Estimate PCLOCK
	 */
	if (sh_clock.pclock == 0) {
		uint32_t t1;

		/* set TMU channel 1 source to PCLOCK / 4 */
		_reg_write_2(SH_(TCR1), TCR_TPSC_P4);
		s = _cpu_exception_suspend();
		_cpu_spin(1);	/* load function on cache. */
		TMU_START(0);
		TMU_START(1);
		_cpu_spin(sh_clock.cpuclock); /* 1 sec. */
		t0 = TMU_ELAPSED(0);
		t1 = TMU_ELAPSED(1);
		_cpu_exception_resume(s);

		sh_clock.pclock
		    = ((uint64_t)t1 * 4 * SH_RTC_CLOCK + t0/2) / t0;
	}

	/* Stop all counter */
	_reg_write_1(SH_(TSTR), 0);

#undef TMU_START
#undef TMU_ELAPSED
}

int
sh_clock_get_cpuclock()
{

	return (sh_clock.cpuclock);
}

int
sh_clock_get_pclock()
{

	return (sh_clock.pclock);
}

void
setstatclockrate(int newhz)
{
	/* XXX not yet */
}

u_int
sh_timecounter_get(struct timecounter *tc)
{

	return 0xffffffff - _reg_read_4(SH_(TCNT2));
}

/*
 *  Wait at least `n' usec.
 */
void
delay(int n)
{

	_cpu_spin(sh_clock.cpucycle_1us * n);
}

/*
 * Start the clock interrupt.
 */
void
cpu_initclocks()
{

	if (sh_clock.pclock == 0)
		panic("No PCLOCK information.");

	/* Set global variables. */
	hz = HZ;
	tick = 1000000 / hz;

	/*
	 * Use TMU channel 0 as hard clock
	 */
	_reg_bclr_1(SH_(TSTR), TSTR_STR0);

	if (sh_clock.flags & SH_CLOCK_NORTC) {
		/* use PCLOCK/16 as TMU0 source */
		_reg_write_2(SH_(TCR0), TCR_UNIE | TCR_TPSC_P16);
	} else {
		/* use RTC clock as TMU0 source */
		_reg_write_2(SH_(TCR0), TCR_UNIE |
		    (CPU_IS_SH3 ? SH3_TCR_TPSC_RTC : SH4_TCR_TPSC_RTC));
	}
	sh_clock.hz_cnt = sh_clock.tmuclk / hz - 1;

	_reg_write_4(SH_(TCOR0), sh_clock.hz_cnt);
	_reg_write_4(SH_(TCNT0), sh_clock.hz_cnt);

	evcnt_attach_static(&sh_hardclock_evcnt);
	intc_intr_establish(SH_INTEVT_TMU0_TUNI0, IST_LEVEL, IPL_CLOCK,
	    CPU_IS_SH3 ? sh3_clock_intr : sh4_clock_intr, 0);
	/* start hardclock */
	_reg_bset_1(SH_(TSTR), TSTR_STR0);

	/*
	 * TMU channel 1 is one shot timer for softintr(9).
	 */
	_reg_write_2(SH_(TCR1), TCR_UNIE | TCR_TPSC_P4);
	_reg_write_4(SH_(TCOR1), 0xffffffff);

	/*
	 * TMU channel 2 is freerunning counter for timecounter(9).
	 */
	_reg_write_2(SH_(TCR2), TCR_TPSC_P4);
	_reg_write_4(SH_(TCOR2), 0xffffffff);

	/*
	 * Start and initialize timecounter.
	 */
	_reg_bset_1(SH_(TSTR), TSTR_STR2);

	sh_clock.tc.tc_get_timecount = sh_timecounter_get;
	sh_clock.tc.tc_frequency = sh_clock.pclock / 4;
	sh_clock.tc.tc_name = "tmu_pclock_4";
	sh_clock.tc.tc_quality = 0;
	sh_clock.tc.tc_counter_mask = 0xffffffff;
	tc_init(&sh_clock.tc);
}


#ifdef SH3
int
sh3_clock_intr(void *arg) /* trap frame */
{
#if (NWDOG > 0)
	uint32_t i;

	i = (uint32_t)SHREG_WTCNT_R;
	if (i > maxwdog)
		maxwdog = i;
	wdog_wr_cnt(0);			/* reset to zero */
#endif

	sh_hardclock_evcnt.ev_count++;

	/* clear underflow status */
	_reg_bclr_2(SH3_TCR0, TCR_UNF);

	hardclock(arg);

	return (1);
}
#endif /* SH3 */
#ifdef SH4
int
sh4_clock_intr(void *arg) /* trap frame */
{
#if (NWDOG > 0)
	uint32_t i;

	i = (uint32_t)SHREG_WTCNT_R;
	if (i > maxwdog)
		maxwdog = i;
	wdog_wr_cnt(0);			/* reset to zero */
#endif

	sh_hardclock_evcnt.ev_count++;

	/* clear underflow status */
	_reg_bclr_2(SH4_TCR0, TCR_UNF);

	hardclock(arg);

	return (1);
}
#endif /* SH4 */
