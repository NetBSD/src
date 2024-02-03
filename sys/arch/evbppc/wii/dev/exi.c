/* $NetBSD: exi.c,v 1.1.2.2 2024/02/03 11:47:05 martin Exp $ */

/*-
 * Copyright (c) 2024 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: exi.c,v 1.1.2.2 2024/02/03 11:47:05 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bitops.h>
#include <sys/mutex.h>
#include <uvm/uvm_extern.h>

#include <machine/wii.h>
#include <machine/pio.h>

#include "locators.h"
#include "mainbus.h"
#include "exi.h"

#define	EXI_NUM_CHAN		3
#define	EXI_NUM_DEV		3

/* This is an arbitrary limit. The real limit is probably much higher. */
#define	EXI_MAX_DMA		4096

#define	EXI_CSR(n)		(0x00 + (n) * 0x14)
#define	 EXI_CSR_CS		__BITS(9,7)
#define	EXI_MAR(n)		(0x04 + (n) * 0x14)
#define	EXI_LENGTH(n)		(0x08 + (n) * 0x14)
#define	EXI_CR(n)		(0x0c + (n) * 0x14)
#define	 EXI_CR_TLEN		__BITS(5,4)
#define  EXI_CR_RW		__BITS(3,2)
#define  EXI_CR_RW_READ		__SHIFTIN(0, EXI_CR_RW)
#define  EXI_CR_RW_WRITE	__SHIFTIN(1, EXI_CR_RW)
#define	 EXI_CR_DMA		__BIT(1)
#define  EXI_CR_TSTART		__BIT(0)
#define	EXI_DATA(n)		(0x10 + (n) * 0x14)

#define	ASSERT_CHAN_VALID(chan)	KASSERT((chan) >= 0 && (chan) < EXI_NUM_CHAN)
#define	ASSERT_DEV_VALID(dev)	KASSERT((dev) >= 0 && (dev) < EXI_NUM_DEV)
#define ASSERT_LEN_VALID(len)	KASSERT((len) == 1 || (len) == 2 || (len) == 4)

struct exi_channel {
	kmutex_t		ch_lock;

	bus_dmamap_t		ch_dmamap;

	device_t		ch_child[EXI_NUM_DEV];
};

struct exi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;

	struct exi_channel	sc_chan[EXI_NUM_CHAN];
};

static struct exi_softc *exi_softc;

#define RD4(sc, reg)							\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	exi_match(device_t, cfdata_t, void *);
static void	exi_attach(device_t, device_t, void *);

static int	exi_rescan(device_t, const char *, const int *);
static int	exi_print(void *, const char *);

CFATTACH_DECL_NEW(exi, sizeof(struct exi_softc),
	exi_match, exi_attach, NULL, NULL);

static int
exi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	return strcmp(maa->maa_name, "exi") == 0;
}

static void
exi_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args * const maa = aux;
	struct exi_softc * const sc = device_private(self);
	uint8_t chan;
	int error;

	KASSERT(device_unit(self) == 0);

	aprint_naive("\n");
	aprint_normal(": External Interface\n");

	exi_softc = sc;
	sc->sc_dev = self;
	sc->sc_bst = maa->maa_bst;
	if (bus_space_map(sc->sc_bst, maa->maa_addr, EXI_SIZE, 0,
	    &sc->sc_bsh) != 0) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}
	sc->sc_dmat = maa->maa_dmat;
	for (chan = 0; chan < EXI_NUM_CHAN; chan++) {
		mutex_init(&sc->sc_chan[chan].ch_lock, MUTEX_DEFAULT, IPL_VM);
		error = bus_dmamap_create(exi_softc->sc_dmat, EXI_MAX_DMA, 1,
		    EXI_MAX_DMA, 0, BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW,
		    &sc->sc_chan[chan].ch_dmamap);
		if (error != 0) {
			aprint_error_dev(self, "couldn't create dmamap: %d\n",
			    error);
			return;
		}
	}

	exi_rescan(self, NULL, NULL);
}

static int
exi_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct exi_softc * const sc = device_private(self);
	uint8_t chan, dev;

	for (chan = 0; chan < EXI_NUM_CHAN; chan++) {
		struct exi_channel *ch = &sc->sc_chan[chan];
		for (dev = 0; dev < EXI_NUM_DEV; dev++) {
			struct exi_attach_args eaa = {};
			uint16_t command = 0x0000; /* ID command */
			uint32_t id = 0;

			if (ch->ch_child[dev] != NULL) {
				continue;
			}

			exi_select(chan, dev);
			exi_send_imm(chan, dev, &command, sizeof(command));
			exi_recv_imm(chan, dev, &id, sizeof(id));
			exi_unselect(chan);

			if (id == 0 || id == 0xffffffff) {
				continue;
			}

			eaa.eaa_id = id;
			eaa.eaa_chan = chan;
			eaa.eaa_device = dev;

			ch->ch_child[dev] = config_found(self, &eaa, exi_print,
			    CFARGS(.submatch = config_stdsubmatch,
				   .locators = locs));
		}
	}

	return 0;
}

