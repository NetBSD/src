/*	$NetBSD: ralink_ehci.c,v 1.3.12.1 2014/08/10 06:54:02 tls Exp $	*/
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

/* ralink_ehci.c -- Ralink EHCI USB Driver */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ralink_ehci.c,v 1.3.12.1 2014/08/10 06:54:02 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <mips/ralink/ralink_usbhcvar.h>

#include <mips/ralink/ralink_var.h>
#include <mips/ralink/ralink_reg.h>

struct ralink_ehci_softc {
	struct ehci_softc	sc_ehci;
	void			*sc_ih;
};

static int  ralink_ehci_match(device_t, cfdata_t, void *);
static void ralink_ehci_attach(device_t, device_t, void *);
static int  ralink_ehci_detach(device_t, int);

CFATTACH_DECL2_NEW(ralink_ehci, sizeof(struct ralink_ehci_softc),
	ralink_ehci_match, ralink_ehci_attach, ralink_ehci_detach,
	ehci_activate, NULL, ehci_childdet);

static TAILQ_HEAD(, ralink_usb_hc) ralink_usb_alldevs =
	TAILQ_HEAD_INITIALIZER(ralink_usb_alldevs);

#if 0
struct usb_hc_alldevs ralink_usb_alldevs = TAILQ_HEAD_INITIALIZER(ralink_usb_alldevs);
#endif

/*
 * ralink_ehci_match
 */
static int
ralink_ehci_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

/*
 * ralink_ehci_attach
 */
static void
ralink_ehci_attach(device_t parent, device_t self, void *aux)
{
	struct ralink_ehci_softc * const sc = device_private(self);
	const struct mainbus_attach_args *ma = aux;
	struct ralink_usb_hc *ruh;
	usbd_status status;
	int error;

	aprint_naive(": EHCI USB controller\n");
	aprint_normal(": EHCI USB controller\n");

	sc->sc_ehci.sc_dev = self;
	sc->sc_ehci.sc_bus.hci_private = sc;
	sc->sc_ehci.iot = ma->ma_memt;
	sc->sc_ehci.sc_bus.dmatag = ma->ma_dmat;

	/* Map EHCI registers */
	if ((error = bus_space_map(sc->sc_ehci.iot, RA_USB_EHCI_BASE,
	    RA_USB_BLOCK_SIZE, 0, &sc->sc_ehci.ioh)) != 0) {
		aprint_error_dev(self, "can't map EHCI registers, "
			"error=%d\n", error);
		return;
	}

	sc->sc_ehci.sc_size = RA_USB_BLOCK_SIZE;
	sc->sc_ehci.sc_bus.usbrev = USBREV_2_0;

#if defined(RALINK_EHCI_DEBUG)
	aprint_normal_dev(self, "sc %p ma %p\n", sc, ma);
	aprint_normal_dev(self, "memt %p dmat %p\n", ma->ma_memt, ma->ma_dmat);
	aprint_normal_dev(self, "EHCI HCCAPBASE=%#x\n",
		EREAD4(&sc->sc_ehci, EHCI_CAPLENGTH));
	aprint_normal_dev(self, "EHCI HCSPARAMS=%#x\n",
		EREAD4(&sc->sc_ehci, EHCI_HCSPARAMS));
	aprint_normal_dev(self, "EHCI HCCPARAMS=%#x\n",
		EREAD4(&sc->sc_ehci, EHCI_HCCPARAMS));
	aprint_normal_dev(self, "EHCI HCSP_PORTROUTE=%#x\n",
		EREAD4(&sc->sc_ehci, EHCI_HCSP_PORTROUTE));
#endif

	/* Disable EHCI interrupts. */
	sc->sc_ehci.sc_offs = EREAD1(&sc->sc_ehci, EHCI_CAPLENGTH);
	EOWRITE4(&sc->sc_ehci, EHCI_USBINTR, 0);

#if defined(RALINK_EHCI_DEBUG)
	aprint_normal_dev(self, "EHCI USBCMD=%#x USBSTS=%#x USBINTR=%#x\n",
		EOREAD4(&sc->sc_ehci, EHCI_USBCMD),
		EOREAD4(&sc->sc_ehci, EHCI_USBSTS),
		EOREAD4(&sc->sc_ehci, EHCI_USBINTR));
#endif

	/* Establish the MIPS level interrupt */
	sc->sc_ih = ra_intr_establish(RA_IRQ_USB, ehci_intr, sc, 1);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to establish irq %d\n",
			RA_IRQ_USB);
		goto fail_0;
	}

	/*
	 * Find companion controllers.  According to the spec they always
	 * have lower function numbers so they should be enumerated already.
	 */
	int ncomp = 0;
	TAILQ_FOREACH(ruh, &ralink_usb_alldevs, next) {
		aprint_normal_dev(self, "companion %s\n",
			device_xname(ruh->usb));
		sc->sc_ehci.sc_comps[ncomp++] = ruh->usb;
		if (ncomp >= EHCI_COMPANION_MAX)
			break;
	}
	sc->sc_ehci.sc_ncomp = ncomp;

	/* set vendor for root hub descriptor. */
	sc->sc_ehci.sc_id_vendor = 0x1814;	/* XXX */
	strlcpy(sc->sc_ehci.sc_vendor, "Ralink", sizeof(sc->sc_ehci.sc_vendor));

	/* Initialize EHCI */
	status = ehci_init(&sc->sc_ehci);
	if (status != USBD_NORMAL_COMPLETION) {
		aprint_error_dev(self, "init failed, error=%d\n", status);
		goto fail_1;
	}

	if (!pmf_device_register1(self, ehci_suspend, ehci_resume,
	    ehci_shutdown))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/* Attach usb device. */
	sc->sc_ehci.sc_child = config_found(self, &sc->sc_ehci.sc_bus,
		usbctlprint);

	return;

 fail_1:
	ra_intr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;
 fail_0:
	bus_space_unmap(sc->sc_ehci.iot, sc->sc_ehci.ioh, sc->sc_ehci.sc_size);
	sc->sc_ehci.sc_size = 0;
}

static int
ralink_ehci_detach(device_t self, int flags)
{
	struct ralink_ehci_softc * const sc = device_private(self);
	int rv;

	pmf_device_deregister(self);

	rv = ehci_detach(&sc->sc_ehci, flags);
	if (rv)
		return rv;

	if (sc->sc_ih != NULL) {
		ra_intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}

	if (sc->sc_ehci.sc_size != 0) {
		bus_space_unmap(sc->sc_ehci.iot, sc->sc_ehci.ioh,
			sc->sc_ehci.sc_size);
		sc->sc_ehci.sc_size = 0;
	}

	return 0;
}

void
ralink_usb_hc_add(struct ralink_usb_hc *ruh, device_t usbp)
{
	TAILQ_INSERT_TAIL(&ralink_usb_alldevs, ruh, next);
	ruh->usb = usbp;
}

void
ralink_usb_hc_rem(struct ralink_usb_hc *ruh)
{
	TAILQ_REMOVE(&ralink_usb_alldevs, ruh, next);
}
