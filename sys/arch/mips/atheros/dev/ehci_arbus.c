/*	$NetBSD: ehci_arbus.c,v 1.2.2.1 2017/12/03 11:36:26 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ehci_arbus.c,v 1.2.2.1 2017/12/03 11:36:26 jdolecek Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <mips/locore.h>
#include <mips/atheros/include/ar9344reg.h>
#include <mips/atheros/include/arbusvar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

/*
 * This is relative to the start of the unreserved registers in USB contoller
 * block and not the full USB block which would be 0x1a8.
 */
#define	ARBUS_USBMODE		0xa8			/* USB mode */
#define	 USBMODE_CM		__BITS(0,1)		/* Controller Mode */
#define	 USBMODE_CM_IDLE	__SHIFTIN(0,USBMODE_CM)	/* Idle (both) */
#define	 USBMODE_CM_DEVICE	__SHIFTIN(2,USBMODE_CM)	/* Device Controller */
#define	 USBMODE_CM_HOST	__SHIFTIN(3,USBMODE_CM)	/* Host Controller */

static int	ehci_arbus_match(device_t, cfdata_t, void *);
static void	ehci_arbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ehci_arbus, sizeof (ehci_softc_t),
    ehci_arbus_match, ehci_arbus_attach, NULL, NULL);

static void ehci_arbus_init(struct ehci_softc *);

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

	sc->iot = aa->aa_bst_le;
	sc->sc_size = aa->aa_size;
	//sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_dmatag = aa->aa_dmat;
	sc->sc_bus.ub_revision = USBREV_1_0;
	sc->sc_flags |= EHCIF_ETTF;
	sc->sc_vendor_init = ehci_arbus_init;

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

	error = ehci_init(sc);
	if (error) {
		aprint_error("%s: init failed, error=%d\n", device_xname(self),
		    error);
		if (ih != NULL)
			arbus_intr_disestablish(ih);
		return;
	}

	/* Attach USB device */
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
}

static void
ehci_arbus_init(struct ehci_softc *sc)
{
	/* Set host mode */
	uint32_t old = bus_space_read_4(sc->iot, sc->ioh, ARBUS_USBMODE);
	uint32_t reg = old;

	reg &= ~USBMODE_CM;
	reg |= USBMODE_CM_HOST;
	if (reg != old)
		bus_space_write_4(sc->iot, sc->ioh, ARBUS_USBMODE, reg);

}
