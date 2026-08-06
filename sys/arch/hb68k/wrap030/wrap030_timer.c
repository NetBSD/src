/*	$NetBSD: wrap030_timer.c,v 1.1 2026/08/06 14:41:09 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: wrap030_timer.c,v 1.1 2026/08/06 14:41:09 thorpej Exp $");

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
	struct clock_attach_args sc_clock_args;
	void		(*sc_handler)(struct clockframe *);
	struct evcnt	*sc_evcnt;
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

static void
wraptmr_initclock(void *arg, unsigned int interval_us,
    struct evcnt *ev, void (*func)(struct clockframe *))
{
	struct wraptmr_softc *sc = arg;

	sc->sc_reload = wraptmr_us_to_ticks(sc, interval_us);
	sc->sc_handler = func;
	sc->sc_evcnt = ev;

	TIMER_ARM(sc);
}

#define	CLOCK_HANDLER()							\
do {									\
	TIMER_ARM(sc);							\
									\
	/* Increment the counter and call the handler. */		\
	sc->sc_evcnt->ev_count++;					\
	sc->sc_handler((struct clockframe *)v);				\
} while (/*CONSTCOND*/0)

static int
wraptmr_hardclock(void *v)
{
	struct wraptmr_softc *sc = clock_devices[CLOCK_HARDCLOCK];

	CLOCK_HANDLER();
	return 1;
}

static void *wraptmr_isrs[NCLOCKS] = {
[CLOCK_HARDCLOCK]	=	wraptmr_hardclock,
};

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
	char intrstr[128];
	struct clk *clk;
	int which, error;

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

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_freq = clk_get_rate(clk);

	aprint_naive("\n");
	aprint_normal(": frequency %u Hz\n", sc->sc_freq);

	which = clock_from_phandle(phandle);
	if (which == CLOCK_NONE || wraptmr_isrs[which] == NULL) {
		return;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_SCHED,
	    FDT_INTR_MPSAFE, wraptmr_isrs[which], NULL, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt at %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	sc->sc_clock_args.ca_initfunc = wraptmr_initclock;
	sc->sc_clock_args.ca_arg = sc;
	sc->sc_clock_args.ca_which = which;
	clock_attach(self, &sc->sc_clock_args);
}

CFATTACH_DECL_NEW(wraptmr, sizeof(struct wraptmr_softc),
    wraptmr_match, wraptmr_attach, NULL, NULL);
