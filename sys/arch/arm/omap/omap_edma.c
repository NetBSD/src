/* $NetBSD: omap_edma.c,v 1.1.4.4 2016/10/05 20:55:25 skrll Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: omap_edma.c,v 1.1.4.4 2016/10/05 20:55:25 skrll Exp $");

#include "opt_omap.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/intr.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/bitops.h>

#include <arm/mainbus/mainbus.h>

#include <arm/omap/am335x_prcm.h>
#include <arm/omap/omap2_prcm.h>
#include <arm/omap/sitara_cm.h>
#include <arm/omap/sitara_cmreg.h>

#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap_edma.h>
#include <arm/omap/omap_var.h>

#ifdef TI_AM335X
static const struct omap_module edma3cc_module =
	{ AM335X_PRCM_CM_PER, CM_PER_TPCC_CLKCTRL };
static const struct omap_module edma3tc0_module =
	{ AM335X_PRCM_CM_PER, CM_PER_TPTC0_CLKCTRL };
static const struct omap_module edma3tc1_module =
	{ AM335X_PRCM_CM_PER, CM_PER_TPTC1_CLKCTRL };
static const struct omap_module edma3tc2_module =
	{ AM335X_PRCM_CM_PER, CM_PER_TPTC2_CLKCTRL };
#endif

#define NUM_DMA_CHANNELS	64
#define NUM_PARAM_SETS		256
#define MAX_PARAM_PER_CHANNEL	32

#ifdef EDMA_DEBUG
int edmadebug = 1;
#define DPRINTF(n,s)    do { if ((n) <= edmadebug) device_printf s; } while (0)
#else
#define DPRINTF(n,s)    do {} while (0)
#endif

struct edma_softc;

struct edma_channel {
	struct edma_softc *ch_sc;
	enum edma_type ch_type;
	uint8_t ch_index;
	void (*ch_callback)(void *);
	void *ch_callbackarg;
	unsigned int ch_nparams;
};

struct edma_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	kmutex_t sc_lock;
	struct edma_channel sc_dma[NUM_DMA_CHANNELS];

	void *sc_ih;
	void *sc_mperr_ih;
	void *sc_errint_ih;

	uint32_t sc_dmamask[NUM_DMA_CHANNELS / 32];
	uint32_t sc_parammask[NUM_PARAM_SETS / 32];
};

static int edma_match(device_t, cfdata_t, void *);
static void edma_attach(device_t, device_t, void *);

static void edma_init(struct edma_softc *);
static int edma_intr(void *);
static int edma_mperr_intr(void *);
static int edma_errint_intr(void *);
static void edma_write_param(struct edma_softc *,
				  unsigned int, const struct edma_param *);
static bool edma_bit_isset(uint32_t *, unsigned int);
static void edma_bit_set(uint32_t *, unsigned int);
static void edma_bit_clr(uint32_t *, unsigned int);

CFATTACH_DECL_NEW(edma, sizeof(struct edma_softc),
    edma_match, edma_attach, NULL, NULL);

#define EDMA_READ(sc, reg) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define EDMA_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

static int
edma_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *mb = aux;

#ifdef TI_AM335X
	if (mb->mb_iobase == AM335X_TPCC_BASE &&
	    mb->mb_iosize == AM335X_TPCC_SIZE &&
	    mb->mb_intrbase == AM335X_INT_EDMACOMPINT)
		return 1;
#endif

	return 0;
}

static void
edma_attach(device_t parent, device_t self, void *aux)
{
	struct edma_softc *sc = device_private(self);
	struct mainbus_attach_args *mb = aux;
	int idx;

	sc->sc_dev = self;
	sc->sc_iot = &omap_bs_tag;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SCHED);
	if (bus_space_map(sc->sc_iot, mb->mb_iobase, mb->mb_iosize,
	    0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map address spcae\n");
		return;
	}

	aprint_normal("\n");
	aprint_naive("\n");

	for (idx = 0; idx < NUM_DMA_CHANNELS; idx++) {
		struct edma_channel *ch = &sc->sc_dma[idx];
		ch->ch_sc = sc;
		ch->ch_type = EDMA_TYPE_DMA;
		ch->ch_index = idx;
		ch->ch_callback = NULL;
		ch->ch_callbackarg = NULL;
		ch->ch_nparams = 0;
	}

	edma_init(sc);

	sc->sc_ih = intr_establish(mb->mb_intrbase + 0,
	    IPL_SCHED, IST_LEVEL, edma_intr, sc);
	KASSERT(sc->sc_ih != NULL);

	sc->sc_mperr_ih = intr_establish(mb->mb_intrbase + 1,
	    IPL_SCHED, IST_LEVEL, edma_mperr_intr, sc);
	sc->sc_errint_ih = intr_establish(mb->mb_intrbase + 2,
	    IPL_SCHED, IST_LEVEL, edma_errint_intr, sc);
}

/*
 * Hardware initialization
 */
