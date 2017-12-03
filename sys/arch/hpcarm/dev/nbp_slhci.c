/*	$NetBSD: nbp_slhci.c,v 1.1.12.1 2017/12/03 11:36:14 jdolecek Exp $ */
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nbp_slhci.c,v 1.1.12.1 2017/12/03 11:36:14 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/ic/sl811hsvar.h>

#define NETBOOKPRO_SLHCI_REG_ADDR	0x00000
#define NETBOOKPRO_SLHCI_REG_DATA	0x10000
#define NETBOOKPRO_SLHCI_REG_SIZE	0x20000

static int nbp_slhci_match(device_t, cfdata_t, void *);
static void nbp_slhci_attach(device_t, device_t, void *);

static void nbp_slhci_power(void *, enum power_change);

CFATTACH_DECL_NEW(nbp_slhci, sizeof(struct slhci_softc), 
    nbp_slhci_match, nbp_slhci_attach, NULL, NULL);


/* ARGSUSED */
static int
nbp_slhci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (strcmp(pxa->pxa_name, match->cf_name) != 0 ||
	    !platid_match(&platid, &platid_mask_MACH_PSIONTEKLOGIX_NETBOOK_PRO))
		return 0;

	return 1;
}

/* ARGSUSED */
static void
nbp_slhci_attach(device_t parent, device_t self, void *aux)
{
	struct slhci_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;
	bus_space_handle_t ioh;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;

	if (bus_space_map(pxa->pxa_iot, pxa->pxa_addr,
				NETBOOKPRO_SLHCI_REG_SIZE, 0, &ioh) != 0) {
		aprint_error_dev(self, "can't map I/O space\n");
		return;
	}

	slhci_preinit(sc, nbp_slhci_power, pxa->pxa_iot, ioh, 0,
	    NETBOOKPRO_SLHCI_REG_DATA - NETBOOKPRO_SLHCI_REG_ADDR);

	/* GPIO 2 connects IRQ */
	pxa2x0_gpio_set_function(2, GPIO_IN);
	if (pxa2x0_gpio_intr_establish(2, IST_EDGE_RISING, IPL_USB,
						slhci_intr, sc) == NULL) {
		aprint_error_dev(self, "unable to establish interrupt\n");
		return;
	}

	/* Reset Host Controller */
	pxa2x0_gpio_set_function(14, GPIO_OUT);
	pxa2x0_gpio_clear_bit(14);
	delay(1);			/* XXXX: nRST for 16+ clocks @ 48MHz */
	pxa2x0_gpio_set_bit(14);

	if (slhci_attach(sc))
		aprint_error_dev(self, "slhci_attach failed\n");
	return;
}

static void
nbp_slhci_power(void *arg, enum power_change onoff)
{

//	nbp_pcon_command(I2C_SETUSBHOSTPOWER, onoff, PCON_FLAG_ASYNC, NULL);
}
