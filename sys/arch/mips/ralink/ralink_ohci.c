/*	$NetBSD: ralink_ohci.c,v 1.2.12.2 2017/12/03 11:36:28 jdolecek Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* ralink_ohci.c -- Ralink OHCI USB Driver */

#include "ehci.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ralink_ohci.c,v 1.2.12.2 2017/12/03 11:36:28 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

#include <mips/ralink/ralink_var.h>
#include <mips/ralink/ralink_reg.h>
#include <mips/ralink/ralink_usbhcvar.h>

#define OREAD4(sc, a) bus_space_read_4((sc)->iot, (sc)->ioh, (a))
#define OWRITE4(sc, a, x) bus_space_write_4((sc)->iot, (sc)->ioh, (a), (x))

struct ralink_ohci_softc {
	struct ohci_softc sc_ohci;
#if NEHCI > 0
	struct ralink_usb_hc sc_hc;
#endif
	void  *sc_ih;
};

static int  ralink_ohci_match(device_t, cfdata_t, void *);
static void ralink_ohci_attach(device_t, device_t, void *);
static int  ralink_ohci_detach(device_t, int);

CFATTACH_DECL2_NEW(ralink_ohci, sizeof(struct ralink_ohci_softc),
	ralink_ohci_match, ralink_ohci_attach, ralink_ohci_detach,
	ohci_activate, NULL, ohci_childdet);

/*
 * ralink_ohci_match
 */
static int
ralink_ohci_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

/*
 * ralink_ohci_attach
 */
static void
ralink_ohci_attach(device_t parent, device_t self, void *aux)
{
	struct ralink_ohci_softc * const sc = device_private(self);
	const struct mainbus_attach_args * const ma = aux;
	int error;
#ifdef RALINK_OHCI_DEBUG
	const char * const devname = device_xname(self);
#endif

	aprint_naive(": OHCI USB controller\n");
	aprint_normal(": OHCI USB controller\n");

	sc->sc_ohci.sc_dev = self;
	sc->sc_ohci.sc_bus.ub_hcpriv = sc;
	sc->sc_ohci.iot = ma->ma_memt;
	sc->sc_ohci.sc_bus.ub_dmatag = ma->ma_dmat;

	/* Map I/O registers */
	if ((error = bus_space_map(sc->sc_ohci.iot, RA_USB_OHCI_BASE,
	    RA_USB_BLOCK_SIZE, 0, &sc->sc_ohci.ioh)) != 0) {
		aprint_error_dev(self, "can't map OHCI registers, "
			"error=%d\n", error);
		return;
	}

	sc->sc_ohci.sc_size = RA_USB_BLOCK_SIZE;

#ifdef RALINK_OHCI_DEBUG
	printf("%s sc: %p ma: %p\n", devname, sc, ma);
	printf("%s memt: %p dmat: %p\n", devname, ma->ma_memt, ma->ma_dmat);

	printf("%s: OHCI HcRevision=0x%x\n", devname,
		OREAD4(&sc->sc_ohci, OHCI_REVISION));
	printf("%s: OHCI HcControl=0x%x\n", devname,
		OREAD4(&sc->sc_ohci, OHCI_CONTROL));
	printf("%s: OHCI HcCommandStatus=0x%x\n", devname,
		OREAD4(&sc->sc_ohci, OHCI_COMMAND_STATUS));
	printf("%s: OHCI HcInterruptStatus=0x%x\n", devname,
		OREAD4(&sc->sc_ohci, OHCI_INTERRUPT_STATUS));
#endif

	/* Disable OHCI interrupts. */
	OWRITE4(&sc->sc_ohci, OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTRS);

	/* establish the MIPS level interrupt */
	sc->sc_ih = ra_intr_establish(RA_IRQ_USB, ohci_intr, sc, 0);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to establish irq %d\n",
			RA_IRQ_USB);
		goto fail_0;
	}

	/* Set vendor for root hub descriptor. */
	sc->sc_ohci.sc_id_vendor = 0x1814;
	strlcpy(sc->sc_ohci.sc_vendor, "Ralink", sizeof(sc->sc_ohci.sc_vendor));

	/* Initialize OHCI */
	error = ohci_init(&sc->sc_ohci);
	if (error) {
		aprint_error_dev(self, "init failed, error=%d\n", error);
		goto fail_0;
	}

#if NEHCI > 0
	ralink_usb_hc_add(&sc->sc_hc, self);
#endif

	if (!pmf_device_register1(self, ohci_suspend, ohci_resume,
	    ohci_shutdown))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/* Attach usb device. */
	sc->sc_ohci.sc_child = config_found(self, &sc->sc_ohci.sc_bus,
		usbctlprint);

	return;

 fail_0:
	bus_space_unmap(sc->sc_ohci.iot, sc->sc_ohci.ioh, sc->sc_ohci.sc_size);
	sc->sc_ohci.sc_size = 0;
}

static int
ralink_ohci_detach(device_t self, int flags)
{
	struct ralink_ohci_softc *sc = device_private(self);
	int rv;

	pmf_device_deregister(self);

	rv = ohci_detach(&sc->sc_ohci, flags);
	if (rv != 0)
		return rv;

	if (sc->sc_ih != NULL) {
		ra_intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}

	if (sc->sc_ohci.sc_size == 0) {
		bus_space_unmap(sc->sc_ohci.iot, sc->sc_ohci.ioh,
			sc->sc_ohci.sc_size);
		sc->sc_ohci.sc_size = 0;
	}

#if NEHCI > 0
	ralink_usb_hc_rem(&sc->sc_hc);
#endif

	return 0;
}
