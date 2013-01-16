/*	$NetBSD: gb225_slhci.c,v 1.6.2.2 2013/01/16 05:32:53 yamt Exp $ */

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tetsuya Isaki.
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

/*
 * SL811HS USB host controller for GB-225 Option board of G4250EBX.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/cpu.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>

#include <dev/ic/sl811hsreg.h>
#include <dev/ic/sl811hsvar.h>

#include <arch/arm/xscale/pxa2x0reg.h>
#include <arch/arm/xscale/pxa2x0var.h>
#include <arch/evbarm/g42xxeb/g42xxeb_reg.h>
#include <arch/evbarm/g42xxeb/g42xxeb_var.h>
#include <arch/evbarm/g42xxeb/gb225reg.h>
#include <arch/evbarm/g42xxeb/gb225var.h>

struct slhci_opio_softc {
	struct slhci_softc sc_sc;

	void *sc_ih;
};

static int  slhci_opio_match(device_t, cfdata_t, void *);
static void slhci_opio_attach(device_t, device_t, void *);
static void slhci_opio_enable_power(void *, int);
static void slhci_opio_enable_intr(void *, int);
static int  slhci_opio_intr(void *);

CFATTACH_DECL_NEW(slhci_opio, sizeof(struct slhci_opio_softc),
    slhci_opio_match, slhci_opio_attach, NULL, NULL);

#define PORTSIZE	(SL11_PORTSIZE*4)

static int
slhci_opio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oba = aux;
	bus_space_tag_t iot = &pxa2x0_a4x_bs_tag; /* Use special BS funcs */
	bus_space_handle_t ioh;
	struct obio_softc *bsc = device_private(device_parent(parent));
	struct pxa2x0_softc *psc;
	int type;
	uint32_t reg;

	struct slhci_softc sc;

	obio_peripheral_reset(bsc, 2, 0);
	psc = device_private(device_parent(bsc->sc_dev));

	reg = bus_space_read_4(psc->saip.sc_iot, psc->sc_memctl_ioh,
	    MEMCTL_MSC2);
#if 0
	bus_space_write_4(psc->saip.sc_iot, psc->sc_memctl_ioh, MEMCTL_MSC2,xxx)
#endif	    
	    

	oba->oba_iot = iot;
	if (oba->oba_addr == OBIOCF_ADDR_DEFAULT)
		oba->oba_addr = PXA2X0_CS5_START;
	if (oba->oba_intr == OBIOCF_INTR_DEFAULT)
		oba->oba_intr = G4250EBX_INT_EXT2;

	if (bus_space_map(iot, oba->oba_addr, PORTSIZE, 0, &ioh))
		return 0;

	/* construct fake softc to call sl811hs */
	sc.sc_iot = iot;
	sc.sc_ioh = ioh;
	type = sl811hs_find(&sc);

	bus_space_unmap(iot, ioh, PORTSIZE);

	return type >= 0;
}

static void
slhci_opio_attach(device_t parent, device_t self, void *aux)
{
	struct slhci_opio_softc *sc = device_private(self);
	struct obio_attach_args *oba = aux;
	struct opio_softc *psc =
	    device_private(device_parent(self));
	struct obio_softc *bsc =
	    device_private(device_parent(psc->sc_dev));
	bus_space_tag_t iot = oba->oba_iot;
	bus_space_handle_t ioh;

	printf("\n");

	/* Map I/O space */
	if (bus_space_map(iot, oba->oba_addr, PORTSIZE, 0, &ioh)) {
		aprint_error_dev(self, "can't map I/O space\n");
		return;
	}

	/* Initialize sc */
	sc->sc_sc.sc_iot = iot;
	sc->sc_sc.sc_ioh = ioh;
	sc->sc_sc.sc_dmat = &pxa2x0_bus_dma_tag;	/* XXX */
	sc->sc_sc.sc_enable_power = slhci_opio_enable_power;
	sc->sc_sc.sc_enable_intr  = slhci_opio_enable_intr;
	sc->sc_sc.sc_arg = sc;

	/* Establish the interrupt handler */
	sc->sc_ih = obio_intr_establish(bsc, oba->oba_intr, IPL_BIO, 
	    IST_LEVEL_HIGH, slhci_opio_intr, sc);
	if( sc->sc_ih == NULL) {
		aprint_error_dev(self, "can't establish interrupt\n");
		return;
	}

#if 0
	/* Reset controller */
	obio_peripheral_reset(bsc, 2, 1);
	delay(100);
	obio_peripheral_reset(bsc, 2, 0);
	delay(40000);
#endif

	bus_space_write_1(iot, ioh, SL11_IDX_ADDR, SL11_CTRL);
	bus_space_write_1(iot, ioh, SL11_IDX_DATA, 0x01);

	/* Attach SL811HS/T */
	if (slhci_attach(&sc->sc_sc, self))
		return;
}

static void
slhci_opio_enable_power(void *arg, int mode)
{
#if 0
	struct slhci_opio_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_sc.sc_iot;
	uint8_t r;

	r = bus_space_read_1(iot, sc->sc_nch, NEREID_CTRL);
	if (mode == POWER_ON)
		bus_space_write_1(iot, sc->sc_nch, NEREID_CTRL,
			r |  NEREID_CTRL_POWER);
	else
		bus_space_write_1(iot, sc->sc_nch, NEREID_CTRL,
			r & ~NEREID_CTRL_POWER);
#endif
}

static void
slhci_opio_enable_intr(void *arg, int mode)
{
	struct slhci_opio_softc *sc = arg;
	struct obio_softc *bsc;

	bsc = device_private(device_parent(
	    device_parent(sc->sc_sc.sc_bus.bdev)));

	if (mode == INTR_ON)
		obio_intr_unmask(bsc, sc->sc_ih);
	else
		obio_intr_mask(bsc, sc->sc_ih);
}

static int
slhci_opio_intr(void *arg)
{
	struct slhci_opio_softc *sc = arg;

	return slhci_intr(&sc->sc_sc);
}
