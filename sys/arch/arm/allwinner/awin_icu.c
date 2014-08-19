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

__KERNEL_RCSID(1, "$NetBSD: awin_icu.c,v 1.2.12.2 2014/08/20 00:02:44 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

static int awin_icu_match(device_t, cfdata_t, void *);
static void awin_icu_attach(device_t, device_t, void *);

struct awin_icu_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
};

CFATTACH_DECL_NEW(awin_icu, sizeof(struct awin_icu_softc),
	awin_icu_match, awin_icu_attach, NULL, NULL);

static int
awin_icu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const int port = cf->cf_loc[AWINIOCF_PORT];

	KASSERT(loc->loc_port == AWINIOCF_PORT_DEFAULT);

	if (strcmp(cf->cf_name, loc->loc_name)
	    || (port != AWINIOCF_PORT_DEFAULT && port != loc->loc_port))
		return 0;

	return 1;
}

static void
awin_icu_attach(device_t parent, device_t self, void *aux)
{
	struct awin_icu_softc * const sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	sc->sc_dev = self;

	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal("\n");
}
