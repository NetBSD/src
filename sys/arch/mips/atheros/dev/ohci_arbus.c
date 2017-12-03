/*	$NetBSD: ohci_arbus.c,v 1.1.12.1 2017/12/03 11:36:26 jdolecek Exp $	*/

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

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ohci_arbus.c,v 1.1.12.1 2017/12/03 11:36:26 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

#include <mips/locore.h>

#include <mips/atheros/include/arbusvar.h>

static int	ohci_arbus_match(device_t, cfdata_t, void *);
static void	ohci_arbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ohci_arbus, sizeof (ohci_softc_t),
    ohci_arbus_match, ohci_arbus_attach, NULL, NULL);

int
ohci_arbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct arbus_attach_args *aa = aux;
	bus_space_handle_t bsh;

	if (strcmp(aa->aa_name, cf->cf_name))
		return 0;

	if (bus_space_map(aa->aa_bst, aa->aa_addr, aa->aa_size, 0, &bsh) != 0)
		return 0;

	return 0; /* XXX */

	if (badaddr((void *)bsh, 4) == -1) {
		bus_space_unmap(aa->aa_bst, bsh, aa->aa_size);
		return 0;
	}

	bus_space_unmap(aa->aa_bst, bsh, aa->aa_size);
	return 1;
}

void
ohci_arbus_attach(device_t parent, device_t self, void *aux)
{
	ohci_softc_t * const sc = device_private(self);
	struct arbus_attach_args * const aa = aux;
	void *ih = NULL;

	sc->sc_dev = self;
	sc->iot = aa->aa_bst;
	sc->sc_size = aa->aa_size;
	sc->sc_bus.ub_dmatag = aa->aa_dmat;
	sc->sc_bus.ub_hcpriv = sc;

	if (bus_space_map(sc->iot, aa->aa_addr, sc->sc_size, 0, &sc->ioh)) {
		aprint_error_dev(self, "unable to map registers\n");
		return;
	}

	/* Disable OHCI interrupts */
	bus_space_write_4(sc->iot, sc->ioh, OHCI_INTERRUPT_DISABLE,
	    OHCI_ALL_INTRS);

	/* establish interrupt */
	ih = arbus_intr_establish(aa->aa_cirq, aa->aa_mirq, ohci_intr, sc);
	if (ih == NULL)
		panic("%s: couldn't establish interrupt", device_xname(self));

	/* we don't handle endianess in bus space */
	sc->sc_endian = OHCI_LITTLE_ENDIAN;

	int err = ohci_init(sc);
	if (err) {
		aprint_error_dev(self, "init failed, error=%d\n", err);
		if (ih != NULL)
			arbus_intr_disestablish(ih);
		return;
	}

#if 0
	if (psc->sc_ohci_devs[0] == NULL) {
		psc->sc_ohci_devs[0] = self;
	} else if (psc->sc_ohci_devs[1] == NULL) {
		psc->sc_ohci_devs[1] = self;
	} else {
		panic("%s: too many ohci devices", __func__);
	}
#endif

	/* Attach USB device */
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
}
