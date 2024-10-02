/* $NetBSD: sdhc_hollywood.c,v 1.2.2.3 2024/10/02 12:28:15 martin Exp $ */

/*-
 * Copyright (c) 2024 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sdhc_hollywood.c,v 1.2.2.3 2024/10/02 12:28:15 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <machine/wii.h>
#include "hollywood.h"

#define SDHC_SIZE			0x200
#define SDHC_HOLLYWOOD_WRITE_DELAY	5

extern struct powerpc_bus_dma_tag wii_mem2_bus_dma_tag;

static int	sdhc_hollywood_match(device_t, cfdata_t, void *);
static void	sdhc_hollywood_attach(device_t, device_t, void *);

struct sdhc_hollywood_softc {
	struct sdhc_softc	sc_base;
	struct sdhc_host	*sc_host[1];
};

CFATTACH_DECL_NEW(sdhc_hollywood, sizeof(struct sdhc_hollywood_softc),
	sdhc_hollywood_match, sdhc_hollywood_attach, NULL, NULL);

static int
sdhc_hollywood_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
sdhc_hollywood_attach(device_t parent, device_t self, void *aux)
{
	struct hollywood_attach_args *haa = aux;
	struct sdhc_hollywood_softc *sc = device_private(self);
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	int error;

	sc->sc_base.sc_dev = self;
	sc->sc_base.sc_host = sc->sc_host;
	sc->sc_base.sc_dmat = &wii_mem2_bus_dma_tag;
	sc->sc_base.sc_flags = SDHC_FLAG_SINGLE_POWER_WRITE |
			       SDHC_FLAG_32BIT_ACCESS |
			       SDHC_FLAG_USE_DMA;
	sc->sc_base.sc_write_delay = SDHC_HOLLYWOOD_WRITE_DELAY;

	bst = haa->haa_bst;
	if (bus_space_map(bst, haa->haa_addr, SDHC_SIZE, 0, &bsh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": SDHC\n");

	hollywood_claim_device(self,
	    device_unit(self) == 0 ? IOPSD0EN : IOPSD1EN);

	hollywood_intr_establish(haa->haa_irq, IPL_SDMMC, sdhc_intr,
	    &sc->sc_base, device_xname(self));

	error = sdhc_host_found(&sc->sc_base, bst, bsh, SDHC_SIZE);
	if (error != 0) {
		aprint_error_dev(self,
		    "couldn't initialize host, error = %d\n", error);
	}
}
