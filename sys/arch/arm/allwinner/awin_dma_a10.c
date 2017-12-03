/* $NetBSD: awin_dma_a10.c,v 1.3.18.2 2017/12/03 11:35:50 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: awin_dma_a10.c,v 1.3.18.2 2017/12/03 11:35:50 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>
#include <arm/allwinner/awin_dma.h>

#define NDMA_CHANNELS	8
#define DDMA_CHANNELS	8

enum awin_dma_a10_type {
	CH_NDMA,
	CH_DDMA
};

struct awin_dma_a10_channel {
	struct awin_dma_softc *ch_sc;
	uint8_t ch_index;
	enum awin_dma_a10_type ch_type;
	void (*ch_callback)(void *);
	void *ch_callbackarg;
	uint32_t ch_regoff;
};

#define DMA_READ(sc, reg)		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define DMA_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define DMACH_READ(ch, reg)		\
    DMA_READ((ch)->ch_sc, (reg) + (ch)->ch_regoff)
#define DMACH_WRITE(ch, reg, val)	\
    DMA_WRITE((ch)->ch_sc, (reg) + (ch)->ch_regoff, (val))

static kmutex_t awin_dma_a10_lock;
static struct awin_dma_a10_channel awin_ndma_channels[NDMA_CHANNELS];
static struct awin_dma_a10_channel awin_ddma_channels[DDMA_CHANNELS];

static int awin_dma_a10_intr(void *);

static void *awin_dma_a10_alloc(struct awin_dma_softc *, const char *,
				void (*)(void *), void *);
static void awin_dma_a10_free(void *);
static uint32_t awin_dma_a10_get_config(void *);
static void awin_dma_a10_set_config(void *, uint32_t);
static int awin_dma_a10_transfer(void *, paddr_t, paddr_t, size_t);
static void awin_dma_a10_halt(void *);

static const struct awin_dma_controller awin_dma_a10_controller = {
	.dma_alloc = awin_dma_a10_alloc,
	.dma_free = awin_dma_a10_free,
	.dma_get_config = awin_dma_a10_get_config,
	.dma_set_config = awin_dma_a10_set_config,
	.dma_transfer = awin_dma_a10_transfer,
	.dma_halt = awin_dma_a10_halt,
};

void
awin_dma_a10_attach(struct awin_dma_softc *sc, struct awinio_attach_args *aio,
    const struct awin_locators * const loc)
{
	unsigned int index;

	sc->sc_dc = &awin_dma_a10_controller;

	mutex_init(&awin_dma_a10_lock, MUTEX_DEFAULT, IPL_SCHED);

	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_AHB_GATING0_REG, AWIN_AHB_GATING0_DMA, 0);

	DMA_WRITE(sc, AWIN_DMA_IRQ_EN_REG, 0);
	DMA_WRITE(sc, AWIN_DMA_IRQ_PEND_STA_REG, ~0);

	for (index = 0; index < NDMA_CHANNELS; index++) {
		awin_ndma_channels[index].ch_sc = sc;
		awin_ndma_channels[index].ch_index = index;
		awin_ndma_channels[index].ch_type = CH_NDMA;
		awin_ndma_channels[index].ch_callback = NULL;
		awin_ndma_channels[index].ch_callbackarg = NULL;
		awin_ndma_channels[index].ch_regoff = AWIN_NDMA_REG(index);
		DMACH_WRITE(&awin_ndma_channels[index], AWIN_NDMA_CTL_REG, 0);
	}
	for (index = 0; index < DDMA_CHANNELS; index++) {
		awin_ddma_channels[index].ch_sc = sc;
		awin_ddma_channels[index].ch_index = index;
		awin_ddma_channels[index].ch_type = CH_DDMA;
		awin_ddma_channels[index].ch_callback = NULL;
		awin_ddma_channels[index].ch_callbackarg = NULL;
		awin_ddma_channels[index].ch_regoff = AWIN_DDMA_REG(index);
		DMACH_WRITE(&awin_ddma_channels[index], AWIN_DDMA_CTL_REG, 0);
	}

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_SCHED, IST_LEVEL,
	    awin_dma_a10_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish interrupt %d\n", loc->loc_intr);
		return;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting on irq %d\n",
	    loc->loc_intr);
}

static int
awin_dma_a10_intr(void *priv)
{
	struct awin_dma_softc *sc = priv;
	uint32_t sta, bit, mask;
	uint8_t index;

	sta = DMA_READ(sc, AWIN_DMA_IRQ_PEND_STA_REG);
	if (!sta)
		return 0;

	DMA_WRITE(sc, AWIN_DMA_IRQ_PEND_STA_REG, sta);

	while ((bit = ffs(sta & AWIN_DMA_IRQ_END_MASK)) != 0) {
		mask = __BIT(bit - 1);
		sta &= ~mask;
		index = ((bit - 1) / 2) & 7;
		if (mask & AWIN_DMA_IRQ_NDMA) {
			if (awin_ndma_channels[index].ch_callback == NULL)
				continue;
			awin_ndma_channels[index].ch_callback(
			    awin_ndma_channels[index].ch_callbackarg);
		} else {
			if (awin_ddma_channels[index].ch_callback == NULL)
				continue;
			awin_ddma_channels[index].ch_callback(
			    awin_ddma_channels[index].ch_callbackarg);
		}
	}

	return 1;
}

static void *
awin_dma_a10_alloc(struct awin_dma_softc *sc, const char *type,
    void (*cb)(void *), void *cbarg)
{
	struct awin_dma_a10_channel *ch_list;
	struct awin_dma_a10_channel *ch = NULL;
	uint32_t irqen;
	uint8_t ch_count, index;

	if (strcmp(type, "ddma") == 0 ||
	    strcmp(type, "hdmiaudio") == 0) {
		ch_list = awin_ddma_channels;
		ch_count = DDMA_CHANNELS;
	} else {
		ch_list = awin_ndma_channels;
		ch_count = NDMA_CHANNELS;
	}

	mutex_enter(&awin_dma_a10_lock);
	for (index = 0; index < ch_count; index++) {
		if (ch_list[index].ch_callback == NULL) {
			ch = &ch_list[index];
			ch->ch_callback = cb;
			ch->ch_callbackarg = cbarg;

			irqen = DMA_READ(sc, AWIN_DMA_IRQ_EN_REG);
			if (ch->ch_type == CH_NDMA)
				irqen |= AWIN_DMA_IRQ_NDMA_END(index);
			else
				irqen |= AWIN_DMA_IRQ_DDMA_END(index);
			DMA_WRITE(sc, AWIN_DMA_IRQ_EN_REG, irqen);

			break;
		}
	}
	mutex_exit(&awin_dma_a10_lock);

	return ch;
}

static void
awin_dma_a10_free(void *priv)
{
	struct awin_dma_a10_channel *ch = priv;
	struct awin_dma_softc *sc = ch->ch_sc;
	uint32_t irqen, cfg;

	irqen = DMA_READ(sc, AWIN_DMA_IRQ_EN_REG);
	cfg = awin_dma_a10_get_config(ch);
	if (ch->ch_type == CH_NDMA) {
		irqen &= ~AWIN_DMA_IRQ_NDMA_END(ch->ch_index);
		cfg &= ~AWIN_NDMA_CTL_DMA_LOADING;
	} else {
		irqen &= ~AWIN_DMA_IRQ_DDMA_END(ch->ch_index);
		cfg &= ~AWIN_DDMA_CTL_DMA_LOADING;
	}
	awin_dma_a10_set_config(ch, cfg);
	DMA_WRITE(sc, AWIN_DMA_IRQ_EN_REG, irqen);

	mutex_enter(&awin_dma_a10_lock);
	ch->ch_callback = NULL;
	ch->ch_callbackarg = NULL;
	mutex_exit(&awin_dma_a10_lock);
}

static uint32_t
awin_dma_a10_get_config(void *priv)
{
	struct awin_dma_a10_channel *ch = priv;

	if (ch->ch_type == CH_NDMA) {
		return DMACH_READ(ch, AWIN_NDMA_CTL_REG);
	} else {
		return DMACH_READ(ch, AWIN_DDMA_CTL_REG);
	}
}

static void
awin_dma_a10_set_config(void *priv, uint32_t val)
{
	struct awin_dma_a10_channel *ch = priv;

	if (ch->ch_type == CH_NDMA) {
		DMACH_WRITE(ch, AWIN_NDMA_CTL_REG, val);
	} else {
		DMACH_WRITE(ch, AWIN_DDMA_CTL_REG, val);
	}
}

static int
awin_dma_a10_transfer(void *priv, paddr_t src, paddr_t dst,
    size_t nbytes)
{
	struct awin_dma_a10_channel *ch = priv;
	uint32_t cfg;

	cfg = awin_dma_a10_get_config(ch);
	if (ch->ch_type == CH_NDMA) {
		if (cfg & AWIN_NDMA_CTL_DMA_LOADING)
			return EBUSY;

		DMACH_WRITE(ch, AWIN_NDMA_SRC_ADDR_REG, src);
		DMACH_WRITE(ch, AWIN_NDMA_DEST_ADDR_REG, dst);
		DMACH_WRITE(ch, AWIN_NDMA_BC_REG, nbytes);

		cfg |= AWIN_NDMA_CTL_DMA_LOADING;
		awin_dma_a10_set_config(ch, cfg);
	} else {
		if (cfg & AWIN_DDMA_CTL_DMA_LOADING)
			return EBUSY;

		DMACH_WRITE(ch, AWIN_DDMA_SRC_START_ADDR_REG, src);
		DMACH_WRITE(ch, AWIN_DDMA_DEST_START_ADDR_REG, dst);
		DMACH_WRITE(ch, AWIN_DDMA_BC_REG, nbytes);
		DMACH_WRITE(ch, AWIN_DDMA_PARA_REG,
		    __SHIFTIN(31, AWIN_DDMA_PARA_DST_DATA_BLK_SIZ) |
		    __SHIFTIN(7, AWIN_DDMA_PARA_DST_WAIT_CYC) |
		    __SHIFTIN(31, AWIN_DDMA_PARA_SRC_DATA_BLK_SIZ) |
		    __SHIFTIN(7, AWIN_DDMA_PARA_SRC_WAIT_CYC));

		cfg |= AWIN_DDMA_CTL_DMA_LOADING;
		awin_dma_a10_set_config(ch, cfg);
	}

	return 0;
}

static void
awin_dma_a10_halt(void *priv)
{
	struct awin_dma_a10_channel *ch = priv;
	uint32_t cfg;

	cfg = awin_dma_a10_get_config(ch);
	if (ch->ch_type == CH_NDMA) {
		cfg &= ~AWIN_NDMA_CTL_DMA_LOADING;
	} else {
		cfg &= ~AWIN_DDMA_CTL_DMA_LOADING;
	}
	awin_dma_a10_set_config(ch, cfg);
}

#if defined(DDB)
void
awin_dma_a10_dump_regs(struct awin_dma_softc *sc)
{
	int i;

	printf("IRQ_EN:          %08X\n", DMA_READ(sc, AWIN_DMA_IRQ_EN_REG));
	printf("PEND_STA:        %08X\n",
	    DMA_READ(sc, AWIN_DMA_IRQ_PEND_STA_REG));
	for (i = 0; i < NDMA_CHANNELS; i++) {
		printf("NDMA%d CTL:       %08X\n", i,
		    DMA_READ(sc, AWIN_NDMA_REG(i) + AWIN_NDMA_CTL_REG));
		printf("NDMA%d SRC_ADDR:  %08X\n", i,
		    DMA_READ(sc, AWIN_NDMA_REG(i) + AWIN_NDMA_SRC_ADDR_REG));
		printf("NDMA%d DEST_ADDR: %08X\n", i,
		    DMA_READ(sc, AWIN_NDMA_REG(i) + AWIN_NDMA_DEST_ADDR_REG));
		printf("NDMA%d BC:        %08X\n", i,
		    DMA_READ(sc, AWIN_NDMA_REG(i) + AWIN_NDMA_BC_REG));
	}
	for (i = 0; i < DDMA_CHANNELS; i++) {
		printf("DDMA%d CTL:       %08X\n", i,
		    DMA_READ(sc, AWIN_DDMA_REG(i) + AWIN_DDMA_CTL_REG));
		printf("DDMA%d SRC_ADDR:  %08X\n", i,
		    DMA_READ(sc, AWIN_DDMA_REG(i) + AWIN_DDMA_SRC_START_ADDR_REG));
		printf("DDMA%d DEST_ADDR: %08X\n", i,
		    DMA_READ(sc, AWIN_DDMA_REG(i) + AWIN_DDMA_DEST_START_ADDR_REG));
		printf("DDMA%d BC:        %08X\n", i,
		    DMA_READ(sc, AWIN_DDMA_REG(i) + AWIN_DDMA_BC_REG));
		printf("DDMA%d PARA:      %08X\n", i,
		    DMA_READ(sc, AWIN_DDMA_REG(i) + AWIN_DDMA_PARA_REG));
	}
}
#endif
