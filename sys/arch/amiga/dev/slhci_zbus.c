/*	$NetBSD: slhci_zbus.c,v 1.1.4.3 2017/12/03 11:35:48 jdolecek Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
__KERNEL_RCSID(0, "$NetBSD: slhci_zbus.c,v 1.1.4.3 2017/12/03 11:35:48 jdolecek Exp $");

/*
 * Thylacine driver.
 * Inspired by slhci_intio.c.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>

#include <amiga/dev/zbusvar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>

#include <dev/ic/sl811hsreg.h>
#include <dev/ic/sl811hsvar.h>

#define THYLACINE_SLHCI_PWR_OFFSET	0x100
#define THYLACINE_SLHCI_ADDR_OFFSET	0x1
#define THYLACINE_SLHCI_DATA_STRIDE	0x4000
#define THYLACINE_SIZE			0x8000

static int	slhci_zbus_match(device_t, cfdata_t , void *);
static void	slhci_zbus_attach(device_t, device_t, void *);
static void	slhci_zbus_enable_power(void *, enum power_change);

struct slhci_zbus_softc {
	struct slhci_softc	sc_sc;

	struct bus_space_tag	sc_bst;
	struct isr		sc_isr;
};

CFATTACH_DECL_NEW(slhci_zbus, sizeof(struct slhci_zbus_softc),
    slhci_zbus_match, slhci_zbus_attach, NULL, NULL);

static int
slhci_zbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap = aux;

	/* Thylacine */
	if (zap->manid == 0x1392 && zap->prodid == 0x1) {
			return 1;
	}

	return 0;
}

static void
slhci_zbus_attach(device_t parent, device_t self, void *aux)
{
	struct slhci_zbus_softc *zsc;
	struct slhci_softc *sc;
	struct zbus_args *zap;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	zap = aux;
	zsc = device_private(self);
	sc = &zsc->sc_sc;
	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;

	zsc->sc_bst.base = (bus_addr_t)zap->va;
	zsc->sc_bst.absm = &amiga_bus_stride_1;
	iot = &zsc->sc_bst;

	aprint_normal(": Thylacine USB Host Controller\n");

	if (bus_space_map(iot, THYLACINE_SLHCI_ADDR_OFFSET, THYLACINE_SIZE, 0, 
	    &ioh)) {
		aprint_error_dev(sc->sc_dev, "can't map the bus\n");
	}

	slhci_preinit(sc, slhci_zbus_enable_power, iot, ioh, 500,
	   THYLACINE_SLHCI_DATA_STRIDE);

	/* Attach interrupt routine. */
	zsc->sc_isr.isr_intr = slhci_intr;
	zsc->sc_isr.isr_arg = sc;
	zsc->sc_isr.isr_ipl = 6;
	add_isr(&zsc->sc_isr);

	slhci_attach(sc);

}

static void
slhci_zbus_enable_power(void *arg, enum power_change mode)
{
	struct slhci_zbus_softc *zsc = arg;
   
	/*
	 * The hardware is dumb. Changing power mode depends only on A8 being 
	 * low or high.
	 */ 
	if (mode == POWER_ON)
		bus_space_read_1(zsc->sc_sc.sc_iot, zsc->sc_sc.sc_ioh,
		    THYLACINE_SLHCI_DATA_STRIDE);
	else
		bus_space_read_1(zsc->sc_sc.sc_iot, zsc->sc_sc.sc_ioh, 
		    THYLACINE_SLHCI_PWR_OFFSET);
}

