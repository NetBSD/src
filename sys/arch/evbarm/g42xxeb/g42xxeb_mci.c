/*	$NetBSD: g42xxeb_mci.c,v 1.2.10.1 2012/04/17 00:06:14 yamt Exp $ */

/*-
 * Copyright (c) 2009  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* derived from zaurus/dev/zmci.c */

/*-
 * Copyright (C) 2006-2008 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: g42xxeb_mci.c,v 1.2.10.1 2012/04/17 00:06:14 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/pmf.h>

#include <machine/intr.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/xscale/pxa2x0_mci.h>

#include <dev/sdmmc/sdmmcreg.h>

#include <arm/xscale/pxa2x0var.h>
#include <evbarm/g42xxeb/g42xxeb_var.h>

struct g42xxeb_mci_softc {
	struct pxamci_softc sc_mci;

	bus_space_tag_t sc_obio_iot;
	bus_space_handle_t sc_obio_ioh;

	void *sc_detect_ih;
};

static int pxamci_match(device_t, cfdata_t, void *);
static void pxamci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pxamci_obio, sizeof(struct g42xxeb_mci_softc),
    pxamci_match, pxamci_attach, NULL, NULL);

static int g42xxeb_mci_intr(void *arg);

static uint32_t	g42xxeb_mci_get_ocr(void *);
static int g42xxeb_mci_set_power(void *, uint32_t);
static int g42xxeb_mci_card_detect(void *);
static int g42xxeb_mci_write_protect(void *);

static int
pxamci_match(device_t parent, cfdata_t cf, void *aux)
{

	if (strcmp(cf->cf_name, "pxamci") == 0)
		return 1;
	return 0;
}

struct pxa2x0_gpioconf g42xxeb_pxamci_gpioconf[] = {
	{  6, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCLK */
	{  8, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCS0 */
	{  9, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCS1 */
	{  -1 }
};


static void
pxamci_attach(device_t parent, device_t self, void *aux)
{
	struct g42xxeb_mci_softc *sc = device_private(self);
	struct obio_attach_args *oba = aux;
	struct obio_softc *osc = device_private(parent);
	struct pxaip_attach_args pxa;
	struct pxa2x0_gpioconf *gpioconf[] = {
		g42xxeb_pxamci_gpioconf,
		NULL
	};

	bus_space_tag_t iot = oba->oba_iot;

	sc->sc_mci.sc_dev = self;

	pxa2x0_gpio_config(gpioconf);

	/* Establish card detect interrupt */
	sc->sc_detect_ih = obio_intr_establish(osc, G42XXEB_INT_MMCSD,
	    IPL_BIO, IST_EDGE_BOTH, g42xxeb_mci_intr, sc);
	if (sc->sc_detect_ih == NULL) {
		aprint_error_dev(self,
		    "unable to establish SD detect interrupt\n");
		return;
	}

	sc->sc_mci.sc_tag.cookie = sc;
	sc->sc_mci.sc_tag.get_ocr = g42xxeb_mci_get_ocr;
	sc->sc_mci.sc_tag.set_power = g42xxeb_mci_set_power;
	sc->sc_mci.sc_tag.card_detect = g42xxeb_mci_card_detect;
	sc->sc_mci.sc_tag.write_protect = g42xxeb_mci_write_protect;
	sc->sc_mci.sc_caps = 0;

	pxa.pxa_iot = iot; 	/* actually, here we want I/O tag for
				   pxaip, not for obio. */
				   
#if 0
	/* pxamci_attach_sub() ignores following values and uses
	 * constants. */
	pxa.pxa_addr = PXA2X0_MMC_BASE;
	pxa.pxa_size = PXA2X0_MMC_SIZE;
	pxa.pxa_intr = PXA2X0_INT_MMC;
#endif

	/* disable DMA for sdmmc for now. */
	SET(sc->sc_mci.sc_caps, PMC_CAPS_NO_DMA);

	if (pxamci_attach_sub(self, &pxa)) {
		aprint_error_dev(self,
		    "unable to attach MMC controller\n");
		goto free_intr;
	}

	if (!pmf_device_register(self, NULL, NULL)) {
		aprint_error_dev(self,
		    "couldn't establish power handler\n");
	}

	sc->sc_obio_iot = iot;
	sc->sc_obio_ioh = osc->sc_obioreg_ioh;

	return;

free_intr:
	obio_intr_disestablish(osc, G42XXEB_INT_MMCSD, sc->sc_detect_ih);
	sc->sc_detect_ih = NULL;
}

static int
g42xxeb_mci_intr(void *arg)
{
	struct g42xxeb_mci_softc *sc = (struct g42xxeb_mci_softc *)arg;

	pxamci_card_detect_event(&sc->sc_mci);

	return 1;
}

static uint32_t
g42xxeb_mci_get_ocr(void *arg)
{

	return MMC_OCR_3_2V_3_3V;
}

static int
g42xxeb_mci_set_power(void *arg, uint32_t ocr)
{
	/* nothing to do */
	return 0;
}

/*
 * Return non-zero if the card is currently inserted.
 */
static int
g42xxeb_mci_card_detect(void *arg)
{
	struct g42xxeb_mci_softc *sc = (struct g42xxeb_mci_softc *)arg;
	uint16_t reg;

	reg = bus_space_read_2(sc->sc_obio_iot, sc->sc_obio_ioh,
	    G42XXEB_INTSTS2);

	return !(reg & (1<<G42XXEB_INT_MMCSD));
}

/*
 * Return non-zero if the card is currently write-protected.
 */
static int
g42xxeb_mci_write_protect(void *arg)
{
	struct g42xxeb_mci_softc *sc = (struct g42xxeb_mci_softc *)arg;
	uint16_t reg;

	reg = bus_space_read_2(sc->sc_obio_iot, sc->sc_obio_ioh,
	    G42XXEB_WP);

	return reg & 1;
}
