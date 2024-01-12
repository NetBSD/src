/*	$NetBSD: gfrtc_mainbus.c,v 1.2 2024/01/12 06:23:20 mlelstv Exp $	*/

/*-
 * Copyright (c) 2023, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson and by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: gfrtc_mainbus.c,v 1.2 2024/01/12 06:23:20 mlelstv Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <machine/clockvar.h>

#include <virt68k/dev/mainbusvar.h>

#include <dev/goldfish/gfrtcvar.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "google,goldfish-rtc",
	  .value = CLOCK_NONE },

	{ .compat = "netbsd,goldfish-rtc-hardclock",
	  .value = CLOCK_HARDCLOCK },

	DEVICE_COMPAT_EOL
};

struct gfrtc_mainbus_softc {
	struct gfrtc_softc	sc_gfrtc;
	struct clock_attach_args sc_clock_args;
	uint64_t		sc_interval_ns;
	void			(*sc_handler)(struct clockframe *);
	struct evcnt *		sc_evcnt;
	void *			sc_ih;
};

#define	CLOCK_HANDLER()							\
do {									\
	/* Clear the interrupt condition. */				\
	gfrtc_clear_interrupt(&sc->sc_gfrtc);				\
									\
	/* Get the next alarm set ASAP. */				\
	gfrtc_set_alarm(&sc->sc_gfrtc, sc->sc_interval_ns);		\
									\
	/* Increment the counter and call the handler. */		\
	sc->sc_evcnt->ev_count++;					\
	sc->sc_handler((struct clockframe *)v);				\
} while (/*CONSTCOND*/0)

static int
gfrtc_mainbus_hardclock(void *v)
{
	struct gfrtc_mainbus_softc *sc = clock_devices[CLOCK_HARDCLOCK];

	CLOCK_HANDLER();
	return 1;
}

static void *gfrtc_isrs[] = {
[CLOCK_HARDCLOCK]	=	gfrtc_mainbus_hardclock,
};

static void
gfrtc_mainbus_initclock(void *arg, unsigned int interval_us,
    struct evcnt *ev, void (*func)(struct clockframe *))
{
	struct gfrtc_mainbus_softc *sc = arg;

	sc->sc_interval_ns = (uint64_t)interval_us * 1000;
	sc->sc_handler = func;
	sc->sc_evcnt = ev;

	/* Enable the interrupt. */
	gfrtc_enable_interrupt(&sc->sc_gfrtc);

	/* Start the first alarm! */
	gfrtc_set_alarm(&sc->sc_gfrtc, sc->sc_interval_ns);
}

static int
gfrtc_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;

	return mainbus_compatible_match(ma, compat_data);
}

static void
gfrtc_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct gfrtc_mainbus_softc * const sc = device_private(self);
	struct mainbus_attach_args * const ma = aux;
	const struct device_compatible_entry *dce;
	char strbuf[INTR_STRING_BUFSIZE];

	sc->sc_gfrtc.sc_dev = self;
	sc->sc_gfrtc.sc_bst = ma->ma_st;
	if (bus_space_map(sc->sc_gfrtc.sc_bst, ma->ma_addr, ma->ma_size, 0,
			  &sc->sc_gfrtc.sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	dce = mainbus_compatible_lookup(ma, compat_data);
	KASSERT(dce != NULL);

	gfrtc_attach(&sc->sc_gfrtc, (dce->value == CLOCK_NONE));

	switch (dce->value) {
	case CLOCK_HARDCLOCK:
		/*
		 * We are the one of the clock interrupts.
		 */
		sc->sc_ih = intr_establish(gfrtc_isrs[(int)dce->value],
		    NULL, ma->ma_irq, IPL_SCHED, 0);
		KASSERT(sc->sc_ih != NULL);
		aprint_normal_dev(self, "%s interrupting at %s\n",
		    clock_name((int)dce->value),
		    intr_string(sc->sc_ih, strbuf, sizeof(strbuf)));
		sc->sc_clock_args.ca_initfunc = gfrtc_mainbus_initclock;
		sc->sc_clock_args.ca_arg = sc;
		sc->sc_clock_args.ca_which = (int)dce->value;
		clock_attach(self, &sc->sc_clock_args, gfrtc_delay);
		break;

	default:
		break;
	}
}

CFATTACH_DECL_NEW(gfrtc_mainbus, sizeof(struct gfrtc_mainbus_softc),
	gfrtc_mainbus_match, gfrtc_mainbus_attach, NULL, NULL);
