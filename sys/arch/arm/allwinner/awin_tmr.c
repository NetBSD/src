/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_tmr.c,v 1.3.4.2 2014/08/20 00:02:44 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

static int awin_tmr_match(device_t, cfdata_t, void *);
static void awin_tmr_attach(device_t, device_t, void *);

struct awin_tmr_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	void *sc_ih;
	/* register mirrors */
	uint32_t sc_intv_value;
	uint32_t sc_ctrl;
};

static int awin_tmr_intr(void *);

static struct awin_tmr_softc awin_tmr_softc;

CFATTACH_DECL_NEW(awin_tmr, 0,
	awin_tmr_match, awin_tmr_attach, NULL, NULL);

static inline uint32_t
awin_tmr_read(struct awin_tmr_softc *sc, bus_size_t off)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, off);
}

static inline void
awin_tmr_write(struct awin_tmr_softc *sc, bus_size_t off, uint32_t val)
{
	return bus_space_write_4(sc->sc_bst, sc->sc_bsh, off, val);
}

static int
awin_tmr_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const int port = cf->cf_loc[AWINIOCF_PORT];

	if (awin_tmr_softc.sc_dev != NULL)
		return 0;

	if (strcmp(cf->cf_name, loc->loc_name)
	    || (port != AWINIOCF_PORT_DEFAULT && port != loc->loc_port))
		return 0;

	KASSERT(loc->loc_port == AWINIOCF_PORT_DEFAULT);

	return 1;
}

static void
awin_tmr_attach(device_t parent, device_t self, void *aux)
{
	struct awin_tmr_softc * const sc = &awin_tmr_softc;
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	sc->sc_dev = self;
	self->dv_private = sc;

	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal("\n");

	awin_tmr_write(sc, AWIN_TMR_IRQ_EN_REG, 0);

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_SCHED,
	    IST_LEVEL | IST_MPSAFE, awin_tmr_intr, NULL);
	if (sc->sc_ih != NULL) {
		aprint_normal_dev(self, "interrupting on irq %d\n",
		    loc->loc_intr);
	}

	sc->sc_intv_value = AWIN_REF_FREQ / hz;
}

int
awin_tmr_intr(void *arg)
{
	struct awin_tmr_softc * const sc = &awin_tmr_softc;
	struct clockframe * const cf = arg;

	/* clear interrupt pending */
	const uint32_t sts = awin_tmr_read(sc, AWIN_TMR_IRQ_STA_REG);
	awin_tmr_write(sc, AWIN_TMR_IRQ_STA_REG, sts);

	int32_t now = 0 - awin_tmr_read(sc, AWIN_TMR0_CUR_VALUE_REG);
	int32_t slop = now % sc->sc_intv_value;
	awin_tmr_write(sc, AWIN_TMR0_INTV_VALUE_REG,
	    sc->sc_intv_value - slop);
	awin_tmr_write(sc, AWIN_TMR0_CTRL_REG,
	    sc->sc_ctrl | AWIN_TMR_CTRL_RELOAD);

	hardclock(cf);
	for (; now > sc->sc_intv_value; now -= sc->sc_intv_value) {
		hardclock(cf);
	}

	return 1;
}

void
awin_tmr_cpu_init(struct cpu_info *ci)
{
}
