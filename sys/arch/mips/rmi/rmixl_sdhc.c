/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#include <sys/param.h>

__KERNEL_RCSID(1, "$NetBSD: rmixl_sdhc.c,v 1.1.2.1 2011/12/24 01:57:54 matt Exp $");

#include <sys/device.h>
#include <sys/bus.h>
#include <sys/malloc.h>

#include <dev/sdmmc/sdhcreg.h>

#include <mips/rmi/rmixl_sdhcvar.h>

#include "locators.h"

static int sdhc_xlsdio_match(device_t, cfdata_t, void *);
static void sdhc_xlsdio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sdhc_xlsdio, sizeof(struct sdhc_softc),
	sdhc_xlsdio_match, sdhc_xlsdio_attach, 0, 0);

static int
sdhc_xlsdio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct xlsdio_softc * const psc = device_private(parent);
	struct xlsdio_attach_args * const xa = aux;

	if (cf->cf_loc[XLSDIOCF_SLOT] == XLSDIOCF_SLOT_DEFAULT) {
		for (int slot = 0; slot < __arraycount(psc->sc_slots); slot++) {
			if (psc->sc_slots[slot] == NULL) {
				xa->xa_slot = slot;
				return 1;
			}
		}
		return 0;
	}
	if (xa->xa_slot != cf->cf_loc[XLSDIOCF_SLOT])
		return 0;

	if (xa->xa_slot < 0 || xa->xa_slot >= __arraycount(psc->sc_slots))
		return 0;

	if (psc->sc_slots[xa->xa_slot] == NULL)
		return 1;

	return 0;
}

static void
sdhc_xlsdio_attach(device_t parent, device_t self, void *aux)
{
	struct xlsdio_softc * const psc = device_private(parent);
	struct sdhc_softc * const sc = device_private(self);
	struct xlsdio_attach_args * const xa = aux;
	bus_space_handle_t bsh;

	KASSERT((size_t)xa->xa_slot < __arraycount(psc->sc_slots));
	KASSERT(psc->sc_slots[xa->xa_slot] == NULL);
	psc->sc_slots[xa->xa_slot] = sc;

	sc->sc_flags = SDHC_FLAG_HAS_CGM;
	sc->sc_dev = self;
	sc->sc_dmat = xa->xa_dmat;
	sc->sc_host = malloc(1 * sizeof(sc->sc_host[0]),
	    M_DEVBUF, M_NOWAIT|M_ZERO);
	if (sc->sc_host == NULL) {
		aprint_error(": couldn't allocate memory\n");
		return;
	}

	if (bus_space_subregion(xa->xa_bst, xa->xa_bsh, xa->xa_addr,
	    xa->xa_size, &bsh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_normal("\n");

	if (sdhc_host_found(sc, xa->xa_bst, bsh, xa->xa_size)) {
		aprint_error_dev(self, "can't initialize slot\n");
		/* disable all interrupts */
                bus_space_write_4(xa->xa_bst, bsh, SDHC_NINTR_STATUS_EN, 0);
                bus_space_write_4(xa->xa_bst, bsh, SDHC_NINTR_SIGNAL_EN, 0);
		/* now ack them */
                bus_space_write_4(xa->xa_bst, bsh, SDHC_NINTR_STATUS,
		    0xffffffff);
		return;
	}
}
