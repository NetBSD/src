/*	$NetBSD: dwcsata.c,v 1.1 2026/06/14 00:02:35 rkujawa Exp $	*/

/*
 * Copyright (c) 2025, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * 460EX on-chip Synopsys DWC SATA-II host
 *
 * DMA is broken despite many attempts to get it fixed.
 *
 * Btw. SATA PHY shares its SerDes lane with PCIE0; firmware routes
 * and initializes it. Only one of these can work at a given time.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dwcsata.c,v 1.1 2026/06/14 00:02:35 rkujawa Exp $");

#ifdef _KERNEL_OPT
#include "opt_dwcsata.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/bus.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/amcc460ex.h>
#include <powerpc/ibm4xx/dev/plbvar.h>
#include <powerpc/ibm4xx/dev/dwcsatareg.h>
#include <powerpc/ibm4xx/dev/dwcdmacreg.h>

#include <dev/ata/atavar.h>
#include <dev/ata/atareg.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>

#include "locators.h"

/* the DMAC channel wired to the SATA core's handshake interface */
#define	DWCSATA_DMACH		0
/*
 * Burst sizing (CTL SRC/DST_MSIZE encoding). 
 */
#define	DWCSATA_DMA_MEM_MSIZE	3	/* 16 items = 64 bytes, memory side */
#define	DWCSATA_DMA_FIFO_MSIZE	0	/* single DWORD, FIFO side */
/* matching FIFO-side burst in bytes for the SATA core's DBTSR */
#define	DWCSATA_DMA_FIFO_BURST	4	/* single DWORD */
/*
 * Worst case for a MAXPHYS transfer is one LLI per map segment plus
 * one extra per 8KB FIS boundary split; 64 is comfortably above both.
 */
#define	DWCSATA_NLLI		64

struct dwcsata_softc {
	struct wdc_softc sc_wdcdev;
	struct ata_channel *sc_chanlist[1];
	struct ata_channel sc_channel;
	struct wdc_regs sc_wdc_regs;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	bus_dma_tag_t sc_dmat;
	int sc_irq;		/* for interrupt-driven operation, later */

#ifndef DWCSATA_PIO_ONLY
	bus_space_handle_t sc_dmac_ioh;	/* companion AHB DMAC */
	bus_dmamap_t sc_dmamap_xfer;	/* current data buffer */
	struct dwcdmac_lli *sc_lli;	/* uncached LLI table */
	bus_addr_t sc_lli_phys;
	bus_dmamap_t sc_lli_map;
	bus_dma_segment_t sc_lli_seg;
	int sc_lli_nseg;
	uint32_t sc_dmadr_phys;		/* AHB address of the DMADR window */
	int sc_dma_flags;		/* WDC_DMA_* of the loaded transfer */
	int sc_dma_nlli;		/* LLIs in the loaded chain */
	bool sc_dma_loaded;		/* sc_dmamap_xfer is loaded */
	bool sc_dma_ok;			/* resources up, hooks registered */
	bool sc_dma_active;		/* between dma_start and teardown */
#endif
};

static int	dwcsata_match(device_t, cfdata_t, void *);
static void	dwcsata_attach(device_t, device_t, void *);
static void	dwcsata_probe(struct ata_channel *);
static void	dwcsata_reset(struct ata_channel *, int);

#ifndef DWCSATA_PIO_ONLY
CTASSERT(sizeof(struct dwcdmac_lli) == 28);

static bool	dwcsata_dma_setup(device_t, struct dwcsata_softc *,
		    struct plb_attach_args *);
static int	dwcsata_dma_init(void *, int, int, void *, size_t, int);
static void	dwcsata_dma_start(void *, int, int);
static int	dwcsata_dma_finish(void *, int, int, int);
static int	dwcsata_intr(void *);
static int	dwcsata_dmac_intr(void *);
#endif

CFATTACH_DECL_NEW(dwcsata, sizeof(struct dwcsata_softc),
    dwcsata_match, dwcsata_attach, NULL, NULL);

static struct powerpc_bus_space dwcsata_tag = {
	_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE,
	0x00000000,
};
static char dwcsata_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));

static int
dwcsata_match(device_t parent, cfdata_t match, void *aux)
{
	struct plb_attach_args *paa = aux;

	if (strcmp(paa->plb_name, match->cf_name) != 0)
		return 0;

	if (match->cf_loc[PLBCF_ADDR] == PLBCF_ADDR_DEFAULT)
		panic("dwcsata_match: wildcard addr not allowed");
	if (match->cf_loc[PLBCF_IRQ] == PLBCF_IRQ_DEFAULT)
		panic("dwcsata_match: wildcard IRQ not allowed");

	paa->plb_addr = match->cf_loc[PLBCF_ADDR];
	paa->plb_irq = match->cf_loc[PLBCF_IRQ];
	return 1;
}

