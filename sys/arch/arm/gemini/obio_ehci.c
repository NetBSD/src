/*	$NetBSD: obio_ehci.c,v 1.5 2018/04/09 16:21:09 jakllsch Exp $	*/

/*
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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
__KERNEL_RCSID(0, "$NetBSD: obio_ehci.c,v 1.5 2018/04/09 16:21:09 jakllsch Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <sys/bus.h>

#include <arm/gemini/gemini_reg.h>
#include <arm/gemini/gemini_obiovar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#ifdef EHCI_DEBUG
#define DPRINTF(x)	if (ehcidebug) printf x
extern int ehcidebug;
#else
#define DPRINTF(x)
#endif

#define	EHCI_HCOTGDEV_INTR_STATUS		0xc0
#define	EHCI_HCOTGDEV_INTR_MASK			0xc4
#define	HC_INT					0x04
#define	OTG_INT					0x02
#define	DEV_INT					0x01

static int
ehci_obio_intr(void *arg)
{
	struct ehci_softc * const sc = arg;
	int rv = 0;
	uint32_t v;

	v = bus_space_read_4(sc->iot, sc->ioh, EHCI_HCOTGDEV_INTR_STATUS);
	bus_space_write_4(sc->iot, sc->ioh, EHCI_HCOTGDEV_INTR_STATUS, v);
	if (v & HC_INT)
		rv = ehci_intr(sc);
	return rv;
}

static int
ehci_obio_match(device_t parent, cfdata_t match, void *aux)
{
	struct obio_attach_args * const obio = aux;

	if (obio->obio_addr == GEMINI_USB0_BASE
	    || obio->obio_addr == GEMINI_USB1_BASE)
		return 1;

	return 0;
}

static void
ehci_obio_attach(device_t parent, device_t self, void *aux)
{
	struct ehci_softc * const sc = device_private(self);
	struct obio_attach_args * const obio = aux;
	const char * const devname = device_xname(self);

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->iot = obio->obio_iot;

	aprint_naive(": EHCI USB controller\n");
	aprint_normal(": EHCI USB controller\n");

	/* Map I/O registers */
	if (bus_space_map(sc->iot, obio->obio_addr, obio->obio_size, 0,
			   &sc->ioh)) {
		aprint_error("%s: can't map memory space\n", devname);
		return;
	}

	sc->sc_bus.ub_dmatag = obio->obio_dmat;

	/* Disable interrupts, so we don't get any spurious ones. */
	sc->sc_offs = EREAD1(sc, EHCI_CAPLENGTH);
	DPRINTF(("%s: offs=%d\n", devname, sc->sc_offs));
	EOWRITE4(sc, EHCI_USBINTR, 0);
	bus_space_write_4(sc->iot, sc->ioh, EHCI_HCOTGDEV_INTR_MASK,
	    OTG_INT|DEV_INT);

	if (obio->obio_intr != OBIOCF_INTR_DEFAULT) {
		intr_establish(obio->obio_intr, IPL_USB, IST_LEVEL,
		    ehci_obio_intr, sc);
	}

	sc->sc_bus.ub_revision = USBREV_2_0;

	int err = ehci_init(sc);
	if (err) {
		aprint_error("%s: init failed, error=%d\n", devname, err);
		return;
	}

	/* Attach usb device. */
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
}

CFATTACH_DECL2_NEW(ehci_obio, sizeof(struct ehci_softc),
    ehci_obio_match, ehci_obio_attach, NULL, NULL, NULL, ehci_childdet);
