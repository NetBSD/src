/* $NetBSD: slhci_tcu.c,v 1.1.18.2 2017/12/03 11:37:33 jdolecek Exp $ */

/*-
 * Copyright (c) 2016, Felix Deichmann
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
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

/*
 * flxd TC-USB - TURBOchannel USB host option
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: slhci_tcu.c,v 1.1.18.2 2017/12/03 11:37:33 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>

#include <dev/ic/sl811hsreg.h>
#include <dev/ic/sl811hsvar.h>

#include <dev/tc/tcvar.h>

struct slhci_tcu_softc {
	struct slhci_softc sc;
};

static int  slhci_tcu_match(device_t, cfdata_t, void *);
static void slhci_tcu_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(slhci_tcu, sizeof(struct slhci_tcu_softc),
    slhci_tcu_match, slhci_tcu_attach, NULL, slhci_activate);

static int
slhci_tcu_match(device_t parent, cfdata_t cf, void *aux)
{

	/* Always match. */
	return 1;
}

#define SLHCI_TCU_STRIDE	4
#define SLHCI_TCU_IMAX		500 /* mA */

static void
slhci_tcu_attach(device_t parent, device_t self, void *aux)
{
	struct slhci_tcu_softc *tsc = device_private(self);
	struct slhci_softc *sc = &tsc->sc;
	struct tc_attach_args *ta = aux;
	bus_space_tag_t iot = ta->ta_memt;
	bus_space_handle_t ioh;
	int error;

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;

	aprint_normal("\n");

	error = bus_space_map(iot, ta->ta_addr,
	    SLHCI_TCU_STRIDE * SL11_PORTSIZE, 0, &ioh);
	if (error) {
		aprint_error_dev(self, "bus_space_map() failed (%d)\n", error);
		return;
	}

	slhci_preinit(sc, NULL, iot, ioh, SLHCI_TCU_IMAX, SLHCI_TCU_STRIDE);

	tc_intr_establish(device_parent(parent), ta->ta_cookie, IPL_USB,
	    slhci_intr, sc);

	if (slhci_attach(sc))
		aprint_error_dev(self, "slhci_attach() failed\n");
}
