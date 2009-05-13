/*	$NetBSD: gxmci.c,v 1.1.6.2 2009/05/13 17:16:38 jym Exp $	*/

/*-
 * Copyright (c) 2007 NONAKA Kimihiro <nonaka@netbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: gxmci.c,v 1.1.6.2 2009/05/13 17:16:38 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/xscale/pxa2x0_mci.h>

#include <dev/sdmmc/sdmmcreg.h>

#if defined(PXAMCI_DEBUG)
#define	DPRINTF(s)	printf s
#else
#define	DPRINTF(s)
#endif

struct gxmci_softc {
	struct pxamci_softc sc;
};

static int pxamci_match(device_t, cfdata_t, void *);
static void pxamci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gxmci, sizeof(struct gxmci_softc),
    pxamci_match, pxamci_attach, NULL, NULL);

static uint32_t gxmci_get_ocr(void *);
static int gxmci_set_power(void *, uint32_t);
static int gxmci_card_detect(void *);
static int gxmci_write_protect(void *);

static int
pxamci_match(device_t parent, cfdata_t cf, void *aux)
{
	static struct pxa2x0_gpioconf pxa25x_gxmci_gpioconf[] = {
		{  8, GPIO_ALT_FN_1_OUT },	/* MMCCS0 */
		{ 53, GPIO_ALT_FN_1_OUT },	/* MMCLK */
		{ -1 }
	};
	struct pxa2x0_gpioconf *gpioconf;
	u_int reg;
	int i;

	/*
	 * Check GPIO configuration.  If you use these, it is sure already
	 * to have been set by gxio.
	 */
	gpioconf =
	    CPU_IS_PXA250 ? pxa25x_gxmci_gpioconf : pxa27x_pxamci_gpioconf;
	for (i = 0; gpioconf[i].pin != -1; i++) {
		reg = pxa2x0_gpio_get_function(gpioconf[i].pin);
		if (GPIO_FN(reg) != GPIO_FN(gpioconf[i].value) ||
		    GPIO_FN_IS_OUT(reg) != GPIO_FN_IS_OUT(gpioconf[i].value))
		return 0;
	}

	return 1;
}

static void
pxamci_attach(device_t parent, device_t self, void *aux)
{
	struct gxmci_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;

	sc->sc.sc_tag.cookie = sc;
	sc->sc.sc_tag.get_ocr = gxmci_get_ocr;
	sc->sc.sc_tag.set_power = gxmci_set_power;
	sc->sc.sc_tag.card_detect = gxmci_card_detect;
	sc->sc.sc_tag.write_protect = gxmci_write_protect;
	sc->sc.sc_caps = 0;

	if (pxamci_attach_sub(self, pxa)) {
		aprint_error_dev(self, "unable to attach MMC controller\n");
	}

	if (!pmf_device_register(self, NULL, NULL)) {
		aprint_error_dev(self, "couldn't establish power handler\n");
	}
}

static uint32_t
gxmci_get_ocr(void *arg)
{

	return MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V;
}

static int
gxmci_set_power(void *arg, uint32_t ocr)
{

	return 0;
}

/*
 * Return non-zero if the card is currently inserted.
 */
static int
gxmci_card_detect(void *arg)
{

	return 1;
}

/*
 * Return non-zero if the card is currently write-protected.
 */
static int
gxmci_write_protect(void *arg)
{

	return 0;
}
