/* $NetBSD: atppc_isa_subr.c,v 1.2 2004/01/21 00:33:37 bjh21 Exp $ */

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

#include <dev/ic/atppcreg.h>
#include <dev/ic/atppcvar.h>

#include <dev/isa/atppc_isa_subr.h>
#include <dev/isa/isadmavar.h>
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>


/* Initialize interrupt handler. */
int 
atppc_isa_intr_establish(struct atppc_softc * lsc)
{
	struct device * dev = (struct device *) lsc;
	struct atppc_isa_softc * sc = (struct atppc_isa_softc *) lsc;

	/* 
	 * Make sure a handler has not already been established (use
	 * atppc_isa_intr_disestablish() before calling this in that case).
	 */
	if(lsc->sc_ieh != NULL) {
		panic("%s: attempt to establish interrupt handler when" 
			" another handler has already been established.\n", 
			dev->dv_xname);
	}

	/* Assign interrupt handler */
	lsc->sc_ieh = isa_intr_establish(sc->sc_ic, sc->sc_irq, IST_EDGE,
		IPL_ATPPC, atppcintr, dev);

	if(lsc->sc_ieh == NULL) {
		ATPPC_DPRINTF(("%s: establishment of interrupt handler "
			"failed.\n", dev->dv_xname));
		return 1;
	}

	return 0;
}

/* Disestablish the interrupt handler: panics if no handler exists. */
void 
atppc_isa_intr_disestablish(struct atppc_softc * lsc)
{
	struct device * dev = (struct device *) lsc;
	struct atppc_isa_softc * sc = (struct atppc_isa_softc *) lsc;

	/* If a handler exists, disestablish it */
	if(lsc->sc_ieh != NULL) {
		isa_intr_disestablish(sc->sc_ic, lsc->sc_ieh);
		lsc->sc_ieh = NULL;
	}
	else {
		panic("%s: attempt to disestablish interrupt handler when" 
			" no handler has been established.\n", dev->dv_xname);
	}
}

/* Enable DMA */
int 
atppc_isa_dma_setup(struct atppc_softc * lsc)
{
	struct atppc_isa_softc * sc = (struct atppc_isa_softc *) lsc;
#ifdef ATPPC_DEBUG
	struct device * dev = (struct device *) lsc;
#endif
	int error = 1;

	/* Reserve DRQ */
	if(isa_drq_alloc(sc->sc_ic, sc->sc_drq)) {
		ATPPC_DPRINTF(("%s(%s): cannot reserve DRQ line.\n", __func__,
			dev->dv_xname));
		return error;
	}

	/* Get maximum DMA size for isa bus */
	lsc->sc_dma_maxsize = isa_dmamaxsize(sc->sc_ic, sc->sc_drq);

	/* Create dma mapping */
	error = isa_dmamap_create(sc->sc_ic, sc->sc_drq, lsc->sc_dma_maxsize, 
		BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW);
	
	if(!error) {
		lsc->sc_dma_start = atppc_isa_dma_start;
		lsc->sc_dma_finish = atppc_isa_dma_finish;
		lsc->sc_dma_abort = atppc_isa_dma_abort;
		lsc->sc_dma_malloc = atppc_isa_dma_malloc;
		lsc->sc_dma_free = atppc_isa_dma_free;
	}

	return error;
}

/* Disable DMA: if no DMA mapping exists, does nothing. */
int
atppc_isa_dma_remove(struct atppc_softc * lsc)
{
	struct atppc_isa_softc * sc = (struct atppc_isa_softc *) lsc;
#ifdef ATPPC_DEBUG
	struct device * dev = (struct device *) lsc;
#endif

	if(sc->sc_drq != ISACF_DRQ_DEFAULT && 
		lsc->sc_has & ATPPC_HAS_DMA) {

		/* Free DRQ for use */
		if(isa_drq_free(sc->sc_ic, sc->sc_drq)) {
			ATPPC_DPRINTF(("%s(%s): could not free DRQ line.\n", 
				__func__, dev->dv_xname));
		}

		/* Remove DMA mapping */
		isa_dmamap_destroy(sc->sc_ic, sc->sc_drq);
	}
	else {
		ATPPC_DPRINTF(("%s: attempt to remove non-existent DMA "
			"mapping.\n", dev->dv_xname));
	}
	
	return 0;
}

/* Start DMA operation over ISA bus */
int 
atppc_isa_dma_start(struct atppc_softc * lsc, void * buf, u_int nbytes,
	u_int8_t mode)
{
	struct atppc_isa_softc * sc = (struct atppc_isa_softc *) lsc;
	
	return (isa_dmastart(sc->sc_ic, sc->sc_drq, buf, nbytes, NULL,  
		((mode == ATPPC_DMA_MODE_WRITE) ? DMAMODE_WRITE : 
		DMAMODE_READ) | DMAMODE_DEMAND, ((mode == ATPPC_DMA_MODE_WRITE) 
		? BUS_DMA_WRITE : BUS_DMA_READ) | BUS_DMA_NOWAIT));
}

/* Stop DMA operation over ISA bus */
int 
atppc_isa_dma_finish(struct atppc_softc * lsc)
{
	struct atppc_isa_softc * sc = (struct atppc_isa_softc *) lsc;
	
	isa_dmadone(sc->sc_ic, sc->sc_drq);
	return 0;
}

/* Abort DMA operation over ISA bus */
int 
atppc_isa_dma_abort(struct atppc_softc * lsc)
{
	struct atppc_isa_softc * sc = (struct atppc_isa_softc *) lsc;
	
	isa_dmaabort(sc->sc_ic, sc->sc_drq);
	return 0;
}

/* Allocate memory for DMA over ISA bus */ 
int
atppc_isa_dma_malloc(struct device * dev, caddr_t * buf, bus_addr_t * bus_addr,
	bus_size_t size)
{
	int error;
	struct atppc_isa_softc * sc = (struct atppc_isa_softc *) dev;

	error = isa_dmamem_alloc(sc->sc_ic, sc->sc_drq, size, bus_addr, 
		BUS_DMA_WAITOK);
	if(error)
		return error;

	error = isa_dmamem_map(sc->sc_ic, sc->sc_drq, *bus_addr, size, buf, 
		BUS_DMA_WAITOK);
	if(error) {
		isa_dmamem_free(sc->sc_ic, sc->sc_drq, *bus_addr, size);
	}

	return error;
}

/* Free memory allocated by atppc_isa_dma_malloc() */ 
void 
atppc_isa_dma_free(struct device * dev, caddr_t * buf, bus_addr_t * bus_addr, 
	bus_size_t size)
{
	struct atppc_isa_softc * sc = (struct atppc_isa_softc *) dev;
	
	isa_dmamem_unmap(sc->sc_ic, sc->sc_drq, *buf, size);
	isa_dmamem_free(sc->sc_ic, sc->sc_drq, *bus_addr, size);
}