#ifndef DWCSATA_PIO_ONLY

static inline uint32_t
dwcdmac_read(struct dwcsata_softc *sc, bus_size_t reg)
{

	return bus_space_read_4(sc->sc_iot, sc->sc_dmac_ioh, reg);
}

static inline void
dwcdmac_write(struct dwcsata_softc *sc, bus_size_t reg, uint32_t val)
{

	bus_space_write_4(sc->sc_iot, sc->sc_dmac_ioh, reg, val);
}

static void
dwcdmac_clear_intrs(struct dwcsata_softc *sc)
{

	dwcdmac_write(sc, DWCDMAC_CLEAR_TFR, DWCDMAC_CHANBIT(DWCSATA_DMACH));
	dwcdmac_write(sc, DWCDMAC_CLEAR_BLOCK, DWCDMAC_CHANBIT(DWCSATA_DMACH));
	dwcdmac_write(sc, DWCDMAC_CLEAR_SRCTRAN,
	    DWCDMAC_CHANBIT(DWCSATA_DMACH));
	dwcdmac_write(sc, DWCDMAC_CLEAR_DSTTRAN,
	    DWCDMAC_CHANBIT(DWCSATA_DMACH));
	dwcdmac_write(sc, DWCDMAC_CLEAR_ERR, DWCDMAC_CHANBIT(DWCSATA_DMACH));
}

static bool
dwcsata_dma_setup(device_t self, struct dwcsata_softc *sc,
    struct plb_attach_args *paa)
{
	int error;

	if (bus_space_map(sc->sc_iot, paa->plb_addr - DWCDMAC_OFFSET,
	    DWCDMAC_SIZE, 0, &sc->sc_dmac_ioh)) {
		aprint_error_dev(self, "can't map the AHB DMAC\n");
		return false;
	}

	aprint_debug_dev(self, "AHB DMAC id 0x%08x\n",
	    dwcdmac_read(sc, DWCDMAC_ID));

	/*
	 * 460EX errata workaround for concurrent AHB use
	 */
	{
		uint32_t ahbcfg = mfsdr(DCR_SDR0_AHB_CFG);

		aprint_normal_dev(self, "SDR0_AHB_CFG 0x%08x", ahbcfg);
		ahbcfg |= SDR0_AHB_CFG_A2P_INCR4;
		ahbcfg &= ~SDR0_AHB_CFG_A2P_PROT2;
		mtsdr(DCR_SDR0_AHB_CFG, ahbcfg);
		aprint_normal(" -> 0x%08x (460EX AHB errata)\n",
		    mfsdr(DCR_SDR0_AHB_CFG));

		aprint_normal_dev(self,
		    "arbiter: PLB4A0_ACR 0x%08x PLB4A1_ACR 0x%08x "
		    "SDR0_USB2HOST_CFG 0x%08x\n",
		    mfdcr(DCR_PLB4A0_ACR), mfdcr(DCR_PLB4A1_ACR),
		    mfsdr(DCR_SDR0_USB2HOST_CFG));
	}

	dwcdmac_write(sc, DWCDMAC_MASK_TFR, DWCDMAC_CH_ENABLE(DWCSATA_DMACH));
	dwcdmac_write(sc, DWCDMAC_MASK_BLOCK,
	    DWCDMAC_CH_DISABLE(DWCSATA_DMACH));
	dwcdmac_write(sc, DWCDMAC_MASK_SRCTRAN,
	    DWCDMAC_CH_DISABLE(DWCSATA_DMACH));
	dwcdmac_write(sc, DWCDMAC_MASK_DSTTRAN,
	    DWCDMAC_CH_DISABLE(DWCSATA_DMACH));
	dwcdmac_write(sc, DWCDMAC_MASK_ERR, DWCDMAC_CH_ENABLE(DWCSATA_DMACH));
	dwcdmac_clear_intrs(sc);
	dwcdmac_write(sc, DWCDMAC_CHEN, DWCDMAC_CH_DISABLE(DWCSATA_DMACH));
	dwcdmac_write(sc, DWCDMAC_DMACFG, DWCDMAC_DMACFG_EN);

