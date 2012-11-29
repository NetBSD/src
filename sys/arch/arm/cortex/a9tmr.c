/*	$NetBSD: a9tmr.c,v 1.4 2012/11/29 17:36:56 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: a9tmr.c,v 1.4 2012/11/29 17:36:56 matt Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <prop/proplib.h>

#include <arm/cortex/a9tmr_reg.h>
#include <arm/cortex/a9tmr_var.h>

#include <arm/cortex/mpcore_var.h>

static int a9tmr_match(device_t, cfdata_t, void *);
static void a9tmr_attach(device_t, device_t, void *);

static int clockhandler(void *);

static u_int a9tmr_get_timecount(struct timecounter *);

static struct a9tmr_softc a9tmr_sc;

static struct timecounter a9tmr_timecounter = {
	.tc_get_timecount = a9tmr_get_timecount,
	.tc_poll_pps = 0,
	.tc_counter_mask = ~0u,
	.tc_frequency = 0,			/* set by cpu_initclocks() */
	.tc_name = NULL,			/* set by attach */
	.tc_quality = 500,
	.tc_priv = &a9tmr_sc,
	.tc_next = NULL,
};

CFATTACH_DECL_NEW(a9tmr, 0, a9tmr_match, a9tmr_attach, NULL, NULL);

static inline uint32_t
a9tmr_global_read(struct a9tmr_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_memt, sc->sc_global_memh, o);
}

static inline void
a9tmr_global_write(struct a9tmr_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_memt, sc->sc_global_memh, o, v);
}


/* ARGSUSED */
static int
a9tmr_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mpcore_attach_args * const mpcaa = aux;

	if (a9tmr_sc.sc_dev != NULL)
		return 0;

	if (!CPU_ID_CORTEX_A9_P(curcpu()->ci_arm_cpuid))
		return 0;

	if (strcmp(mpcaa->mpcaa_name, cf->cf_name) != 0)
		return 0;

	/*
	 * This isn't present on UP A9s (since CBAR isn't present).
	 */
	uint32_t mpidr = armreg_mpidr_read();
	if (mpidr == 0 || (mpidr & MPIDR_U))
		return 0;

	return 1;
}

static void
a9tmr_attach(device_t parent, device_t self, void *aux)
{
        struct a9tmr_softc *sc = &a9tmr_sc;
	struct mpcore_attach_args * const mpcaa = aux;
	prop_dictionary_t dict = device_properties(self);
	char freqbuf[sizeof("XXX SHz")];

	/*
	 * This runs at the ARM PERIPHCLOCK which should be 1/2 of the CPU clock.
	 * The MD code should have setup our frequency for us.
	 */
	prop_number_t pn = prop_dictionary_get(dict, "frequency");
	KASSERT(pn != NULL);
	sc->sc_freq = prop_number_unsigned_integer_value(pn);

	humanize_number(freqbuf, sizeof(freqbuf), sc->sc_freq, "Hz", 1000);

	aprint_naive("\n");
	aprint_normal(": A9 Global 64-bit Timer (%s)\n", freqbuf);

	self->dv_private = sc;
	sc->sc_dev = self;
	sc->sc_memt = mpcaa->mpcaa_memt;
	sc->sc_memh = mpcaa->mpcaa_memh;

	evcnt_attach_dynamic(&sc->sc_ev_missing_ticks, EVCNT_TYPE_MISC, NULL,
	    device_xname(self), "missing interrupts");

	bus_space_subregion(sc->sc_memt, sc->sc_memh, 
	    TMR_GLOBAL_BASE, TMR_GLOBAL_BASE, &sc->sc_global_memh);
	bus_space_subregion(sc->sc_memt, sc->sc_memh, 
	    TMR_PRIVATE_BASE, TMR_PRIVATE_SIZE, &sc->sc_private_memh);
	bus_space_subregion(sc->sc_memt, sc->sc_memh, 
	    TMR_WDOG_BASE, TMR_WDOG_SIZE, &sc->sc_wdog_memh);

	sc->sc_global_ih = intr_establish(IRQ_A9TMR_PPI_GTIMER, IPL_CLOCK,
	    IST_EDGE, clockhandler, NULL);
	if (sc->sc_global_ih == NULL)
		panic("%s: unable to register timer interrupt", __func__);
	aprint_normal_dev(sc->sc_dev, "interrupting on irq %d\n",
	    IRQ_A9TMR_PPI_GTIMER);
}

static inline uint64_t
a9tmr_gettime(struct a9tmr_softc *sc)
{
	uint32_t lo, hi;

	do {
		hi = a9tmr_global_read(sc, TMR_GBL_CTR_U);
		lo = a9tmr_global_read(sc, TMR_GBL_CTR_L);
	} while (hi != a9tmr_global_read(sc, TMR_GBL_CTR_U));

	return ((uint64_t)hi << 32) | lo;
}

