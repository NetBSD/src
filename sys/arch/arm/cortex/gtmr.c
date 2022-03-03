/*	$NetBSD: gtmr.c,v 1.49 2022/03/03 06:26:28 riastradh Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas
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
__KERNEL_RCSID(0, "$NetBSD: gtmr.c,v 1.49 2022/03/03 06:26:28 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/percpu.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <sys/cpu.h>

#include <prop/proplib.h>

#include <arm/locore.h>
#include <arm/cpufunc.h>

#include <arm/cortex/gtmr_var.h>
#include <arm/cortex/mpcore_var.h>

static int gtmr_match(device_t, cfdata_t, void *);
static void gtmr_attach(device_t, device_t, void *);

static u_int gtmr_get_timecount(struct timecounter *);

static uint64_t gtmr_read_cntct(struct gtmr_softc *);
static uint32_t gtmr_read_ctl(struct gtmr_softc *);
static void gtmr_write_ctl(struct gtmr_softc *, uint32_t);
static void gtmr_write_tval(struct gtmr_softc *, uint32_t);
static void gtmr_write_cval(struct gtmr_softc *, uint64_t);

static struct gtmr_softc gtmr_sc;

struct gtmr_percpu {
	uint32_t pc_delta;
};

static struct timecounter gtmr_timecounter = {
	.tc_get_timecount = gtmr_get_timecount,
	.tc_poll_pps = 0,
	.tc_counter_mask = ~0u,
	.tc_frequency = 0,			/* set by cpu_initclocks() */
	.tc_name = NULL,			/* set by attach */
	.tc_quality = 500,
	.tc_priv = &gtmr_sc,
	.tc_next = NULL,
};

CFATTACH_DECL_NEW(armgtmr, 0, gtmr_match, gtmr_attach, NULL, NULL);

/* ARGSUSED */
static int
gtmr_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mpcore_attach_args * const mpcaa = aux;

	if (gtmr_sc.sc_dev != NULL)
		return 0;

	/* Generic Timer is always implemented in ARMv8-A */
	if (!cpu_gtmr_exists_p())
		return 0;

	if (strcmp(mpcaa->mpcaa_name, cf->cf_name) != 0)
		return 0;

	return 1;
}

static void
gtmr_attach(device_t parent, device_t self, void *aux)
{
	struct mpcore_attach_args * const mpcaa = aux;
	struct gtmr_softc *sc = &gtmr_sc;
	prop_dictionary_t dict = device_properties(self);
	prop_dictionary_t pdict = device_properties(device_parent(self));
	char freqbuf[sizeof("X.XXX SHz")];
	bool flag;

	/*
	 * This runs at a fixed frequency of 1 to 50MHz.
	 */
	if (!prop_dictionary_get_uint32(dict, "frequency", &sc->sc_freq))
		sc->sc_freq = gtmr_cntfrq_read();

	if (!prop_dictionary_get_bool(dict, "physical", &sc->sc_physical))
	    prop_dictionary_get_bool(pdict, "physical", &sc->sc_physical);

	KASSERT(sc->sc_freq != 0);

	humanize_number(freqbuf, sizeof(freqbuf), sc->sc_freq, "Hz", 1000);

	aprint_naive("\n");
	aprint_normal(": Generic Timer (%s, %s)\n", freqbuf,
	    sc->sc_physical ? "physical" : "virtual");

#if defined(__arm__)
	if (prop_dictionary_get_bool(dict, "arm,cpu-registers-not-fw-configured", &flag) && flag) {
		sc->sc_flags |= GTMR_FLAG_CPU_REGISTERS_NOT_FW_CONFIGURED;
		aprint_debug_dev(self, "CPU registers not initialized by firmware\n");
	}
#endif

	if (prop_dictionary_get_bool(dict, "sun50i-a64-unstable-timer", &flag) && flag) {
		sc->sc_flags |= GTMR_FLAG_SUN50I_A64_UNSTABLE_TIMER;
		aprint_debug_dev(self, "enabling Allwinner A64 timer workaround\n");
	}

	device_set_private(self, sc);
	sc->sc_dev = self;

#ifdef DIAGNOSTIC
	sc->sc_percpu = percpu_alloc(sizeof(struct gtmr_percpu));
#endif

	evcnt_attach_dynamic(&sc->sc_ev_missing_ticks, EVCNT_TYPE_MISC, NULL,
	    device_xname(self), "missing interrupts");

	if (mpcaa->mpcaa_irq != -1) {
		sc->sc_global_ih = intr_establish(mpcaa->mpcaa_irq, IPL_CLOCK,
		    IST_LEVEL | IST_MPSAFE, gtmr_intr, NULL);
		if (sc->sc_global_ih == NULL)
			panic("%s: unable to register timer interrupt", __func__);
		aprint_normal_dev(self, "interrupting on irq %d\n",
		    mpcaa->mpcaa_irq);
	}

	const uint32_t cnt_frq = gtmr_cntfrq_read();
	if (cnt_frq == 0) {
		aprint_verbose_dev(self, "cp15 CNT_FRQ not set\n");
	} else if (cnt_frq != sc->sc_freq) {
		aprint_verbose_dev(self,
		    "cp15 CNT_FRQ (%u) differs from supplied frequency\n",
		    cnt_frq);
	}

	gtmr_timecounter.tc_name = device_xname(sc->sc_dev);
	gtmr_timecounter.tc_frequency = sc->sc_freq;
	gtmr_timecounter.tc_priv = sc;

	tc_init(&gtmr_timecounter);

	/* Disable the timer until we are ready */
	gtmr_write_ctl(sc, 0);
}

