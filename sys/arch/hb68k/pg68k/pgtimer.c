/*	$NetBSD: pgtimer.c,v 1.1 2026/07/19 01:48:24 thorpej Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: pgtimer.c,v 1.1 2026/07/19 01:48:24 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <machine/clockvar.h>

#include <dev/fdt/fdtvar.h>

struct pgtimer_softc {
	device_t	sc_dev;
	bus_space_tag_t	sc_st;
	bus_space_handle_t sc_sh;
	uint32_t	sc_reg_shift;
	void		*sc_ih;
	uint32_t	sc_freq;
	struct clock_attach_args sc_clock_args;
	void		(*sc_handler)(struct clockframe *);
	struct evcnt	*sc_evcnt;
};

#define	TIMER_CSR	0
#define	  CSR_ENAB	__BIT(0)
#define	  CSR_IPEND	__BIT(1)
#define	TIMER_VAL	1

#define	REG_OFF(sc, r)		((r) << (sc)->sc_reg_shift)

#define	REG_READ(sc, r)		\
	bus_space_read_1((sc)->sc_st, (sc)->sc_sh, REG_OFF((sc), (r)))
#define	REG_WRITE(sc, r, v)	\
	bus_space_write_1((sc)->sc_st, (sc)->sc_sh, REG_OFF((sc), (r)), (v))

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "pg68k,ioctl010-timer" },
	DEVICE_COMPAT_EOL
};

static uint32_t
pgtimer_us_to_ticks(struct pgtimer_softc *sc, unsigned int interval_us)
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
pgtimer_initclock(void *arg, unsigned int interval_us,
    struct evcnt *ev, void (*func)(struct clockframe *))
{
	struct pgtimer_softc *sc = arg;
	const uint32_t ticks = pgtimer_us_to_ticks(sc, interval_us);

	sc->sc_handler = func;
	sc->sc_evcnt = ev;

	/*
	 * Changing the counter reload value implicitly disables the
	 * timer.  The reload value is set by writing the high byte
	 * followed by the load byte to the Value register.  The reload
	 * value is unpredictable until both bytes have been written.
	 */
	REG_WRITE(sc, TIMER_VAL, (uint8_t)(ticks >> 8));
	REG_WRITE(sc, TIMER_VAL, (uint8_t)ticks);
	REG_WRITE(sc, TIMER_CSR, CSR_ENAB);
}

#define	CLOCK_HANDLER()							\
do {									\
	/* Clear interrupt condition. */				\
	REG_READ(sc, TIMER_CSR);					\
									\
	/* Timer auto-reloads. */					\
									\
	/* Increment the counter and call the handler. */		\
	sc->sc_evcnt->ev_count++;					\
	sc->sc_handler((struct clockframe *)v);				\
} while (/*CONSTCOND*/0)

static int
pgtimer_hardclock(void *v)
{
	struct pgtimer_softc *sc = clock_devices[CLOCK_HARDCLOCK];

	CLOCK_HANDLER();
	return 1;
}

static int
pgtimer_statclock(void *v)
{
	struct pgtimer_softc *sc = clock_devices[CLOCK_STATCLOCK];

	CLOCK_HANDLER();
	return 1;
}

static void *pgtimer_isrs[NCLOCKS] = {
[CLOCK_HARDCLOCK]	=	pgtimer_hardclock,
[CLOCK_STATCLOCK]	=	pgtimer_statclock,
};

static int
pgtimer_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
pgtimer_attach(device_t parent, device_t self, void *aux)
{
	struct pgtimer_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	char intrstr[128];
	struct clk *clk;
	uint32_t clk_div;
	int which, error;

	sc->sc_dev = self;
	sc->sc_st = faa->faa_bst;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (of_getprop_uint32(phandle, "reg-shift", &sc->sc_reg_shift)) {
		/* missing or bad reg-shift property, assume 0 */
		sc->sc_reg_shift = 0;
	}

	error = bus_space_map(sc->sc_st, addr, size, 0, &sc->sc_sh);
	if (error) {
		aprint_error(": couldn't map registers (error=%d)\n", error);
		return;
	}

	/* Make sure the timer is disabled. */
	REG_WRITE(sc, TIMER_CSR, 0);

	clk = fdtbus_clock_get_index(phandle, 0);
	if (clk == NULL) {
		aprint_error(": couldn't get clock handle\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	/*
	 * The input to the I/O controller is the main system clock,
	 * which the timer divides itself internally.  Firmware will
	 * provide this value in the device tree.
	 */
	if (of_getprop_uint32(phandle, "clock-div", &clk_div)) {
		/* default to divide-by-16 */
		clk_div = 16;
	}

	sc->sc_freq = clk_get_rate(clk) / clk_div;

	aprint_naive("\n");
	aprint_normal(": frequency %u Hz\n", sc->sc_freq);

	which = clock_from_phandle(phandle);
	if (which == CLOCK_NONE || pgtimer_isrs[which] == NULL) {
		return;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_SCHED,
	    FDT_INTR_MPSAFE, pgtimer_isrs[which], NULL, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt at %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	sc->sc_clock_args.ca_initfunc = pgtimer_initclock;
	sc->sc_clock_args.ca_arg = sc;
	sc->sc_clock_args.ca_which = which;
	clock_attach(self, &sc->sc_clock_args);
}

CFATTACH_DECL_NEW(pgtimer, sizeof(struct pgtimer_softc),
    pgtimer_match, pgtimer_attach, NULL, NULL);
