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
 * $FreeBSD: src/sys/isa/ppc.c,v 1.26.2.5 2001/10/02 05:21:45 nsouch Exp $
 *
 */

#include "opt_atppc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>
#include <dev/isa/atppc_isa_subr.h>

#include <dev/ic/atppcreg.h>
#include <dev/ic/atppcvar.h>

/*
 * ISA bus attach code for atppc driver.
 * Note on capabilities: capabilites may exist in the chipset but may not 
 * necessarily be useable. I.e. you may specify an IRQ in the autoconfig, but 
 * will the port actually have an IRQ assigned to it at the hardware level?
 * How can you test if the capabilites can be used? For interrupts, see if a 
 * handler exists (sc_intr != NULL). For DMA, see if the sc_dma_start() and 
 * sc_dma_finish() function pointers are not NULL.
 */

/* Probe, attach, and detach functions for a atppc device on the ISA bus. */
static int atppc_isa_probe __P((struct device *, struct cfdata *, void *));
static void atppc_isa_attach __P((struct device *, struct device *, void *));
static int atppc_isa_detach __P((struct device *, int));

CFATTACH_DECL(atppc_isa, sizeof(struct atppc_isa_softc), atppc_isa_probe,
	atppc_isa_attach, atppc_isa_detach, NULL);

/* 
 * Probe function: find parallel port controller on isa bus. Combined from 
 * lpt_isa_probe() in lpt.c and atppc_detect_port() from FreeBSD's ppc.c. 
 */
static int
atppc_isa_probe(struct device * parent, struct cfdata * cf, void * aux)
{
	bus_space_handle_t ioh;
	struct isa_attach_args * ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	int addr = ia->ia_io->ir_addr;
	int rval = 0;

	if(ia->ia_nio < 1 || addr == ISACF_PORT_DEFAULT) {
		printf("%s(%s): io port unknown.\n", __func__, 
			parent->dv_xname);
	}
	else if(bus_space_map(iot, addr, IO_LPTSIZE, 0, &ioh) == 0) {
		if (atppc_detect_port(iot, ioh) == 0) 
			rval = 1;
		else 
			printf("%s(%s): unable to write/read I/O port.\n", 
				__func__, parent->dv_xname);
		bus_space_unmap(iot, ioh, IO_LPTSIZE);
	}
	else {
		printf("%s(%s): attempt to map bus space failed.\n", __func__, 
			parent->dv_xname);
	}

	if(rval) {
		ia->ia_nio = 1;
		ia->ia_io[0].ir_size = IO_LPTSIZE;
		ia->ia_nirq = 1;
		ia->ia_ndrq = 1;
		ia->ia_niomem = 0;
	}

	return rval;
}

/* Attach function: attach and configure parallel port controller on isa bus. */
static void 
atppc_isa_attach(struct device * parent, struct device * self, void * aux)
{
	struct atppc_isa_softc * sc = (struct atppc_isa_softc *)self;
	struct atppc_softc * lsc = (struct atppc_softc *)self; 
	struct isa_attach_args * ia = aux;

	lsc->sc_iot = ia->ia_iot;
	lsc->sc_dmat = ia->ia_dmat; 
	lsc->sc_has = 0;
	sc->sc_ic = ia->ia_ic;
	sc->sc_iobase = ia->ia_io->ir_addr;

	if(bus_space_map(lsc->sc_iot, sc->sc_iobase, IO_LPTSIZE, 0, 
		&lsc->sc_ioh) != 0) {
		printf("%s: attempt to map bus space failed, device not "
			"properly attached.\n", self->dv_xname);
		lsc->sc_dev_ok = ATPPC_NOATTACH;
		return;
	}
	else {
		lsc->sc_dev_ok = ATPPC_ATTACHED;
	}

	/* Assign interrupt handler */
	if(!(self->dv_cfdata->cf_flags & ATPPC_FLAG_DISABLE_INTR)) {
		if((ia->ia_irq->ir_irq != ISACF_IRQ_DEFAULT) && 
			(ia->ia_nirq > 0)) {
		
			sc->sc_irq = ia->ia_irq->ir_irq;
			/* Establish interrupt handler. */
			if(!(atppc_isa_intr_establish(lsc))) {
				lsc->sc_has |= ATPPC_HAS_INTR;
			}
		}
		else {
			ATPPC_DPRINTF(("%s: IRQ not assigned or bad number of "
				"IRQs.\n", self->dv_xname));
		}
	}
	else {
		ATPPC_VPRINTF(("%s: interrupts not configured due to flags.\n", 
			self->dv_xname));
	}

	/* Configure DMA */
	if(!(self->dv_cfdata->cf_flags & ATPPC_FLAG_DISABLE_DMA)) {
		if((ia->ia_drq->ir_drq != ISACF_DRQ_DEFAULT) && 
			(ia->ia_ndrq > 0)) {

			sc->sc_drq = ia->ia_drq->ir_drq;
			if(!(atppc_isa_dma_setup(lsc))) {
				lsc->sc_has |= ATPPC_HAS_DMA;
			}
		}
		else {
			ATPPC_DPRINTF(("%s: DRQ not assigned or bad number of "
				"DRQs.\n", self->dv_xname));
		}
	}
	else {
		ATPPC_VPRINTF(("%s: dma not configured due to flags.\n", 
			self->dv_xname));
	}

	/* Run soft configuration attach */
	atppc_sc_attach(lsc);

	return;
}

/* Detach function: used to detach atppc driver at run time. */
static int 
atppc_isa_detach(struct device * dev, int flag)
{
	struct atppc_softc * lsc = (struct atppc_softc *) dev;
	int rval;

	if(lsc->sc_dev_ok == ATPPC_ATTACHED) {
		/* Run soft configuration detach first */
		rval = atppc_sc_detach(lsc, flag);

		/* Disable DMA */
		atppc_isa_dma_remove(lsc);

		/* Disestablish interrupt handler */
		atppc_isa_intr_disestablish(lsc);
	
		/* Unmap bus space */
		bus_space_unmap(lsc->sc_iot, lsc->sc_ioh, IO_LPTSIZE);
	
		/* Mark config data */
		lsc->sc_dev_ok = ATPPC_NOATTACH;
	}
	else {
		rval = 0;
		if(!(flag & DETACH_QUIET)) {
			printf("%s not properly attached! Detach routines "
				"skipped.\n", dev->dv_xname);
		}
	}

	if(!(flag & DETACH_QUIET)) {
		printf("%s: detached.", dev->dv_xname);
	}

	return rval;
}
