/*	$NetBSD: com_iop.c,v 1.2.4.1 2008/05/16 02:23:04 yamt Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
__KERNEL_RCSID(0, "$NetBSD: com_iop.c,v 1.2.4.1 2008/05/16 02:23:04 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/tty.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/mca/mcavar.h>
#include <dev/mca/mcadevs.h>

#include <rs6000/ioplanar/ioplanarvar.h>

struct com_iop_softc {
	struct	com_softc sc_com;	/* real com softc */
	void *sc_ih;
};

int com_iop_probe(device_t, cfdata_t , void *);
void com_iop_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_iop, sizeof(struct com_iop_softc),
    com_iop_probe, com_iop_attach, NULL, NULL);

#define COM_RAINBOW_FREQ	8000000

/*
 * This probe is very unorthodox.  Basically, we have no way to actually
 * probe the device, so instead, we check that the specific device should
 * exist on the ioplanar, and just return true.
 */

int
com_iop_probe(device_t parent, cfdata_t match, void *aux)
{
	struct ioplanar_dev_attach_args *idaa = aux;

	if (idaa->idaa_devid == MCA_PRODUCT_IBM_SIO_RAINBOW &&
	    (idaa->idaa_device == IOP_COM0 || idaa->idaa_device == IOP_COM1))
		return 1;

	return 0;
}

void
com_iop_attach(device_t parent, device_t self, void *aux)
{
	struct com_iop_softc *isc = device_private(self);
	struct com_softc *sc = &isc->sc_com;
	int iobase, irq;
	struct ioplanar_dev_attach_args *idaa = aux;
	bus_space_handle_t ioh;

	sc->sc_dev = self;

	switch (idaa->idaa_devid) {
	case MCA_PRODUCT_IBM_SIO_RAINBOW:
		switch (idaa->idaa_device) {
		case IOP_COM0:
			iobase = 0x30;
			irq = 2; /* ??? */
			sc->sc_frequency = COM_RAINBOW_FREQ;
			break;
		case IOP_COM1:
			iobase = 0x38;
			irq = 2; /* ??? */
			sc->sc_frequency = COM_RAINBOW_FREQ;
			break;
		default:
			return;
		}
		break;
	default:
		return;
	}
	if (!com_is_console(idaa->idaa_iot, iobase, &ioh) &&
	    bus_space_map(idaa->idaa_iot, iobase, COM_NPORTS, 0, &ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	COM_INIT_REGS(sc->sc_regs, idaa->idaa_iot, ioh, iobase);

	aprint_normal("i/o %#x-%#x irq %d", iobase, iobase + COM_NPORTS - 1,
	    irq);

	com_attach_subr(sc);

	isc->sc_ih = mca_intr_establish(idaa->idaa_mc, irq, IPL_SERIAL,
	    comintr, sc);

	if (isc->sc_ih == NULL) {
		aprint_error_dev(self,
		    "couldn't establish interrupt handler\n");
		return;
	}
}