static void
edma_init(struct edma_softc *sc)
{
	struct edma_param param;
	uint32_t val;
	int idx;

#ifdef TI_AM335X
	prcm_module_enable(&edma3cc_module);
	prcm_module_enable(&edma3tc0_module);
	prcm_module_enable(&edma3tc1_module);
	prcm_module_enable(&edma3tc2_module);
#endif

	val = EDMA_READ(sc, EDMA_CCCFG_REG);
	if (val & EDMA_CCCFG_CHMAP_EXIST) {
		for (idx = 0; idx < NUM_DMA_CHANNELS; idx++) {
			EDMA_WRITE(sc, EDMA_DCHMAP_REG(idx),
			    __SHIFTIN(0, EDMA_DCHMAP_PAENTRY));
		}
	}

	memset(&param, 0, sizeof(param));
	param.ep_bcnt = 1;
	for (idx = 0; idx < NUM_PARAM_SETS; idx++) {
		edma_write_param(sc, idx, &param);
	}

	/* reserve PaRAM entry 0 for dummy slot */
	edma_bit_set(sc->sc_parammask, 0);
	for (idx = 1; idx <= 32; idx++) {
		edma_bit_set(sc->sc_parammask, idx);
	}
}

/*
 * Write a PaRAM entry
 */
static void
edma_write_param(struct edma_softc *sc,
    unsigned int idx, const struct edma_param *ep)
{
	EDMA_WRITE(sc, EDMA_PARAM_OPT_REG(idx), ep->ep_opt);
	EDMA_WRITE(sc, EDMA_PARAM_SRC_REG(idx), ep->ep_src);
	EDMA_WRITE(sc, EDMA_PARAM_CNT_REG(idx),
	    __SHIFTIN(ep->ep_bcnt, EDMA_PARAM_CNT_BCNT) |
	    __SHIFTIN(ep->ep_acnt, EDMA_PARAM_CNT_ACNT));
	EDMA_WRITE(sc, EDMA_PARAM_DST_REG(idx), ep->ep_dst);
	EDMA_WRITE(sc, EDMA_PARAM_BIDX_REG(idx),
	    __SHIFTIN(ep->ep_dstbidx, EDMA_PARAM_BIDX_DSTBIDX) |
	    __SHIFTIN(ep->ep_srcbidx, EDMA_PARAM_BIDX_SRCBIDX));
	EDMA_WRITE(sc, EDMA_PARAM_LNK_REG(idx),
	    __SHIFTIN(ep->ep_bcntrld, EDMA_PARAM_LNK_BCNTRLD) |
	    __SHIFTIN(ep->ep_link, EDMA_PARAM_LNK_LINK));
	EDMA_WRITE(sc, EDMA_PARAM_CIDX_REG(idx),
	    __SHIFTIN(ep->ep_dstcidx, EDMA_PARAM_CIDX_DSTCIDX) |
	    __SHIFTIN(ep->ep_srccidx, EDMA_PARAM_CIDX_SRCCIDX));
	EDMA_WRITE(sc, EDMA_PARAM_CCNT_REG(idx),
	    __SHIFTIN(ep->ep_ccnt, EDMA_PARAM_CCNT_CCNT));
}

static bool
edma_bit_isset(uint32_t *bits, unsigned int bit)
{
	return !!(bits[bit >> 5] & (1 << (bit & 0x1f)));
}

