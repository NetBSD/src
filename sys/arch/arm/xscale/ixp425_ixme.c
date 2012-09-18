/*	$NetBSD: ixp425_ixme.c,v 1.4 2012/09/18 05:47:28 matt Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
__KERNEL_RCSID(0, "$NetBSD: ixp425_ixme.c,v 1.4 2012/09/18 05:47:28 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#define _ARM32_BUS_DMA_PRIVATE
#include <sys/bus.h>

#include <arm/xscale/ixp425var.h>
#include <arm/xscale/ixp425_qmgr.h>
#include <arm/xscale/ixp425_ixmevar.h>

#include "locators.h"

static int	ixme_match(struct device *, struct cfdata *, void *);
static void	ixme_attach(struct device *, struct device *, void *);
static int	ixme_search(struct device *, struct cfdata *, const int *,
		    void *);
static int	ixme_print(void *, const char *);

struct ixme_softc {
	struct device sc_dev;
	struct arm32_dma_range sc_dr;
	struct arm32_bus_dma_tag sc_dt;
	void *sc_qmgr;
};

CFATTACH_DECL(ixme, sizeof(struct ixme_softc),
    ixme_match, ixme_attach, NULL, NULL);

static int
ixme_match(struct device *parent, struct cfdata *self, void *arg)
{

	return (1);
}

static void
ixme_attach(struct device *parent, struct device *self, void *arg)
{
	struct ixme_softc *sc = (void *)self;
	extern paddr_t physical_start, physical_end;

	aprint_naive("\n");
	aprint_normal(": IXP4xx MicroEngine Support\n");

	sc->sc_qmgr = ixpqmgr_init(&ixp425_bs_tag);
	if (sc->sc_qmgr == NULL) {
		panic("%s: ixme_attach: Failed to init Queue Manager",
		    sc->sc_dev.dv_xname);
	}

	sc->sc_dr.dr_sysbase = physical_start;
	sc->sc_dr.dr_busbase = 0;
	sc->sc_dr.dr_len = physical_end - physical_start;
	sc->sc_dt._ranges = &sc->sc_dr;
	sc->sc_dt._nranges = 1;

	sc->sc_dt._dmamap_create = _bus_dmamap_create;
	sc->sc_dt._dmamap_destroy = _bus_dmamap_destroy;
	sc->sc_dt._dmamap_load = _bus_dmamap_load;
	sc->sc_dt._dmamap_load_mbuf = _bus_dmamap_load_mbuf;
	sc->sc_dt._dmamap_load_uio = _bus_dmamap_load_uio;
	sc->sc_dt._dmamap_load_raw = _bus_dmamap_load_raw;
	sc->sc_dt._dmamap_unload = _bus_dmamap_unload;
	sc->sc_dt._dmamap_sync_pre = _bus_dmamap_sync;
	sc->sc_dt._dmamap_sync_post = NULL;

	sc->sc_dt._dmamem_alloc = _bus_dmamem_alloc;
	sc->sc_dt._dmamem_free = _bus_dmamem_free;
	sc->sc_dt._dmamem_map = _bus_dmamem_map;
	sc->sc_dt._dmamem_unmap = _bus_dmamem_unmap;
	sc->sc_dt._dmamem_mmap = _bus_dmamem_mmap;

	sc->sc_dt._dmatag_subregion = _bus_dmatag_subregion;
	sc->sc_dt._dmatag_destroy = _bus_dmatag_destroy;

	config_search_ia(ixme_search, self, "ixme", NULL);
}

static int
ixme_search(struct device *parent, struct cfdata *cf, const int *ldesc,
    void *arg)
{
	struct ixme_softc *sc = (void *)parent;
	struct ixme_attach_args ixa;

	if (cf->cf_loc[IXMECF_NPE] < 0 || cf->cf_loc[IXMECF_NPE] > 2)
		return (0);

	ixa.ixa_iot = &ixp425_bs_tag;
	ixa.ixa_dt = &sc->sc_dt;
	ixa.ixa_npe = cf->cf_loc[IXMECF_NPE];

	if (config_match(parent, cf, &ixa) > 0) {
		config_attach(parent, cf, &ixa, ixme_print);
		return (1);
	}

	return (0);
}

static int
ixme_print(void *arg, const char *name)
{
	struct ixme_attach_args *ixa = arg;

	aprint_normal(" NPE-%c", 'A' + ixa->ixa_npe);

	return (UNCONF);
}
