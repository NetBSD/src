/*-
 * Copyright (c) 2013, 2014 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(1, "$NetBSD: awin_gige.c,v 1.5 2014/09/08 14:26:16 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/ic/dwc_gmac_var.h>

static int awin_gige_match(device_t, cfdata_t, void *);
static void awin_gige_attach(device_t, device_t, void *);
static int awin_gige_intr(void*);

struct awin_gige_softc {
	struct dwc_gmac_softc sc_core;
	void *sc_ih;
};

static const struct awin_gpio_pinset awin_gige_gpio_pinset = {
	'A', AWIN_PIO_PA_GMAC_FUNC, AWIN_PIO_PA_GMAC_PINS,
};

CFATTACH_DECL_NEW(awin_gige, sizeof(struct awin_gige_softc),
	awin_gige_match, awin_gige_attach, NULL, NULL);

static int
awin_gige_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
#ifdef DIAGNOSTIC
	const struct awin_locators * const loc = &aio->aio_loc;
#endif
	if (cf->cf_flags & 1)
		return 0;

	KASSERT(!strcmp(cf->cf_name, loc->loc_name));
	KASSERT(cf->cf_loc[AWINIOCF_PORT] == AWINIOCF_PORT_DEFAULT
	    || cf->cf_loc[AWINIOCF_PORT] == loc->loc_port);

	if (!awin_gpio_pinset_available(&awin_gige_gpio_pinset))
		return 0;

	return 1;
}

static void
awin_gige_attach(device_t parent, device_t self, void *aux)
{
	struct awin_gige_softc * const sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	prop_dictionary_t dict;
	uint8_t enaddr[ETHER_ADDR_LEN], *ep = NULL;

	sc->sc_core.sc_dev = self;
	dict = device_properties(sc->sc_core.sc_dev);

	awin_gpio_pinset_acquire(&awin_gige_gpio_pinset);

	sc->sc_core.sc_bst = aio->aio_core_bst;
	sc->sc_core.sc_dmat = aio->aio_dmat;
	bus_space_subregion(sc->sc_core.sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_core.sc_bsh);

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");
	
	prop_data_t ea = dict ? prop_dictionary_get(dict, "mac-address") : NULL;
	if (ea != NULL) {
		KASSERT(prop_object_type(ea) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(ea) == ETHER_ADDR_LEN);
		memcpy(enaddr, prop_data_data_nocopy(ea), ETHER_ADDR_LEN);
		ep = enaddr;
	}

	/*
	 * Interrupt handler
	 */
	sc->sc_ih = intr_establish(loc->loc_intr, IPL_VM, IST_LEVEL|IST_MPSAFE,
	    awin_gige_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n",
	     loc->loc_intr);

	dwc_gmac_attach(&sc->sc_core, ep);
}


static int
awin_gige_intr(void *arg)
{
	struct awin_gige_softc *sc = arg;

	return dwc_gmac_intr(&sc->sc_core);
}
