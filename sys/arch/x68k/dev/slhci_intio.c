/*	$NetBSD: slhci_intio.c,v 1.14.18.1 2017/12/03 11:36:48 jdolecek Exp $	*/

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
 * USB part of Nereid Ethernet/USB/Memory board
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: slhci_intio.c,v 1.14.18.1 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>

#include <dev/ic/sl811hsreg.h>
#include <dev/ic/sl811hsvar.h>

#include <arch/x68k/dev/intiovar.h>

#include <arch/x68k/dev/slhci_intio_var.h>

static int  slhci_intio_match(device_t, cfdata_t, void *);
static void slhci_intio_attach(device_t, device_t, void *);
static void slhci_intio_enable_power(void *, enum power_change);
static void slhci_intio_enable_intr(void *, int);

CFATTACH_DECL_NEW(slhci_intio, sizeof(struct slhci_intio_softc),
    slhci_intio_match, slhci_intio_attach, NULL, NULL);

#define INTR_ON 	1
#define INTR_OFF 	0

static int
slhci_intio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_bst;
	bus_space_handle_t ioh;
	bus_space_handle_t nch;
	int nc_addr;
	int nc_size;

	if (ia->ia_addr == INTIOCF_ADDR_DEFAULT)
		ia->ia_addr = SLHCI_INTIO_ADDR1;
	if (ia->ia_intr == INTIOCF_INTR_DEFAULT)
		ia->ia_intr = SLHCI_INTIO_INTR1;

	/* fixed parameters */
	if ( !(ia->ia_addr == SLHCI_INTIO_ADDR1 &&
	       ia->ia_intr == SLHCI_INTIO_INTR1   ) &&
	     !(ia->ia_addr == SLHCI_INTIO_ADDR2 &&
	       ia->ia_intr == SLHCI_INTIO_INTR2   ) )
		return 0;

	/* Whether the SL811 port is accessible or not */
	if (badaddr((void *)IIOV(ia->ia_addr)))
		return 0;

	/* Whether the control port is accessible or not */
	nc_addr = ia->ia_addr + NEREID_ADDR_OFFSET;
	nc_size = 0x02;
	if (badbaddr((void *)IIOV(nc_addr)))
		return 0;

	/* Map two I/O spaces */
	ia->ia_size = SL11_PORTSIZE * 2;
	if (bus_space_map(iot, ia->ia_addr, ia->ia_size,
			BUS_SPACE_MAP_SHIFTED, &ioh))
		return 0;

	if (bus_space_map(iot, nc_addr, nc_size,
			BUS_SPACE_MAP_SHIFTED, &nch))
		return 0;

	bus_space_unmap(iot, ioh, ia->ia_size);
	bus_space_unmap(iot, nch, nc_size);

	return 1;
}

static void
slhci_intio_attach(device_t parent, device_t self, void *aux)
{
	struct slhci_intio_softc *isc = device_private(self);
	struct slhci_softc *sc = &isc->sc_sc;
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_bst;
	bus_space_handle_t ioh;
	int nc_addr;
	int nc_size;

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;

	printf(": Nereid USB\n");

	/* Map I/O space */
	if (bus_space_map(iot, ia->ia_addr, SL11_PORTSIZE * 2,
			BUS_SPACE_MAP_SHIFTED, &ioh)) {
		printf("%s: can't map I/O space\n",
			device_xname(self));
		return;
	}

	nc_addr = ia->ia_addr + NEREID_ADDR_OFFSET;
	nc_size = 0x02;
	if (bus_space_map(iot, nc_addr, nc_size,
			BUS_SPACE_MAP_SHIFTED, &isc->sc_nch)) {
		printf("%s: can't map I/O control space\n",
			device_xname(self));
		return;
	}

	/* Initialize sc */
	slhci_preinit(sc, slhci_intio_enable_power, iot, ioh, 30, 
	    SL11_IDX_DATA);

	/* Establish the interrupt handler */
	if (intio_intr_establish(ia->ia_intr, "slhci", slhci_intr, sc)) {
		printf("%s: can't establish interrupt\n",
			device_xname(self));
		return;
	}

	/* Reset controller */
	bus_space_write_1(iot, isc->sc_nch, NEREID_CTRL, NEREID_CTRL_RESET);
	delay(40000);

	slhci_intio_enable_intr(sc, INTR_ON);

	/* Attach SL811HS/T */
	if (slhci_attach(sc))
		return;
}

static void
slhci_intio_enable_power(void *arg, enum power_change mode)
{
	struct slhci_intio_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_sc.sc_iot;
	u_int8_t r;

	r = bus_space_read_1(iot, sc->sc_nch, NEREID_CTRL);
	if (mode == POWER_ON)
		bus_space_write_1(iot, sc->sc_nch, NEREID_CTRL,
			r |  NEREID_CTRL_POWER);
	else
		bus_space_write_1(iot, sc->sc_nch, NEREID_CTRL,
			r & ~NEREID_CTRL_POWER);
}

static void
slhci_intio_enable_intr(void *arg, int mode)
{
	struct slhci_intio_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_sc.sc_iot;
	u_int8_t r;

	r = bus_space_read_1(iot, sc->sc_nch, NEREID_CTRL);
	if (mode == INTR_ON)
		bus_space_write_1(iot, sc->sc_nch, NEREID_CTRL,
			r |  NEREID_CTRL_INTR);
	else
		bus_space_write_1(iot, sc->sc_nch, NEREID_CTRL,
			r & ~NEREID_CTRL_INTR);
}