static int
exi_print(void *aux, const char *pnp)
{
	struct exi_attach_args *eaa = aux;

	if (pnp != NULL) {
		aprint_normal("EXI device 0x%08x at %s", eaa->eaa_id, pnp);
	}

	aprint_normal(" addr %u-%u", eaa->eaa_chan, eaa->eaa_device);

	return UNCONF;
}

void
exi_select(uint8_t chan, uint8_t dev)
{
	struct exi_channel *ch;
	uint32_t val;

	ASSERT_CHAN_VALID(chan);
	ASSERT_DEV_VALID(dev);

	ch = &exi_softc->sc_chan[chan];
	mutex_enter(&ch->ch_lock);

	val = RD4(exi_softc, EXI_CSR(chan));
	val &= ~EXI_CSR_CS;
	val |= __SHIFTIN(__BIT(dev), EXI_CSR_CS);
	WR4(exi_softc, EXI_CSR(chan), val);
}

void
exi_unselect(uint8_t chan)
{
	struct exi_channel *ch;
	uint32_t val;

	ASSERT_CHAN_VALID(chan);

	ch = &exi_softc->sc_chan[chan];

	val = RD4(exi_softc, EXI_CSR(chan));
	val &= ~EXI_CSR_CS;
	WR4(exi_softc, EXI_CSR(chan), val);

	mutex_exit(&ch->ch_lock);
}

static void
exi_wait(uint8_t chan)
{
	uint32_t val;

	ASSERT_CHAN_VALID(chan);

	do {
		val = RD4(exi_softc, EXI_CR(chan));
	} while ((val & EXI_CR_TSTART) != 0);
}

void
exi_send_imm(uint8_t chan, uint8_t dev, const void *data, size_t datalen)
{
	struct exi_channel *ch;
	uint32_t val = 0;

	ASSERT_CHAN_VALID(chan);
	ASSERT_DEV_VALID(dev);
	ASSERT_LEN_VALID(datalen);

	ch = &exi_softc->sc_chan[chan];
	KASSERT(mutex_owned(&ch->ch_lock));

	switch (datalen) {
	case 1:
		val = *(const uint8_t *)data << 24;
		break;
	case 2:
		val = *(const uint16_t *)data << 16;
		break;
	case 4:
		val = *(const uint32_t *)data;
		break;
	}

	WR4(exi_softc, EXI_DATA(chan), val);
	WR4(exi_softc, EXI_CR(chan),
	    EXI_CR_TSTART | EXI_CR_RW_WRITE |
	    __SHIFTIN(datalen - 1, EXI_CR_TLEN));
	exi_wait(chan);
}

void
exi_recv_imm(uint8_t chan, uint8_t dev, void *data, size_t datalen)
{
	struct exi_channel *ch;
	uint32_t val;

	ASSERT_CHAN_VALID(chan);
	ASSERT_DEV_VALID(dev);
	ASSERT_LEN_VALID(datalen);

	ch = &exi_softc->sc_chan[chan];
	KASSERT(mutex_owned(&ch->ch_lock));

	WR4(exi_softc, EXI_CR(chan),
	    EXI_CR_TSTART | EXI_CR_RW_READ |
	    __SHIFTIN(datalen - 1, EXI_CR_TLEN));
	exi_wait(chan);
	val = RD4(exi_softc, EXI_DATA(chan));

	switch (datalen) {
	case 1:
		*(uint8_t *)data = val >> 24;
		break;
	case 2:
		*(uint16_t *)data = val >> 16;
		break;
	case 4:
		*(uint32_t *)data = val;
		break;
	}
}

void
exi_recv_dma(uint8_t chan, uint8_t dev, void *data, size_t datalen)
{
	struct exi_channel *ch;
	int error;

	ASSERT_CHAN_VALID(chan);
	ASSERT_DEV_VALID(dev);
	KASSERT((datalen & 0x1f) == 0);

	ch = &exi_softc->sc_chan[chan];
	KASSERT(mutex_owned(&ch->ch_lock));

	error = bus_dmamap_load(exi_softc->sc_dmat, ch->ch_dmamap,
	    data, datalen, NULL, BUS_DMA_WAITOK);
	if (error != 0) {
		device_printf(exi_softc->sc_dev, "can't load DMA handle: %d\n",
		    error);
		return;
	}

	KASSERT((ch->ch_dmamap->dm_segs[0].ds_addr & 0x1f) == 0);

	bus_dmamap_sync(exi_softc->sc_dmat, ch->ch_dmamap, 0, datalen,
	    BUS_DMASYNC_PREREAD);

	WR4(exi_softc, EXI_MAR(chan), ch->ch_dmamap->dm_segs[0].ds_addr);
	WR4(exi_softc, EXI_LENGTH(chan), datalen);
	WR4(exi_softc, EXI_CR(chan),
	    EXI_CR_TSTART | EXI_CR_RW_READ | EXI_CR_DMA);
	exi_wait(chan);

	bus_dmamap_sync(exi_softc->sc_dmat, ch->ch_dmamap, 0, datalen,
	    BUS_DMASYNC_POSTREAD);

	bus_dmamap_unload(exi_softc->sc_dmat, ch->ch_dmamap);
}
