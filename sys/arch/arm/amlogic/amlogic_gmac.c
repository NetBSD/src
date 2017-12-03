/* $NetBSD: amlogic_gmac.c,v 1.2.20.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2013, 2014, 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry and Martin Husemann.
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

__KERNEL_RCSID(1, "$NetBSD: amlogic_gmac.c,v 1.2.20.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_var.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/ic/dwc_gmac_var.h>
#include <dev/ic/dwc_gmac_reg.h>

static int amlogic_gmac_match(device_t, cfdata_t, void *);
static void amlogic_gmac_attach(device_t, device_t, void *);
static int amlogic_gmac_intr(void*);

struct amlogic_gmac_softc {
	struct dwc_gmac_softc sc_core;
	void *sc_ih;
};

CFATTACH_DECL_NEW(amlogic_gmac, sizeof(struct amlogic_gmac_softc),
	amlogic_gmac_match, amlogic_gmac_attach, NULL, NULL);

static int
amlogic_gmac_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
amlogic_gmac_attach(device_t parent, device_t self, void *aux)
{
	struct amlogic_gmac_softc * const sc = device_private(self);
	struct amlogicio_attach_args * const aio = aux;
	const struct amlogic_locators * const loc = &aio->aio_loc;

	sc->sc_core.sc_dev = self;
	sc->sc_core.sc_bst = aio->aio_core_bst;
	sc->sc_core.sc_dmat = aio->aio_dmat;
	bus_space_subregion(aio->aio_core_bst, aio->aio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_core.sc_bsh);

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");

	amlogic_eth_init();

	/*
	 * Interrupt handler
	 */
	sc->sc_ih = intr_establish(loc->loc_intr, IPL_NET, IST_EDGE,
	    amlogic_gmac_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n",
	     loc->loc_intr);

	dwc_gmac_attach(&sc->sc_core, GMAC_MII_CLK_100_150M_DIV62);
}

static int
amlogic_gmac_intr(void *arg)
{
	struct amlogic_gmac_softc *sc = arg;

	return dwc_gmac_intr(&sc->sc_core);
}