	/* burst sizes the LLIs are built for */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_DBTSR,
	    DWCSATA_DBTSR_MWR(DWCSATA_DMA_FIFO_BURST) |
	    DWCSATA_DBTSR_MRD(DWCSATA_DMA_FIFO_BURST));

	sc->sc_dmadr_phys = DWCSATA_DMADR_PHYS(paa->plb_addr);

	/*
	 * The LLI table is fetched by the DMAC behind the CPU's back...
	 * map it uncached so building it needs no cache gymnastics.
	 */
	error = bus_dmamem_alloc(sc->sc_dmat,
	    DWCSATA_NLLI * sizeof(struct dwcdmac_lli), 8, 0,
	    &sc->sc_lli_seg, 1, &sc->sc_lli_nseg, BUS_DMA_NOWAIT);
	if (error)
		goto fail0;
	error = bus_dmamem_map(sc->sc_dmat, &sc->sc_lli_seg, sc->sc_lli_nseg,
	    DWCSATA_NLLI * sizeof(struct dwcdmac_lli), (void **)&sc->sc_lli,
	    BUS_DMA_NOWAIT | BUS_DMA_DONTCACHE);
	if (error)
		goto fail1;
	error = bus_dmamap_create(sc->sc_dmat,
	    DWCSATA_NLLI * sizeof(struct dwcdmac_lli), 1,
	    DWCSATA_NLLI * sizeof(struct dwcdmac_lli), 0, BUS_DMA_NOWAIT,
	    &sc->sc_lli_map);
	if (error)
		goto fail2;
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_lli_map, sc->sc_lli,
	    DWCSATA_NLLI * sizeof(struct dwcdmac_lli), NULL, BUS_DMA_NOWAIT);
	if (error)
		goto fail3;
	sc->sc_lli_phys = sc->sc_lli_map->dm_segs[0].ds_addr;

	error = bus_dmamap_create(sc->sc_dmat, MAXPHYS,
	    MAXPHYS / PAGE_SIZE + 8, 0x2000, 0x2000, BUS_DMA_NOWAIT,
	    &sc->sc_dmamap_xfer);
	if (error)
		goto fail4;

	return true;

fail4:
	bus_dmamap_unload(sc->sc_dmat, sc->sc_lli_map);
fail3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_lli_map);
fail2:
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_lli,
	    DWCSATA_NLLI * sizeof(struct dwcdmac_lli));
fail1:
	bus_dmamem_free(sc->sc_dmat, &sc->sc_lli_seg, sc->sc_lli_nseg);
fail0:
	bus_space_unmap(sc->sc_iot, sc->sc_dmac_ioh, DWCDMAC_SIZE);
	return false;
}

