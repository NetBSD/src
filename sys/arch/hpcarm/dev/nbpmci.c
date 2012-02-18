/*	$NetBSD: nbpmci.c,v 1.1.6.1 2012/02/18 07:32:08 mrg Exp $	*/

/*-
 * Copyright (C) 2007 NONAKA Kimihiro <nonaka@netbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: nbpmci.c,v 1.1.6.1 2012/02/18 07:32:08 mrg Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/xscale/pxa2x0_mci.h>

#include <dev/sdmmc/sdmmcreg.h>


static int pxamci_match(device_t, cfdata_t, void *);
static void pxamci_attach(device_t, device_t, void *);

static int nbpmci_intr(void *);

static uint32_t nbpmci_get_ocr(void *);
static int nbpmci_set_power(void *, uint32_t);
static int nbpmci_card_detect(void *);
static int nbpmci_write_protect(void *);

CFATTACH_DECL_NEW(nbpmci, sizeof(struct pxamci_softc),
    pxamci_match, pxamci_attach, NULL, NULL);

/* ARGSUSED */
static int
pxamci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (strcmp(pxa->pxa_name, match->cf_name) != 0 ||
	    !platid_match(&platid, &platid_mask_MACH_PSIONTEKLOGIX_NETBOOK_PRO))
		return 0;

	pxa->pxa_size = PXA2X0_MMC_SIZE;

	return 1;
}

/* ARGSUSED */
static void
pxamci_attach(device_t parent, device_t self, void *aux)
{
	struct pxamci_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;
	void *ih;

	pxa2x0_gpio_set_function(9, GPIO_IN);
	ih = pxa2x0_gpio_intr_establish(9, IST_EDGE_BOTH, IPL_BIO,
	    nbpmci_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self,
		    "unable to establish card detect interrupt\n");
		return;
	}

	sc->sc_tag.cookie = sc;
	sc->sc_tag.get_ocr = nbpmci_get_ocr;
	sc->sc_tag.set_power = nbpmci_set_power;
	sc->sc_tag.card_detect = nbpmci_card_detect;
	sc->sc_tag.write_protect = nbpmci_write_protect;
	sc->sc_caps = 0;

	if (pxamci_attach_sub(self, pxa))
		aprint_error_dev(self, "unable to attach MMC controller\n");

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
nbpmci_intr(void *arg)
{
	struct pxamci_softc *sc = (struct pxamci_softc *)arg;

	pxa2x0_gpio_clear_intr(9);

	pxamci_card_detect_event(sc);

	return 1;
}

/* ARGSUSED */
static uint32_t
nbpmci_get_ocr(void *arg)
{

	return MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V;
}

/* ARGSUSED */
static int
nbpmci_set_power(void *arg, uint32_t ocr)
{

//	nbp_pcon_command(I2C_SETMMCPOWER, vdd_bits ? 1 : 0, 0, NULL);
	return 0;
}

/*
 * Return non-zero if the card is currently inserted.
 */
/* ARGSUSED */
static int
nbpmci_card_detect(void *arg)
{

	if (pxa2x0_gpio_get_bit(9))
		return 0;
	return 1;
}

/*
 * Return non-zero if the card is currently write-protected.
 */
/* ARGSUSED */
static int
nbpmci_write_protect(void *arg)
{

	return 0;
}
