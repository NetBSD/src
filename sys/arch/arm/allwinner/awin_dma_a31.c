/* $NetBSD: awin_dma_a31.c,v 1.3.16.2 2017/12/03 11:35:50 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_ddb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_dma_a31.c,v 1.3.16.2 2017/12/03 11:35:50 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/bitops.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>
#include <arm/allwinner/awin_dma.h>

#define DMA_CHANNELS	16

struct awin_dma_a31_channel {
	struct awin_dma_softc *ch_sc;
	uint8_t ch_index;
	void (*ch_callback)(void *);
	void *ch_callbackarg;

	bus_dma_segment_t ch_dmasegs[1];
	bus_dmamap_t ch_dmamap;
	void *ch_dmadesc;
	bus_size_t ch_dmadesclen;
};

#define DMA_READ(sc, reg)		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define DMA_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static kmutex_t awin_dma_a31_lock;
static struct awin_dma_a31_channel awin_dma_channels[DMA_CHANNELS];

static int awin_dma_a31_intr(void *);

static void *awin_dma_a31_alloc(struct awin_dma_softc *, const char *,
				void (*)(void *), void *);
static void awin_dma_a31_free(void *);
static uint32_t awin_dma_a31_get_config(void *);
static void awin_dma_a31_set_config(void *, uint32_t);
static int awin_dma_a31_transfer(void *, paddr_t, paddr_t, size_t);
static void awin_dma_a31_halt(void *);

static const struct awin_dma_controller awin_dma_a31_controller = {
	.dma_alloc = awin_dma_a31_alloc,
	.dma_free = awin_dma_a31_free,
	.dma_get_config = awin_dma_a31_get_config,
	.dma_set_config = awin_dma_a31_set_config,
	.dma_transfer = awin_dma_a31_transfer,
	.dma_halt = awin_dma_a31_halt,
};

void
awin_dma_a31_attach(struct awin_dma_softc *sc, struct awinio_attach_args *aio,
    const struct awin_locators * const loc)
{
	unsigned int index;
	bus_size_t desclen = sizeof(struct awin_a31_dma_desc);
	int error, nsegs;

	sc->sc_dc = &awin_dma_a31_controller;

	mutex_init(&awin_dma_a31_lock, MUTEX_DEFAULT, IPL_SCHED);

	switch (awin_chip_id()) {
	case AWIN_CHIP_ID_A31:
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_AHB_GATING0_REG, AWIN_AHB_GATING0_DMA, 0);
		break;
	case AWIN_CHIP_ID_A80:
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A80_CCU_SCLK_BUS_CLK_GATING1_REG,
		    AWIN_A80_CCU_SCLK_BUS_CLK_GATING1_DMA, 0);
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A80_CCU_SCLK_BUS_SOFT_RST1_REG,
		    AWIN_A80_CCU_SCLK_BUS_SOFT_RST1_DMA, 0);
		break;
	}

	DMA_WRITE(sc, AWIN_A31_DMA_IRQ_EN_REG0_REG, 0);
	DMA_WRITE(sc, AWIN_A31_DMA_IRQ_EN_REG1_REG, 0);
	DMA_WRITE(sc, AWIN_A31_DMA_IRQ_PEND_REG0_REG, ~0);
	DMA_WRITE(sc, AWIN_A31_DMA_IRQ_PEND_REG1_REG, ~0);

	for (index = 0; index < DMA_CHANNELS; index++) {
		struct awin_dma_a31_channel *ch = &awin_dma_channels[index];
		ch->ch_sc = sc;
		ch->ch_index = index;
		ch->ch_callback = NULL;
		ch->ch_callbackarg = NULL;
		ch->ch_dmadesclen = desclen;

		error = bus_dmamem_alloc(sc->sc_dmat, desclen, 0, 0,
		    ch->ch_dmasegs, 1, &nsegs, BUS_DMA_WAITOK);
		if (error)
			panic("bus_dmamem_alloc failed: %d", error);
		error = bus_dmamem_map(sc->sc_dmat, ch->ch_dmasegs, nsegs,
		    desclen, &ch->ch_dmadesc, BUS_DMA_WAITOK);
		if (error)
			panic("bus_dmamem_map failed: %d", error);
		error = bus_dmamap_create(sc->sc_dmat, desclen, 1, desclen, 0,
		    BUS_DMA_WAITOK, &ch->ch_dmamap);
		if (error)
			panic("bus_dmamap_create failed: %d", error);
		error = bus_dmamap_load(sc->sc_dmat, ch->ch_dmamap,
		    ch->ch_dmadesc, desclen, NULL, BUS_DMA_WAITOK);
		if (error)
			panic("bus_dmamap_load failed: %d", error);

		DMA_WRITE(sc, AWIN_A31_DMA_EN_REG(index), 0);
	}

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_SCHED, IST_LEVEL,
	    awin_dma_a31_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish interrupt %d\n", loc->loc_intr);
		return;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting on irq %d\n",
	    loc->loc_intr);
}

static int
awin_dma_a31_intr(void *priv)
{
	struct awin_dma_softc *sc = priv;
	uint32_t pend0, pend1, bit;
	uint64_t pend, mask;
	uint8_t index;

	pend0 = DMA_READ(sc, AWIN_A31_DMA_IRQ_PEND_REG0_REG);
	pend1 = DMA_READ(sc, AWIN_A31_DMA_IRQ_PEND_REG1_REG);
	if (!pend0 && !pend1)
		return 0;

	DMA_WRITE(sc, AWIN_A31_DMA_IRQ_PEND_REG0_REG, pend0);
	DMA_WRITE(sc, AWIN_A31_DMA_IRQ_PEND_REG1_REG, pend1);

	pend = pend0 | ((uint64_t)pend1 << 32);

	while ((bit = ffs64(pend & AWIN_A31_DMA_IRQ_PKG_MASK)) != 0) {
		mask = __BIT(bit - 1);
		pend &= ~mask;
		index = (bit - 1) / 4;

		if (awin_dma_channels[index].ch_callback == NULL)
			continue;
		awin_dma_channels[index].ch_callback(
		    awin_dma_channels[index].ch_callbackarg);
	}

	return 1;
}

static void *
awin_dma_a31_alloc(struct awin_dma_softc *sc, const char *type,
    void (*cb)(void *), void *cbarg)
{
	struct awin_dma_a31_channel *ch_list = awin_dma_channels;
	struct awin_dma_a31_channel *ch = NULL;
	uint32_t irqen;
	uint8_t index;

	mutex_enter(&awin_dma_a31_lock);
	for (index = 0; index < DMA_CHANNELS; index++) {
		if (ch_list[index].ch_callback == NULL) {
			ch = &ch_list[index];
			ch->ch_callback = cb;
			ch->ch_callbackarg = cbarg;

			irqen = DMA_READ(sc, index < 8 ?
			    AWIN_A31_DMA_IRQ_EN_REG0_REG :
			    AWIN_A31_DMA_IRQ_EN_REG1_REG);
			irqen |= (index < 8 ?
			    AWIN_A31_DMA_IRQ_EN_REG0_PKG_IRQ_EN(index) :
			    AWIN_A31_DMA_IRQ_EN_REG1_PKG_IRQ_EN(index));
			DMA_WRITE(sc, index < 8 ?
			    AWIN_A31_DMA_IRQ_EN_REG0_REG :
			    AWIN_A31_DMA_IRQ_EN_REG1_REG, irqen);

			break;
		}
	}
	mutex_exit(&awin_dma_a31_lock);

	return ch;
}

static void
awin_dma_a31_free(void *priv)
{
	struct awin_dma_a31_channel *ch = priv;
	struct awin_dma_softc *sc = ch->ch_sc;
	uint32_t irqen;
	uint8_t index = ch->ch_index;

	irqen = DMA_READ(sc, index < 8 ?
	    AWIN_A31_DMA_IRQ_EN_REG0_REG :
	    AWIN_A31_DMA_IRQ_EN_REG1_REG);
	irqen &= ~(index < 8 ?
	    AWIN_A31_DMA_IRQ_EN_REG0_PKG_IRQ_EN(index) :
	    AWIN_A31_DMA_IRQ_EN_REG1_PKG_IRQ_EN(index));
	DMA_WRITE(sc, index < 8 ?
	    AWIN_A31_DMA_IRQ_EN_REG0_REG :
	    AWIN_A31_DMA_IRQ_EN_REG1_REG, irqen);

	mutex_enter(&awin_dma_a31_lock);
	ch->ch_callback = NULL;
	ch->ch_callbackarg = NULL;
	mutex_exit(&awin_dma_a31_lock);
}

static uint32_t
awin_dma_a31_get_config(void *priv)
{
	struct awin_dma_a31_channel *ch = priv;
	struct awin_dma_softc *sc = ch->ch_sc;

	return DMA_READ(sc, AWIN_A31_DMA_CFG_REG(ch->ch_index));
}

static void
awin_dma_a31_set_config(void *priv, uint32_t val)
{
	struct awin_dma_a31_channel *ch = priv;
	struct awin_dma_softc *sc = ch->ch_sc;
	struct awin_a31_dma_desc *desc = ch->ch_dmadesc;

	desc->dma_config = htole32(val);

	bus_dmamap_sync(sc->sc_dmat, ch->ch_dmamap, 0, ch->ch_dmadesclen,
	    BUS_DMASYNC_PREWRITE);
}

static int
awin_dma_a31_transfer(void *priv, paddr_t src, paddr_t dst,
    size_t nbytes)
{
	struct awin_dma_a31_channel *ch = priv;
	struct awin_dma_softc *sc = ch->ch_sc;
	struct awin_a31_dma_desc *desc = ch->ch_dmadesc;
#if 0
	uint32_t stat;

	stat = DMA_READ(sc, AWIN_A31_DMA_STA_REG);
	if (stat & __BIT(ch->ch_index))
		return EBUSY;
#endif

	desc->dma_srcaddr = htole32(src);
	desc->dma_dstaddr = htole32(dst);
	desc->dma_bcnt = htole32(nbytes);
	desc->dma_para = htole32(0);
	desc->dma_next = htole32(AWIN_A31_DMA_NULL);

	bus_dmamap_sync(sc->sc_dmat, ch->ch_dmamap, 0, ch->ch_dmadesclen,
	    BUS_DMASYNC_PREWRITE);

	DMA_WRITE(sc, AWIN_A31_DMA_START_ADDR_REG(ch->ch_index),
	    ch->ch_dmamap->dm_segs[0].ds_addr);
	DMA_WRITE(sc, AWIN_A31_DMA_EN_REG(ch->ch_index), AWIN_A31_DMA_EN_EN);

	return 0;
}

static void
awin_dma_a31_halt(void *priv)
{
	struct awin_dma_a31_channel *ch = priv;
	struct awin_dma_softc *sc = ch->ch_sc;

	DMA_WRITE(sc, AWIN_A31_DMA_EN_REG(ch->ch_index), 0);
}

#if defined(DDB)
void
awin_dma_a31_dump_regs(struct awin_dma_softc *sc)
{
	int i;

	printf("IRQ_EN0          %08X\n",
	    DMA_READ(sc, AWIN_A31_DMA_IRQ_EN_REG0_REG));
	printf("IRQ_EN1          %08X\n",
	    DMA_READ(sc, AWIN_A31_DMA_IRQ_EN_REG1_REG));
	printf("PEND0:           %08X\n",
	    DMA_READ(sc, AWIN_A31_DMA_IRQ_PEND_REG0_REG));
	printf("PEND1:           %08X\n",
	    DMA_READ(sc, AWIN_A31_DMA_IRQ_PEND_REG1_REG));
	printf("STA:             %08X\n",
	    DMA_READ(sc, AWIN_A31_DMA_STA_REG));
	for (i = 0; i < DMA_CHANNELS; i++) {
		printf("DMA%d EN:         %08X\n", i,
		    DMA_READ(sc, AWIN_A31_DMA_EN_REG(i)));
		printf("DMA%d PAU:        %08X\n", i,
		    DMA_READ(sc, AWIN_A31_DMA_PAU_REG(i)));
		printf("DMA%d START_ADDR: %08X\n", i,
		    DMA_READ(sc, AWIN_A31_DMA_START_ADDR_REG(i)));
		printf("DMA%d CFG:        %08X\n", i,
		    DMA_READ(sc, AWIN_A31_DMA_CFG_REG(i)));
		printf("DMA%d CUR_SRC:    %08X\n", i,
		    DMA_READ(sc, AWIN_A31_DMA_CUR_SRC_REG(i)));
		printf("DMA%d CUR_DEST:   %08X\n", i,
		    DMA_READ(sc, AWIN_A31_DMA_CUR_DEST_REG(i)));
		printf("DMA%d BCNT_LEFT:  %08X\n", i,
		    DMA_READ(sc, AWIN_A31_DMA_BCNT_LEFT_REG(i)));
		printf("DMA%d PARA:       %08X\n", i,
		    DMA_READ(sc, AWIN_A31_DMA_PARA_REG(i)));
	}
}
#endif