static uint64_t
gtmr_read_cntct(struct gtmr_softc *sc)
{
	isb();

	if (ISSET(sc->sc_flags, GTMR_FLAG_SUN50I_A64_UNSTABLE_TIMER)) {
		/*
		 * The Allwinner A64 SoC has an unstable architectural timer.
		 * To workaround this problem, ignore reads where the lower
		 * 10 bits are all 0s or 1s.
		 */
		uint64_t val;
		u_int bits;
		do {
			val = sc->sc_physical ? gtmr_cntpct_read() : gtmr_cntvct_read();
			bits = val & __BITS(9,0);
		} while (bits == 0 || bits == __BITS(9,0));
		return val;
	}

	return sc->sc_physical ? gtmr_cntpct_read() : gtmr_cntvct_read();
}

static uint32_t
gtmr_read_ctl(struct gtmr_softc *sc)
{
	isb();

	if (sc->sc_physical)
		return gtmr_cntp_ctl_read();
	else
		return gtmr_cntv_ctl_read();
}

static void
gtmr_write_ctl(struct gtmr_softc *sc, uint32_t val)
{
	if (sc->sc_physical)
		gtmr_cntp_ctl_write(val);
	else
		gtmr_cntv_ctl_write(val);

	isb();
}

static void
gtmr_write_tval(struct gtmr_softc *sc, uint32_t val)
{
	if (sc->sc_physical)
		gtmr_cntp_tval_write(val);
	else
		gtmr_cntv_tval_write(val);

	isb();
}

static void
gtmr_write_cval(struct gtmr_softc *sc, uint64_t val)
{
	if (sc->sc_physical)
		gtmr_cntp_cval_write(val);
	else
		gtmr_cntv_cval_write(val);

	isb();
}


void
gtmr_init_cpu_clock(struct cpu_info *ci)
{
	struct gtmr_softc * const sc = &gtmr_sc;
	uint32_t cntk;
	uint64_t ctl;

	KASSERT(ci == curcpu());

	/* XXX hmm... called from cpu_hatch which hasn't lowered ipl yet */
	int s = splsched();

#if defined(__arm__)
	if ((sc->sc_flags & GTMR_FLAG_CPU_REGISTERS_NOT_FW_CONFIGURED) != 0) {
		armreg_cnt_frq_write(sc->sc_freq);
	}
#endif

	/*
	 * Allow the virtual and physical counters to be accessed from
	 * usermode. (PL0)
	 */
	cntk = gtmr_cntk_ctl_read();
	cntk &= ~(CNTKCTL_PL0PTEN | CNTKCTL_PL0VTEN | CNTKCTL_EVNTEN);
	if (sc->sc_physical) {
		cntk |= CNTKCTL_PL0PCTEN;
		cntk &= ~CNTKCTL_PL0VCTEN;
	} else {
		cntk |= CNTKCTL_PL0VCTEN;
		cntk &= ~CNTKCTL_PL0PCTEN;
	}
	gtmr_cntk_ctl_write(cntk);
	isb();

	/*
	 * enable timer and stop masking the timer.
	 */
	ctl = gtmr_read_ctl(sc);
	ctl &= ~CNTCTL_IMASK;
	ctl |= CNTCTL_ENABLE;
	gtmr_write_ctl(sc, ctl);

	/*
	 * Get now and update the compare timer.
	 */
	ci->ci_lastintr = gtmr_read_cntct(sc);
	gtmr_write_tval(sc, sc->sc_autoinc);

	splx(s);

	KASSERT(gtmr_read_cntct(sc) != 0);
}

