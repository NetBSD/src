/*	$NetBSD: wzero3_mci.c,v 1.4 2012/01/21 19:44:29 nonaka Exp $	*/

/*-
 * Copyright (C) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: wzero3_mci.c,v 1.4 2012/01/21 19:44:29 nonaka Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/pmf.h>
#include <sys/bus.h>

#include <machine/bootinfo.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/xscale/pxa2x0_mci.h>

#include <dev/sdmmc/sdmmcreg.h>

#include <hpcarm/dev/wzero3_reg.h>

#if defined(PXAMCI_DEBUG)
#define	DPRINTF(s)	do { printf s; } while (0)
#else
#define	DPRINTF(s)	do {} while (0)
#endif

struct wzero3mci_softc {
	struct pxamci_softc sc;

	void *sc_detect_ih;
	int sc_detect_pin;
	int sc_power_pin;
};

static int pxamci_match(device_t, cfdata_t, void *);
static void pxamci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wzero3mci, sizeof(struct wzero3mci_softc),
    pxamci_match, pxamci_attach, NULL, NULL);

static int wzero3mci_intr(void *arg);

static uint32_t	wzero3mci_get_ocr(void *);
static int wzero3mci_set_power(void *, uint32_t);
static int wzero3mci_card_detect(void *);
static int wzero3mci_write_protect(void *);

struct wzero3mci_model {
	platid_mask_t *platid;
	int detect_pin;	/* card detect pin# */
	int power_pin;	/* card power pin # */
} wzero3mci_table[] = {
	/* WS003SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS003SH,
		GPIO_WS003SH_SD_DETECT,
		GPIO_WS003SH_SD_POWER,
	},
	/* WS004SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS004SH,
		GPIO_WS003SH_SD_DETECT,
		GPIO_WS003SH_SD_POWER,
	},
	/* WS007SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS007SH,
		GPIO_WS007SH_SD_DETECT,
		GPIO_WS007SH_SD_POWER,
	},
	/* WS011SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS011SH,
		GPIO_WS011SH_SD_DETECT,
		GPIO_WS011SH_SD_POWER,
	},
	/* WS0020H */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS020SH,
		GPIO_WS020SH_SD_DETECT,
		GPIO_WS020SH_SD_POWER,	
	},

	{
		NULL, -1, -1
	},
};


static const struct wzero3mci_model *
wzero3mci_lookup(void)
{
	const struct wzero3mci_model *model;

	for (model = wzero3mci_table; model->platid != NULL; model++) {
		if (platid_match(&platid, model->platid)) {
			return model;
		}
	}
	return NULL;
}

static int
pxamci_match(device_t parent, cfdata_t cf, void *aux)
{

	if (strcmp(cf->cf_name, "pxamci") != 0)
		return 0;
	if (wzero3mci_lookup() == NULL)
		return 0;
	return 1;
}

static void
pxamci_attach(device_t parent, device_t self, void *aux)
{
	struct wzero3mci_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;
	const struct wzero3mci_model *model;

	sc->sc.sc_dev = self;

	model = wzero3mci_lookup();
	if (model == NULL) {
		aprint_error(": Unknown model.");
		return;
	}

	sc->sc_detect_pin = model->detect_pin;
	sc->sc_power_pin = model->power_pin;

	/* Establish SD detect interrupt */
	if (sc->sc_detect_pin >= 0) {
		pxa2x0_gpio_set_function(sc->sc_detect_pin, GPIO_IN);
		sc->sc_detect_ih = pxa2x0_gpio_intr_establish(sc->sc_detect_pin,
		    IST_EDGE_BOTH, IPL_BIO, wzero3mci_intr, sc);
		if (sc->sc_detect_ih == NULL) {
			aprint_error_dev(self,
			    "unable to establish card detect interrupt\n");
			return;
		}
	}

	sc->sc.sc_tag.cookie = sc;
	sc->sc.sc_tag.get_ocr = wzero3mci_get_ocr;
	sc->sc.sc_tag.set_power = wzero3mci_set_power;
	sc->sc.sc_tag.card_detect = wzero3mci_card_detect;
	sc->sc.sc_tag.write_protect = wzero3mci_write_protect;
	sc->sc.sc_caps = PMC_CAPS_4BIT;

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
wzero3mci_intr(void *arg)
{
	struct wzero3mci_softc *sc = (struct wzero3mci_softc *)arg;

	pxa2x0_gpio_clear_intr(sc->sc_detect_pin);

	pxamci_card_detect_event(&sc->sc);

	return 1;
}

/*ARGSUSED*/
static uint32_t
wzero3mci_get_ocr(void *arg)
{

	return MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V;
}

static int
wzero3mci_set_power(void *arg, uint32_t ocr)
{
	struct wzero3mci_softc *sc = (struct wzero3mci_softc *)arg;
	int error = 0;

	if (sc->sc_power_pin >= 0) {
		if (ISSET(ocr, MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V)) {
			/* power on */
			pxa2x0_gpio_set_bit(sc->sc_power_pin);
		} else if (ocr == 0) {
			/* power off */
			pxa2x0_gpio_clear_bit(sc->sc_power_pin);
		} else {
			aprint_error_dev(sc->sc.sc_dev,
			    "unsupported OCR (%#x)\n", ocr);
			error = EINVAL;
		}
	}
	return error;
}

/*
 * Return non-zero if the card is currently inserted.
 */
static int
wzero3mci_card_detect(void *arg)
{
	struct wzero3mci_softc *sc = (struct wzero3mci_softc *)arg;

	if (sc->sc_detect_pin >= 0) {
		if (pxa2x0_gpio_get_bit(sc->sc_detect_pin))
			return 0;
	}
	return 1;
}

/*
 * Return non-zero if the card is currently write-protected.
 */
/*ARGSUSED*/
static int
wzero3mci_write_protect(void *arg)
{

	return 0;
}
