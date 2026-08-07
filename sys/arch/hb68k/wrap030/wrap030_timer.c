/*	$NetBSD: wrap030_timer.c,v 1.2 2026/08/07 23:37:46 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: wrap030_timer.c,v 1.2 2026/08/07 23:37:46 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <machine/clockvar.h>

#include <dev/fdt/fdtvar.h>

struct wraptmr_softc {
	device_t	sc_dev;
	bus_space_tag_t	sc_st;
	bus_space_handle_t sc_sh;
	void		*sc_ih;
	uint32_t	sc_freq;
	uint32_t	sc_reload;
	struct clock_if sc_clkif;
};

#define	TIMER_ARM(sc)							\
	bus_space_write_1((sc)->sc_st, (sc)->sc_sh, (sc)->sc_reload, 0)
#define	TIMER_DISARM(sc)						\
	bus_space_write_1((sc)->sc_st, (sc)->sc_sh, 0, 0)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "techav,wrap030-timer" },
	DEVICE_COMPAT_EOL
};

static uint32_t
wraptmr_us_to_ticks(struct wraptmr_softc *sc, unsigned int interval_us)
{
	const uint32_t tick_ns = 1000000000 / sc->sc_freq;
	const uint64_t interval_ns = interval_us * 1000;
	const uint64_t ticks = interval_ns / tick_ns;

	if (ticks == 0 || ticks > 0xffff) {
		panic("%s: impossible interval %u us (ticks=%llu)", __func__,
		    interval_us, ticks);
	}

	return (uint32_t)ticks;
}

static int
wraptmr_init(struct clock_if *clk, int (*intrfunc)(void *))
{
	struct wraptmr_softc *sc =
	    container_of(clk, struct wraptmr_softc, sc_clkif);
	int phandle = devhandle_to_of(device_handle(sc->sc_dev));
	char intrstr[128];

	KASSERT(sc->sc_dev == clk->clk_dev);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(sc->sc_dev, "failed to decode interrupt\n");
		return ENXIO;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_SCHED,
	    FDT_INTR_MPSAFE, intrfunc, NULL, device_xname(sc->sc_dev));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "failed to establish interrupt at %s\n", intrstr);
		return ENXIO;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting at %s\n", intrstr);

	return 0;
}

static int
wraptmr_arm(struct clock_if *clk, unsigned int interval_us)
{
	struct wraptmr_softc *sc =
	    container_of(clk, struct wraptmr_softc, sc_clkif);

	sc->sc_reload = wraptmr_us_to_ticks(sc, interval_us);
	TIMER_ARM(sc);

	return 0;
}

static void
wraptmr_intr(struct clock_if *clk)
{
	struct wraptmr_softc *sc =
	    container_of(clk, struct wraptmr_softc, sc_clkif);

	TIMER_ARM(sc);
}

static void
wraptmr_disarm(struct clock_if *clk)
{
	struct wraptmr_softc *sc =
	    container_of(clk, struct wraptmr_softc, sc_clkif);

	TIMER_DISARM(sc);
}

static int
wraptmr_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
wraptmr_attach(device_t parent, device_t self, void *aux)
{
	struct wraptmr_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	struct clk *clk;
	int error;

	sc->sc_dev = self;
	sc->sc_st = faa->faa_bst;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	error = bus_space_map(sc->sc_st, addr, size, 0, &sc->sc_sh);
	if (error) {
		aprint_error(": couldn't map registers (error=%d)\n", error);
		return;
	}

	/* Make sure the timer is disabled. */
	TIMER_DISARM(sc);

	clk = fdtbus_clock_get_index(phandle, 0);
	if (clk == NULL) {
		aprint_error(": couldn't get clock handle\n");
		return;
	}

	sc->sc_freq = clk_get_rate(clk);

	aprint_naive("\n");
	aprint_normal(": frequency %u Hz\n", sc->sc_freq);

	sc->sc_clkif.clk_dev = self;
	sc->sc_clkif.clk_init = wraptmr_init;
	sc->sc_clkif.clk_arm = wraptmr_arm;
	sc->sc_clkif.clk_intr = wraptmr_intr;
	sc->sc_clkif.clk_disarm = wraptmr_disarm;
	clock_attach(&sc->sc_clkif);
}

CFATTACH_DECL_NEW(wraptmr, sizeof(struct wraptmr_softc),
    wraptmr_match, wraptmr_attach, NULL, NULL);
