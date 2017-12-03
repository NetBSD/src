/*	$NetBSD: rmixl_ehci.c,v 1.5.2.1 2017/12/03 11:36:28 jdolecek Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000, 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Herb Peyerl of Middle Digital Inc.
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
__KERNEL_RCSID(0, "$NetBSD: rmixl_ehci.c,v 1.5.2.1 2017/12/03 11:36:28 jdolecek Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_usbivar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>


static int	rmixl_ehci_match(device_t, cfdata_t, void *);
static void	rmixl_ehci_attach(device_t, device_t, void *);


CFATTACH_DECL_NEW(rmixl_ehci, sizeof (ehci_softc_t),
    rmixl_ehci_match, rmixl_ehci_attach, NULL, NULL);

int
rmixl_ehci_match(device_t parent, cfdata_t match, void *aux)
{
	struct rmixl_usbi_attach_args *usbi = aux;

        if (usbi->usbi_addr == (RMIXL_IO_DEV_USB_A + RMIXL_USB_HOST_EHCI_BASE))
                return rmixl_probe_4((volatile uint32_t *)
			RMIXL_IOREG_VADDR(usbi->usbi_addr));

        return 0;
}

void
rmixl_ehci_attach(device_t parent, device_t self, void *aux)
{
	ehci_softc_t * const sc = device_private(self);
	rmixl_usbi_softc_t * const psc = device_private(parent);
	struct rmixl_usbi_attach_args * const usbi = aux;
	void *ih = NULL;
	uint32_t r;

	/* check state of IO_AD9 signal latched in GPIO Reset Config reg */
	r = RMIXL_IOREG_READ(RMIXL_IO_DEV_GPIO + RMIXL_GPIO_RESET_CFG);
	if ((r & RMIXL_GPIO_RESET_CFG_USB_DEV) == 0) {
		aprint_error_dev(self,
			"IO_AD9 selects Device mode, abort Host attach\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->iot = usbi->usbi_el_bst;
	sc->sc_size = usbi->usbi_size;
	sc->sc_bus.ub_dmatag = usbi->usbi_dmat;
	sc->sc_bus.ub_revision = USBREV_1_0;

	/*
	 * Grab the companion OHCI devices from our parent.
	 */
	if (psc->sc_ohci_devs[1] != NULL) {
		sc->sc_comps[0] = psc->sc_ohci_devs[0];
		sc->sc_comps[1] = psc->sc_ohci_devs[1];
		sc->sc_ncomp = 2;
	} else {
		sc->sc_comps[0] = psc->sc_ohci_devs[0];
		sc->sc_ncomp = 1;
	}

	if (bus_space_map(sc->iot, usbi->usbi_addr, sc->sc_size, 0, &sc->ioh)) {
		aprint_error_dev(self, "unable to map registers\n");
		return;
	}

	/* get offset to operational regs */
	sc->sc_offs = EREAD1(sc, EHCI_CAPLENGTH);

	/* Disable EHCI interrupts */
	EOWRITE4(sc, EHCI_USBINTR, 0);

	/* establish interrupt */
	if (usbi->usbi_intr != RMIXL_USBICF_INTR_DEFAULT) {
		ih = rmixl_usbi_intr_establish(device_private(parent),
			usbi->usbi_intr, ehci_intr, sc);
		if (ih == NULL)
			panic("%s: couldn't establish interrupt",
				device_xname(self));
	}

	int err = ehci_init(sc);
	if (err) {
		aprint_error_dev(self, "init failed, error=%d\n", err);
		if (ih != NULL)
			rmixl_intr_disestablish(ih);
		return;
	}

	/* Attach USB device */
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
}

