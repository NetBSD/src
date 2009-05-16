/* $NetBSD: atppc_pioc.c,v 1.4.4.1 2009/05/16 10:41:11 yamt Exp $ */

/*-
 * Copyright (c) 2001 Alcove - Nicolas Souchu
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * FreeBSD: src/sys/isa/ppc.c,v 1.26.2.5 2001/10/02 05:21:45 nsouch Exp 
 *
 */

#include "opt_atppc.h"

#include <sys/param.h>
__KERNEL_RCSID(0, "$NetBSD: atppc_pioc.c,v 1.4.4.1 2009/05/16 10:41:11 yamt Exp $");

#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arch/acorn32/mainbus/piocvar.h>

#include <dev/ic/atppcreg.h>
#include <dev/ic/atppcvar.h>

#include "locators.h"

/* Probe and attach functions for a atppc device on the PIOC. */
static int atppc_pioc_probe(device_t, cfdata_t, void *);
static void atppc_pioc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(atppc_pioc, sizeof(struct atppc_softc), atppc_pioc_probe,
	atppc_pioc_attach, NULL, NULL);

#define IO_LPTSIZE 8

/* 
 * Probe function: find parallel port controller on isa bus. Combined from 
 * lpt_isa_probe() in lpt.c and atppc_detect_port() from FreeBSD's ppc.c. 
 */
static int
atppc_pioc_probe(device_t parent, cfdata_t cf, void *aux)
{
	bus_space_handle_t ioh;
	struct pioc_attach_args *pa = aux;
	bus_space_tag_t iot = pa->pa_iot;
	int addr = pa->pa_iobase + pa->pa_offset;
	int rval = 0;

	if (pa->pa_name && strcmp(pa->pa_name, "lpt") != 0)
		return 0;

	if (pa->pa_offset == PIOCCF_OFFSET_DEFAULT) {
		aprint_error_dev(parent, "(%s): io port unknown.\n", __func__);
	} else if (bus_space_map(iot, addr, IO_LPTSIZE, 0, &ioh) == 0) {
		if (atppc_detect_port(iot, ioh) == 0) 
			rval = 1;
		else 
			aprint_error_dev(parent,
			    "(%s): unable to write/read I/O port.\n",
			    __func__);
		bus_space_unmap(iot, ioh, IO_LPTSIZE);
	} else {
		aprint_error_dev(parent, "(%s): attempt to map bus space failed.\n",
		    __func__);
	}

	return rval;
}

/* Attach function: attach and configure parallel port controller on isa bus. */
static void 
atppc_pioc_attach(device_t parent, device_t self, void *aux)
{
	struct atppc_softc *sc = device_private(self); 
	struct pioc_attach_args *pa = aux;
	bus_addr_t iobase;

	sc->sc_dev = self;
	sc->sc_iot = pa->pa_iot;
	sc->sc_has = 0;
	iobase = pa->pa_iobase + pa->pa_offset;

	if (bus_space_map(sc->sc_iot, iobase, IO_LPTSIZE, 0, 
		&sc->sc_ioh) != 0) {
		aprint_error(": attempt to map bus space failed, device not "
			"properly attached.\n");
		sc->sc_dev_ok = ATPPC_NOATTACH;
		return;
	}
	sc->sc_dev_ok = ATPPC_ATTACHED;

	/* Assign interrupt handler */
	if (!(device_cfdata(self)->cf_flags & ATPPC_FLAG_DISABLE_INTR)) {
		if (pa->pa_irq != MAINBUSCF_IRQ_DEFAULT) {
			/* Establish interrupt handler. */
			sc->sc_ieh = intr_claim(pa->pa_irq, IPL_TTY, "atppc",
			    atppcintr, sc);
			sc->sc_has |= ATPPC_HAS_INTR;
		} else
			ATPPC_DPRINTF(("%s: IRQ not assigned or bad number of "
				"IRQs.\n", device_xname(self)));
	} else
		ATPPC_VPRINTF(("%s: interrupts not configured due to flags.\n", 
			device_xname(self)));

	/* Run soft configuration attach */
	atppc_sc_attach(sc);
}
