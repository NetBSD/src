/*	$NetBSD: bcm2835_emmc.c,v 1.33 2018/08/19 09:18:48 rin Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_emmc.c,v 1.33 2018/08/19 09:18:48 rin Exp $");

#include "bcmdmac.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/kernel.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_dmac.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/fdt/arm_fdtvar.h>

enum bcmemmc_dma_state {
	EMMC_DMA_STATE_IDLE,
	EMMC_DMA_STATE_BUSY,
};

struct bcmemmc_softc {
	struct sdhc_softc	sc;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_iob;
	bus_size_t		sc_ios;
	struct sdhc_host	*sc_hosts[1];
	void			*sc_ih;
	int			sc_phandle;

	kcondvar_t		sc_cv;

	enum bcmemmc_dma_state	sc_state;

	struct bcm_dmac_channel	*sc_dmac;

	bus_dmamap_t		sc_dmamap;
	bus_dma_segment_t	sc_segs[1];	/* XXX assumes enough descriptors fit in one page */
	struct bcm_dmac_conblk	*sc_cblk;
};

static int bcmemmc_match(device_t, struct cfdata *, void *);
static void bcmemmc_attach(device_t, device_t, void *);
static void bcmemmc_attach_i(device_t);
#if NBCMDMAC > 0
static int bcmemmc_xfer_data_dma(struct sdhc_softc *, struct sdmmc_command *);
static void bcmemmc_dma_done(uint32_t, uint32_t, void *);
#endif

CFATTACH_DECL_NEW(bcmemmc, sizeof(struct bcmemmc_softc),
    bcmemmc_match, bcmemmc_attach, NULL, NULL);

/* ARGSUSED */
static int
bcmemmc_match(device_t parent, struct cfdata *match, void *aux)
{
	const char * const compatible[] = {
	    "brcm,bcm2835-sdhci",
	    NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

/* ARGSUSED */
static void
bcmemmc_attach(device_t parent, device_t self, void *aux)
{
	struct bcmemmc_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	prop_dictionary_t dict = device_properties(self);
	bool disable = false;
	int error;

	sc->sc.sc_dev = self;
	sc->sc.sc_dmat = faa->faa_dmat;
	sc->sc.sc_flags = 0;
	sc->sc.sc_flags |= SDHC_FLAG_32BIT_ACCESS;
	sc->sc.sc_flags |= SDHC_FLAG_HOSTCAPS;
	sc->sc.sc_flags |= SDHC_FLAG_NO_HS_BIT;
	sc->sc.sc_caps = SDHC_VOLTAGE_SUPP_3_3V | SDHC_HIGH_SPEED_SUPP |
	    (SDHC_MAX_BLK_LEN_1024 << SDHC_MAX_BLK_LEN_SHIFT);

	sc->sc.sc_host = sc->sc_hosts;
	sc->sc.sc_clkbase = 50000;	/* Default to 50MHz */
	sc->sc_iot = faa->faa_bst;

	prop_dictionary_get_bool(dict, "disable", &disable);
	if (disable) {
		aprint_naive(": disabled\n");
		aprint_normal(": disabled\n");
		return;
	}

	bus_addr_t addr;
	bus_size_t size;

	const int phandle = faa->faa_phandle;
	error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error_dev(sc->sc.sc_dev, "unable to map device\n");
		return;
	}
	sc->sc_phandle = phandle;

	/* Enable clocks */
	struct clk *clk;
	for (int i = 0; (clk = fdtbus_clock_get_index(phandle, i)); i++) {
		if (clk_enable(clk) != 0) {
			aprint_error(": failed to enable clock #%d\n", i);
			return;
		}
		if (i == 0)
			sc->sc.sc_clkbase = clk_get_rate(clk) / 1000;
	}
	aprint_debug_dev(self, "ref freq %u kHz\n", sc->sc.sc_clkbase);

	error = bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh);
	if (error) {
		aprint_error_dev(sc->sc.sc_dev, "unable to map device\n");
		return;
	}
	sc->sc_iob = addr;
	sc->sc_ios = size;

	aprint_naive(": SDHC controller\n");
	aprint_normal(": SDHC controller\n");

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_SDMMC, 0,
	    sdhc_intr, &sc->sc);

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %s\n",
		    intrstr);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