void
gtmr_cpu_initclocks(void)
{
	struct gtmr_softc * const sc = &gtmr_sc;

	KASSERT(sc->sc_dev != NULL);
	KASSERT(sc->sc_freq != 0);

	sc->sc_autoinc = sc->sc_freq / hz;

	gtmr_init_cpu_clock(curcpu());
}

void
gtmr_delay(unsigned int n)
{
	struct gtmr_softc * const sc = &gtmr_sc;

	KASSERT(sc != NULL);

	uint32_t freq = sc->sc_freq ? sc->sc_freq : gtmr_cntfrq_read();
	KASSERT(freq != 0);

	const unsigned int incr_per_us = howmany(freq, 1000000);
	int64_t ticks = (int64_t)n * incr_per_us;

	uint64_t last = gtmr_read_cntct(sc);

	while (ticks > 0) {
		uint64_t curr = gtmr_read_cntct(sc);
		if (curr >= last)
			ticks -= (curr - last);
		else
			ticks -= (UINT64_MAX - curr + last);
		last = curr;
	}
}

/*
 * gtmr_intr:
 *
 *	Handle the hardclock interrupt.
 */
int
gtmr_intr(void *arg)
{
	struct cpu_info * const ci = curcpu();
	struct clockframe * const cf = arg;
	struct gtmr_softc * const sc = &gtmr_sc;

	const uint32_t ctl = gtmr_read_ctl(sc);
	if ((ctl & (CNTCTL_ENABLE|CNTCTL_ISTATUS)) != (CNTCTL_ENABLE|CNTCTL_ISTATUS)) {
		aprint_debug_dev(ci->ci_dev, "spurious timer interrupt (ctl=%#x)\n", ctl);
		return 0;
	}

	const uint64_t now = gtmr_read_cntct(sc);
	uint64_t delta = now - ci->ci_lastintr;

#ifdef DIAGNOSTIC
	struct gtmr_percpu *pc = NULL;
	if (!ISSET(sc->sc_flags, GTMR_FLAG_SUN50I_A64_UNSTABLE_TIMER)) {
		const uint64_t then = sc->sc_physical ? gtmr_cntp_cval_read() : gtmr_cntv_cval_read();
		pc = percpu_getref(sc->sc_percpu);
		KASSERTMSG(then <= now, "%"PRId64, now - then);
		KASSERTMSG(then + pc->pc_delta >= ci->ci_lastintr + sc->sc_autoinc,
		    "%"PRId64, then + pc->pc_delta - ci->ci_lastintr - sc->sc_autoinc);
	}
#endif

	if (!ISSET(sc->sc_flags, GTMR_FLAG_SUN50I_A64_UNSTABLE_TIMER)) {
		KASSERTMSG(delta > sc->sc_autoinc / 100,
		    "%s: interrupting too quickly (delta=%"PRIu64") autoinc=%lu",
		    ci->ci_data.cpu_name, delta, sc->sc_autoinc);
	}

	/*
	 * If we got interrupted too soon (delta < sc->sc_autoinc)
	 * or we missed (or almost missed) a tick
	 * (delta >= 7 * sc->sc_autoinc / 4), don't try to adjust for jitter.
	 */
	if (delta >= sc->sc_autoinc && delta <= 7 * sc->sc_autoinc / 4) {
		delta -= sc->sc_autoinc;
	} else {
		delta = 0;
	}

	isb();
	if (ISSET(sc->sc_flags, GTMR_FLAG_SUN50I_A64_UNSTABLE_TIMER)) {
		gtmr_write_cval(sc, now + sc->sc_autoinc - delta);
	} else {
		gtmr_write_tval(sc, sc->sc_autoinc - delta);
	}

	ci->ci_lastintr = now;

#ifdef DIAGNOSTIC
	if (!ISSET(sc->sc_flags, GTMR_FLAG_SUN50I_A64_UNSTABLE_TIMER)) {
		KASSERT(delta == (uint32_t) delta);
		pc->pc_delta = delta;
		percpu_putref(sc->sc_percpu);
	}
#endif

	hardclock(cf);

	sc->sc_ev_missing_ticks.ev_count += delta / sc->sc_autoinc;

	return 1;
}

void
setstatclockrate(int newhz)
{
}

static u_int
gtmr_get_timecount(struct timecounter *tc)
{
	struct gtmr_softc * const sc = tc->tc_priv;

	return (u_int) gtmr_read_cntct(sc);
}
