/*	$NetBSD: ti_omaptimer.c,v 1.11 2021/11/07 17:12:45 jmcneill Exp $	*/

/*
 * Copyright (c) 2017 Jonathan A. Kollasch
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ti_omaptimer.c,v 1.11 2021/11/07 17:12:45 jmcneill Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/timetc.h>
#include <sys/kernel.h>

#include <arm/locore.h>
#include <arm/fdt/arm_fdtvar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/ti/ti_prcm.h>

enum omaptimer_type {
	DM_TIMER_AM335X,
	DM_TIMER_OMAP3430,
	_DM_NTIMER
};

enum {
	TIMER_TISR,
	TIMER_TIER,
	TIMER_TCLR,
	TIMER_TCRR,
	TIMER_TLDR,
	_TIMER_NREG
};

/* TISR bits */
#define	 OVF_IT_FLAG		__BIT(1)

/* TIER bits */
#define	 MAT_EN_FLAG		__BIT(0)
#define	 OVF_EN_FLAG		__BIT(1)
#define	 TCAR_EN_FLAG		__BIT(2)

/* TCLR bits */
#define	 TCLR_ST		__BIT(0)
#define	 TCLR_AR		__BIT(1)

static uint8_t omaptimer_regmap[_DM_NTIMER][_TIMER_NREG] = {
	[DM_TIMER_AM335X] = {
		[TIMER_TISR]	= 0x28,
		[TIMER_TIER]	= 0x2c,
		[TIMER_TCLR] 	= 0x38,
		[TIMER_TCRR]	= 0x3c,
		[TIMER_TLDR]	= 0x40,
	},
	[DM_TIMER_OMAP3430] = {
		[TIMER_TISR]	= 0x18,
		[TIMER_TIER]	= 0x1c,
		[TIMER_TCLR] 	= 0x24,
		[TIMER_TCRR]	= 0x28,
		[TIMER_TLDR]	= 0x2c,
	},
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,am335x-timer-1ms",	.value = DM_TIMER_AM335X },
	{ .compat = "ti,am335x-timer",		.value = DM_TIMER_AM335X },
	{ .compat = "ti,omap3430-timer",	.value = DM_TIMER_OMAP3430 },
	DEVICE_COMPAT_EOL
};

struct omaptimer_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_phandle;
	enum omaptimer_type sc_type;
	struct timecounter sc_tc;
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, omaptimer_regmap[(sc)->sc_type][(reg)])
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, omaptimer_regmap[(sc)->sc_type][(reg)], val)

static struct omaptimer_softc *timer_softc;

static int
omaptimer_intr(void *arg)
{
	struct omaptimer_softc * const sc = timer_softc;
	struct clockframe * const frame = arg;

	WR4(sc, TIMER_TISR, OVF_IT_FLAG);
	hardclock(frame);

	return 1;
}

static void
omaptimer_cpu_initclocks(void)
{
	struct omaptimer_softc * const sc = timer_softc;
	char intrstr[128];
	void *ih;

	KASSERT(sc != NULL);
	if (!fdtbus_intr_str(sc->sc_phandle, 0, intrstr, sizeof(intrstr)))
		panic("%s: failed to decode interrupt", __func__);
	ih = fdtbus_intr_establish_xname(sc->sc_phandle, 0, IPL_CLOCK,
	    FDT_INTR_MPSAFE, omaptimer_intr, NULL, device_xname(sc->sc_dev));
	if (ih == NULL)
		panic("%s: failed to establish timer interrupt", __func__);

	aprint_normal_dev(sc->sc_dev, "interrupting on %s\n", intrstr);

	/* Enable interrupts */
	WR4(sc, TIMER_TIER, OVF_EN_FLAG);
}

static u_int
omaptimer_get_timecount(struct timecounter *tc)
{
	struct omaptimer_softc * const sc = tc->tc_priv;

	return RD4(sc, TIMER_TCRR);
}

static void
omaptimer_enable(struct omaptimer_softc *sc, uint32_t value)
{
	/* Configure the timer */
	WR4(sc, TIMER_TLDR, value);
	WR4(sc, TIMER_TCRR, value);
	WR4(sc, TIMER_TIER, 0);
	WR4(sc, TIMER_TCLR, TCLR_ST | TCLR_AR);
}

static int
omaptimer_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
omaptimer_attach(device_t parent, device_t self, void *aux)
{
	struct omaptimer_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct timecounter *tc = &sc->sc_tc;
	struct clk *hwmod;
	bus_addr_t addr;
	bus_size_t size;
	u_int rate;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	sc->sc_type = of_compatible_lookup(phandle, compat_data)->value;

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		device_printf(self, "unable to map bus space");
		return;
	}

	hwmod = ti_prcm_get_hwmod(phandle, 0);
	if (hwmod == NULL || clk_enable(hwmod) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Timer\n");

	rate = clk_get_rate(hwmod);

	if (device_unit(self) == 1) {
		omaptimer_enable(sc, 0);

		/* Install timecounter */
		tc->tc_get_timecount = omaptimer_get_timecount;
		tc->tc_counter_mask = ~0u;
		tc->tc_frequency = rate;
		tc->tc_name = device_xname(self);
		tc->tc_quality = 200;
		tc->tc_priv = sc;
		tc_init(tc);

	} else if (device_unit(self) == 2) {
		const uint32_t value = (0xffffffff - ((rate / hz) - 1));
		omaptimer_enable(sc, value);

		/* Use this as the OS timer in UP configurations */
		if (!arm_has_mpext_p) {
			timer_softc = sc;
			arm_fdt_timer_register(omaptimer_cpu_initclocks);
		}
	}
}

CFATTACH_DECL_NEW(omaptimer, sizeof(struct omaptimer_softc),
    omaptimer_match, omaptimer_attach, NULL, NULL);