static int
dwcsata_dma_init(void *v, int channel, int drive, void *databuf,
    size_t datalen, int flags)
{
	struct dwcsata_softc *sc = v;
	bus_dmamap_t map = sc->sc_dmamap_xfer;
	const bool read = (flags & WDC_DMA_READ) != 0;
	const u_int lsize = curcpu()->ci_ci.dcache_line_size;
	uint32_t ctl, serror;
	u_int idx, fis_len;
	int i, error;

	KASSERT(channel == 0 && drive == 0);

	/*
	 * The channel enable self-clears at the end of the chain; if it
	 * is still set here, a previous abort failed to stop the engine.
	 */
	if (dwcdmac_read(sc, DWCDMAC_CHEN) & DWCDMAC_CHANBIT(DWCSATA_DMACH)) {
		aprint_error_dev(sc->sc_wdcdev.sc_atac.atac_dev,
		    "dma_init: channel still enabled (CHEN 0x%08x)\n",
		    dwcdmac_read(sc, DWCDMAC_CHEN));
		return EBUSY;
	}

	/*
	 * If the MI code bailed between dma_init and dma_start (e.g. a
	 * not-ready timeout) it never calls dma_finish, leaking the
	 * loaded map; recover instead of failing every later load.
	 */
	if (sc->sc_dma_loaded) {
		aprint_error_dev(sc->sc_wdcdev.sc_atac.atac_dev,
		    "dma_init: recovering leaked dmamap\n");
		bus_dmamap_unload(sc->sc_dmat, map);
		sc->sc_dma_loaded = false;
	}

	/* drop any latched SError bits before issuing the command */
	serror = bus_space_read_4(sc->sc_iot, sc->sc_ioh, DWCSATA_SERROR);
	if (serror & DWCSATA_ERRMR_ERR_BITS)
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_SERROR,
		    serror);

	error = bus_dmamap_load(sc->sc_dmat, map, databuf, datalen, NULL,
	    BUS_DMA_NOWAIT | BUS_DMA_STREAMING |
	    (read ? BUS_DMA_READ : BUS_DMA_WRITE));
	if (error) {
		aprint_error_dev(sc->sc_wdcdev.sc_atac.atac_dev,
		    "dma_init: dmamap_load failed: error %d "
		    "(buf %p len %zu)\n", error, databuf, datalen);
		return error;
	}
	sc->sc_dma_loaded = true;

	/*
	 * The DMAC moves 32-bit items, so segments must be word-aligned.
	 */
	for (i = 0; i < map->dm_nsegs; i++) {
		if ((map->dm_segs[i].ds_addr | map->dm_segs[i].ds_len) & 3)
			goto fallback;
		if (read && ((map->dm_segs[i].ds_addr |
		    map->dm_segs[i].ds_len) & (lsize - 1)))
			goto fallback;
	}

	bus_dmamap_sync(sc->sc_dmat, map, 0, datalen,
	    read ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	if (read) {
		/* device->memory: source is the FIFO */
		ctl = DWCDMAC_CTL_TTFC(DWCDMAC_TTFC_P2M_DMAC) |
		    DWCDMAC_CTL_SMS(DWCDMAC_MS_PERIPH) |
		    DWCDMAC_CTL_DMS(DWCDMAC_MS_MEM) |
		    DWCDMAC_CTL_SINC_NOCHANGE |
		    DWCDMAC_CTL_SRC_MSIZE(DWCSATA_DMA_FIFO_MSIZE) |
		    DWCDMAC_CTL_DST_MSIZE(DWCSATA_DMA_MEM_MSIZE);
	} else {
		/* memory->device: destination is the FIFO */
		ctl = DWCDMAC_CTL_TTFC(DWCDMAC_TTFC_M2P_PER) |
		    DWCDMAC_CTL_SMS(DWCDMAC_MS_MEM) |
		    DWCDMAC_CTL_DMS(DWCDMAC_MS_PERIPH) |
		    DWCDMAC_CTL_DINC_NOCHANGE |
		    DWCDMAC_CTL_SRC_MSIZE(DWCSATA_DMA_MEM_MSIZE) |
		    DWCDMAC_CTL_DST_MSIZE(DWCSATA_DMA_FIFO_MSIZE);
	}
	ctl |= DWCDMAC_CTL_SRC_TRWID(2) | DWCDMAC_CTL_DST_TRWID(2) |
	    DWCDMAC_CTL_INT_EN |
	    DWCDMAC_CTL_LLP_SRC_EN | DWCDMAC_CTL_LLP_DST_EN;

	idx = 0;
	fis_len = 0;
	for (i = 0; i < map->dm_nsegs; i++) {
		uint32_t addr = map->dm_segs[i].ds_addr;
		bus_size_t left = map->dm_segs[i].ds_len;

		while (left > 0) {
			struct dwcdmac_lli *lli;
			uint32_t len;

			if (idx >= DWCSATA_NLLI) {
				aprint_error_dev(
				    sc->sc_wdcdev.sc_atac.atac_dev,
				    "dma_init: LLI overflow (len %zu, "
				    "%d segs)\n", datalen, map->dm_nsegs);
				goto fallback;
			}

			len = uimin(left, DWCDMAC_MAX_BLOCK_ITEMS * 4);
			if (fis_len + len > 8192) {
				len = 8192 - fis_len;
				fis_len = 0;
			} else {
				fis_len += len;
				if (fis_len == 8192)
					fis_len = 0;
			}

			lli = &sc->sc_lli[idx];
			if (read) {
				lli->sar = htole32(sc->sc_dmadr_phys);
				lli->dar = htole32(addr);
			} else {
				lli->sar = htole32(addr);
				lli->dar = htole32(sc->sc_dmadr_phys);
			}
			lli->llp = htole32((sc->sc_lli_phys +
			    (idx + 1) * sizeof(struct dwcdmac_lli)) |
			    DWCDMAC_MS_MEM);
			lli->ctl_lo = htole32(ctl);
			lli->ctl_hi = htole32(DWCDMAC_CTL_BLOCK_TS(len / 4));
			lli->dstat_lo = 0;
			lli->dstat_hi = 0;

			idx++;
			addr += len;
			left -= len;
		}
	}
	KASSERT(idx > 0);
	sc->sc_lli[idx - 1].llp = 0;
	sc->sc_lli[idx - 1].ctl_lo &=
	    htole32(~(DWCDMAC_CTL_LLP_SRC_EN | DWCDMAC_CTL_LLP_DST_EN));
	/* the table is uncached; order the stores before the enables */
	__asm volatile("sync" ::: "memory");

	dwcdmac_clear_intrs(sc);
	dwcdmac_write(sc, DWCDMAC_CFG_HI(DWCSATA_DMACH),
	    DWCDMAC_CFG_HS_SRC(DWCSATA_DMACH) |
	    DWCDMAC_CFG_HS_DST(DWCSATA_DMACH) |
	    DWCDMAC_CFG_PROTCTL | DWCDMAC_CFG_FCMODE_REQ);
	dwcdmac_write(sc, DWCDMAC_CFG(DWCSATA_DMACH),
	    DWCDMAC_CFG_CH_PRIOR(DWCSATA_DMACH));
	dwcdmac_write(sc, DWCDMAC_LLP(DWCSATA_DMACH),
	    sc->sc_lli_phys | DWCDMAC_MS_MEM);
	dwcdmac_write(sc, DWCDMAC_CTL(DWCSATA_DMACH),
	    DWCDMAC_CTL_LLP_SRC_EN | DWCDMAC_CTL_LLP_DST_EN);
	dwcdmac_write(sc, DWCDMAC_CTL_HI(DWCSATA_DMACH), 0);

	sc->sc_dma_flags = flags;
	sc->sc_dma_nlli = idx;
	return 0;

fallback:
	bus_dmamap_unload(sc->sc_dmat, map);
	sc->sc_dma_loaded = false;
	return EINVAL;
}

