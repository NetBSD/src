/*	$NetBSD: dmac.c,v 1.3 2005/12/24 20:07:32 perry Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
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
 * SH-5 DMA Controller.
 * Currently only used for copy/zero page ops.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dmac.c,v 1.3 2005/12/24 20:07:32 perry Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/dev/superhywayvar.h>
#include <sh5/dev/dmacreg.h>

#include "locators.h"

#define	DMAC_PMAP_CHANNEL	0

struct dmac_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_dma_tag_t sc_dmat;
	bus_space_handle_t sc_bush;
#ifdef DMAC_PMAP_CHANNEL
	bus_dmamap_t sc_zero_map;
	bus_dma_segment_t sc_zero_seg;
	caddr_t sc_zero_kva;
#endif
};

static int dmacmatch(struct device *, struct cfdata *, void *);
static void dmacattach(struct device *, struct device *, void *);

CFATTACH_DECL(dmac, sizeof(struct dmac_softc),
    dmacmatch, dmacattach, NULL, NULL);
extern struct cfdriver dmac_cd;

#ifdef DMAC_PMAP_CHANNEL
static void dmac_zero_page(void *, paddr_t);
static void dmac_copy_page(void *, paddr_t, paddr_t);
#endif

#define	dmac_reg_read(sc,r)	\
	bus_space_read_8((sc)->sc_bust, (sc)->sc_bush, (r))
#define	dmac_reg_write(sc,r,v)	\
	bus_space_write_8((sc)->sc_bust, (sc)->sc_bush, (r), (v))

/*ARGSUSED*/
static int
dmacmatch(struct device *parent, struct cfdata *cf, void *args)
{
	struct superhyway_attach_args *sa = args;
	bus_space_handle_t bh;
	u_int64_t vcr;

	if (strcmp(sa->sa_name, dmac_cd.cd_name))
		return (0);

	sa->sa_pport = 0;

	if (bus_space_map(sa->sa_bust,
	    SUPERHYWAY_PPORT_TO_BUSADDR(cf->cf_loc[SUPERHYWAYCF_PPORT]),
	    SUPERHYWAY_REG_SZ, 0, &bh))
		return (0);
	vcr = bus_space_read_8(sa->sa_bust, bh, SUPERHYWAY_REG_VCR);
	bus_space_unmap(sa->sa_bust, bh, SUPERHYWAY_REG_SZ);

	if (SUPERHYWAY_VCR_MOD_ID(vcr) != DMAC_MODULE_ID)
		return (0);

	sa->sa_pport = cf->cf_loc[SUPERHYWAYCF_PPORT];

	return (1);
}

