/* $NetBSD: bcm2835_dmac.c,v 1.8 2014/09/12 21:22:13 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_dmac.c,v 1.8 2014/09/12 21:22:13 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/mutex.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_intr.h>

#include <arm/broadcom/bcm2835_dmac.h>

#define BCM_DMAC_CHANNELMASK	0x00000fff

struct bcm_dmac_softc;

struct bcm_dmac_channel {
	struct bcm_dmac_softc *ch_sc;
	void *ch_ih;
	uint8_t ch_index;
	void (*ch_callback)(void *);
	void *ch_callbackarg;
	uint32_t ch_debug;
};

#define DMAC_CHANNEL_TYPE(ch) \
    (((ch)->ch_debug & DMAC_DEBUG_LITE) ? \
     BCM_DMAC_TYPE_LITE : BCM_DMAC_TYPE_NORMAL)
#define DMAC_CHANNEL_USED(ch) \
    ((ch)->ch_callback != NULL)

struct bcm_dmac_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	kmutex_t sc_lock;
	struct bcm_dmac_channel *sc_channels;
	int sc_nchannels;
	uint32_t sc_channelmask;
};

#define DMAC_READ(sc, reg)		\
    bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define DMAC_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

static int	bcm_dmac_match(device_t, cfdata_t, void *);
static void	bcm_dmac_attach(device_t, device_t, void *);

static int	bcm_dmac_intr(void *);

#if defined(DDB)
void		bcm_dmac_dump_regs(void);
#endif

CFATTACH_DECL_NEW(bcmdmac_amba, sizeof(struct bcm_dmac_softc),
	bcm_dmac_match, bcm_dmac_attach, NULL, NULL);

static int
bcm_dmac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct amba_attach_args *aaa = aux;

	if (strcmp(aaa->aaa_name, "bcmdmac") != 0)
		return 0;

	if (aaa->aaa_addr != BCM2835_DMA0_BASE)
		return 0;

	return 1;
}

static void
bcm_dmac_attach(device_t parent, device_t self, void *aux)
{
	struct bcm_dmac_softc *sc = device_private(self);
	const prop_dictionary_t cfg = device_properties(self);
	struct bcm_dmac_channel *ch;
	struct amba_attach_args *aaa = aux;
	uint32_t val;
	int index;

	sc->sc_dev = self;
	sc->sc_iot = aaa->aaa_iot;

	if (bus_space_map(aaa->aaa_iot, aaa->aaa_addr, aaa->aaa_size, 0,
	    &sc->sc_ioh)) {
		aprint_error(": unable to map device\n");
		return;
	}

	prop_dictionary_get_uint32(cfg, "chanmask", &sc->sc_channelmask);
	sc->sc_channelmask &= BCM_DMAC_CHANNELMASK;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SCHED);

	sc->sc_nchannels = 31 - __builtin_clz(sc->sc_channelmask);
	sc->sc_channels = kmem_alloc(
	    sizeof(*sc->sc_channels) * sc->sc_nchannels, KM_SLEEP);
	if (sc->sc_channels == NULL) {
		aprint_error(": couldn't allocate channels\n");
		return;
	}

	aprint_normal(":");
	for (index = 0; index < sc->sc_nchannels; index++) {
		ch = &sc->sc_channels[index];
		ch->ch_sc = sc;
		ch->ch_index = index;
		ch->ch_callback = NULL;
		ch->ch_callbackarg = NULL;
		ch->ch_ih = NULL;
		if ((__BIT(index) & sc->sc_channelmask) == 0)
			continue;

		aprint_normal(" DMA%d", index);

		ch->ch_debug = DMAC_READ(sc, DMAC_DEBUG(index));

		val = DMAC_READ(sc, DMAC_CS(index));
		val |= DMAC_CS_RESET;
		DMAC_WRITE(sc, DMAC_CS(index), val);
	}
	aprint_normal("\n");
	aprint_naive("\n");
}

static int
bcm_dmac_intr(void *priv)
{
	struct bcm_dmac_channel *ch = priv;
	struct bcm_dmac_softc *sc = ch->ch_sc;
	uint32_t cs;

	cs = DMAC_READ(sc, DMAC_CS(ch->ch_index));
	if (!(cs & DMAC_CS_INTMASK))
		return 0;

	DMAC_WRITE(sc, DMAC_CS(ch->ch_index), cs);

	if (ch->ch_callback)
		ch->ch_callback(ch->ch_callbackarg);

	return 1;
}

struct bcm_dmac_channel *
bcm_dmac_alloc(enum bcm_dmac_type type, int ipl, void (*cb)(void *),
    void *cbarg)
{
	struct bcm_dmac_softc *sc;
	struct bcm_dmac_channel *ch = NULL;
	device_t dev;
	int index;

	dev = device_find_by_driver_unit("bcmdmac", 0);
	if (dev == NULL)
		return NULL;
	sc = device_private(dev);

	mutex_enter(&sc->sc_lock);
	for (index = 0; index < sc->sc_nchannels; index++) {
		if ((sc->sc_channelmask & __BIT(index)) == 0)
			continue;
		if (DMAC_CHANNEL_TYPE(&sc->sc_channels[index]) != type)
			continue;
		if (DMAC_CHANNEL_USED(&sc->sc_channels[index]))
			continue;

		ch = &sc->sc_channels[index];
		ch->ch_callback = cb;
		ch->ch_callbackarg = cbarg;
		break;
	}
	mutex_exit(&sc->sc_lock);

	if (ch == NULL)
		return NULL;

	KASSERT(ch->ch_ih == NULL);
	ch->ch_ih = bcm2835_intr_establish(BCM2835_INT_DMA0 + ch->ch_index,
	    ipl, bcm_dmac_intr, ch);
	if (ch->ch_ih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "failed to establish interrupt for DMA%d\n", ch->ch_index);
		ch->ch_callback = NULL;
		ch->ch_callbackarg = NULL;
	}

	return ch;
}

void
bcm_dmac_free(struct bcm_dmac_channel *ch)
{
	struct bcm_dmac_softc *sc = ch->ch_sc;
	uint32_t val;

	bcm_dmac_halt(ch);

	val = DMAC_READ(sc, DMAC_CS(ch->ch_index));
	val |= DMAC_CS_RESET;
	val |= DMAC_CS_ABORT;
	val &= ~DMAC_CS_ACTIVE;
	DMAC_WRITE(sc, DMAC_CS(ch->ch_index), val);

	mutex_enter(&sc->sc_lock);
	intr_disestablish(ch->ch_ih);
	ch->ch_ih = NULL;
	ch->ch_callback = NULL;
	ch->ch_callbackarg = NULL;
	mutex_exit(&sc->sc_lock);
}

void
bcm_dmac_set_conblk_addr(struct bcm_dmac_channel *ch, bus_addr_t addr)
{
	struct bcm_dmac_softc *sc = ch->ch_sc;

	DMAC_WRITE(sc, DMAC_CONBLK_AD(ch->ch_index), addr);
}

int
bcm_dmac_transfer(struct bcm_dmac_channel *ch)
{
	struct bcm_dmac_softc *sc = ch->ch_sc;
	uint32_t val;

	val = DMAC_READ(sc, DMAC_CS(ch->ch_index));
	if (val & DMAC_CS_ACTIVE)
		return EBUSY;

	val |= DMAC_CS_ACTIVE;
	DMAC_WRITE(sc, DMAC_CS(ch->ch_index), val);

	return 0;
}

void
bcm_dmac_halt(struct bcm_dmac_channel *ch)
{
	bcm_dmac_set_conblk_addr(ch, 0);
}

#if defined(DDB)
void
bcm_dmac_dump_regs(void)
{
	struct bcm_dmac_softc *sc;
	device_t dev;
	int index;

	dev = device_find_by_driver_unit("bcmdmac", 0);
	if (dev == NULL)
		return;
	sc = device_private(dev);

	for (index = 0; index < sc->sc_nchannels; index++) {
		if ((sc->sc_channelmask & __BIT(index)) == 0)
			continue;
		printf("%d_CS:        %08X\n", index,
		    DMAC_READ(sc, DMAC_CS(index)));
		printf("%d_CONBLK_AD: %08X\n", index,
		    DMAC_READ(sc, DMAC_CONBLK_AD(index)));
		printf("%d_DEBUG:     %08X\n", index,
		    DMAC_READ(sc, DMAC_DEBUG(index)));
	}
}
#endif
