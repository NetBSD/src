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

__KERNEL_RCSID(1, "$NetBSD: awin_sdhc.c,v 1.3 2013/09/07 00:35:52 matt Exp $");

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>

static int awin_sdhc_match(device_t, cfdata_t, void *);
static void awin_sdhc_attach(device_t, device_t, void *);

struct awin_sdhc_softc {
	struct sdhc_softc asc_sc;
	bus_space_tag_t asc_bst;
	bus_space_handle_t asc_bsh;
	struct sdhc_host *asc_hosts[1];
	void *asc_ih;
};

static const struct awin_gpio_pinset awin_sdhc_pinsets[] = {
	{ 'F', AWIN_PIO_PF_SDC0_FUNC, AWIN_PIO_PF_SDC0_PINS },
	{ 'G', AWIN_PIO_PG_SDC1_FUNC, AWIN_PIO_PG_SDC1_PINS },
	{ 'C', AWIN_PIO_PC_SDC2_FUNC, AWIN_PIO_PC_SDC2_PINS },
	{ 'I', AWIN_PIO_PI_SDC3_FUNC, AWIN_PIO_PI_SDC3_PINS },
};

static const struct awin_gpio_pinset awin_sdhc_alt_pinsets[] = {
	{ 0, 0, 0 },
	{ 'H', AWIN_PIO_PH_SDC1_FUNC, AWIN_PIO_PH_SDC1_PINS },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
};

CFATTACH_DECL_NEW(awin_sdhc, sizeof(struct awin_sdhc_softc),
	awin_sdhc_match, awin_sdhc_attach, NULL, NULL);

static int awin_sdhc_ports;

static int
awin_sdhc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const struct awin_gpio_pinset * const pinset = loc->loc_port
	    + ((cf->cf_flags & 1) ? awin_sdhc_alt_pinsets : awin_sdhc_pinsets);

	KASSERT(!strcmp(cf->cf_name, loc->loc_name));
	KASSERT(cf->cf_loc[AWINIOCF_PORT] == AWINIOCF_PORT_DEFAULT
	    || cf->cf_loc[AWINIOCF_PORT] == loc->loc_port);
	KASSERT((awin_sdhc_ports & __BIT(loc->loc_port)) == 0);

	if (!awin_gpio_pinset_available(pinset))
		return 0;

	return 1;
}

static void
awin_sdhc_attach(device_t parent, device_t self, void *aux)
{
	struct awin_sdhc_softc * const asc = device_private(self);
	struct sdhc_softc * const sc = &asc->asc_sc;
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	cfdata_t cf = device_cfdata(self);
	const struct awin_gpio_pinset * const pinset = loc->loc_port
	    + ((cf->cf_flags & 1) ? awin_sdhc_alt_pinsets : awin_sdhc_pinsets);
	int error;

	awin_sdhc_ports |= __BIT(loc->loc_port);

	awin_gpio_pinset_acquire(pinset);

	asc->asc_bst = aio->aio_core_bst;
	bus_space_subregion(asc->asc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &asc->asc_bsh);

	sc->sc_dev = self;
	sc->sc_dmat = aio->aio_dmat;
	sc->sc_host = asc->asc_hosts;
	sc->sc_flags |= SDHC_FLAG_32BIT_ACCESS;
	sc->sc_flags |= SDHC_FLAG_HAVE_CGM;
	//sc->sc_flags |= SDHC_FLAG_USE_DMA;

	aprint_naive(": SDHC controller\n");
	aprint_normal(": SDHC controller%s\n",
	   (sc->sc_flags & SDHC_FLAG_USE_DMA) ? " (DMA enabled)" : "");

	asc->asc_ih = intr_establish(loc->loc_intr, IPL_VM, IST_LEVEL,
	    sdhc_intr, sc);
	if (asc->asc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intr);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n",
	     loc->loc_intr);

	error = sdhc_host_found(sc, asc->asc_bst, asc->asc_bsh,
	    loc->loc_size);
	if (error != 0) {
		aprint_error_dev(self, "couldn't initialize host, error=%d\n",
		    error);
		goto fail;
	}

	return;
fail:

	if (asc->asc_ih) {
		intr_disestablish(asc->asc_ih);
		asc->asc_ih = NULL;
	}
}
