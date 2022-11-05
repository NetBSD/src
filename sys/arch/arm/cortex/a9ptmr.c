/*	$NetBSD: a9ptmr.c,v 1.3 2022/11/05 17:30:20 jmcneill Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: a9ptmr.c,v 1.3 2022/11/05 17:30:20 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/xcall.h>

#include <prop/proplib.h>

#include <arm/cortex/a9tmr_reg.h>
#include <arm/cortex/a9ptmr_var.h>

#include <arm/cortex/mpcore_var.h>

static struct a9ptmr_softc *a9ptmr_sc;

static int a9ptmr_match(device_t, cfdata_t, void *);
static void a9ptmr_attach(device_t, device_t, void *);

struct a9ptmr_softc {
	device_t sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;

	uint32_t sc_ctl;
	uint32_t sc_freq;
	uint32_t sc_load;

	uint32_t sc_prescaler;
};


CFATTACH_DECL_NEW(arma9ptmr, sizeof(struct a9ptmr_softc),
    a9ptmr_match, a9ptmr_attach, NULL, NULL);

static bool attached;

static inline uint32_t
a9ptmr_read(struct a9ptmr_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_memt, sc->sc_memh, o);
}

static inline void
a9ptmr_write(struct a9ptmr_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_memt, sc->sc_memh, o, v);
}

/* ARGSUSED */
static int
a9ptmr_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mpcore_attach_args * const mpcaa = aux;

	if (attached)
		return 0;

	if (!CPU_ID_CORTEX_A9_P(curcpu()->ci_arm_cpuid) &&
	    !CPU_ID_CORTEX_A5_P(curcpu()->ci_arm_cpuid))
		return 0;

	if (strcmp(mpcaa->mpcaa_name, cf->cf_name) != 0)
		return 0;

#if 0
	/*
	 * This isn't present on UP A9s (since CBAR isn't present).
	 */
	uint32_t mpidr = armreg_mpidr_read();
	if (mpidr == 0 || (mpidr & MPIDR_U))
		return 0;
#endif

	return 1;
}


static void
a9ptmr_attach(device_t parent, device_t self, void *aux)
{
	struct a9ptmr_softc * const sc = device_private(self);
	struct mpcore_attach_args * const mpcaa = aux;
	prop_dictionary_t dict = device_properties(self);
	char freqbuf[sizeof("XXX SHz")];
	const char *cpu_type;


	sc->sc_dev = self;
	sc->sc_memt = mpcaa->mpcaa_memt;

	bus_space_subregion(sc->sc_memt, mpcaa->mpcaa_memh,
	    mpcaa->mpcaa_off1, TMR_PRIVATE_SIZE, &sc->sc_memh);

	/*
	 * This runs at the ARM PERIPHCLOCK.
	 * The MD code should have setup our frequency for us.
	 */
	if (!prop_dictionary_get_uint32(dict, "frequency", &sc->sc_freq)) {
		dict = device_properties(parent);
		prop_dictionary_get_uint32(dict, "frequency", &sc->sc_freq);
	}

	humanize_number(freqbuf, sizeof(freqbuf), sc->sc_freq, "Hz", 1000);

	a9ptmr_sc = sc;
	sc->sc_dev = self;
	sc->sc_memt = mpcaa->mpcaa_memt;
	sc->sc_memh = mpcaa->mpcaa_memh;

	sc->sc_ctl = a9ptmr_read(sc, TMR_CTL);

	sc->sc_prescaler = 1;
#if 0
	/*
	 * Let's hope the timer frequency isn't prime.
	 */
	for (size_t div = 256; div >= 2; div--) {
		if (sc->sc_freq % div == 0) {
			sc->sc_prescaler = div;
			break;
		}
	}
	sc->sc_freq /= sc->sc_prescaler;
#endif

	aprint_debug(": freq %d prescaler %d", sc->sc_freq,
	    sc->sc_prescaler);
	sc->sc_ctl = TMR_CTL_INT_ENABLE | TMR_CTL_AUTO_RELOAD | TMR_CTL_ENABLE;
	sc->sc_ctl |= __SHIFTIN(sc->sc_prescaler - 1, TMR_CTL_PRESCALER);

	sc->sc_load = (sc->sc_freq / hz) - 1;

	aprint_debug(": load %d ", sc->sc_load);

	a9ptmr_init_cpu_clock(curcpu());

	aprint_naive("\n");
	if (CPU_ID_CORTEX_A5_P(curcpu()->ci_arm_cpuid)) {
		cpu_type = "A5";
	} else {
		cpu_type = "A9";
	}
	aprint_normal(": %s Private Timer (%s)\n", cpu_type, freqbuf);

	attached = true;
}



void
a9ptmr_delay(unsigned int n)
{
	struct a9ptmr_softc * const sc = a9ptmr_sc;

	KASSERT(sc != NULL);

	uint32_t freq = sc->sc_freq ? sc->sc_freq :
	    curcpu()->ci_data.cpu_cc_freq / 2;
	KASSERT(freq != 0);

	const uint64_t counts_per_usec = freq / 1000000;
	uint32_t delta, usecs, last, curr;

	KASSERT(sc != NULL);

	last = a9ptmr_read(sc, TMR_CTR);

	delta = usecs = 0;
	while (n > usecs) {
		curr = a9ptmr_read(sc, TMR_CTR);

		/* Check to see if the timer has reloaded. */
		if (curr > last)
			delta += (sc->sc_load - curr) + last;
		else
			delta += last - curr;

		last = curr;

		if (delta >= counts_per_usec) {
			usecs += delta / counts_per_usec;
			delta %= counts_per_usec;
		}
	}
}


void
a9ptmr_cpu_initclocks(void)
{
	struct a9ptmr_softc * const sc __diagused = a9ptmr_sc;

	KASSERT(sc->sc_dev != NULL);
	KASSERT(sc->sc_freq != 0);

}

void
a9ptmr_init_cpu_clock(struct cpu_info *ci)
{
	struct a9ptmr_softc * const sc = a9ptmr_sc;

	/* Disable Private timer and acknowledge any event */
	a9ptmr_write(sc, TMR_CTL, 0);
	a9ptmr_write(sc, TMR_INT, TMR_INT_EVENT);

	/*
	 * Provide the auto load value for the decrementing counter and
	 * start it.
	 */
	a9ptmr_write(sc, TMR_LOAD, sc->sc_load);
	a9ptmr_write(sc, TMR_CTL, sc->sc_ctl);

}



/*
 * a9ptmr_intr:
 *
 *	Handle the hardclock interrupt.
 */
int
a9ptmr_intr(void *arg)
{
	struct clockframe * const cf = arg;
	struct a9ptmr_softc * const sc = a9ptmr_sc;

	a9ptmr_write(sc, TMR_INT, TMR_INT_EVENT);
	hardclock(cf);

	return 1;
}

static void
a9ptmr_update_freq_cb(void *arg1, void *arg2)
{
	a9ptmr_init_cpu_clock(curcpu());
}

void
a9ptmr_update_freq(uint32_t freq)
{
	struct a9ptmr_softc * const sc = a9ptmr_sc;
	uint64_t xc;

	KASSERT(sc->sc_dev != NULL);
	KASSERT(freq != 0);

	sc->sc_freq = freq;
	sc->sc_load = (sc->sc_freq / hz) - 1;

	xc = xc_broadcast(0, a9ptmr_update_freq_cb, NULL, NULL);
	xc_wait(xc);
}
