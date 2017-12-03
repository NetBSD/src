/*	$NetBSD: exynos_wdt.c,v 1.4.10.3 2017/12/03 11:35:56 jdolecek Exp $	*/

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

#include "exynos_wdt.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exynos_wdt.c,v 1.4.10.3 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <prop/proplib.h>

#include <dev/sysmon/sysmonvar.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>

#include <dev/fdt/fdtvar.h>

#if NEXYNOS_WDT > 0
static int exynos_wdt_match(device_t, cfdata_t, void *);
static void exynos_wdt_attach(device_t, device_t, void *);

struct exynos_wdt_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_wdog_bsh;
	struct sysmon_wdog sc_smw;
	u_int sc_wdog_period;
	u_int sc_wdog_clock_select;
	u_int sc_wdog_prescaler;
	uint32_t sc_freq;
	uint32_t sc_wdog_wtdat;
	uint32_t sc_wdog_wtcon;
	bool sc_wdog_armed;
};

#ifndef EXYNOS_WDT_PERIOD_DEFAULT
#define	EXYNOS_WDT_PERIOD_DEFAULT	60
#endif

CFATTACH_DECL_NEW(exynos_wdt, sizeof(struct exynos_wdt_softc),
    exynos_wdt_match, exynos_wdt_attach, NULL, NULL);

static inline uint32_t
exynos_wdt_wdog_read(struct exynos_wdt_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_wdog_bsh, o);
}

static inline void
exynos_wdt_wdog_write(struct exynos_wdt_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_wdog_bsh, o, v);
}

/* ARGSUSED */
static int
exynos_wdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,exynos5420-wdt", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static int
exynos_wdt_tickle(struct sysmon_wdog *smw)
{
	struct exynos_wdt_softc * const sc = smw->smw_cookie;

	/*
	 * Cause the WDOG to restart counting.
	 */
	exynos_wdt_wdog_write(sc, EXYNOS_WDT_WTCNT, sc->sc_wdog_wtdat);
	aprint_debug_dev(sc->sc_dev, "tickle\n");
	return 0;
}

static int
exynos_wdt_setmode(struct sysmon_wdog *smw)
{
	struct exynos_wdt_softc * const sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		/*
		 * Emit magic sequence to turn off WDOG
		 */
		sc->sc_wdog_wtcon &= ~(WTCON_ENABLE|WTCON_RESET_ENABLE);
		exynos_wdt_wdog_write(sc, EXYNOS_WDT_WTCON, sc->sc_wdog_wtcon);
		delay(1);
		aprint_debug_dev(sc->sc_dev, "setmode disable\n");
		return 0;
	}

	/*
	 * If no changes, just tickle it and return.
	 */
	if (sc->sc_wdog_armed && smw->smw_period == sc->sc_wdog_period) {
		sc->sc_wdog_wtdat = sc->sc_freq * sc->sc_wdog_period - 1;
		sc->sc_wdog_wtcon = WTCON_ENABLE | WTCON_RESET_ENABLE
		    | __SHIFTIN(sc->sc_wdog_clock_select, WTCON_CLOCK_SELECT)
		    | __SHIFTIN(sc->sc_wdog_prescaler - 1, WTCON_PRESCALER);

		exynos_wdt_wdog_write(sc, EXYNOS_WDT_WTCNT, sc->sc_wdog_wtdat);
		exynos_wdt_wdog_write(sc, EXYNOS_WDT_WTDAT, sc->sc_wdog_wtdat);
		exynos_wdt_wdog_write(sc, EXYNOS_WDT_WTCON, sc->sc_wdog_wtcon);
		aprint_debug_dev(sc->sc_dev, "setmode refresh\n");
		return 0;
	}

	if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
		sc->sc_wdog_period = EXYNOS_WDT_PERIOD_DEFAULT;
		smw->smw_period = EXYNOS_WDT_PERIOD_DEFAULT;
	}

	/*
	 * Make sure we don't overflow the counter.
	 */
	if (smw->smw_period * sc->sc_freq >= UINT16_MAX) {
		return EINVAL;
	}

	sc->sc_wdog_wtdat = sc->sc_freq * sc->sc_wdog_period - 1;
	sc->sc_wdog_wtcon = WTCON_ENABLE | WTCON_RESET_ENABLE
	    | __SHIFTIN(sc->sc_wdog_clock_select, WTCON_CLOCK_SELECT)
	    | __SHIFTIN(sc->sc_wdog_prescaler - 1, WTCON_PRESCALER);

	/*
	 * Have to disable to be able to write WTDAT
	 */
	exynos_wdt_wdog_write(sc, EXYNOS_WDT_WTCON,
	    sc->sc_wdog_wtcon & ~(WTCON_ENABLE | WTCON_RESET_ENABLE));
	exynos_wdt_wdog_write(sc, EXYNOS_WDT_WTCNT, sc->sc_wdog_wtdat);
	exynos_wdt_wdog_write(sc, EXYNOS_WDT_WTDAT, sc->sc_wdog_wtdat);
	exynos_wdt_wdog_write(sc, EXYNOS_WDT_WTCON, sc->sc_wdog_wtcon);

	aprint_debug_dev(sc->sc_dev, "setmode enable\n");
	return 0;
}


