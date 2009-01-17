/* $NetBSD: upc_iobus.c,v 1.8.74.1 2009/01/17 13:27:46 mjf Exp $ */
/*-
 * Copyright (c) 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * upc_iobus.c - attachment of the 82C7xx to the Archimedes I/O bus
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: upc_iobus.c,v 1.8.74.1 2009/01/17 13:27:46 mjf Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <arch/acorn26/iobus/iobusvar.h>

#include <arch/acorn26/iobus/iocreg.h>
#include <machine/irq.h>

#include <dev/ic/upcreg.h>
#include <dev/ic/upcvar.h>

#include "ioeb.h"

#if NIOEB > 0
#include <arch/acorn26/ioc/ioebvar.h>
#endif

static int upc_iobus_match(device_t , cfdata_t , void *);
static void upc_iobus_attach(device_t , device_t , void *);

struct upc_iobus_softc {
	struct upc_softc	sc_upc;
	struct evcnt		sc_intrcnt4;
	struct evcnt		sc_intrcntw;
	struct evcnt		sc_intrcntf;
	struct evcnt		sc_intrcntp;
};

CFATTACH_DECL(upc_iobus, sizeof(struct upc_iobus_softc),
    upc_iobus_match, upc_iobus_attach, NULL, NULL);

static device_t the_upc_iobus;

static int
upc_iobus_match(device_t parent, cfdata_t cf, void *aux)
{

	/*
	 * As is traditional, probing for iobus devices is impossible
	 * (The machine hangs if there's nothing there).  In this case,
	 * assume that if there's an IOEB, we've got a UPC too.
	 */
#if NIOEB > 0
	if (the_ioeb != NULL && the_upc_iobus == NULL)
		return 1;
#endif
	return 0;
}

static void
upc_iobus_attach(device_t parent, device_t self, void *aux)
{
	struct iobus_attach_args *ioa = aux;
	struct upc_iobus_softc *sc = device_private(self);
	struct upc_softc *upc = &sc->sc_upc;

	upc->sc_iot = ioa->ioa_tag;
	bus_space_map(ioa->ioa_tag, ioa->ioa_base, UPC_BUS_SIZE, 0,
		      &upc->sc_ioh);
	upc_attach(upc);

	if (upc->sc_irq4.uih_func != NULL) {
		evcnt_attach_dynamic(&sc->sc_intrcnt4, EVCNT_TYPE_INTR, NULL,
		    device_xname(self), "irq4");
		irq_establish(IOC_IRQ_IL2, upc->sc_irq4.uih_level,
		    upc->sc_irq4.uih_func, upc->sc_irq4.uih_arg,
		    &sc->sc_intrcnt4);
	}
	if (upc->sc_wintr.uih_func != NULL) {
		evcnt_attach_dynamic(&sc->sc_intrcntw, EVCNT_TYPE_INTR, NULL,
		    device_xname(self), "wdc intr");
		irq_establish(IOC_IRQ_IL3, upc->sc_wintr.uih_level,
		    upc->sc_wintr.uih_func, upc->sc_wintr.uih_arg,
		    &sc->sc_intrcntw);
	}
	if (upc->sc_fintr.uih_func != NULL) {
		evcnt_attach_dynamic(&sc->sc_intrcntf, EVCNT_TYPE_INTR, NULL,
		    device_xname(self), "fdc intr");
		irq_establish(IOC_IRQ_IL4, upc->sc_fintr.uih_level,
		    upc->sc_fintr.uih_func, upc->sc_fintr.uih_arg,
		    &sc->sc_intrcntf);
	}
	if (upc->sc_pintr.uih_func != NULL) {
		evcnt_attach_dynamic(&sc->sc_intrcntp, EVCNT_TYPE_INTR, NULL,
		    device_xname(self), "lpt intr");
		irq_establish(IOC_IRQ_IL6, upc->sc_pintr.uih_level,
		    upc->sc_pintr.uih_func, upc->sc_pintr.uih_arg,
		    &sc->sc_intrcntp);
	}
	/* IRQ3 on the 82C71x is not connected */
}
