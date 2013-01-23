/*	$NetBSD: bcm2835_emmc.c,v 1.1.6.4 2013/01/23 00:05:41 yamt Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_emmc.c,v 1.1.6.4 2013/01/23 00:05:41 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm_amba.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

struct bcmemmc_softc {
	struct sdhc_softc	sc;
	device_t		sc_sdmmc;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct sdhc_host	*sc_hosts[1];
	void			*sc_ih;
};

static int bcmemmc_match(device_t, struct cfdata *, void *);
static void bcmemmc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bcmemmc, sizeof(struct bcmemmc_softc),
    bcmemmc_match, bcmemmc_attach, NULL, NULL);

/* ARGSUSED */
static int
bcmemmc_match(device_t parent, struct cfdata *match, void *aux)
{
	struct amba_attach_args *aaa = aux;

	if (strcmp(aaa->aaa_name, "emmc") != 0)
		return 0;

	return 1;
}

/* ARGSUSED */
static void
bcmemmc_attach(device_t parent, device_t self, void *aux)
{
	struct bcmemmc_softc *sc = device_private(self);
	prop_dictionary_t dict = device_properties(self);
	struct amba_attach_args *aaa = aux;
	prop_number_t frequency;
	int error;

	sc->sc.sc_dev = self;
 	sc->sc.sc_dmat = aaa->aaa_dmat;
	sc->sc.sc_flags = 0;
	sc->sc.sc_flags |= SDHC_FLAG_32BIT_ACCESS;
	sc->sc.sc_flags |= SDHC_FLAG_HOSTCAPS;
	sc->sc.sc_flags |= SDHC_FLAG_NO_HS_BIT;
	sc->sc.sc_caps = SDHC_VOLTAGE_SUPP_3_3V | SDHC_HIGH_SPEED_SUPP |
	    SDHC_MAX_BLK_LEN_1024;
#if notyet
 	sc->sc.sc_flags |= SDHC_FLAG_USE_DMA;
	sc->sc.sc_caps |= SDHC_DMA_SUPPORT;
#endif
	sc->sc.sc_host = sc->sc_hosts;
	sc->sc.sc_clkbase = 50000;	/* Default to 50MHz */
	sc->sc_iot = aaa->aaa_iot;

	/* Fetch the EMMC clock frequency from property if set. */
	frequency = prop_dictionary_get(dict, "frequency");
	if (frequency != NULL) {
		sc->sc.sc_clkbase = prop_number_integer_value(frequency) / 1000;
	}    
	
	error = bus_space_map(sc->sc_iot, aaa->aaa_addr, aaa->aaa_size, 0,
	    &sc->sc_ioh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers for %s: %d\n", aaa->aaa_name, error);
		return;
	}

	aprint_naive(": SDHC controller\n");
	aprint_normal(": SDHC controller\n");

 	sc->sc_ih = bcm2835_intr_establish(aaa->aaa_intr, IPL_SDMMC, sdhc_intr,
 	    &sc->sc);

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		    aaa->aaa_intr);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on intr %d\n", aaa->aaa_intr);

	error = sdhc_host_found(&sc->sc, sc->sc_iot, sc->sc_ioh,
 	    aaa->aaa_size);
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
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, aaa->aaa_size);
}
