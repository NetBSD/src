/*	$NetBSD: bcm2835_emmc.c,v 1.19 2014/10/04 18:10:04 jmcneill Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_emmc.c,v 1.19 2014/10/04 18:10:04 jmcneill Exp $");

#include "bcmdmac.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/kernel.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835_dmac.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

enum bcmemmc_dma_state {
	EMMC_DMA_STATE_IDLE,
	EMMC_DMA_STATE_BUSY,
};

struct bcmemmc_softc {
	struct sdhc_softc	sc;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_ios;
	struct sdhc_host	*sc_hosts[1];
	void			*sc_ih;

	kmutex_t		sc_lock;
	kcondvar_t		sc_cv;

	enum bcmemmc_dma_state	sc_state;

	struct bcm_dmac_channel	*sc_dmac;

	bus_dmamap_t		sc_dmamap;
	bus_dma_segment_t	sc_segs[1];	/* XXX assumes enough descriptors fit in one page */
	struct bcm_dmac_conblk	*sc_cblk;

	uint32_t		sc_physaddr;
};

static int bcmemmc_match(device_t, struct cfdata *, void *);
static void bcmemmc_attach(device_t, device_t, void *);
static void bcmemmc_attach_i(device_t);
#if NBCMDMAC > 0
static int bcmemmc_xfer_data_dma(struct sdhc_softc *, struct sdmmc_command *);
static void bcmemmc_dma_done(void *);
#endif

CFATTACH_DECL_NEW(bcmemmc, sizeof(struct bcmemmc_softc),
    bcmemmc_match, bcmemmc_attach, NULL, NULL);

/* ARGSUSED */
static int
bcmemmc_match(device_t parent, struct cfdata *match, void *aux)
{
	struct amba_attach_args *aaa = aux;

	if (strcmp(aaa->aaa_name, "emmc") != 0)
		return 0;

	return 1;
}

/* ARGSUSED */
static void
bcmemmc_attach(device_t parent, device_t self, void *aux)
{
	struct bcmemmc_softc *sc = device_private(self);
	prop_dictionary_t dict = device_properties(self);
	struct amba_attach_args *aaa = aux;
	prop_number_t frequency;
	int error;

	sc->sc.sc_dev = self;
 	sc->sc.sc_dmat = aaa->aaa_dmat;
	sc->sc.sc_flags = 0;
	sc->sc.sc_flags |= SDHC_FLAG_32BIT_ACCESS;
	sc->sc.sc_flags |= SDHC_FLAG_HOSTCAPS;
	sc->sc.sc_flags |= SDHC_FLAG_NO_HS_BIT;
	sc->sc.sc_caps = SDHC_VOLTAGE_SUPP_3_3V | SDHC_HIGH_SPEED_SUPP |
	    (SDHC_MAX_BLK_LEN_1024 << SDHC_MAX_BLK_LEN_SHIFT);

	sc->sc.sc_host = sc->sc_hosts;
	sc->sc.sc_clkbase = 50000;	/* Default to 50MHz */
	sc->sc_iot = aaa->aaa_iot;

	/* Fetch the EMMC clock frequency from property if set. */
	frequency = prop_dictionary_get(dict, "frequency");
	if (frequency != NULL) {
		sc->sc.sc_clkbase = prop_number_integer_value(frequency) / 1000;
	}

	error = bus_space_map(sc->sc_iot, aaa->aaa_addr, aaa->aaa_size, 0,
	    &sc->sc_ioh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers for %s: %d\n", aaa->aaa_name, error);
		return;
	}
	sc->sc_ios = aaa->aaa_size;
	sc->sc_physaddr = aaa->aaa_addr;

	aprint_naive(": SDHC controller\n");
	aprint_normal(": SDHC controller\n");

 	sc->sc_ih = bcm2835_intr_establish(aaa->aaa_intr, IPL_SDMMC, sdhc_intr,
 	    &sc->sc);

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		    aaa->aaa_intr);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on intr %d\n", aaa->aaa_intr);

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
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SDMMC);
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
		intr_disestablish(sc->sc_ih);
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
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
}

#if NBCMDMAC > 0
static int
bcmemmc_xfer_data_dma(struct sdhc_softc *sdhc_sc, struct sdmmc_command *cmd)
{
	struct bcmemmc_softc * const sc = device_private(sdhc_sc->sc_dev);
	size_t seg;
	int error;

	for (seg = 0; seg < cmd->c_dmamap->dm_nsegs; seg++) {
		sc->sc_cblk[seg].cb_ti =
		    __SHIFTIN(11, DMAC_TI_PERMAP); /* e.MMC */
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_DEST_INC;
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_DEST_WIDTH;
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_SRC_DREQ;
			sc->sc_cblk[seg].cb_source_ad =
			    BCM2835_PERIPHERALS_TO_BUS(sc->sc_physaddr +
			    SDHC_DATA);
			sc->sc_cblk[seg].cb_dest_ad =
			    cmd->c_dmamap->dm_segs[seg].ds_addr;
		} else {
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_SRC_INC;
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_SRC_WIDTH;
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_DEST_DREQ;
			sc->sc_cblk[seg].cb_source_ad =
			    cmd->c_dmamap->dm_segs[seg].ds_addr;
			sc->sc_cblk[seg].cb_dest_ad =
			    BCM2835_PERIPHERALS_TO_BUS(sc->sc_physaddr +
			    SDHC_DATA);
		}
		sc->sc_cblk[seg].cb_txfr_len =
		    cmd->c_dmamap->dm_segs[seg].ds_len;
		sc->sc_cblk[seg].cb_stride = 0;
		if (seg == cmd->c_dmamap->dm_nsegs - 1) {
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_WAIT_RESP;
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

	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_state == EMMC_DMA_STATE_IDLE);
	sc->sc_state = EMMC_DMA_STATE_BUSY;
	bcm_dmac_set_conblk_addr(sc->sc_dmac,
	    sc->sc_dmamap->dm_segs[0].ds_addr);
	bcm_dmac_transfer(sc->sc_dmac);
	while (sc->sc_state == EMMC_DMA_STATE_BUSY) {
		error = cv_timedwait(&sc->sc_cv, &sc->sc_lock, hz * 10);
		if (error == EWOULDBLOCK) {
			device_printf(sc->sc.sc_dev, "transfer timeout!\n");
			bcm_dmac_halt(sc->sc_dmac);
			sc->sc_state = EMMC_DMA_STATE_IDLE;
			error = ETIMEDOUT;
			break;
		}
	}
	mutex_exit(&sc->sc_lock);

	bus_dmamap_sync(sc->sc.sc_dmat, sc->sc_dmamap, 0,
	    sc->sc_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);

	return error;
}

static void
bcmemmc_dma_done(void *arg)
{
	struct bcmemmc_softc * const sc = arg;

	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_state == EMMC_DMA_STATE_BUSY);
	sc->sc_state = EMMC_DMA_STATE_IDLE;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);

}
#endif