void
a9tmr_init_cpu_clock(struct cpu_info *ci)
{
	struct a9tmr_softc * const sc = &a9tmr_sc;
	uint64_t now = a9tmr_gettime(sc);

	KASSERT(ci == curcpu());

	ci->ci_lastintr = now;

	a9tmr_global_write(sc, TMR_GBL_AUTOINC, sc->sc_autoinc);

	/*
	 * To update the compare register we have to disable comparisions first.
	 */
	uint32_t ctl = a9tmr_global_read(sc, TMR_GBL_CTL);
	if (ctl & TMR_GBL_CTL_CMP_ENABLE) {
		a9tmr_global_write(sc, TMR_GBL_CTL, ctl & ~TMR_GBL_CTL_CMP_ENABLE);
	}

	/*
	 * Schedule the next interrupt.
	 */
	now += sc->sc_autoinc;
	a9tmr_global_write(sc, TMR_GBL_CMP_L, (uint32_t) now);
	a9tmr_global_write(sc, TMR_GBL_CMP_H, (uint32_t) (now >> 32));

	/*
	 * Re-enable the comparator and now enable interrupts.
	 */
	a9tmr_global_write(sc, TMR_GBL_INT, 1);	/* clear interrupt pending */
	ctl |= TMR_GBL_CTL_CMP_ENABLE | TMR_GBL_CTL_INT_ENABLE | TMR_GBL_CTL_AUTO_INC | TMR_CTL_ENABLE;
	a9tmr_global_write(sc, TMR_GBL_CTL, ctl);
#if 0
	printf("%s: %s: ctl %#x autoinc %u cmp %#x%08x now %#"PRIx64"\n",
	    __func__, ci->ci_data.cpu_name,
	    a9tmr_global_read(sc, TMR_GBL_CTL),
	    a9tmr_global_read(sc, TMR_GBL_AUTOINC),
	    a9tmr_global_read(sc, TMR_GBL_CMP_H),
	    a9tmr_global_read(sc, TMR_GBL_CMP_L),
	    a9tmr_gettime(sc));

	int s = splsched();
	uint64_t when = now;
	u_int n = 0;
	while ((now = a9tmr_gettime(sc)) < when) {
		/* spin */
		n++;
		KASSERTMSG(n <= sc->sc_autoinc,
		    "spun %u times but only %"PRIu64" has passed",
		    n, when - now);
	}
	printf("%s: %s: status %#x cmp %#x%08x now %#"PRIx64"\n",
	    __func__, ci->ci_data.cpu_name,
	    a9tmr_global_read(sc, TMR_GBL_INT),
	    a9tmr_global_read(sc, TMR_GBL_CMP_H),
	    a9tmr_global_read(sc, TMR_GBL_CMP_L),
	    a9tmr_gettime(sc));
	splx(s);
#elif 0
	delay(1000000 / hz + 1000); 
#endif
}

void
cpu_initclocks(void)
{
	struct a9tmr_softc * const sc = &a9tmr_sc;
	
	KASSERT(sc->sc_dev != NULL);
	KASSERT(sc->sc_freq != 0);

	sc->sc_autoinc = sc->sc_freq / hz;

	a9tmr_init_cpu_clock(curcpu());

	a9tmr_timecounter.tc_name = device_xname(sc->sc_dev);
	a9tmr_timecounter.tc_frequency = sc->sc_freq;

	tc_init(&a9tmr_timecounter);
}

void
a9tmr_delay(unsigned int n)
{
	struct a9tmr_softc * const sc = &a9tmr_sc;

	KASSERT(sc != NULL);

	uint32_t freq = sc->sc_freq ? sc->sc_freq : curcpu()->ci_data.cpu_cc_freq / 2;
	KASSERT(freq != 0);

	/*
	 * not quite divide by 1000000 but close enough
	 * (higher by 1.3% which means we wait 1.3% longer).
	 */
	const uint64_t incr_per_us = (freq >> 20) + (freq >> 24);

	const uint64_t delta = n * incr_per_us;
	const uint64_t base = a9tmr_gettime(sc);
	const uint64_t finish = base + delta;

	while (a9tmr_gettime(sc) < finish) {
		/* spin */
	}
}

/*
 * clockhandler:
 *
 *	Handle the hardclock interrupt.
 */
static int
clockhandler(void *arg)
{
	struct clockframe * const cf = arg;
	struct a9tmr_softc * const sc = &a9tmr_sc;
	struct cpu_info * const ci = curcpu();
	
	const uint64_t now = a9tmr_gettime(sc);
	uint64_t delta = now - ci->ci_lastintr;

	a9tmr_global_write(sc, TMR_GBL_INT, 1);	// Ack the interrupt

#if 0
	printf("%s(%p): %s: now %#"PRIx64" delta %"PRIu64"\n", 
	     __func__, cf, ci->ci_data.cpu_name, now, delta);
#endif
	KASSERTMSG(delta > sc->sc_autoinc / 100,
	    "%s: interrupting too quickly (delta=%"PRIu64")",
	    ci->ci_data.cpu_name, delta);

	ci->ci_lastintr = now;

	hardclock(cf);

#if 0
	/*
	 * Try to make up up to a seconds amount of missed clock interrupts
	 */
	u_int ticks = hz;
	for (delta -= sc->sc_autoinc;
	     ticks > 0 && delta >= sc->sc_autoinc;
	     delta -= sc->sc_autoinc, ticks--) {
		hardclock(cf);
	}
#else
	if (delta > sc->sc_autoinc)
		sc->sc_ev_missing_ticks.ev_count += delta / sc->sc_autoinc;
#endif

	return 1;
}

void
setstatclockrate(int newhz)
{
}

static u_int
a9tmr_get_timecount(struct timecounter *tc)
{
	struct a9tmr_softc * const sc = tc->tc_priv;

	return (u_int) (a9tmr_gettime(sc));
}