static void
exynos_wdt_attach(device_t parent, device_t self, void *aux)
{
        struct exynos_wdt_softc * const sc = device_private(self);
//	prop_dictionary_t dict = device_properties(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_wdog_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	/*
	 * This runs at the Exynos Pclk.
	 */
//	prop_dictionary_get_uint32(dict, "frequency", &sc->sc_freq);
	sc->sc_freq = 12000000;	/* MJF: HACK hardwire for now */
		/* Need to figure out how to get freq from dtb */
	sc->sc_wdog_wtcon = exynos_wdt_wdog_read(sc, EXYNOS_WDT_WTCON);
	sc->sc_wdog_armed = (sc->sc_wdog_wtcon & WTCON_ENABLE)
	    && (sc->sc_wdog_wtcon & WTCON_RESET_ENABLE);
	if (sc->sc_wdog_armed) {
		sc->sc_wdog_prescaler =
		    __SHIFTOUT(sc->sc_wdog_wtcon, WTCON_PRESCALER);
		sc->sc_wdog_clock_select =
		    __SHIFTOUT(sc->sc_wdog_wtcon, WTCON_CLOCK_SELECT);
		sc->sc_freq /= sc->sc_wdog_prescaler;
		sc->sc_freq >>= 4 + sc->sc_wdog_clock_select;
		sc->sc_wdog_wtdat = exynos_wdt_wdog_read(sc, EXYNOS_WDT_WTDAT);
		sc->sc_wdog_period = (sc->sc_wdog_wtdat + 1) / sc->sc_freq;
	} else {
		sc->sc_wdog_period = EXYNOS_WDT_PERIOD_DEFAULT;
		sc->sc_wdog_prescaler = 1;
		/*
		 * Let's see what clock select we should use.
		 */
		u_int n = __builtin_ffs(sc->sc_freq) - 1;
		if (n > 7) {
			sc->sc_wdog_clock_select = WTCON_CLOCK_SELECT_128;
			sc->sc_freq >>= 7;
		} else if (n >= 4) {
			sc->sc_wdog_clock_select = n - 4;
			sc->sc_freq >>= n;
		}
		/*
		 * Let's hope the timer frequency isn't prime.  If it is, find
		 * the highest divisor which gives us the least remainder.
		 */
		sc->sc_wdog_prescaler = 0;
		u_int best_remainder = 256;
		u_int max_period = 2 * EXYNOS_WDT_PERIOD_DEFAULT * sc->sc_freq;
		for (size_t div = 256; UINT16_MAX > div * max_period; div++) {
			u_int remainder = sc->sc_freq % div;
			if (remainder == 0) {
				sc->sc_wdog_prescaler = div;
				break;
			}
			if (remainder < best_remainder) {
				sc->sc_wdog_prescaler = div;
				best_remainder = remainder;
			}
		}
		KASSERT(sc->sc_wdog_prescaler != 0);
		sc->sc_freq /= sc->sc_wdog_prescaler;
	}

	/*
	 * Does the config file tell us to turn on the watchdog?
	 */
	if (device_cfdata(self)->cf_flags & 1)
		sc->sc_wdog_armed = true;

	aprint_naive("\n");
	aprint_normal(": Exynos Watchdog Timer, default period is %u seconds%s\n",
	    sc->sc_wdog_period,
	    sc->sc_wdog_armed ? " (armed)" : "");

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = exynos_wdt_setmode;
	sc->sc_smw.smw_tickle = exynos_wdt_tickle;
	sc->sc_smw.smw_period = sc->sc_wdog_period;

	if (sc->sc_wdog_armed) {
		error = sysmon_wdog_setmode(&sc->sc_smw, WDOG_MODE_KTICKLE,
		    sc->sc_wdog_period);
		if (error)
			aprint_error_dev(self,
			    "failed to start kernel tickler: %d\n", error);
 	}
}
#endif /* NEXYNOS_WDOG > 0 */

void
exynos_wdt_reset(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh = exynos_wdt_bsh;

	(void) splhigh();
	bus_space_write_4(bst, bsh, EXYNOS_WDT_WTCON, 0);
	bus_space_write_4(bst, bsh, EXYNOS_WDT_WTCNT, 1);
	bus_space_write_4(bst, bsh, EXYNOS_WDT_WTCON,
	   WTCON_ENABLE | WTCON_RESET_ENABLE);
}

