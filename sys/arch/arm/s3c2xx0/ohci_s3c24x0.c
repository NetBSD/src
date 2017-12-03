/*	$NetBSD: ohci_s3c24x0.c,v 1.8.12.1 2017/12/03 11:35:55 jdolecek Exp $ */

/* derived from ohci_pci.c */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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
__KERNEL_RCSID(0, "$NetBSD: ohci_s3c24x0.c,v 1.8.12.1 2017/12/03 11:35:55 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <sys/bus.h>

#include <arm/s3c2xx0/s3c24x0var.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

int	ohci_ssio_match(device_t, cfdata_t, void *);
void	ohci_ssio_attach(device_t, device_t, void *);
int	ohci_ssio_detach(device_t, int);

extern	int ohcidebug;

struct ohci_ssio_softc {
	ohci_softc_t		sc;

	void 			*sc_ih;		/* interrupt vectoring */
};

CFATTACH_DECL_NEW(ohci_ssio, sizeof(struct ohci_ssio_softc),
    ohci_ssio_match, ohci_ssio_attach, ohci_ssio_detach, ohci_activate);

int
ohci_ssio_match(device_t parent, cfdata_t match, void *aux)
{
	struct s3c2xx0_attach_args *sa = (struct s3c2xx0_attach_args *)aux;
	/* XXX: check some registers */

	if (sa->sa_dmat == NULL) {
		/* busdma tag is not initialized. */
		return 0;
	}

	return 1;
}

void
ohci_ssio_attach(device_t parent, device_t self, void *aux)
{
	struct ohci_ssio_softc *sc = device_private(self);
	struct s3c2xx0_attach_args *sa = (struct s3c2xx0_attach_args *)aux;

	aprint_normal("\n");
	aprint_naive("\n");

	sc->sc.sc_dev = self;
	sc->sc.sc_bus.ub_hcpriv = sc;

	sc->sc.iot = sa->sa_iot;
	/*ohcidebug=15;*/

	/* Map I/O registers */
	if (bus_space_map(sc->sc.iot, sa->sa_addr, 0x5c, 0, &sc->sc.ioh)) {
		aprint_error_dev(self, "can't map mem space\n");
		return;
	}

	/* Disable interrupts, so we don't get any spurious ones. */
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OHCI_INTERRUPT_DISABLE,
			  OHCI_ALL_INTRS);

	sc->sc.sc_bus.ub_dmatag = sa->sa_dmat;

	/* Enable the device. */
	/* XXX: provide clock to USB block */

	/* establish the interrupt. */
	sc->sc_ih = s3c24x0_intr_establish(sa->sa_intr, IPL_USB, IST_NONE, ohci_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		return;
	}

	strlcpy(sc->sc.sc_vendor, "Samsung", sizeof sc->sc.sc_vendor);

	int err = ohci_init(&sc->sc);
	if (err) {
		aprint_error_dev(self, "init failed, error=%d\n", err);
		return;
	}

	/* Attach usb device. */
	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);
}

int
ohci_ssio_detach(device_t self, int flags)
{
	return 0;
}