#if NBCMDMAC > 0
	sc->sc_dmac = bcm_dmac_alloc(BCM_DMAC_TYPE_NORMAL, IPL_SDMMC,
	    bcmemmc_dma_done, sc);
	if (sc->sc_dmac == NULL)
		goto done;

 	sc->sc.sc_flags |= SDHC_FLAG_USE_DMA;
	sc->sc.sc_flags |= SDHC_FLAG_EXTERNAL_DMA;
	sc->sc.sc_caps |= SDHC_DMA_SUPPORT;
	sc->sc.sc_vendor_transfer_data_dma = bcmemmc_xfer_data_dma;

	sc->sc_state = EMMC_DMA_STATE_IDLE;
	cv_init(&sc->sc_cv, "bcmemmcdma");

	int rseg;
	error = bus_dmamem_alloc(sc->sc.sc_dmat, PAGE_SIZE, PAGE_SIZE,
	     PAGE_SIZE, sc->sc_segs, 1, &rseg, BUS_DMA_WAITOK);
	if (error) {
		aprint_error_dev(self, "dmamem_alloc failed (%d)\n", error);
		goto fail;
	}

	error = bus_dmamem_map(sc->sc.sc_dmat, sc->sc_segs, rseg, PAGE_SIZE,
	    (void **)&sc->sc_cblk, BUS_DMA_WAITOK);
	if (error) {
		aprint_error_dev(self, "dmamem_map failed (%d)\n", error);
		goto fail;
	}
	KASSERT(sc->sc_cblk != NULL);

	memset(sc->sc_cblk, 0, PAGE_SIZE);

	error = bus_dmamap_create(sc->sc.sc_dmat, PAGE_SIZE, 1, PAGE_SIZE, 0,
	    BUS_DMA_WAITOK, &sc->sc_dmamap);
	if (error) {
		aprint_error_dev(self, "dmamap_create failed (%d)\n", error);
		goto fail;
	}

	error = bus_dmamap_load(sc->sc.sc_dmat, sc->sc_dmamap, sc->sc_cblk,
	    PAGE_SIZE, NULL, BUS_DMA_WAITOK|BUS_DMA_WRITE);
	if (error) {
		aprint_error_dev(self, "dmamap_load failed (%d)\n", error);
		goto fail;
	}

done:
#endif
	config_interrupts(self, bcmemmc_attach_i);
	return;

fail:
	/* XXX add bus_dma failure cleanup */
	if (sc->sc_ih) {
		fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);
		sc->sc_ih = NULL;
	}
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
}

static void
bcmemmc_attach_i(device_t self)
{
	struct bcmemmc_softc * const sc = device_private(self);
	int error;

	error = sdhc_host_found(&sc->sc, sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	if (error != 0) {
		aprint_error_dev(self, "couldn't initialize host, error=%d\n",
		    error);
		goto fail;
	}
	return;

fail:
	/* XXX add bus_dma failure cleanup */
	if (sc->sc_ih) {
		fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);
		sc->sc_ih = NULL;
	}
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
}

