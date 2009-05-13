/*	$NetBSD: zmci.c,v 1.1.6.2 2009/05/13 17:18:51 jym Exp $	*/

/*-
 * Copyright (c) 2006-2008 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zmci.c,v 1.1.6.2 2009/05/13 17:18:51 jym Exp $");

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

#include <zaurus/dev/scoopreg.h>
#include <zaurus/dev/scoopvar.h>

#include <zaurus/zaurus/zaurus_reg.h>
#include <zaurus/zaurus/zaurus_var.h>

#if defined(PXAMCI_DEBUG)
#define	DPRINTF(s)	do { printf s; } while (0)
#else
#define	DPRINTF(s)	do {} while (0)
#endif

struct zmci_softc {
	struct pxamci_softc sc;

	void *sc_detect_ih;
	int sc_detect_pin;
	int sc_wp_pin;
};

static int pxamci_match(device_t, cfdata_t, void *);
static void pxamci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(zmci, sizeof(struct zmci_softc),
    pxamci_match, pxamci_attach, NULL, NULL);

static int zmci_intr(void *arg);

static uint32_t	zmci_get_ocr(void *);
static int zmci_set_power(void *, uint32_t);
static int zmci_card_detect(void *);
static int zmci_write_protect(void *);

static int
pxamci_match(device_t parent, cfdata_t cf, void *aux)
{

	if (!ZAURUS_ISC3000)
		return 0;
	return 1;
}

static void
pxamci_attach(device_t parent, device_t self, void *aux)
{
	struct zmci_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;

	sc->sc.sc_dev = self;

	if (ZAURUS_ISC3000) {
		sc->sc_detect_pin = C3000_GPIO_SD_DETECT_PIN;
		sc->sc_wp_pin = C3000_GPIO_SD_WP_PIN;
		pxa2x0_gpio_set_function(sc->sc_detect_pin, GPIO_IN);
		pxa2x0_gpio_set_function(sc->sc_wp_pin, GPIO_IN);
	} else {
		/* XXX: C7x0/C8x0 */
		return;
	}

	/* Establish SD detect interrupt */
	sc->sc_detect_ih = pxa2x0_gpio_intr_establish(sc->sc_detect_pin,
	    IST_EDGE_BOTH, IPL_BIO, zmci_intr, sc);
	if (sc->sc_detect_ih == NULL) {
		aprint_error_dev(self,
		    "unable to establish nSD_DETECT interrupt\n");
		return;
	}

	sc->sc.sc_tag.cookie = sc;
	sc->sc.sc_tag.get_ocr = zmci_get_ocr;
	sc->sc.sc_tag.set_power = zmci_set_power;
	sc->sc.sc_tag.card_detect = zmci_card_detect;
	sc->sc.sc_tag.write_protect = zmci_write_protect;
	sc->sc.sc_caps = 0;

	if (pxamci_attach_sub(self, pxa)) {
		aprint_error_dev(self, "unable to attach MMC controller\n");
		goto free_intr;
	}

	if (!pmf_device_register(self, NULL, NULL)) {
		aprint_error_dev(self, "couldn't establish power handler\n");
	}

	return;

free_intr:
	pxa2x0_gpio_intr_disestablish(sc->sc_detect_ih);
	sc->sc_detect_ih = NULL;
}

static int
zmci_intr(void *arg)
{
	struct zmci_softc *sc = (struct zmci_softc *)arg;

	pxa2x0_gpio_clear_intr(sc->sc_detect_pin);

	pxamci_card_detect_event(&sc->sc);

	return 1;
}

static uint32_t
zmci_get_ocr(void *arg)
{

	return MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V;
}

static int
zmci_set_power(void *arg, uint32_t ocr)
{
	struct zmci_softc *sc = (struct zmci_softc *)arg;

	if (ISSET(ocr, MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V)) {
		scoop_set_sdmmc_power(1);
		return 0;
	}

	if (ocr != 0) {
		printf("%s: zmci_set_power(): unsupported OCR (%#x)\n",
		    device_xname(sc->sc.sc_dev), ocr);
		return EINVAL;
	}

	/* power off */
	scoop_set_sdmmc_power(0);
	return 0;
}

/*
 * Return non-zero if the card is currently inserted.
 */
static int
zmci_card_detect(void *arg)
{
	struct zmci_softc *sc = (struct zmci_softc *)arg;

	if (!pxa2x0_gpio_get_bit(sc->sc_detect_pin))
		return 1;
	return 0;
}

/*
 * Return non-zero if the card is currently write-protected.
 */
static int
zmci_write_protect(void *arg)
{
	struct zmci_softc *sc = (struct zmci_softc *)arg;

	if (pxa2x0_gpio_get_bit(sc->sc_wp_pin))
		return 1;
	return 0;
}
