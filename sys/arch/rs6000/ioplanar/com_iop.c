/*	$NetBSD: com_iop.c,v 1.1 2007/12/17 19:09:38 garbled Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_iop.c,v 1.1 2007/12/17 19:09:38 garbled Exp $");

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

int com_iop_probe(struct device *, struct cfdata *, void *);
void com_iop_attach(struct device *, struct device *, void *);

CFATTACH_DECL(com_iop, sizeof(struct com_iop_softc),
    com_iop_probe, com_iop_attach, NULL, NULL);

#define COM_RAINBOW_FREQ	8000000

/*
 * This probe is very unorthodox.  Basically, we have no way to actually
 * probe the device, so instead, we check that the specific device should
 * exist on the ioplanar, and just return true.
 */

int
com_iop_probe(struct device *parent, struct cfdata *match, void *aux)
{
	struct ioplanar_dev_attach_args *idaa = aux;

	if (idaa->idaa_devid == MCA_PRODUCT_IBM_SIO_RAINBOW &&
	    (idaa->idaa_device == IOP_COM0 || idaa->idaa_device == IOP_COM1))
		return 1;

	return 0;
}

void
com_iop_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_iop_softc *isc = device_private(self);
	struct com_softc *sc = &isc->sc_com;
	int iobase, irq;
	struct ioplanar_dev_attach_args *idaa = aux;
	bus_space_handle_t ioh;

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
		aprint_error("%s: couldn't establish interrupt handler\n",
		    sc->sc_dev.dv_xname);
		return;
	}
}
