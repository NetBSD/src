/*	$NetBSD: obmem.c,v 1.23.28.1 2008/01/09 01:49:15 matt Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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

/*
 * On-board MEMory (OBMEM)
 * Used by frame buffers...
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obmem.c,v 1.23.28.1 2008/01/09 01:49:15 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <sun3/sun3/obmem.h>

static int  obmem_match(struct device *, struct cfdata *, void *);
static void obmem_attach(struct device *, struct device *, void *);

struct obmem_softc {
	struct device	sc_dev;
	bus_space_tag_t	sc_bustag;
	bus_dma_tag_t	sc_dmatag;
};

CFATTACH_DECL(obmem, sizeof(struct obmem_softc),
    obmem_match, obmem_attach, NULL, NULL);

static int obmem_attached;

static int obmem_bus_map(bus_space_tag_t, bus_type_t, bus_addr_t, bus_size_t,
    int, vaddr_t, bus_space_handle_t *);
static paddr_t obmem_bus_mmap(bus_space_tag_t, bus_type_t, bus_addr_t, off_t,
    int, int);

static struct sun68k_bus_space_tag obmem_space_tag = {
	NULL,				/* cookie */
	NULL,				/* parent bus space tag */
	obmem_bus_map,			/* bus_space_map */
	NULL,				/* bus_space_unmap */
	NULL,				/* bus_space_subregion */
	NULL,				/* bus_space_barrier */
	obmem_bus_mmap,			/* bus_space_mmap */
	NULL,				/* bus_intr_establish */
	NULL,				/* bus_space_peek_N */
	NULL				/* bus_space_poke_N */
};

static int 
obmem_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;

	if (obmem_attached)
		return 0;

	if (ca->ca_bustype != BUS_OBMEM)
		return 0;

	if (ca->ca_name != NULL && strcmp(cf->cf_name, ca->ca_name) != 0)
		return 0;

	return 1;
}

static void 
obmem_attach(struct device *parent, struct device *self, void *aux)
{
	struct confargs *ca = aux;
	struct obmem_softc *sc = (void *)self;
	struct confargs obma;

	obmem_attached = 1;

	printf("\n");

	sc->sc_bustag = ca->ca_bustag;
	sc->sc_dmatag = ca->ca_dmatag;

	obmem_space_tag.cookie = sc;
	obmem_space_tag.parent = sc->sc_bustag;

	obma = *ca;
	obma.ca_bustag = &obmem_space_tag;

	/* We know ca_bustype == BUS_OBMEM */
	config_search_ia(bus_scan, self, "obmem", &obma);
}

int
obmem_bus_map(bus_space_tag_t t, bus_type_t btype, bus_addr_t paddr,
    bus_size_t size, int flags, vaddr_t vaddr, bus_space_handle_t *hp)
{
	struct obmem_softc *sc = t->cookie;

	return bus_space_map2(sc->sc_bustag, PMAP_OBMEM, paddr, size, flags,
	    vaddr, hp);
}

paddr_t
obmem_bus_mmap(bus_space_tag_t t, bus_type_t btype, bus_addr_t paddr, off_t off,
    int prot, int flags)
{
	struct obmem_softc *sc = t->cookie;

	return bus_space_mmap2(sc->sc_bustag, PMAP_OBMEM, paddr, off, prot,
	    flags);
}