static void
edma_bit_set(uint32_t *bits, unsigned int bit)
{
	bits[bit >> 5] |= (1 << (bit & 0x1f));
}

static void
edma_bit_clr(uint32_t *bits, unsigned int bit)
{
	bits[bit >> 5] &= ~(1 << (bit & 0x1f));
}

static int
edma_intr(void *priv)
{
	struct edma_softc *sc = priv;
	uint64_t ipr, ier;
	int bit, idx;

	ipr = EDMA_READ(sc, EDMA_IPR_REG);
	ipr |= (uint64_t)EDMA_READ(sc, EDMA_IPRH_REG) << 32;
	if (ipr == 0)
		return 0;

	ier = EDMA_READ(sc, EDMA_IER_REG);
	ier |= (uint64_t)EDMA_READ(sc, EDMA_IERH_REG) << 32;

	DPRINTF(2, (sc->sc_dev, "ipr = 0x%016llx ier 0x%016llx\n", ipr, ier));

	EDMA_WRITE(sc, EDMA_ICR_REG, ipr & 0xffffffff);
	EDMA_WRITE(sc, EDMA_ICRH_REG, ipr >> 32);

	while ((bit = ffs64(ipr)) != 0) {
		idx = bit - 1;
		ipr &= ~__BIT(idx);
		if (!(ier & __BIT(idx)))
			continue;
		if (!edma_bit_isset(sc->sc_dmamask, idx))
			continue;

		sc->sc_dma[idx].ch_callback(sc->sc_dma[idx].ch_callbackarg);
	}

	EDMA_WRITE(sc, EDMA_IEVAL_REG, EDMA_IEVAL_EVAL);

	return 1;
}

static int
edma_mperr_intr(void *priv)
{
	printf(" ===== edma mperr!\n");
	return 0;
}

static int
edma_errint_intr(void *priv)
{
	printf(" ===== edma errint!\n");
	return 0;
}


/*
 * Allocate a DMA channel. Currently only DMA types are supported, not QDMA.
 * Returns NULL on failure.
 */
struct edma_channel *
edma_channel_alloc(enum edma_type type, unsigned int drq,
    void (*cb)(void *), void *cbarg)
{
	struct edma_softc *sc;
	device_t dev;
	struct edma_channel *ch = NULL;

	KASSERT(drq < __arraycount(sc->sc_dma));
	KASSERT(type == EDMA_TYPE_DMA);	/* QDMA not implemented */
	KASSERT(cb != NULL);
	KASSERT(cbarg != NULL);

	dev = device_find_by_driver_unit("edma", 0);
	if (dev == NULL)
		return NULL;
	sc = device_private(dev);

	mutex_enter(&sc->sc_lock);
	if (!edma_bit_isset(sc->sc_dmamask, drq)) {
		ch = &sc->sc_dma[drq];
		KASSERT(ch->ch_callback == NULL);
		KASSERT(ch->ch_index == drq);
		ch->ch_callback = cb;
		ch->ch_callbackarg = cbarg;
		edma_bit_set(sc->sc_dmamask, drq);
	}

	if (ch == NULL)
		goto done;

	EDMA_WRITE(sc, EDMA_DRAE_REG(0), sc->sc_dmamask[0]);
	EDMA_WRITE(sc, EDMA_DRAEH_REG(0), sc->sc_dmamask[1]);

	if (ch->ch_index < 32) {
		EDMA_WRITE(sc, EDMA_ICR_REG, __BIT(ch->ch_index));
		EDMA_WRITE(sc, EDMA_IESR_REG, __BIT(ch->ch_index));
	} else {
		EDMA_WRITE(sc, EDMA_ICRH_REG, __BIT(ch->ch_index - 32));
		EDMA_WRITE(sc, EDMA_IESRH_REG, __BIT(ch->ch_index - 32));
	}

done:
	mutex_exit(&sc->sc_lock);

	return ch;
}

/*
 * Free a DMA channel allocated with edma_channel_alloc
 */