/*ARGSUSED*/
static void
dmacattach(struct device *parent, struct device *self, void *args)
{
	struct dmac_softc *sc = (void *)self;
	struct superhyway_attach_args *sa = args;
	int i;
#ifdef DMAC_PMAP_CHANNEL
	int error, nsegs;
#endif

	sc->sc_bust = sa->sa_bust;
	sc->sc_dmat = sa->sa_dmat;

	if (bus_space_map(sa->sa_bust,
	    SUPERHYWAY_PPORT_TO_BUSADDR(sa->sa_pport), DMAC_REG_SZ, 0,
	    &sc->sc_bush)) {
		aprint_error("%s: Failed to map registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	aprint_naive(": DMAC\n");
	aprint_normal(": DMA Controller, Version 0x%x\n",
	    (int)SUPERHYWAY_VCR_MOD_VERS(dmac_reg_read(sc,SUPERHYWAY_REG_VCR)));

	dmac_reg_write(sc, DMAC_COMMON, 0);
	for (i = 0; i < DMAC_N_CHANNELS; i++)
		dmac_reg_write(sc, DMAC_CTRL(i), 0);

#ifdef DMAC_PMAP_CHANNEL
	error = bus_dmamem_alloc(sc->sc_dmat, PAGE_SIZE, PAGE_SIZE, 0,
	    &sc->sc_zero_seg, 1, &nsegs, BUS_DMA_NOWAIT);
	if (error)
		goto error;

	error = bus_dmamem_map(sc->sc_dmat, &sc->sc_zero_seg, 1, PAGE_SIZE,
	    &sc->sc_zero_kva, BUS_DMA_NOWAIT);
	if (error)
		goto free_dmamem;

	error = bus_dmamap_create(sc->sc_dmat, PAGE_SIZE, 1, PAGE_SIZE, 0,
	    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &sc->sc_zero_map);
	if (error)
		goto unmap_dmamem;

	error = bus_dmamap_load(sc->sc_dmat, sc->sc_zero_map, sc->sc_zero_kva,
	    PAGE_SIZE, NULL, BUS_DMA_NOWAIT);
	if (error) {
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_zero_map);
unmap_dmamem:	bus_dmamem_unmap(sc->sc_dmat, sc->sc_zero_kva, PAGE_SIZE);
free_dmamem:	bus_dmamem_free(sc->sc_dmat, &sc->sc_zero_seg, 1);
error:		aprint_error("%s: Failed to allocate zero page (%d)\n",
		    sc->sc_dev.dv_xname, error);
		return;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_zero_map, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREREAD);
	memset(sc->sc_zero_kva, 0, PAGE_SIZE);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_zero_map, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREWRITE);

	pmap_register_copyzero_helpers(dmac_zero_page, dmac_copy_page, sc);
#endif /* DMAC_PMAP_CHANNEL */

	dmac_reg_write(sc, DMAC_COMMON, DMAC_COMMON_MASTER_ENABLE);
}

#ifdef DMAC_PMAP_CHANNEL
static inline void
dmac_page_copy_zero(const struct dmac_softc *sc, paddr_t src, paddr_t dst)
{
	u_int64_t reg;

	dmac_reg_write(sc, DMAC_SAR(DMAC_PMAP_CHANNEL), DMAC_SAR_MASK(src));
	dmac_reg_write(sc, DMAC_DAR(DMAC_PMAP_CHANNEL), DMAC_DAR_MASK(dst));
	dmac_reg_write(sc, DMAC_COUNT(DMAC_PMAP_CHANNEL),
	    DMAC_COUNT_MASK(PAGE_SIZE / 32));
	dmac_reg_write(sc, DMAC_CTRL(DMAC_PMAP_CHANNEL),
	    DMAC_CTRL_TRANSFER_SIZE_32 |
	    DMAC_CTRL_SOURCE_INCREMENT_INC |
	    DMAC_CTRL_DESTINATION_INCREMENT_INC |
	    DMAC_CTRL_RESOURCE_SELECT_AUTO |
	    DMAC_CTRL_TRANSFER_ENABLE);

	/*
	 * It takes ~17uS for a PAGE_SIZE transfer to complete. As there
	 * isn't much we can do in such a short period of time, we might
	 * as well busy-wait.
	 */
	while (((reg = dmac_reg_read(sc, DMAC_STATUS(DMAC_PMAP_CHANNEL))) &
	    DMAC_STATUS_TRANSFER_END) == 0)
		;

	dmac_reg_write(sc, DMAC_CTRL(DMAC_PMAP_CHANNEL), 0);

	if (reg & DMAC_STATUS_ADDRESS_ALIGN_ERROR) {
		reg = dmac_reg_read(sc, DMAC_COMMON);
		reg &= ~(DMAC_COMMON_ERROR_RESPONSE(DMAC_PMAP_CHANNEL) |
		    DMAC_COMMON_ADDRESS_ALIGNMENT_ERROR(DMAC_PMAP_CHANNEL));
		dmac_reg_write(sc, DMAC_COMMON, reg);
	}
}

static void
dmac_zero_page(void *arg, paddr_t pa)
{
	struct dmac_softc *sc = arg;

	dmac_page_copy_zero(sc, sc->sc_zero_seg.ds_addr, pa);
}

static void
dmac_copy_page(void *arg, paddr_t src, paddr_t dst)
{
	struct dmac_softc *sc = arg;

	dmac_page_copy_zero(sc, src, dst);
}
#endif /* DMAC_PMAP_CHANNEL */