static void
dwcsata_dma_dump(struct dwcsata_softc *sc, uint32_t tfr, uint32_t err)
{
	device_t dev = sc->sc_wdcdev.sc_atac.atac_dev;
	uint32_t intpr;
	int i;

	intpr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, DWCSATA_INTPR);
	aprint_error_dev(dev,
	    "DMA state: CHEN 0x%08x tfr/blk/err 0x%08x/0x%08x/0x%08x "
	    "CTL 0x%08x.%08x LLP 0x%08x CFG 0x%08x.%08x DMACR 0x%08x "
	    "INTPR 0x%08x (erraddr 0x%03x) SError 0x%08x\n",
	    dwcdmac_read(sc, DWCDMAC_CHEN), tfr,
	    dwcdmac_read(sc, DWCDMAC_RAW_BLOCK), err,
	    dwcdmac_read(sc, DWCDMAC_CTL_HI(DWCSATA_DMACH)),
	    dwcdmac_read(sc, DWCDMAC_CTL(DWCSATA_DMACH)),
	    dwcdmac_read(sc, DWCDMAC_LLP(DWCSATA_DMACH)),
	    dwcdmac_read(sc, DWCDMAC_CFG_HI(DWCSATA_DMACH)),
	    dwcdmac_read(sc, DWCDMAC_CFG(DWCSATA_DMACH)),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, DWCSATA_DMACR),
	    intpr, DWCSATA_INTPR_ERRADDR_GET(intpr),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, DWCSATA_SERROR));
	for (i = 0; i < sc->sc_dma_nlli; i++) {
		const struct dwcdmac_lli *lli = &sc->sc_lli[i];

		aprint_error_dev(dev,
		    "  lli[%d]: sar 0x%08x dar 0x%08x llp 0x%08x "
		    "ctl 0x%08x.%08x\n", i,
		    le32toh(lli->sar), le32toh(lli->dar), le32toh(lli->llp),
		    le32toh(lli->ctl_hi), le32toh(lli->ctl_lo));
	}
}

static void
dwcsata_dma_start(void *v, int channel, int drive)
{
	struct dwcsata_softc *sc = v;
	const bool read = (sc->sc_dma_flags & WDC_DMA_READ) != 0;

	/* open the FIS data channel in the SATA core first... */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_DMACR,
	    read ? DWCSATA_DMACR_RX_START : DWCSATA_DMACR_TX_START);
	/* ...then let the AHB DMAC run the linked list */
	dwcdmac_write(sc, DWCDMAC_CHEN, DWCDMAC_CH_ENABLE(DWCSATA_DMACH));
	sc->sc_dma_active = true;
}

