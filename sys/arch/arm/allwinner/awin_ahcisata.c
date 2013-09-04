/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(1, "$NetBSD: awin_ahcisata.c,v 1.1 2013/09/04 02:39:01 matt Exp $");

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/ata/atavar.h>
#include <dev/ic/ahcisatavar.h>

static int awin_ahci_match(device_t, cfdata_t, void *);
static void awin_ahci_attach(device_t, device_t, void *);

struct awin_ahci_softc {
	struct ahci_softc asc_sc;
	void *asc_ih;
};

CFATTACH_DECL_NEW(awin_ahcisata, sizeof(struct awin_ahci_softc),
	awin_ahci_match, awin_ahci_attach, NULL, NULL);

static int
awin_ahci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const int port = cf->cf_loc[AWINIOCF_PORT];

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	if (port != AWINIOCF_PORT_DEFAULT && port != loc->loc_port)
		return 0;

	return 1;
}

static void
awin_ahci_attach(device_t parent, device_t self, void *aux)
{
	struct awin_ahci_softc * const asc = device_private(self);
	struct ahci_softc * const sc = &asc->asc_sc;
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

        sc->sc_atac.atac_dev = self;
	sc->sc_dmat = aio->aio_dmat;
	sc->sc_ahcit = aio->aio_core_bst;
	sc->sc_ahcis = loc->loc_size;

	bus_space_subregion(aio->aio_core_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_ahcih);

	aprint_naive(": AHCI SATA controller\n");
	aprint_normal(": AHCI SATA controller\n");

	asc->asc_ih = intr_establish(loc->loc_intr, IPL_VM, IST_LEVEL,
	    ahci_intr, sc);
	if (asc->asc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intr);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n",
	     loc->loc_intr);

	return;

fail:
	if (asc->asc_ih) {
		intr_disestablish(asc->asc_ih);
		asc->asc_ih = NULL;
	}
}