void
edma_channel_free(struct edma_channel *ch)
{
	struct edma_softc *sc = ch->ch_sc;

	KASSERT(ch->ch_nparams == 0);

	mutex_enter(&sc->sc_lock);
	if (ch->ch_index < 32) {
		EDMA_WRITE(sc, EDMA_IECR_REG, __BIT(ch->ch_index));
	} else {
		EDMA_WRITE(sc, EDMA_IECRH_REG, __BIT(ch->ch_index - 32));
	}
	ch->ch_callback = NULL;
	ch->ch_callbackarg = NULL;
	edma_bit_clr(sc->sc_dmamask, ch->ch_index);
	mutex_exit(&sc->sc_lock);
}

/*
 * Allocate a PaRAM entry. The driver artifically restricts the number
 * of PaRAM entries available for each channel to MAX_PARAM_PER_CHANNEL.
 * If the number of entries for the channel has been exceeded, or there
 * are no entries available, 0xffff is returned.
 */
uint16_t
edma_param_alloc(struct edma_channel *ch)
{
	struct edma_softc *sc = ch->ch_sc;
	uint16_t param_entry = 0xffff;
	int idx;

	if (ch->ch_nparams == MAX_PARAM_PER_CHANNEL)
		return param_entry;

	mutex_enter(&sc->sc_lock);
	for (idx = 0; idx < NUM_PARAM_SETS; idx++) {
		if (!edma_bit_isset(sc->sc_parammask, idx)) {
			param_entry = idx;
			edma_bit_set(sc->sc_parammask, idx);
			ch->ch_nparams++;
			break;
		}
	}
	mutex_exit(&sc->sc_lock);

	return param_entry;
}

/*
 * Free a PaRAM entry allocated with edma_param_alloc
 */
void
edma_param_free(struct edma_channel *ch, uint16_t param_entry)
{
	struct edma_softc *sc = ch->ch_sc;

	KASSERT(param_entry < NUM_PARAM_SETS);
	KASSERT(ch->ch_nparams > 0);
	KASSERT(edma_bit_isset(sc->sc_parammask, param_entry));

	mutex_enter(&sc->sc_lock);
	edma_bit_clr(sc->sc_parammask, param_entry);
	ch->ch_nparams--;
	mutex_exit(&sc->sc_lock);
}

/*
 * Update a PaRAM entry register set with caller-provided values
 */
void
edma_set_param(struct edma_channel *ch, uint16_t param_entry,
    struct edma_param *ep)
{
	struct edma_softc *sc = ch->ch_sc;

	KASSERT(param_entry < NUM_PARAM_SETS);
	KASSERT(ch->ch_nparams > 0);
	KASSERT(edma_bit_isset(sc->sc_parammask, param_entry));

	DPRINTF(1, (sc->sc_dev, "write param entry ch# %d pe %d: 0x%08x -> 0x%08x (%u, %u, %u)\n", ch->ch_index, param_entry, ep->ep_src, ep->ep_dst, ep->ep_acnt, ep->ep_bcnt, ep->ep_ccnt));
	edma_write_param(sc, param_entry, ep);
}

/*
 * Enable a DMA channel: Point channel to the PaRam entry,
 * clear error if any, and only set the Event Enable bit.
 * The Even will either be generated by hardware, or with
 * edma_transfer_start()
 */
int
edma_transfer_enable(struct edma_channel *ch, uint16_t param_entry)
{
	struct edma_softc *sc = ch->ch_sc;
	bus_size_t off = (ch->ch_index < 32 ? 0 : 4);
	uint32_t bit = __BIT(ch->ch_index < 32 ?
			     ch->ch_index : ch->ch_index - 32);

	DPRINTF(1, (sc->sc_dev, "enable transfer ch# %d off %d bit %x pe %d\n", ch->ch_index, (int)off, bit, param_entry));

	EDMA_WRITE(sc, EDMA_DCHMAP_REG(ch->ch_index),
	    __SHIFTIN(param_entry, EDMA_DCHMAP_PAENTRY));

	uint32_t ccerr = EDMA_READ(sc, EDMA_CCERR_REG);
	if (ccerr) {
		device_printf(sc->sc_dev, " !!! CCER %08x\n", ccerr);
		EDMA_WRITE(sc, EDMA_CCERRCLR_REG, ccerr);
	}

	EDMA_WRITE(sc, EDMA_EESR_REG + off, bit);
	return 0;
}

