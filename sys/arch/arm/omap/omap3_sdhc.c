/*	$NetBSD: omap3_sdhc.c,v 1.1 2012/07/12 03:08:26 matt Exp $	*/
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap3_sdhc.c,v 1.1 2012/07/12 03:08:26 matt Exp $");

#include "opt_omap.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <sys/bus.h>

#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap3_sdmmcreg.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>

static int obiosdhc_match(device_t, cfdata_t, void *);
static void obiosdhc_attach(device_t, device_t, void *);

struct obiosdhc_softc {
	struct sdhc_softc	sc;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_space_handle_t	sc_sdhc_bsh;
	struct sdhc_host	*sc_hosts[1];
	void 			*sc_ih;		/* interrupt vectoring */
};

CFATTACH_DECL_NEW(obiosdhc, sizeof(struct obiosdhc_softc),
    obiosdhc_match, obiosdhc_attach, NULL, NULL);

static int
obiosdhc_match(device_t parent, cfdata_t cf, void *aux)
{
#ifdef OMAP_3530
	struct obio_attach_args * const oa = aux;
#endif

#ifdef OMAP_3530
	if (oa->obio_addr == SDMMC1_BASE_3530
	    || oa->obio_addr == SDMMC2_BASE_3530
	    || oa->obio_addr == SDMMC3_BASE_3530)
                return 1;
#endif

        return 0;
}

static void
obiosdhc_attach(device_t parent, device_t self, void *aux)
{
	struct obiosdhc_softc * const sc = device_private(self);
	struct obio_attach_args * const oa = aux;
	int error;

	sc->sc.sc_dmat = oa->obio_dmat;
	sc->sc.sc_dev = self;
	//sc->sc.sc_flags |= SDHC_FLAG_USE_DMA;
	sc->sc.sc_flags |= SDHC_FLAG_32BIT_ACCESS;
	sc->sc.sc_flags |= SDHC_FLAG_HAVE_CGM;
	sc->sc.sc_flags |= SDHC_FLAG_NO_LED_ON;
	sc->sc.sc_host = sc->sc_hosts;
	sc->sc.sc_clkbase = 96000;	/* 96MHZ */
	sc->sc_bst = oa->obio_iot;

	error = bus_space_map(sc->sc_bst, oa->obio_addr, oa->obio_size, 0,
	    &sc->sc_bsh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers: %d\n", error);
		return;
	}

	bus_space_subregion(sc->sc_bst, sc->sc_bsh, OMAP3_SDMMC_SDHC_OFFSET,
	    OMAP3_SDMMC_SDHC_SIZE, &sc->sc_sdhc_bsh);

	aprint_naive(": SDHC controller\n");
	aprint_normal(": SDHC controller\n");

	sc->sc_ih = intr_establish(oa->obio_intr, IPL_VM, IST_LEVEL, 
	    sdhc_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     oa->obio_intr);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n",
	     oa->obio_intr);

	error = sdhc_host_found(&sc->sc, sc->sc_bst, sc->sc_sdhc_bsh,
	    oa->obio_size - OMAP3_SDMMC_SDHC_OFFSET);
	if (error != 0) {
		aprint_error_dev(self, "couldn't initialize host, error=%d\n",
		    error);
		goto fail;
	}
	return;

fail:
	if (sc->sc_ih) {
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}
	bus_space_unmap(sc->sc_bst, sc->sc_bsh, oa->obio_size);
}