#if NBCMDMAC > 0
static int
bcmemmc_xfer_data_dma(struct sdhc_softc *sdhc_sc, struct sdmmc_command *cmd)
{
	struct bcmemmc_softc * const sc = device_private(sdhc_sc->sc_dev);
	kmutex_t *plock = sdhc_host_lock(sc->sc_hosts[0]);
	const bus_addr_t ad_sdhcdata = sc->sc_iob + SDHC_DATA;
	size_t seg;
	int error;

	KASSERT(mutex_owned(plock));

	for (seg = 0; seg < cmd->c_dmamap->dm_nsegs; seg++) {
		sc->sc_cblk[seg].cb_ti =
		    __SHIFTIN(11, DMAC_TI_PERMAP); /* e.MMC */
		sc->sc_cblk[seg].cb_txfr_len =
		    cmd->c_dmamap->dm_segs[seg].ds_len;
		/*
		 * All transfers are assumed to be multiples of 32-bits.
		 */
		KASSERTMSG((sc->sc_cblk[seg].cb_txfr_len & 0x3) == 0,
		    "seg %zu len %d", seg, sc->sc_cblk[seg].cb_txfr_len);
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_DEST_INC;
			/*
			 * Use 128-bit mode if transfer is a multiple of
			 * 16-bytes.
			 */
			if ((sc->sc_cblk[seg].cb_txfr_len & 0xf) == 0)
				sc->sc_cblk[seg].cb_ti |= DMAC_TI_DEST_WIDTH;
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_SRC_DREQ;
			sc->sc_cblk[seg].cb_source_ad = ad_sdhcdata;
			sc->sc_cblk[seg].cb_dest_ad =
			    cmd->c_dmamap->dm_segs[seg].ds_addr;
		} else {
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_SRC_INC;
			/*
			 * Use 128-bit mode if transfer is a multiple of
			 * 16-bytes.
			 */
			if ((sc->sc_cblk[seg].cb_txfr_len & 0xf) == 0)
				sc->sc_cblk[seg].cb_ti |= DMAC_TI_SRC_WIDTH;
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_DEST_DREQ;
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_WAIT_RESP;
			sc->sc_cblk[seg].cb_source_ad =
			    cmd->c_dmamap->dm_segs[seg].ds_addr;
			sc->sc_cblk[seg].cb_dest_ad = ad_sdhcdata;
		}
		sc->sc_cblk[seg].cb_stride = 0;
		if (seg == cmd->c_dmamap->dm_nsegs - 1) {
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_INTEN;
			sc->sc_cblk[seg].cb_nextconbk = 0;
		} else {
			sc->sc_cblk[seg].cb_nextconbk =
			    sc->sc_dmamap->dm_segs[0].ds_addr +
			    sizeof(struct bcm_dmac_conblk) * (seg+1);
		}
		sc->sc_cblk[seg].cb_padding[0] = 0;
		sc->sc_cblk[seg].cb_padding[1] = 0;
	}

	bus_dmamap_sync(sc->sc.sc_dmat, sc->sc_dmamap, 0,
	    sc->sc_dmamap->dm_mapsize, BUS_DMASYNC_PREWRITE);

	error = 0;

	KASSERT(sc->sc_state == EMMC_DMA_STATE_IDLE);
	sc->sc_state = EMMC_DMA_STATE_BUSY;
	bcm_dmac_set_conblk_addr(sc->sc_dmac,
	    sc->sc_dmamap->dm_segs[0].ds_addr);
	error = bcm_dmac_transfer(sc->sc_dmac);
	if (error)
		return error;

	while (sc->sc_state == EMMC_DMA_STATE_BUSY) {
		error = cv_timedwait(&sc->sc_cv, plock, hz * 10);
		if (error == EWOULDBLOCK) {
			device_printf(sc->sc.sc_dev, "transfer timeout!\n");
			bcm_dmac_halt(sc->sc_dmac);
			sc->sc_state = EMMC_DMA_STATE_IDLE;
			error = ETIMEDOUT;
			break;
		}
	}

	bus_dmamap_sync(sc->sc.sc_dmat, sc->sc_dmamap, 0,
	    sc->sc_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);

	return error;
}

static void
bcmemmc_dma_done(uint32_t status, uint32_t error, void *arg)
{
	struct bcmemmc_softc * const sc = arg;
	kmutex_t *plock = sdhc_host_lock(sc->sc_hosts[0]);

	if (status != (DMAC_CS_INT|DMAC_CS_END))
		device_printf(sc->sc.sc_dev, "status %#x error %#x\n",
			status,error);

	mutex_enter(plock);
	KASSERT(sc->sc_state == EMMC_DMA_STATE_BUSY);
	if (status & DMAC_CS_END)
		sc->sc_state = EMMC_DMA_STATE_IDLE;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(plock);
}
#endif
