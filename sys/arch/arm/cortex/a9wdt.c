/*	$NetBSD: a9wdt.c,v 1.1.18.1 2014/08/10 06:53:51 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: a9wdt.c,v 1.1.18.1 2014/08/10 06:53:51 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <prop/proplib.h>

#include <dev/sysmon/sysmonvar.h>

#include <arm/cortex/a9tmr_reg.h>

#include <arm/cortex/mpcore_var.h>

static int a9wdt_match(device_t, cfdata_t, void *);
static void a9wdt_attach(device_t, device_t, void *);

struct a9wdt_softc {
	struct sysmon_wdog sc_smw;
	device_t sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_wdog_memh;
	u_int sc_wdog_max_period;
	u_int sc_wdog_period;
	u_int sc_wdog_prescaler;
	uint32_t sc_freq;
	uint32_t sc_wdog_load;
	uint32_t sc_wdog_ctl;
	bool sc_wdog_armed;
};

#ifndef A9WDT_PERIOD_DEFAULT
#define	A9WDT_PERIOD_DEFAULT	12
#endif

CFATTACH_DECL_NEW(a9wdt, sizeof(struct a9wdt_softc),
    a9wdt_match, a9wdt_attach, NULL, NULL);

static bool attached;

static inline uint32_t
a9wdt_wdog_read(struct a9wdt_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_memt, sc->sc_wdog_memh, o);
}

static inline void
a9wdt_wdog_write(struct a9wdt_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_memt, sc->sc_wdog_memh, o, v);
}


/* ARGSUSED */
static int
a9wdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mpcore_attach_args * const mpcaa = aux;

	if (attached)
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

static int
a9wdt_tickle(struct sysmon_wdog *smw)
{
	struct a9wdt_softc * const sc = smw->smw_cookie;

	/*
	 * Cause the WDOG to restart counting.
	 */
	a9wdt_wdog_write(sc, TMR_LOAD, sc->sc_wdog_load);
	aprint_debug_dev(sc->sc_dev, "tickle\n");
	return 0;
}

static int
a9wdt_setmode(struct sysmon_wdog *smw)
{
	struct a9wdt_softc * const sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		/*
		 * Emit magic sequence to turn off WDOG
		 */
		a9wdt_wdog_write(sc, TMR_WDOGDIS, TMR_WDOG_DISABLE_MAGIC1);
		a9wdt_wdog_write(sc, TMR_WDOGDIS, TMR_WDOG_DISABLE_MAGIC2);
		delay(1);
		sc->sc_wdog_ctl = a9wdt_wdog_read(sc, TMR_CTL);
		KASSERT((sc->sc_wdog_ctl & TMR_CTL_WDOG_MODE) == 0);
		aprint_debug_dev(sc->sc_dev, "setmode disable\n");
		return 0;
	}

	/*
	 * If no changes, just tickle it and return.
	 */
	if (sc->sc_wdog_armed && smw->smw_period == sc->sc_wdog_period) {
		sc->sc_wdog_load = sc->sc_freq * sc->sc_wdog_period - 1;
		sc->sc_wdog_ctl = TMR_CTL_ENABLE | TMR_CTL_WDOG_MODE
		    | __SHIFTIN(sc->sc_wdog_prescaler - 1, TMR_CTL_PRESCALER);

		a9wdt_wdog_write(sc, TMR_LOAD, sc->sc_wdog_load);
		a9wdt_wdog_write(sc, TMR_CTL, sc->sc_wdog_ctl);
		aprint_debug_dev(sc->sc_dev, "setmode refresh\n");
		return 0;
	}

	if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
		sc->sc_wdog_period = A9WDT_PERIOD_DEFAULT;
		smw->smw_period = A9WDT_PERIOD_DEFAULT;
	}

	/*
	 * Make sure we don't overflow the counter.
	 */
	if (smw->smw_period >= sc->sc_wdog_max_period) {
		return EINVAL;
	}

	sc->sc_wdog_load = sc->sc_freq * sc->sc_wdog_period - 1;
	sc->sc_wdog_ctl = TMR_CTL_ENABLE | TMR_CTL_WDOG_MODE
	    | __SHIFTIN(sc->sc_wdog_prescaler - 1, TMR_CTL_PRESCALER);

	a9wdt_wdog_write(sc, TMR_LOAD, sc->sc_wdog_load);
	a9wdt_wdog_write(sc, TMR_CTL, sc->sc_wdog_ctl);

	aprint_debug_dev(sc->sc_dev, "setmode enable\n");
	return 0;
}


static void
a9wdt_attach(device_t parent, device_t self, void *aux)
{
        struct a9wdt_softc * const sc = device_private(self);
	struct mpcore_attach_args * const mpcaa = aux;
	prop_dictionary_t dict = device_properties(self);

	sc->sc_dev = self;
	sc->sc_memt = mpcaa->mpcaa_memt;

	bus_space_subregion(sc->sc_memt, mpcaa->mpcaa_memh, 
	    TMR_WDOG_BASE, TMR_WDOG_SIZE, &sc->sc_wdog_memh);

	/*
	 * This runs at the ARM PERIPHCLOCK which should be 1/2 of the
	 * CPU clock.  The MD code should have setup our frequency for us.
	 */
	prop_dictionary_get_uint32(dict, "frequency", &sc->sc_freq);

	sc->sc_wdog_ctl = a9wdt_wdog_read(sc, TMR_CTL);
	sc->sc_wdog_armed = (sc->sc_wdog_ctl & TMR_CTL_WDOG_MODE) != 0;
	if (sc->sc_wdog_armed) {
		sc->sc_wdog_prescaler = 
		    __SHIFTOUT(sc->sc_wdog_ctl, TMR_CTL_PRESCALER) + 1;
		sc->sc_freq /= sc->sc_wdog_prescaler;
		sc->sc_wdog_load = a9wdt_wdog_read(sc, TMR_LOAD);
		sc->sc_wdog_period = (sc->sc_wdog_load + 1) / sc->sc_freq;
	} else {
		sc->sc_wdog_period = A9WDT_PERIOD_DEFAULT;
		sc->sc_wdog_prescaler = 1;
		/*
		 * Let's hope the timer frequency isn't prime.
		 */
		for (size_t div = 256; div >= 2; div++) {
			if (sc->sc_freq % div == 0) {
				sc->sc_wdog_prescaler = div;
				break;
			}
		}
		sc->sc_freq /= sc->sc_wdog_prescaler;
	}
	sc->sc_wdog_max_period = UINT32_MAX / sc->sc_freq;

	/*
	 * Does the config file tell us to turn on the watchdog?
	 */
	if (device_cfdata(self)->cf_flags & 1)
		sc->sc_wdog_armed = true;

	aprint_naive("\n");
	aprint_normal(": A9 Watchdog Timer, default period is %u seconds%s\n",
	    sc->sc_wdog_period,
	    sc->sc_wdog_armed ? " (armed)" : "");

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = a9wdt_setmode;
	sc->sc_smw.smw_tickle = a9wdt_tickle;
	sc->sc_smw.smw_period = sc->sc_wdog_period;

	if (sc->sc_wdog_armed) {
		int error = sysmon_wdog_setmode(&sc->sc_smw, WDOG_MODE_KTICKLE,
		    sc->sc_wdog_period);
		if (error)
			aprint_error_dev(self,
			    "failed to start kernel tickler: %d\n", error);
 	}
}
