/*	$NetBSD: gtmr.c,v 1.2.2.4 2017/12/03 11:35:52 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: gtmr.c,v 1.2.2.4 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/percpu.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <prop/proplib.h>

#include <arm/locore.h>

#include <arm/cortex/gtmr_var.h>

#include <arm/cortex/mpcore_var.h>

static int gtmr_match(device_t, cfdata_t, void *);
static void gtmr_attach(device_t, device_t, void *);

static u_int gtmr_get_timecount(struct timecounter *);

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

	if ((armreg_pfr1_read() & ARM_PFR1_GTIMER_MASK) == 0)
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
	char freqbuf[sizeof("X.XXX SHz")];

	/*
	 * This runs at a fixed frequency of 1 to 50MHz.
	 */
	if (!prop_dictionary_get_uint32(dict, "frequency", &sc->sc_freq))
		sc->sc_freq = armreg_cnt_frq_read();

	KASSERT(sc->sc_freq != 0);

	humanize_number(freqbuf, sizeof(freqbuf), sc->sc_freq, "Hz", 1000);

	aprint_naive("\n");
	aprint_normal(": ARMv7 Generic 64-bit Timer (%s)\n", freqbuf);

	/*
	 * Enable the virtual counter to be accessed from usermode.
	 */
	armreg_cntk_ctl_write(armreg_cntk_ctl_read() |
	    ARM_CNTKCTL_PL0VCTEN | ARM_CNTKCTL_PL0PCTEN);

	self->dv_private = sc;
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

	const uint32_t cnt_frq = armreg_cnt_frq_read();
	if (cnt_frq == 0) {
		aprint_verbose_dev(self, "cp15 CNT_FRQ not set\n");
	} else if (cnt_frq != sc->sc_freq) {
		aprint_verbose_dev(self,
		    "cp15 CNT_FRQ (%u) differs from supplied frequency\n",
		    cnt_frq);
	}

	gtmr_timecounter.tc_name = device_xname(sc->sc_dev);
	gtmr_timecounter.tc_frequency = sc->sc_freq;

	tc_init(&gtmr_timecounter);

	/* Disable the timer until we are ready */
	armreg_cntv_ctl_write(0);
	armreg_cntp_ctl_write(0);
}

void
gtmr_init_cpu_clock(struct cpu_info *ci)
{
	struct gtmr_softc * const sc = &gtmr_sc;

	KASSERT(ci == curcpu());

	int s = splsched();

	/*
	 * enable timer and stop masking the timer.
	 */
	armreg_cntv_ctl_write(ARM_CNTCTL_ENABLE);
	armreg_cntp_ctl_write(ARM_CNTCTL_ENABLE);
#if 0
	printf("%s: cntctl=%#x\n", __func__, armreg_cntv_ctl_read());
#endif

	/*
	 * Get now and update the compare timer.
	 */
	arm_isb();
	ci->ci_lastintr = armreg_cntv_ct_read();
	armreg_cntv_tval_write(sc->sc_autoinc);
#if 0
	printf("%s: %s: delta cval = %"PRIu64"\n",
	    __func__, ci->ci_data.cpu_name,
	    armreg_cntv_cval_read() - ci->ci_lastintr);
#endif
	splx(s);
	KASSERT(armreg_cntv_ct_read() != 0);
#if 0
	printf("%s: %s: ctl %#x cmp %#"PRIx64" now %#"PRIx64"\n",
	    __func__, ci->ci_data.cpu_name, armreg_cntv_ctl_read(),
	    armreg_cntv_cval_read(), armreg_cntv_ct_read());

	s = splsched();

	arm_isb();
	uint64_t now64;
	uint64_t start64 = armreg_cntv_ct_read();
	do {
		arm_isb();
		now64 = armreg_cntv_ct_read();
	} while (start64 == now64);
	start64 = now64;
	uint64_t end64 = start64 + 64;
	uint32_t start32 = armreg_pmccntr_read();
	do {
		arm_isb();
		now64 = armreg_cntv_ct_read();
	} while (end64 != now64);
	uint32_t end32 = armreg_pmccntr_read();

	uint32_t diff32 = end64 - start64;
	printf("%s: %s: %u cycles per tick\n",
	    __func__, ci->ci_data.cpu_name, (end32 - start32) / diff32);

	printf("%s: %s: status %#x cmp %#"PRIx64" now %#"PRIx64"\n",
	    __func__, ci->ci_data.cpu_name, armreg_cntv_ctl_read(),
	    armreg_cntv_cval_read(), armreg_cntv_ct_read());
	splx(s);
#elif 0
	delay(1000000 / hz + 1000);
#endif
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

	uint32_t freq = sc->sc_freq ? sc->sc_freq : armreg_cnt_frq_read();
	KASSERT(freq != 0);

	const unsigned int incr_per_us = howmany(freq, 1000000);
	unsigned int delta = 0, usecs = 0;

	arm_isb();
	uint64_t last = armreg_cntp_ct_read();

	while (n > usecs) {
		arm_isb();
		uint64_t curr = armreg_cntp_ct_read();
		if (curr < last)
			delta += curr + (UINT64_MAX - last);
		else
			delta += curr - last;

		last = curr;
		if (delta >= incr_per_us) {
			usecs += delta / incr_per_us;
			delta %= incr_per_us;
		}
	}
}

void
gtmr_bootdelay(unsigned int ticks)
{
	const uint32_t ctl = armreg_cntv_ctl_read();
	armreg_cntv_ctl_write(ctl | ARM_CNTCTL_ENABLE | ARM_CNTCTL_IMASK);

	/* Write Timer/Value to set new compare time */
	armreg_cntv_tval_write(ticks);

	/* Spin until compare time is hit */
	while ((armreg_cntv_ctl_read() & ARM_CNTCTL_ISTATUS) == 0) {
		/* spin */
	}

	armreg_cntv_ctl_write(ctl);
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

	arm_isb();

	const uint32_t ctl = armreg_cntv_ctl_read();
	if ((ctl & ARM_CNTCTL_ISTATUS) == 0)
		return 0;

	const uint64_t now = armreg_cntv_ct_read();
	uint64_t delta = now - ci->ci_lastintr;

#ifdef DIAGNOSTIC
	const uint64_t then = armreg_cntv_cval_read();
	struct gtmr_percpu * const pc = percpu_getref(sc->sc_percpu);
	KASSERTMSG(then <= now, "%"PRId64, now - then);
	KASSERTMSG(then + pc->pc_delta >= ci->ci_lastintr + sc->sc_autoinc,
	    "%"PRId64, then + pc->pc_delta - ci->ci_lastintr - sc->sc_autoinc);
#endif

#if 0
	printf("%s(%p): %s: now %#"PRIx64" delta %"PRIu64"\n",
	     __func__, cf, ci->ci_data.cpu_name, now, delta);
#endif
	KASSERTMSG(delta > sc->sc_autoinc / 100,
	    "%s: interrupting too quickly (delta=%"PRIu64") autoinc=%lu",
	    ci->ci_data.cpu_name, delta, sc->sc_autoinc);

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
	armreg_cntv_tval_write(sc->sc_autoinc - delta);

	ci->ci_lastintr = now;

#ifdef DIAGNOSTIC
	KASSERT(delta == (uint32_t) delta);
	pc->pc_delta = delta;
	percpu_putref(sc->sc_percpu);
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
	arm_isb();	// we want the time NOW, not some instructions later.
	return (u_int) armreg_cntp_ct_read();
}
