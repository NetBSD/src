/*	$NetBSD: if_smsh_axi.c,v 1.2.2.2 2015/04/06 15:17:56 skrll Exp $	*/
/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Sergio L. Pascual.
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
__KERNEL_RCSID(0, "$NetBSD: if_smsh_axi.c,v 1.2.2.2 2015/04/06 15:17:56 skrll Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/bus.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/ic/lan9118var.h>
#include <dev/ic/lan9118reg.h>

#include <evbarm/vexpress/vexpress_var.h>


static int smsh_axi_match(device_t, struct cfdata *, void *);
static void smsh_axi_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(smsh_axi, sizeof(struct lan9118_softc),
    smsh_axi_match, smsh_axi_attach, NULL, NULL);


/* ARGSUSED */
static int
smsh_axi_match(device_t parent, struct cfdata * match, void *aux)
{
	struct axi_attach_args *aa = aux;

	/* Disallow wildcarded values. */
	if (aa->aa_addr == 0)
		return 0;
	if (aa->aa_irq == 0)
		return 0;

	return 1;
}

/* ARGSUSED */
static void
smsh_axi_attach(device_t parent, device_t self, void *aux)
{
	struct lan9118_softc *sc = device_private(self);
	struct axi_attach_args *aa = aux;
	prop_dictionary_t dict = device_properties(self);
	void *ih;

	sc->sc_dev = self;

	/*
	 * Prefer the Ethernet address in device properties.
	 */
	prop_data_t ea = prop_dictionary_get(dict, "mac-address");
	if (ea != NULL) {
		KASSERT(prop_object_type(ea) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(ea) == ETHER_ADDR_LEN);
		memcpy(sc->sc_enaddr, prop_data_data_nocopy(ea),
		    ETHER_ADDR_LEN);
		sc->sc_flags |= LAN9118_FLAGS_NO_EEPROM;
	}
	/* Map i/o space. */
	if (bus_space_map(aa->aa_iot, aa->aa_addr, LAN9118_IOSIZE, 0,
		&sc->sc_ioh))
		panic("smsh_axi_attach: can't map i/o space");
	sc->sc_iot = aa->aa_iot;

	if (lan9118_attach(sc) != 0) {
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, LAN9118_IOSIZE);
		return;
	}
	/* Establish the interrupt handler. */
	ih = intr_establish(aa->aa_irq, IPL_NET, IST_LEVEL,
	    lan9118_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self,
		    "couldn't establish interrupt handler\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, LAN9118_IOSIZE);
		return;
	}
}
