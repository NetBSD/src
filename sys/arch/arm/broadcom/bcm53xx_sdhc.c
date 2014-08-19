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

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_sdhc.c,v 1.1.2.1 2014/08/20 00:02:45 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>

static int sdhc_ccb_match(device_t, cfdata_t, void *);
static void sdhc_ccb_attach(device_t, device_t, void *);

struct sdhc_ccb_softc {
	struct sdhc_softc ccbsc_sc;
	bus_space_tag_t ccbsc_bst;
	bus_space_handle_t ccbsc_bsh;
	struct sdhc_host *ccbsc_hosts[1];
	void *ccbsc_ih;
};

CFATTACH_DECL_NEW(sdhc_ccb, sizeof(struct sdhc_ccb_softc),
	sdhc_ccb_match, sdhc_ccb_attach, NULL, NULL);

static int
sdhc_ccb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	KASSERT(cf->cf_loc[BCMCCBCF_PORT] == BCMCCBCF_PORT_DEFAULT);

	return 1;
}

static void
sdhc_ccb_attach(device_t parent, device_t self, void *aux)
{
	struct sdhc_ccb_softc * const ccbsc = device_private(self);
	struct sdhc_softc * const sc = &ccbsc->ccbsc_sc;
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	ccbsc->ccbsc_bst = ccbaa->ccbaa_ccb_bst;
	bus_space_subregion(ccbsc->ccbsc_bst, ccbaa->ccbaa_ccb_bsh,
	    loc->loc_offset, loc->loc_size, &ccbsc->ccbsc_bsh);

	sc->sc_dev = self;
	sc->sc_dmat = ccbaa->ccbaa_dmat;
	sc->sc_host = ccbsc->ccbsc_hosts;
	sc->sc_flags |= SDHC_FLAG_32BIT_ACCESS;
	sc->sc_flags |= SDHC_FLAG_HAVE_CGM;
	//sc->sc_flags |= SDHC_FLAG_USE_DMA;

	aprint_naive(": SDHC controller\n");
	aprint_normal(": SDHC controller%s\n",
	   (sc->sc_flags & SDHC_FLAG_USE_DMA) ? " (DMA enabled)" : "");

#if 1
	int error = sdhc_host_found(sc, ccbsc->ccbsc_bst, ccbsc->ccbsc_bsh,
	    loc->loc_size);
	if (error != 0) {
		aprint_error_dev(self, "couldn't initialize host, error=%d\n",
		    error);
		goto fail;
	}
#endif

	ccbsc->ccbsc_ih = intr_establish(loc->loc_intrs[0], IPL_VM, IST_LEVEL,
	    sdhc_intr, sc);
	if (ccbsc->ccbsc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intrs[0]);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n",
	     loc->loc_intrs[0]);

	return;

fail:
	if (ccbsc->ccbsc_ih) {
		intr_disestablish(ccbsc->ccbsc_ih);
		ccbsc->ccbsc_ih = NULL;
	}
}
