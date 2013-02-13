/*	$NetBSD: bcm2835_dotg.c,v 1.1.4.2 2013/02/13 01:36:14 riz Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_dotg.c,v 1.1.4.2 2013/02/13 01:36:14 riz Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm_amba.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/dwc_otgvar.h>

struct bcmdotg_softc {
	struct dwc_otg_softc	sc_dotg;

	void			*sc_ih;
};

static int bcmdotg_match(device_t, struct cfdata *, void *);
static void bcmdotg_attach(device_t, device_t, void *);
static void bcmdotg_deferred(device_t);

CFATTACH_DECL_NEW(dotg_amba, sizeof(struct bcmdotg_softc),
    bcmdotg_match, bcmdotg_attach, NULL, NULL);

/* ARGSUSED */
static int
bcmdotg_match(device_t parent, struct cfdata *match, void *aux)
{
	struct amba_attach_args *aaa = aux;

	if (strcmp(aaa->aaa_name, "dotg") != 0)
		return 0;

	return 1;
}

/* ARGSUSED */
static void
bcmdotg_attach(device_t parent, device_t self, void *aux)
{
	struct bcmdotg_softc *sc = device_private(self);
	struct amba_attach_args *aaa = aux;
	int error;

	sc->sc_dotg.sc_dev = self;

	sc->sc_dotg.sc_iot = aaa->aaa_iot;
	sc->sc_dotg.sc_bus.dmatag = aaa->aaa_dmat;

	error = bus_space_map(aaa->aaa_iot, aaa->aaa_addr, aaa->aaa_size, 0,
	    &sc->sc_dotg.sc_ioh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers for %s: %d\n", aaa->aaa_name, error);
		return;
	}

	aprint_naive(": USB controller\n");
	aprint_normal(": USB controller\n");

	sc->sc_ih = bcm2835_intr_establish(aaa->aaa_intr, IPL_USB,
	   dwc_otg_intr, &sc->sc_dotg);

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     aaa->aaa_intr);
		goto fail;
	}
	config_defer(self, bcmdotg_deferred);

	return;

fail:
	if (sc->sc_ih) {
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}
	bus_space_unmap(sc->sc_dotg.sc_iot, sc->sc_dotg.sc_ioh, aaa->aaa_size);
}

static void
bcmdotg_deferred(device_t self)
{
	struct bcmdotg_softc *sc = device_private(self);
	int error;

	error = dwc_otg_init(&sc->sc_dotg);
	if (error != 0) {
		aprint_error_dev(self, "couldn't initialize host, error=%d\n",
		    error);
		return;
	}
	sc->sc_dotg.sc_child = config_found(sc->sc_dotg.sc_dev, &sc->sc_dotg.sc_bus,
	    usbctlprint);
}

