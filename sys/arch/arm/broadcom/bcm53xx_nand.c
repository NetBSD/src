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

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_nand.c,v 1.2 2014/02/19 22:21:16 matt Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#define NAND_PRIVATE

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

#include <dev/nand/nand.h>
#include <dev/nand/onfi.h>

static int bcmnand_ccb_match(device_t, cfdata_t, void *);
static void bcmnand_ccb_attach(device_t, device_t, void *);

struct bcmnand_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
};

CFATTACH_DECL_NEW(bcmnand_ccb, sizeof(struct bcmnand_softc),
	bcmnand_ccb_match, bcmnand_ccb_attach, NULL, NULL);

static int
bcmnand_ccb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	KASSERT(cf->cf_loc[BCMCCBCF_PORT] == BCMCCBCF_PORT_DEFAULT);

	return 1;
}

static void
bcmnand_ccb_attach(device_t parent, device_t self, void *aux)
{
	struct bcmnand_softc * const sc = device_private(self);
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	sc->sc_dev = self;

	sc->sc_bst = ccbaa->ccbaa_ccb_bst;
	sc->sc_dmat = ccbaa->ccbaa_dmat;
	bus_space_subregion(sc->sc_bst, ccbaa->ccbaa_ccb_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal("\n");
}
