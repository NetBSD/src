/*	$NetBSD: slhci_intio.c,v 1.5 2003/07/15 01:44:52 lukem Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: slhci_intio.c,v 1.5 2003/07/15 01:44:52 lukem Exp $");

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

static int  slhci_intio_match(struct device *, struct cfdata *, void *);
static void slhci_intio_attach(struct device *, struct device *, void *);
static void slhci_intio_enable_power(void *, int);
static void slhci_intio_enable_intr(void *, int);
static int  slhci_intio_intr(void *);

CFATTACH_DECL(slhci_intio, sizeof(struct slhci_intio_softc),
    slhci_intio_match, slhci_intio_attach, NULL, NULL);

static int
slhci_intio_match(struct device *parent, struct cfdata *cf, void *aux)
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

	/* Whether the control port is accessible or not */
	nc_addr = ia->ia_addr + NEREID_ADDR_OFFSET;
	nc_size = 0x02;
	if (badbaddr((caddr_t)INTIO_ADDR(nc_addr)))
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
slhci_intio_attach(struct device *parent, struct device *self, void *aux)
{
	struct slhci_intio_softc *sc = (struct slhci_intio_softc *)self;
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_bst;
	bus_space_handle_t ioh;
	int nc_addr;
	int nc_size;

	printf(": Nereid USB\n");

	/* Map I/O space */
	if (bus_space_map(iot, ia->ia_addr, SL11_PORTSIZE * 2,
			BUS_SPACE_MAP_SHIFTED, &ioh)) {
		printf("%s: can't map I/O space\n",
			sc->sc_sc.sc_bus.bdev.dv_xname);
		return;
	}

	nc_addr = ia->ia_addr + NEREID_ADDR_OFFSET;
	nc_size = 0x02;
	if (bus_space_map(iot, nc_addr, nc_size,
			BUS_SPACE_MAP_SHIFTED, &sc->sc_nch)) {
		printf("%s: can't map I/O control space\n",
			sc->sc_sc.sc_bus.bdev.dv_xname);
		return;
	}

	/* Initialize sc */
	sc->sc_sc.sc_iot = iot;
	sc->sc_sc.sc_ioh = ioh;
	sc->sc_sc.sc_dmat = ia->ia_dmat;
	sc->sc_sc.sc_enable_power = slhci_intio_enable_power;
	sc->sc_sc.sc_enable_intr  = slhci_intio_enable_intr;
	sc->sc_sc.sc_arg = sc;

	/* Establish the interrupt handler */
	if (intio_intr_establish(ia->ia_intr, "slhci", slhci_intio_intr, sc)) {
		printf("%s: can't establish interrupt\n",
			sc->sc_sc.sc_bus.bdev.dv_xname);
		return;
	}

	/* Reset controller */
	bus_space_write_1(iot, sc->sc_nch, NEREID_CTRL, NEREID_CTRL_RESET);
	delay(40000);

	/* Attach SL811HS/T */
	if (slhci_attach(&sc->sc_sc, self))
		return;
}

static void
slhci_intio_enable_power(void *arg, int mode)
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

static int
slhci_intio_intr(void *arg)
{
	struct slhci_intio_softc *sc = arg;

	return slhci_intr(&sc->sc_sc);
}