static int
dwcsata_dma_finish(void *v, int channel, int drive, int force)
{
	struct dwcsata_softc *sc = v;
	const bool read = (sc->sc_dma_flags & WDC_DMA_READ) != 0;
	uint32_t tfr, err, dmacr, intpr;
	int i, status = 0;

	if (!sc->sc_dma_active)
		return 0;

	if (force == WDC_DMAEND_END) {
		/*
		 * Plain register polling: this core does not write
		 * the DONE bit back to the LLI (measured: the memory
		 * fast path never fired), and bridge traffic during
		 * the transfer has been ruled out as the stall
		 * trigger.
		 */
		tfr = dwcdmac_read(sc, DWCDMAC_RAW_TFR);
		err = dwcdmac_read(sc, DWCDMAC_RAW_ERR);
		/* still running?  wdc_dmawait() polls us again */
		if (((tfr | err) & DWCDMAC_CHANBIT(DWCSATA_DMACH)) == 0)
			return WDC_DMAST_NOIRQ;
		/* a detectable error deserves the full dump */
		if (err & DWCDMAC_CHANBIT(DWCSATA_DMACH))
			dwcsata_dma_dump(sc, tfr, err);
	} else {
		tfr = dwcdmac_read(sc, DWCDMAC_RAW_TFR);
		err = dwcdmac_read(sc, DWCDMAC_RAW_ERR);
		const uint32_t chanbit = DWCDMAC_CHANBIT(DWCSATA_DMACH);

		/*
		 * Abort. Close the SATA-side channel first so the
		 * handshake stops feeding the DMAC, then follow the
		 * databook: suspend the channel, let its FIFO drain,
		 * and only then clear the enable.
		 */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_DMACR,
		    DWCSATA_DMACR_TXRXCH_CLEAR);
		dwcdmac_write(sc, DWCDMAC_CFG(DWCSATA_DMACH),
		    dwcdmac_read(sc, DWCDMAC_CFG(DWCSATA_DMACH)) |
		    DWCDMAC_CFG_CH_SUSP);
		for (i = 0; i < 100; i++) {
			if (dwcdmac_read(sc, DWCDMAC_CFG(DWCSATA_DMACH)) &
			    DWCDMAC_CFG_FIFO_EMPTY)
				break;
			delay(10);
		}
		dwcdmac_write(sc, DWCDMAC_CHEN,
		    DWCDMAC_CH_DISABLE(DWCSATA_DMACH));
		for (i = 0; i < 100; i++) {
			if ((dwcdmac_read(sc, DWCDMAC_CHEN) & chanbit) == 0)
				break;
			delay(10);
		}
		if (dwcdmac_read(sc, DWCDMAC_CHEN) & chanbit) {
			/*
			 * This happens. Not sure why.
			 */
			dwcdmac_write(sc, DWCDMAC_DMACFG, 0);
			delay(100);
			dwcdmac_write(sc, DWCDMAC_DMACFG, DWCDMAC_DMACFG_EN);
			if (force != WDC_DMAEND_ABRT_QUIET)
				aprint_error_dev(
				    sc->sc_wdcdev.sc_atac.atac_dev,
				    "DMA channel wedged, reset the DMAC\n");
		}
		/* drop the suspend again for the next transfer */
		dwcdmac_write(sc, DWCDMAC_CFG(DWCSATA_DMACH),
		    dwcdmac_read(sc, DWCDMAC_CFG(DWCSATA_DMACH)) &
		    ~DWCDMAC_CFG_CH_SUSP);

		if (force != WDC_DMAEND_ABRT_QUIET) {
			dwcsata_dma_dump(sc, tfr, err);
			if ((tfr & chanbit) == 0)
				status |= WDC_DMAST_NOIRQ;
		}
	}

	if (err & DWCDMAC_CHANBIT(DWCSATA_DMACH))
		status |= WDC_DMAST_ERR;

	/* close the SATA core side, keeping TXMODE set */
	dmacr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, DWCSATA_DMACR);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_DMACR,
	    read ? DWCSATA_DMACR_RX_CLEAR(dmacr) :
	    DWCSATA_DMACR_TX_CLEAR(dmacr));

	intpr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, DWCSATA_INTPR);
	if (intpr & DWCSATA_INTPR_ERR)
		status |= WDC_DMAST_ERR;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_INTPR,
	    DWCSATA_INTPR_DMAT | DWCSATA_INTPR_ERR);
	if ((status & WDC_DMAST_ERR) != 0 &&
	    force != WDC_DMAEND_ABRT_QUIET) {
		uint32_t serror = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    DWCSATA_SERROR);
		aprint_error_dev(sc->sc_wdcdev.sc_atac.atac_dev,
		    "DMA error: DMAC err 0x%08x INTPR 0x%08x SError 0x%08x\n",
		    err, intpr, serror);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_SERROR,
		    serror);
	}

	dwcdmac_clear_intrs(sc);

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap_xfer, 0,
	    sc->sc_dmamap_xfer->dm_mapsize,
	    read ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, sc->sc_dmamap_xfer);
	sc->sc_dma_loaded = false;
	sc->sc_dma_active = false;

	return status;
}

static int
dwcsata_intr(void *arg)
{
	struct dwcsata_softc *sc = arg;
	struct ata_channel *chp = &sc->sc_channel;
	struct wdc_regs *wdr = &sc->sc_wdc_regs;
	uint32_t intpr;

	intpr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, DWCSATA_INTPR);
	if (intpr == 0)
		return 0;		/* nothing pending; not our line */

	if (intpr & (DWCSATA_INTPR_ERR | DWCSATA_INTPR_CMDABORT |
	    DWCSATA_INTPR_PRIMERR)) {
		uint32_t serror = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    DWCSATA_SERROR);
		aprint_error_dev(sc->sc_wdcdev.sc_atac.atac_dev,
		    "controller error interrupt: INTPR 0x%08x SError 0x%08x\n",
		    intpr, serror);
		if (serror & DWCSATA_ERRMR_ERR_BITS)
			bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			    DWCSATA_SERROR, serror);
	}

	/*
	 * Acknowledge the latched INTPR status bits
	 */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_INTPR,
	    intpr & ~DWCSATA_INTPR_ERRADDR);

	if (chp->ch_flags & ATACH_DMA_WAIT) {
		/*
		 * A DMA transfer is in flight
		 */
		(void)bus_space_read_1(wdr->cmd_iot,
		    wdr->cmd_iohs[wd_status], 0);
		return 1;
	}

	/*
	 * No DMA pending: a PIO command completion or a stray drive
	 * interrupt. wdcintr reads the status register.
	 */
	return wdcintr(chp);
}

