/* $NetBSD: atppc_isa_subr.h,v 1.3 2004/01/25 00:28:01 bjh21 Exp $ */

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

#ifndef __ATPPC_ISA_SUBR_H
#define __ATPPC_ISA_SUBR_H

#include <dev/ic/atppcvar.h>
#include <dev/isa/isavar.h>


/* Configuration data for atppc on isa bus. */
struct atppc_isa_softc {
	/* Machine independent device data */
	struct atppc_softc sc_atppc;

	/* IRQ/DRQ/IO Port assignments on ISA bus */
	int sc_irq;
	int sc_drq;
	int sc_iobase;

	/* ISA chipset tag */
	isa_chipset_tag_t sc_ic;
};

#ifdef _KERNEL

/* Interrupt setup/teardown functions */
int atppc_isa_intr_establish(struct atppc_softc *);
void atppc_isa_intr_disestablish(struct atppc_softc *);

/* DMA setup functions */
int atppc_isa_dma_setup(struct atppc_softc *);
int atppc_isa_dma_remove(struct atppc_softc *);
int atppc_isa_dma_start(struct atppc_softc *, void *, u_int, 
	u_int8_t);
int atppc_isa_dma_finish(struct atppc_softc *);
int atppc_isa_dma_abort(struct atppc_softc *);
int atppc_isa_dma_malloc(struct device *, caddr_t *, bus_addr_t *, 
	bus_size_t);
void atppc_isa_dma_free(struct device *, caddr_t *, bus_addr_t *, 
	bus_size_t);

#endif /* _KERNEL */

#endif /* __ATPPC_ISA_SUBR_H */