/*
 * Software-start a DMA channel: Set the Event bit.
 */
int
edma_transfer_start(struct edma_channel *ch)
{
	struct edma_softc *sc = ch->ch_sc;
	bus_size_t off = (ch->ch_index < 32 ? 0 : 4);
	uint32_t bit = __BIT(ch->ch_index < 32 ?
			     ch->ch_index : ch->ch_index - 32);

	DPRINTF(1, (sc->sc_dev, "start transfer ch# %d off %d bit %x pe %d\n", ch->ch_index, (int)off, bit));

	EDMA_WRITE(sc, EDMA_ESR_REG + off, bit);
	return 0;
}

/*
 * Halt a DMA transfer. Called after successfull transfer, or to abort
 * a transfer.
 */
void
edma_halt(struct edma_channel *ch)
{
	struct edma_softc *sc = ch->ch_sc;
	bus_size_t off = (ch->ch_index < 32 ? 0 : 4);
	uint32_t bit = __BIT(ch->ch_index < 32 ?
			     ch->ch_index : ch->ch_index - 32);

	EDMA_WRITE(sc, EDMA_EECR_REG + off, bit);
	EDMA_WRITE(sc, EDMA_ECR_REG + off, bit);
	EDMA_WRITE(sc, EDMA_SECR_REG + off, bit);
	EDMA_WRITE(sc, EDMA_EMCR_REG + off, bit);

	EDMA_WRITE(sc, EDMA_DCHMAP_REG(ch->ch_index),
	    __SHIFTIN(0, EDMA_DCHMAP_PAENTRY));
}

uint8_t
edma_channel_index(struct edma_channel *ch)
{
	return ch->ch_index;
}

void
edma_dump(struct edma_channel *ch)
{
	static const struct {
		const char *name;
		uint16_t off;
	} regs[] = {
		{ "ER", EDMA_ER_REG },
		{ "ERH", EDMA_ERH_REG },
		{ "EER", EDMA_EER_REG },
		{ "EERH", EDMA_EERH_REG },
		{ "SER", EDMA_SER_REG },
		{ "SERH", EDMA_SERH_REG },
		{ "IER", EDMA_IER_REG },
		{ "IERH", EDMA_IERH_REG },
		{ "IPR", EDMA_IPR_REG },
		{ "IPRH", EDMA_IPRH_REG },
		{ "CCERR", EDMA_CCERR_REG },
		{ "CCSTAT", EDMA_CCSTAT_REG },
		{ "DRAE0", EDMA_DRAE_REG(0) },
		{ "DRAEH0", EDMA_DRAEH_REG(0) },
		{ NULL, 0 }
	};
	struct edma_softc *sc = ch->ch_sc;
	int i;

	for (i = 0; regs[i].name; i++) {
		device_printf(sc->sc_dev, "%s: %08x\n",
		    regs[i].name, EDMA_READ(sc, regs[i].off));
	}
	device_printf(sc->sc_dev, "DCHMAP%d: %08x\n", ch->ch_index,
	    EDMA_READ(sc, EDMA_DCHMAP_REG(ch->ch_index)));
}

void
edma_dump_param(struct edma_channel *ch, uint16_t param_entry)
{
	struct {
		const char *name;
		uint16_t off;
	} regs[] = {
		{ "OPT", EDMA_PARAM_OPT_REG(param_entry) },
		{ "CNT", EDMA_PARAM_CNT_REG(param_entry) },
		{ "DST", EDMA_PARAM_DST_REG(param_entry) },
		{ "BIDX", EDMA_PARAM_BIDX_REG(param_entry) },
		{ "LNK", EDMA_PARAM_LNK_REG(param_entry) },
		{ "CIDX", EDMA_PARAM_CIDX_REG(param_entry) },
		{ "CCNT", EDMA_PARAM_CCNT_REG(param_entry) },
		{ NULL, 0 }
	};
	struct edma_softc *sc = ch->ch_sc;
	int i;

	for (i = 0; regs[i].name; i++) {
		device_printf(sc->sc_dev, "%s%d: %08x\n",
		    regs[i].name, param_entry, EDMA_READ(sc, regs[i].off));
	}
}
