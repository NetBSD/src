/* $NetBSD: upc_iobus.c,v 1.2.2.5 2001/03/27 15:30:24 bouyer Exp $ */
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * upc_iobus.c - attachment of the 82C7xx to the Archimedes I/O bus
 */

#include <sys/param.h>

__RCSID("$NetBSD: upc_iobus.c,v 1.2.2.5 2001/03/27 15:30:24 bouyer Exp $");

#include <sys/device.h>

#include <arch/arm26/iobus/iobusvar.h>

#include <arch/arm26/iobus/iocreg.h>
#include <machine/irq.h>

#include <dev/ic/upcreg.h>
#include <dev/ic/upcvar.h>

#include "ioeb.h"

#if NIOEB > 0
#include <arch/arm26/ioc/ioebvar.h>
#endif

static int upc_iobus_match(struct device *, struct cfdata *, void *);
static void upc_iobus_attach(struct device *, struct device *, void *);

struct upc_iobus_softc {
	struct upc_softc	sc_upc;
	struct evcnt		sc_intrcnt4;
	struct evcnt		sc_intrcntw;
	struct evcnt		sc_intrcntf;
	struct evcnt		sc_intrcntp;
};

struct cfattach upc_iobus_ca = {
	sizeof(struct upc_iobus_softc), upc_iobus_match, upc_iobus_attach
};

static struct device *the_upc_iobus;

static int
upc_iobus_match(struct device *parent, struct cfdata *cf, void *aux)
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
upc_iobus_attach(struct device *parent, struct device *self, void *aux)
{
	struct iobus_attach_args *ioa = aux;
	struct upc_iobus_softc *sc = (struct upc_iobus_softc *)self;
	struct upc_softc *upc = &sc->sc_upc;

	upc->sc_iot = ioa->ioa_tag;
	bus_space_map(ioa->ioa_tag, ioa->ioa_base, UPC_BUS_SIZE, 0,
		      &upc->sc_ioh);
	upc_attach(upc);

	if (upc->sc_irq4.uih_func != NULL) {
		evcnt_attach_dynamic(&sc->sc_intrcnt4, EVCNT_TYPE_INTR, NULL,
		    self->dv_xname, "irq4");
		irq_establish(IOC_IRQ_IL2, upc->sc_irq4.uih_level,
		    upc->sc_irq4.uih_func, upc->sc_irq4.uih_arg,
		    &sc->sc_intrcnt4);
	}
	if (upc->sc_wintr.uih_func != NULL) {
		evcnt_attach_dynamic(&sc->sc_intrcntw, EVCNT_TYPE_INTR, NULL,
		    self->dv_xname, "wdc intr");
		irq_establish(IOC_IRQ_IL3, upc->sc_wintr.uih_level,
		    upc->sc_wintr.uih_func, upc->sc_wintr.uih_arg,
		    &sc->sc_intrcntw);
	}
	if (upc->sc_fintr.uih_func != NULL) {
		evcnt_attach_dynamic(&sc->sc_intrcntf, EVCNT_TYPE_INTR, NULL,
		    self->dv_xname, "fdc intr");
		irq_establish(IOC_IRQ_IL4, upc->sc_fintr.uih_level,
		    upc->sc_fintr.uih_func, upc->sc_fintr.uih_arg,
		    &sc->sc_intrcntf);
	}
	if (upc->sc_pintr.uih_func != NULL) {
		evcnt_attach_dynamic(&sc->sc_intrcntp, EVCNT_TYPE_INTR, NULL,
		    self->dv_xname, "lpt intr");
		irq_establish(IOC_IRQ_IL6, upc->sc_pintr.uih_level,
		    upc->sc_pintr.uih_func, upc->sc_pintr.uih_arg,
		    &sc->sc_intrcntp);
	}
	/* IRQ3 on the 82C71x is not connected */
}