static int
dwcsata_dmac_intr(void *arg)
{
	struct dwcsata_softc *sc = arg;
	struct ata_channel *chp = &sc->sc_channel;
	const uint32_t chanbit = DWCDMAC_CHANBIT(DWCSATA_DMACH);
	int rv;

	if (((dwcdmac_read(sc, DWCDMAC_RAW_TFR) |
	    dwcdmac_read(sc, DWCDMAC_RAW_ERR)) & chanbit) == 0)
		return 0;		/* not our channel */

	rv = wdcintr(chp);

	if ((dwcdmac_read(sc, DWCDMAC_RAW_TFR) |
	    dwcdmac_read(sc, DWCDMAC_RAW_ERR)) & chanbit) {
		dwcdmac_write(sc, DWCDMAC_CLEAR_TFR, chanbit);
		dwcdmac_write(sc, DWCDMAC_CLEAR_ERR, chanbit);
	}
	return rv ? rv : 1;
}

#endif	/* !DWCSATA_PIO_ONLY */

static void
dwcsata_attach(device_t parent, device_t self, void *aux)
{
	struct dwcsata_softc *sc = device_private(self);
	struct plb_attach_args *paa = aux;
	struct wdc_regs *wdr;
	struct ata_channel *chp = &sc->sc_channel;
	uint32_t idr, versionr;
	int i;

	sc->sc_dmat = paa->plb_dmat;
	sc->sc_irq = paa->plb_irq;

	/* the window also covers the companion AHB DMAC below the core */
	dwcsata_tag.pbs_base = paa->plb_addr - DWCDMAC_OFFSET;
	dwcsata_tag.pbs_limit = paa->plb_addr + DWCSATA_SIZE;
	sc->sc_iot = &dwcsata_tag;

	if (bus_space_init(&dwcsata_tag, "dwcsata", dwcsata_ex_storage,
	      sizeof(dwcsata_ex_storage)) ||
	    bus_space_map(sc->sc_iot, paa->plb_addr, DWCSATA_SIZE, 0,
	      &sc->sc_ioh)) {
		aprint_error(": can't map registers\n");
		return;
	}

	versionr = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    DWCSATA_VERSIONR);
	if (versionr == 0 || versionr == 0xffffffff) {
		aprint_normal(": DWC SATA core not responding\n");
		return;
	}
	idr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, DWCSATA_IDR);
	aprint_normal(": DWC SATA-II controller, core version %c.%c%c "
	    "id 0x%02x\n",
	    (int)(versionr >> 24) & 0xff, (int)(versionr >> 16) & 0xff,
	    (int)(versionr >> 8) & 0xff, idr & 0xff);

	sc->sc_wdcdev.regs = wdr = &sc->sc_wdc_regs;

	wdr->cmd_iot = wdr->ctl_iot = sc->sc_iot;
	wdr->cmd_baseioh = sc->sc_ioh;
	wdr->cmd_ios = DWCSATA_SIZE;
	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
		    DWCSATA_CDR_BASE + i * DWCSATA_CDR_STRIDE,
		    DWCSATA_CDR_STRIDE, &wdr->cmd_iohs[i]) != 0) {
			aprint_error_dev(self,
			    "couldn't subregion registers\n");
			return;
		}
	}
	wdc_init_shadow_regs(wdr);

	if (bus_space_subregion(wdr->ctl_iot, sc->sc_ioh, DWCSATA_CLR0, 4,
	      &wdr->ctl_ioh) ||
	    bus_space_subregion(sc->sc_iot, sc->sc_ioh, DWCSATA_SSTATUS, 4,
	      &wdr->sata_status) ||
	    bus_space_subregion(sc->sc_iot, sc->sc_ioh, DWCSATA_SERROR, 4,
	      &wdr->sata_error) ||
	    bus_space_subregion(sc->sc_iot, sc->sc_ioh, DWCSATA_SCONTROL, 4,
	      &wdr->sata_control)) {
		aprint_error_dev(self, "couldn't subregion registers\n");
		return;
	}
	wdr->ctl_ios = 4;
	wdr->sata_iot = sc->sc_iot;
	wdr->sata_baseioh = sc->sc_ioh;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_INTMR, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_ERRMR,
	    DWCSATA_ERRMR_ERR_BITS);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_DMACR,
	    DWCSATA_DMACR_TXRXCH_CLEAR);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_SERROR,
	    0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_INTPR,
	    0xffffffff);

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.sc_atac.atac_cap = ATAC_CAP_DATA16 | ATAC_CAP_NOIRQ;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 4;

