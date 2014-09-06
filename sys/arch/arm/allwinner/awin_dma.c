/* $NetBSD: awin_dma.c,v 1.1 2014/09/06 00:15:34 jmcneill Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_dma.c,v 1.1 2014/09/06 00:15:34 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#define NDMA_CHANNELS	8
#define DDMA_CHANNELS	8

struct awin_dma_channel {
	uint8_t ch_index;
	enum awin_dma_type ch_type;
	void (*ch_callback)(void *);
	void *ch_callbackarg;
	uint32_t ch_regoff;
};

struct awin_dma_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	void *sc_ih;
};

#define DMA_READ(reg)			\
    bus_space_read_4(awin_dma_sc->sc_bst, awin_dma_sc->sc_bsh, (reg))
#define DMA_WRITE(reg, val)		\
    bus_space_write_4(awin_dma_sc->sc_bst, awin_dma_sc->sc_bsh, (reg), (val))
#define DMACH_READ(ch, reg)		\
    DMA_READ((reg) + (ch)->ch_regoff)
#define DMACH_WRITE(ch, reg, val)	\
    DMA_WRITE((reg) + (ch)->ch_regoff, (val))

static struct awin_dma_softc *awin_dma_sc;
static kmutex_t awin_dma_lock;
static struct awin_dma_channel awin_ndma_channels[NDMA_CHANNELS];
static struct awin_dma_channel awin_ddma_channels[DDMA_CHANNELS];

static int	awin_dma_match(device_t, cfdata_t, void *);
static void	awin_dma_attach(device_t, device_t, void *);

static int	awin_dma_intr(void *);

CFATTACH_DECL_NEW(awin_dma, sizeof(struct awin_dma_softc),
	awin_dma_match, awin_dma_attach, NULL, NULL);

static int
awin_dma_match(device_t parent, cfdata_t cf, void *aux)
{
	return awin_dma_sc == NULL;
}

static void
awin_dma_attach(device_t parent, device_t self, void *aux)
{
	struct awin_dma_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	uint8_t index;

	KASSERT(awin_dma_sc == NULL);
	awin_dma_sc = sc;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal(": DMA\n");

	mutex_init(&awin_dma_lock, MUTEX_DEFAULT, IPL_SCHED);

	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_AHB_GATING0_REG, AWIN_AHB_GATING0_DMA, 0);

	DMA_WRITE(AWIN_DMA_IRQ_EN_REG, 0);
	DMA_WRITE(AWIN_DMA_IRQ_PEND_STA_REG, ~0);

	for (index = 0; index < NDMA_CHANNELS; index++) {
		awin_ndma_channels[index].ch_index = index;
		awin_ndma_channels[index].ch_type = AWIN_DMA_TYPE_NDMA;
		awin_ndma_channels[index].ch_callback = NULL;
		awin_ndma_channels[index].ch_callbackarg = NULL;
		awin_ndma_channels[index].ch_regoff = AWIN_NDMA_REG(index);
		DMACH_WRITE(&awin_ndma_channels[index], AWIN_NDMA_CTL_REG, 0);
	}
	for (index = 0; index < DDMA_CHANNELS; index++) {
		awin_ddma_channels[index].ch_index = index;
		awin_ddma_channels[index].ch_type = AWIN_DMA_TYPE_DDMA;
		awin_ddma_channels[index].ch_callback = NULL;
		awin_ddma_channels[index].ch_callbackarg = NULL;
		awin_ddma_channels[index].ch_regoff = AWIN_DDMA_REG(index);
		DMACH_WRITE(&awin_ddma_channels[index], AWIN_DDMA_CTL_REG, 0);
	}

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_SCHED, IST_LEVEL,
	    awin_dma_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
}

static int
awin_dma_intr(void *priv)
{
	uint32_t sta, bit, mask;
	uint8_t index;

	sta = DMA_READ(AWIN_DMA_IRQ_PEND_STA_REG);
	if (!sta)
		return 0;

	DMA_WRITE(AWIN_DMA_IRQ_PEND_STA_REG, sta);

	while ((bit = ffs(sta & AWIN_DMA_IRQ_END_MASK)) != 0) {
		mask = __BIT(bit - 1);
		sta &= ~mask;
		index = (bit / 2) & 7;
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

struct awin_dma_channel *
awin_dma_alloc(enum awin_dma_type type, void (*cb)(void *), void *cbarg)
{
	struct awin_dma_channel *ch_list;
	struct awin_dma_channel *ch = NULL;
	uint32_t irqen;
	uint8_t ch_count, index;

	if (type == AWIN_DMA_TYPE_NDMA) {
		ch_list = awin_ndma_channels;
		ch_count = NDMA_CHANNELS;
	} else {
		ch_list = awin_ndma_channels;
		ch_count = DDMA_CHANNELS;
	}

	mutex_enter(&awin_dma_lock);
	for (index = 0; index < ch_count; index++) {
		if (ch_list[index].ch_callback == NULL) {
			ch = &ch_list[index];
			ch->ch_callback = cb;
			ch->ch_callbackarg = cbarg;

			irqen = DMA_READ(AWIN_DMA_IRQ_EN_REG);
			if (type == AWIN_DMA_TYPE_NDMA)
				irqen |= AWIN_DMA_IRQ_NDMA_END(index);
			else
				irqen |= AWIN_DMA_IRQ_DDMA_END(index);
			DMA_WRITE(AWIN_DMA_IRQ_EN_REG, irqen);

			break;
		}
	}
	mutex_exit(&awin_dma_lock);

	return ch;
}

void
awin_dma_free(struct awin_dma_channel *ch)
{
	uint32_t irqen, cfg;

	irqen = DMA_READ(AWIN_DMA_IRQ_EN_REG);
	cfg = awin_dma_get_config(ch);
	if (ch->ch_type == AWIN_DMA_TYPE_NDMA) {
		irqen &= ~AWIN_DMA_IRQ_NDMA_END(ch->ch_index);
		cfg &= ~AWIN_NDMA_CTL_DMA_LOADING;
	} else {
		irqen &= ~AWIN_DMA_IRQ_DDMA_END(ch->ch_index);
		cfg &= ~AWIN_DDMA_CTL_DMA_LOADING;
	}
	awin_dma_set_config(ch, cfg);
	DMA_WRITE(AWIN_DMA_IRQ_EN_REG, irqen);

	mutex_enter(&awin_dma_lock);
	ch->ch_callback = NULL;
	ch->ch_callbackarg = NULL;
	mutex_exit(&awin_dma_lock);
}

uint32_t
awin_dma_get_config(struct awin_dma_channel *ch)
{
	return DMACH_READ(ch, AWIN_NDMA_CTL_REG);
}

void
awin_dma_set_config(struct awin_dma_channel *ch, uint32_t val)
{
	DMACH_WRITE(ch, AWIN_NDMA_CTL_REG, val);
}

int
awin_dma_transfer(struct awin_dma_channel *ch, paddr_t src, paddr_t dst,
    size_t nbytes)
{
	uint32_t cfg;

	cfg = awin_dma_get_config(ch);
	if (ch->ch_type == AWIN_DMA_TYPE_NDMA) {
		if (cfg & AWIN_NDMA_CTL_DMA_LOADING)
			return EBUSY;

		DMACH_WRITE(ch, AWIN_NDMA_SRC_ADDR_REG, src);
		DMACH_WRITE(ch, AWIN_NDMA_DEST_ADDR_REG, dst);
		DMACH_WRITE(ch, AWIN_NDMA_BC_REG, nbytes);

		cfg |= AWIN_NDMA_CTL_DMA_LOADING;
		awin_dma_set_config(ch, cfg);
	} else {
		if (cfg & AWIN_DDMA_CTL_DMA_LOADING)
			return EBUSY;

		DMACH_WRITE(ch, AWIN_DDMA_SRC_START_ADDR_REG, src);
		DMACH_WRITE(ch, AWIN_DDMA_DEST_START_ADDR_REG, dst);
		DMACH_WRITE(ch, AWIN_DDMA_BC_REG, nbytes);

		cfg |= AWIN_DDMA_CTL_DMA_LOADING;
		awin_dma_set_config(ch, cfg);
	}

	return 0;
}
