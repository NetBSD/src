/* $NetBSD: ohci_hollywood.c,v 1.2.2.3 2024/10/02 12:28:15 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ohci_hollywood.c,v 1.2.2.3 2024/10/02 12:28:15 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

#include <machine/wii.h>
#include <machine/pio.h>
#include "hollywood.h"

#define	USB_CHICKENBITS		0x0d0400cc
#define	 OHCI_INTR_ENABLE	0x000e1800

extern struct powerpc_bus_dma_tag wii_mem2_bus_dma_tag;

static int	ohci_hollywood_match(device_t, cfdata_t, void *);
static void	ohci_hollywood_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ohci_hollywood, sizeof(struct ohci_softc),
	ohci_hollywood_match, ohci_hollywood_attach, NULL, NULL);

static int
ohci_hollywood_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
ohci_hollywood_attach(device_t parent, device_t self, void *aux)
{
	struct hollywood_attach_args *haa = aux;
	struct ohci_softc *sc = device_private(self);
	int error;

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_dmatag = &wii_mem2_bus_dma_tag;
	sc->sc_flags = 0;
	sc->sc_size = 0x200;
	sc->iot = haa->haa_bst;
	error = bus_space_map(sc->iot, haa->haa_addr, sc->sc_size, 0, &sc->ioh);
	if (error != 0) {
		aprint_error(": couldn't map registers (%d)\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": OHCI\n");

	hollywood_claim_device(self,
	    device_unit(self) == 0 ? IOPOH0EN : IOPOH1EN);

	bus_space_write_4(sc->iot, sc->ioh, OHCI_INTERRUPT_DISABLE,
	    OHCI_ALL_INTRS);

	out32(USB_CHICKENBITS, in32(USB_CHICKENBITS) | OHCI_INTR_ENABLE);

	hollywood_intr_establish(haa->haa_irq, IPL_USB, ohci_intr, sc,
	    device_xname(self));

	error = ohci_init(sc);
	if (error != 0) {
		aprint_error_dev(self, "init failed, error = %d\n", error);
		return;
	}

	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint,
	    CFARGS_NONE);
}
