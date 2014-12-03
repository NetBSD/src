/*	$NetBSD: ehci_arbus.c,v 1.2.16.1 2014/12/03 11:24:44 skrll Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ehci_arbus.c,v 1.2.16.1 2014/12/03 11:24:44 skrll Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <mips/atheros/include/ar9344reg.h>
#include <mips/atheros/include/arbusvar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

static int	ehci_arbus_match(device_t, cfdata_t, void *);
static void	ehci_arbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ehci_arbus, sizeof (ehci_softc_t),
    ehci_arbus_match, ehci_arbus_attach, NULL, NULL);

int
ehci_arbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct arbus_attach_args *aa = aux;

	if (strcmp(aa->aa_name, cf->cf_name) != 0)
		return 0;

	if (badaddr((void *)MIPS_PHYS_TO_KSEG1(aa->aa_addr), 4))
		return 0;

        return 1;	/* XXX */
}

void
ehci_arbus_attach(device_t parent, device_t self, void *aux)
{
	ehci_softc_t *sc = device_private(self);
	struct arbus_attach_args * const aa = aux;
	void *ih = NULL;
	int error;
	int status;

	sc->iot = aa->aa_bst_le;
	sc->sc_size = aa->aa_size;
	//sc->sc_bus.hci_private = sc;
	sc->sc_bus.dmatag = aa->aa_dmat;
	sc->sc_bus.usbrev = USBREV_1_0;
	sc->sc_flags |= EHCIF_ETTF;

	error = bus_space_map(aa->aa_bst, aa->aa_addr, aa->aa_size, 0,
	    &sc->ioh);

	if (error) {
		aprint_error(": failed to map registers: %d\n", error);
		return;
	}

	/* The recommended value is 0x20 for both ports and the host */
	REGVAL(AR9344_USB_CONFIG_BASE) = 0x20c00;	/* magic */
	DELAY(1000);

	/* get offset to operational regs */
	uint32_t r = bus_space_read_4(aa->aa_bst, sc->ioh, 0);
	if (r != 0x40) {
		aprint_error(": error: CAPLENGTH (%#x) != 0x40\n", sc->sc_offs);
		return;
	}

	sc->sc_offs = EREAD1(sc, EHCI_CAPLENGTH);

	aprint_normal("\n");

	/* Disable EHCI interrupts */
	EOWRITE4(sc, EHCI_USBINTR, 0);

	/* establish interrupt */
	ih = arbus_intr_establish(aa->aa_cirq, aa->aa_mirq, ehci_intr, sc);
	if (ih == NULL)
		panic("%s: couldn't establish interrupt",
		    device_xname(self));

	/*
	 * There are no companion controllers
	 */
	sc->sc_ncomp = 0;

	status = ehci_init(sc);
	if (status != USBD_NORMAL_COMPLETION) {
		aprint_error("%s: init failed, error=%d\n", device_xname(self),
		    status);
		if (ih != NULL)
			arbus_intr_disestablish(ih);
		return;
	}

	/* Attach USB device */
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
}