#ifndef DWCSATA_PIO_ONLY
	/*
	 * No atac_set_modes: SATA carries the mode in the FIS, there are
	 * no host timings to program, and without the hook the MI code
	 * also skips the (unneeded) SET FEATURES xfer-mode command.
	 */
	if (dwcsata_dma_setup(self, sc, paa)) {
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DMA |
		    ATAC_CAP_UDMA;
		sc->sc_wdcdev.sc_atac.atac_dma_cap = 2;
		sc->sc_wdcdev.sc_atac.atac_udma_cap = 6;
		sc->sc_wdcdev.dma_arg = sc;
		sc->sc_wdcdev.dma_init = dwcsata_dma_init;
		sc->sc_wdcdev.dma_start = dwcsata_dma_start;
		sc->sc_wdcdev.dma_finish = dwcsata_dma_finish;
		chp->ch_flags |= ATACH_DMA_BEFORE_CMD;
		sc->sc_dma_ok = true;
	} else
		aprint_error_dev(self, "DMA setup failed, PIO only\n");
#endif

	sc->sc_chanlist[0] = chp;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.sc_atac.atac_probe = dwcsata_probe;
	sc->sc_wdcdev.wdc_maxdrives = 1;	/* point-to-point, no PMP */
	sc->sc_wdcdev.reset = dwcsata_reset;	/* settle the core post-SRST */

	chp->ch_channel = 0;
	chp->ch_atac = &sc->sc_wdcdev.sc_atac;

#ifndef DWCSATA_PIO_ONLY
	/*
	 * With the DMA engine up, switch to interrupt-driven operation.
	 */
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.sc_atac.atac_cap &= ~ATAC_CAP_NOIRQ;

		bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_SERROR,
		    0xffffffff);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_INTPR,
		    0xffffffff);
		(void)bus_space_read_1(wdr->cmd_iot, wdr->cmd_iohs[wd_status],
		    0);

		intr_establish_xname(sc->sc_irq, IST_LEVEL, IPL_BIO,
		    dwcsata_intr, sc, device_xname(self));
		intr_establish_xname(AMCC460EX_SATA_DMA_IRQ, IST_LEVEL,
		    IPL_BIO, dwcsata_dmac_intr, sc, "dwcsata dmac");

		/* let SATA link/transport errors raise irq 96 */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_INTMR,
		    DWCSATA_INTMR_ERRM);
	}
#endif

	wdcattach(chp);
}

/*
 * Soft-reset hook (installed as wdc->reset, replacing wdc_do_reset).
 */
static void
dwcsata_reset(struct ata_channel *chp, int poll)
{
	struct wdc_softc *wdc = CHAN_TO_WDC(chp);
	struct wdc_regs *wdr = &wdc->regs[chp->ch_channel];
	uint8_t st = 0;
	int i;

	/* SRST pulse, as in wdc_do_reset() */
	bus_space_write_1(wdr->cmd_iot, wdr->cmd_iohs[wd_sdh], 0, WDSD_IBM);
	delay(10);
	bus_space_write_1(wdr->ctl_iot, wdr->ctl_ioh, wd_aux_ctlr,
	    WDCTL_RST | WDCTL_IDS | WDCTL_4BIT);
	delay(2000);
	(void)bus_space_read_1(wdr->cmd_iot, wdr->cmd_iohs[wd_error], 0);
	bus_space_write_1(wdr->ctl_iot, wdr->ctl_ioh, wd_aux_ctlr,
	    WDCTL_4BIT | WDCTL_IDS);

	/*
	 * Let the core finish the reset before the taskfile is touched
	 * again.
	 */
	delay(150000);
	for (i = 0; i < 100; i++) {
		st = bus_space_read_1(wdr->cmd_iot, wdr->cmd_iohs[wd_status],
		    0);
		if ((st & WDCS_BSY) == 0)
			break;
		delay(10000);
	}
	aprint_normal_dev(wdc->sc_atac.atac_dev,
	    "soft reset: BSY cleared after %dms, status 0x%02x\n",
	    150 + i * 10, st);
}

static void
dwcsata_probe(struct ata_channel *chp)
{
	struct dwcsata_softc *sc = (struct dwcsata_softc *)CHAN_TO_WDC(chp);

	wdc_sataprobe(chp);

	/* drop the diagnostics the PHY bring-up latched into SError */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_SERROR,
	    0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_INTPR,
	    0xffffffff);

#ifndef DWCSATA_PIO_ONLY
	/* link resets do not touch DMACR/DBTSR, but re-init regardless */
	if (sc->sc_dma_ok) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_DMACR,
		    DWCSATA_DMACR_TXRXCH_CLEAR);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, DWCSATA_DBTSR,
		    DWCSATA_DBTSR_MWR(DWCSATA_DMA_FIFO_BURST) |
		    DWCSATA_DBTSR_MRD(DWCSATA_DMA_FIFO_BURST));
	}
#endif
}
