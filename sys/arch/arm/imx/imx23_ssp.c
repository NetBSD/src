/* $Id: imx23_ssp.c,v 1.3 2013/03/03 10:33:56 jkunz Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <arm/pic/picvar.h>

#include <arm/imx/imx23_apbdmavar.h>
#include <arm/imx/imx23_icollreg.h>
#include <arm/imx/imx23_sspreg.h>
#include <arm/imx/imx23var.h>

#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmcreg.h>
#include <dev/sdmmc/sdmmcvar.h>

/*
 * SD/MMC host controller driver for i.MX23.
 *
 * TODO:
 *
 * - Add support for SMC_CAPS_AUTO_STOP. 
 */

#define DMA_MAXNSEGS ((MAXPHYS / PAGE_SIZE) + 1)

typedef struct issp_softc {
	device_t sc_dev;
	apbdma_softc_t sc_dmac;
	bus_dma_tag_t sc_dmat; 
	bus_dmamap_t sc_dmamp;
	bus_size_t sc_chnsiz;
	bus_dma_segment_t sc_ds[1];
	int sc_rseg;
	bus_space_handle_t sc_hdl;
	bus_space_tag_t sc_iot;
	device_t sc_sdmmc;
	kmutex_t sc_lock;
	struct kcondvar sc_intr_cv;
	unsigned int dma_channel;
	uint32_t sc_dma_error;
	uint32_t sc_irq_error;
	uint8_t sc_state;
	uint8_t sc_bus_width;
} *issp_softc_t;

static int	issp_match(device_t, cfdata_t, void *);
static void	issp_attach(device_t, device_t, void *);
static int	issp_activate(device_t, enum devact);

static void	issp_reset(struct issp_softc *);
static void	issp_init(struct issp_softc *);
static uint32_t	issp_set_sck(struct issp_softc *, uint32_t);
static int	issp_dma_intr(void *);
static int	issp_error_intr(void *);
static void	issp_ack_intr(struct issp_softc *);
static void	issp_create_dma_cmd_list_multi(issp_softc_t, void *,
    struct sdmmc_command *);
static void	issp_create_dma_cmd_list_single(issp_softc_t, void *,
    struct sdmmc_command *);
static void	issp_create_dma_cmd_list(issp_softc_t, void *,
    struct sdmmc_command *);

/* sdmmc(4) driver chip function prototypes. */
static int	issp_host_reset(sdmmc_chipset_handle_t);
static uint32_t	issp_host_ocr(sdmmc_chipset_handle_t);
static int	issp_host_maxblklen(sdmmc_chipset_handle_t);
static int	issp_card_detect(sdmmc_chipset_handle_t);
static int	issp_write_protect(sdmmc_chipset_handle_t);
static int	issp_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	issp_bus_clock(sdmmc_chipset_handle_t, int);
static int	issp_bus_width(sdmmc_chipset_handle_t, int);
static int	issp_bus_rod(sdmmc_chipset_handle_t, int);
static void	issp_exec_command(sdmmc_chipset_handle_t,
		struct sdmmc_command *);
static void	issp_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	issp_card_intr_ack(sdmmc_chipset_handle_t);

static struct sdmmc_chip_functions issp_functions = {
	.host_reset	= issp_host_reset,
	.host_ocr	= issp_host_ocr,
	.host_maxblklen	= issp_host_maxblklen,
	.card_detect	= issp_card_detect,
	.write_protect	= issp_write_protect,
	.bus_power	= issp_bus_power,
	.bus_clock	= issp_bus_clock,
	.bus_width	= issp_bus_width,
	.bus_rod	= issp_bus_rod,
	.exec_command	= issp_exec_command,
	.card_enable_intr = issp_card_enable_intr,
	.card_intr_ack	= issp_card_intr_ack
};

CFATTACH_DECL3_NEW(ssp,
	sizeof(struct issp_softc),
	issp_match,
	issp_attach,
	NULL,
	issp_activate,
	NULL,
	NULL,
	0
);

#define SSP_SOFT_RST_LOOP 455	/* At least 1 us ... */

#define SSP_RD(sc, reg)							\
	bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define SSP_WR(sc, reg, val)						\
	bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

#define SSP_CLK		96000000	/* CLK_SSP from PLL is 96 MHz */
#define SSP_CLK_MIN	400		/* 400 kHz */
#define SSP_CLK_MAX	48000		/* 48 MHz */

/* DATA_TIMEOUT is calculated as: * (1 / SSP_CLK) * (DATA_TIMEOUT * 4096) */
#define DATA_TIMEOUT 0x4240	/* 723ms */

#define BUS_WIDTH_1_BIT 0x0
#define BUS_WIDTH_4_BIT 0x1
#define BUS_WIDTH_8_BIT 0x2

#define SSP1_ATTACHED	1
#define SSP2_ATTACHED	2

/* Flags for sc_state. */
#define SSP_STATE_IDLE	0
#define SSP_STATE_DMA	1

#define PIO_WORD_CTRL0	0
#define PIO_WORD_CMD0	1
#define PIO_WORD_CMD1	2

#define HW_SSP_CTRL1_IRQ_MASK (						\
    HW_SSP_CTRL1_SDIO_IRQ |						\
    HW_SSP_CTRL1_RESP_ERR_IRQ |						\
    HW_SSP_CTRL1_RESP_TIMEOUT_IRQ |					\
    HW_SSP_CTRL1_DATA_TIMEOUT_IRQ |					\
    HW_SSP_CTRL1_DATA_CRC_IRQ |						\
    HW_SSP_CTRL1_FIFO_UNDERRUN_IRQ |					\
    HW_SSP_CTRL1_RECV_TIMEOUT_IRQ |					\
    HW_SSP_CTRL1_FIFO_OVERRUN_IRQ)

/* SSP does not support over 64k transfer size. */
#define MAX_TRANSFER_SIZE 65536

static int
issp_match(device_t parent, cfdata_t match, void *aux)
{
	struct apb_attach_args *aa = aux;

	if ((aa->aa_addr == HW_SSP1_BASE) && (aa->aa_size == HW_SSP1_SIZE))
		return 1;

	if ((aa->aa_addr == HW_SSP2_BASE) && (aa->aa_size == HW_SSP2_SIZE))
		return 1;

	return 0;
}

static void
issp_attach(device_t parent, device_t self, void *aux)
{
	struct issp_softc *sc = device_private(self);
	struct apb_softc *sc_parent = device_private(parent);
	struct apb_attach_args *aa = aux;
	struct sdmmcbus_attach_args saa;
	static int ssp_attached = 0;
	int error;
	void *intr;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_dmat = aa->aa_dmat;

	/* Test if device instance is already attached. */
	if (aa->aa_addr == HW_SSP1_BASE && ISSET(ssp_attached, SSP1_ATTACHED)) {
		aprint_error_dev(sc->sc_dev, "SSP1 already attached\n");
		return;
	}
	if (aa->aa_addr == HW_SSP2_BASE && ISSET(ssp_attached, SSP2_ATTACHED)) {
		aprint_error_dev(sc->sc_dev, "SSP2 already attached\n");
		return;
	}

	if (aa->aa_addr == HW_SSP1_BASE) {
		sc->dma_channel = APBH_DMA_CHANNEL_SSP1;
	}
	if (aa->aa_addr == HW_SSP2_BASE) {
		sc->dma_channel = APBH_DMA_CHANNEL_SSP2;
	}

	/* This driver requires DMA functionality from the bus.
	 * Parent bus passes handle to the DMA controller instance. */
	if (sc_parent->dmac == NULL) {
		aprint_error_dev(sc->sc_dev, "DMA functionality missing\n");
		return;
	}
	sc->sc_dmac = device_private(sc_parent->dmac);

	/* Initialize lock. */
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SDMMC);

	/* Condvar to wait interrupt complete. */
	cv_init(&sc->sc_intr_cv, "ssp_intr");

	/* Establish interrupt handlers for SSP errors and SSP DMA. */
	if (aa->aa_addr == HW_SSP1_BASE) {
		intr = intr_establish(IRQ_SSP1_DMA, IPL_SDMMC, IST_LEVEL,
		    issp_dma_intr, sc);
		if (intr == NULL) {
			aprint_error_dev(sc->sc_dev, "Unable to establish "
			    "interrupt for SSP1 DMA\n");
			return;
		}
		intr = intr_establish(IRQ_SSP1_ERROR, IPL_SDMMC, IST_LEVEL,
		    issp_error_intr, sc);
		if (intr == NULL) {
			aprint_error_dev(sc->sc_dev, "Unable to establish "
			    "interrupt for SSP1 ERROR\n");
			return;
		}
	}

	if (aa->aa_addr == HW_SSP2_BASE) {
		intr = intr_establish(IRQ_SSP2_DMA, IPL_SDMMC, IST_LEVEL,
		    issp_dma_intr, sc);
		if (intr == NULL) {
			aprint_error_dev(sc->sc_dev, "Unable to establish "
			    "interrupt for SSP2 DMA\n");
			return;
		}
		intr = intr_establish(IRQ_SSP2_ERROR, IPL_SDMMC, IST_LEVEL,
		    issp_error_intr, sc);
		if (intr == NULL) {
			aprint_error_dev(sc->sc_dev, "Unable to establish "
			    "interrupt for SSP2 ERROR\n");
			return;
		}
	}

	/* Allocate DMA handle. */
	error = bus_dmamap_create(sc->sc_dmat, MAXPHYS, 1, MAXPHYS,
	    0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW, &sc->sc_dmamp);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "Unable to allocate DMA handle\n");
		return;
	}

	/* Allocate memory for DMA command chain. */
	sc->sc_chnsiz = sizeof(struct apbdma_command) *
	    (MAX_TRANSFER_SIZE / SDMMC_SECTOR_SIZE);

	error = bus_dmamem_alloc(sc->sc_dmat, sc->sc_chnsiz, PAGE_SIZE, 0,
	    sc->sc_ds, 1, &sc->sc_rseg, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "Unable to allocate DMA memory\n");
		return;
	}

	/* Initialize DMA channel. */
	apbdma_chan_init(sc->sc_dmac, sc->dma_channel);

	/* Map SSP bus space. */
	if (bus_space_map(sc->sc_iot, aa->aa_addr, aa->aa_size, 0,
	    &sc->sc_hdl)) {
		aprint_error_dev(sc->sc_dev, "Unable to map SSP bus space\n");
		return;
	}

	issp_reset(sc);
	issp_init(sc);

	uint32_t issp_vers = SSP_RD(sc, HW_SSP_VERSION);
	aprint_normal(": SSP Block v%" __PRIuBIT ".%" __PRIuBIT "\n",
	    __SHIFTOUT(issp_vers, HW_SSP_VERSION_MAJOR),
	    __SHIFTOUT(issp_vers, HW_SSP_VERSION_MINOR));

	/* Attach sdmmc to ssp bus. */
	saa.saa_busname = "sdmmc";
	saa.saa_sct	= &issp_functions;
	saa.saa_spi_sct	= NULL;
	saa.saa_sch	= sc;
	saa.saa_dmat	= aa->aa_dmat;
	saa.saa_clkmin	= SSP_CLK_MIN;
	saa.saa_clkmax	= SSP_CLK_MAX;
	saa.saa_caps	= SMC_CAPS_DMA | SMC_CAPS_4BIT_MODE |
	    SMC_CAPS_MULTI_SEG_DMA;

	sc->sc_sdmmc = config_found(sc->sc_dev, &saa, NULL);
	if (sc->sc_sdmmc == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to attach sdmmc\n");
		return;
	}

	/* Device instance was succesfully attached. */
	if (aa->aa_addr == HW_SSP1_BASE)
		ssp_attached |= SSP1_ATTACHED;
	if (aa->aa_addr == HW_SSP2_BASE)
		ssp_attached |= SSP2_ATTACHED;

	return;
}

static int
issp_activate(device_t self, enum devact act)
{
	return EOPNOTSUPP;
}

/*
 * sdmmc chip functions.
 */
static int
issp_host_reset(sdmmc_chipset_handle_t sch)
{
	struct issp_softc *sc = sch;
	issp_reset(sc);
	return 0;
}

static uint32_t
issp_host_ocr(sdmmc_chipset_handle_t sch)
{
	/* SSP supports at least 3.2 - 3.3v */
	return MMC_OCR_3_2V_3_3V;
}

static int
issp_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 512;
}

/*
 * Called at the beginning of sdmmc_task_thread to detect the presence
 * of the SD card.
 */
static int
issp_card_detect(sdmmc_chipset_handle_t sch)
{
	return 1;
}

static int
issp_write_protect(sdmmc_chipset_handle_t sch)
{
	/* The device is not write protected. */
	return 0;
}

static int
issp_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	/* i.MX23 SSP does not support setting bus power. */
	return 0;
}

static int
issp_bus_clock(sdmmc_chipset_handle_t sch, int clock)
{
	struct issp_softc *sc = sch;
	uint32_t sck;

	if (clock < SSP_CLK_MIN)
		sck = issp_set_sck(sc, SSP_CLK_MIN * 1000);
	else
		sck = issp_set_sck(sc, clock * 1000);

	/* Notify user if we didn't get the exact clock rate from SSP that was
	 * requested from the SDMMC subsystem. */
	if (sck != clock * 1000) {
		sck = sck / 1000;
		if (((sck) / 1000) != 0)
			aprint_normal_dev(sc->sc_dev, "bus clock @ %u.%03u "
			    "MHz\n", sck / 1000, sck % 1000);
		else
			aprint_normal_dev(sc->sc_dev, "bus clock @ %u KHz\n",
			    sck % 1000);
	}

	return 0;
}

static int
issp_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct issp_softc *sc = sch;

	switch(width) {
	case(1):
		sc->sc_bus_width = BUS_WIDTH_1_BIT;
		break;
	case(4):
		sc->sc_bus_width = BUS_WIDTH_4_BIT;
		break;
	case(8):
		sc->sc_bus_width = BUS_WIDTH_8_BIT;
		break;
	default:
		return 1;
	}

	return 0;
}

static int
issp_bus_rod(sdmmc_chipset_handle_t sch, int rod)
{
	/* Go to data transfer mode. */
	return 0;
}

static void
issp_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	issp_softc_t sc = sch;
	void *dma_chain;
	int error;

	/* SSP does not support over 64k transfer size. */
	if (cmd->c_data != NULL && cmd->c_datalen > MAX_TRANSFER_SIZE) {
		aprint_error_dev(sc->sc_dev, "transfer size over %d: %d\n",
		    MAX_TRANSFER_SIZE, cmd->c_datalen);
		cmd->c_error = ENODEV;
		return;
	}

	/* Map dma_chain to point allocated previously allocated DMA chain. */
	error = bus_dmamem_map(sc->sc_dmat, sc->sc_ds, 1, sc->sc_chnsiz,
	    &dma_chain, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "bus_dmamem_map: %d\n", error);
		cmd->c_error = error;
		goto out;
	}

	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamp, dma_chain,
	    sc->sc_chnsiz, NULL, BUS_DMA_NOWAIT|BUS_DMA_WRITE);
	if (error) {
		aprint_error_dev(sc->sc_dev, "bus_dmamap_load: %d\n", error);
		cmd->c_error = error;
		goto dmamem_unmap;
	}

	memset(dma_chain, 0, sc->sc_chnsiz);

	/* Setup DMA command chain.*/
	if (cmd->c_data != NULL && (cmd->c_datalen / cmd->c_blklen) > 1) {
		/* Multi block transfer. */
		issp_create_dma_cmd_list_multi(sc, dma_chain, cmd);
	} else if (cmd->c_data != NULL && cmd->c_datalen) {
		/* Single block transfer. */
		issp_create_dma_cmd_list_single(sc, dma_chain, cmd);
	} else {
		/* Only command, no data. */
		issp_create_dma_cmd_list(sc, dma_chain, cmd);
	}

	/* Tell DMA controller where it can find just initialized DMA chain. */
	apbdma_chan_set_chain(sc->sc_dmac, sc->dma_channel, sc->sc_dmamp);

	/* Synchronize command chain before DMA controller accesses it. */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamp, 0, sc->sc_chnsiz,
	    BUS_DMASYNC_PREWRITE);

	sc->sc_state = SSP_STATE_DMA;
	sc->sc_irq_error = 0;
	sc->sc_dma_error = 0;
	cmd->c_error = 0;

	mutex_enter(&sc->sc_lock);

	/* Run DMA command chain. */
	apbdma_run(sc->sc_dmac, sc->dma_channel);

	/* Wait DMA to complete. */
	while (sc->sc_state == SSP_STATE_DMA)
		cv_wait(&sc->sc_intr_cv, &sc->sc_lock);

	mutex_exit(&sc->sc_lock);

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamp, 0, sc->sc_chnsiz,
	    BUS_DMASYNC_POSTWRITE);

	if (sc->sc_dma_error) {
		if (sc->sc_dma_error == DMA_IRQ_TERM) {
			apbdma_chan_reset(sc->sc_dmac, sc->dma_channel);
			cmd->c_error = sc->sc_dma_error;
		}
		else if (sc->sc_dma_error == DMA_IRQ_BUS_ERROR) {
			aprint_error_dev(sc->sc_dev, "DMA_IRQ_BUS_ERROR: %d\n",
			    sc->sc_irq_error);
			cmd->c_error = sc->sc_dma_error;
		}
	}

	if (sc->sc_irq_error) {
		/* Do not log RESP_TIMEOUT_IRQ error if bus width is 0 as it is
		 * expected during SD card initialization phase. */
		if (sc->sc_bus_width) {
			aprint_error_dev(sc->sc_dev, "SSP_ERROR_IRQ: %d\n",
			    sc->sc_irq_error);
		}
		else if(!(sc->sc_irq_error & HW_SSP_CTRL1_RESP_TIMEOUT_IRQ)) {
			aprint_error_dev(sc->sc_dev, "SSP_ERROR_IRQ: %d\n",
			    sc->sc_irq_error);
		}

		/* Shift unsigned error code so it fits nicely to signed int. */
		cmd->c_error = sc->sc_irq_error >> 8;
	}

	/* Check reponse from the card if such was requested. */
	if (ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		cmd->c_resp[0] = SSP_RD(sc, HW_SSP_SDRESP0);
		if (ISSET(cmd->c_flags, SCF_RSP_136)) {
			cmd->c_resp[1] = SSP_RD(sc, HW_SSP_SDRESP1);
			cmd->c_resp[2] = SSP_RD(sc, HW_SSP_SDRESP2);
			cmd->c_resp[3] = SSP_RD(sc, HW_SSP_SDRESP3);
			/*
			 * Remove CRC7 + LSB by rotating all bits right by 8 to
			 * make sdmmc __bitfield() happy.
			 */
			cmd->c_resp[0] >>= 8; /* Remove CRC7 + LSB. */
			cmd->c_resp[0] |= (0x000000FF & cmd->c_resp[1]) << 24;
			cmd->c_resp[1] >>= 8;
			cmd->c_resp[1] |= (0x000000FF & cmd->c_resp[2]) << 24;
			cmd->c_resp[2] >>= 8;
			cmd->c_resp[2] |= (0x000000FF & cmd->c_resp[3]) << 24;
			cmd->c_resp[3] >>= 8;
		}
	}

	bus_dmamap_unload(sc->sc_dmat, sc->sc_dmamp);
dmamem_unmap:
	bus_dmamem_unmap(sc->sc_dmat, dma_chain, sc->sc_chnsiz);
out:

	return;
}

static void
issp_card_enable_intr(sdmmc_chipset_handle_t sch, int irq)
{
	struct issp_softc *sc = sch;
	aprint_error_dev(sc->sc_dev, "issp_card_enable_intr not implemented\n");
	return;
}

static void
issp_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	struct issp_softc *sc = sch;
	aprint_error_dev(sc->sc_dev, "issp_card_intr_ack not implemented\n");
	return;
}

/*
 * Reset the SSP block.
 *
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
issp_reset(struct issp_softc *sc)
{
	unsigned int loop;

	/* Prepare for soft-reset by making sure that SFTRST is not currently
	 * asserted. Also clear CLKGATE so we can wait for its assertion below.
	 */
	SSP_WR(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_SFTRST);

	/* Wait at least a microsecond for SFTRST to deassert. */
	loop = 0;
	while ((SSP_RD(sc, HW_SSP_CTRL0) & HW_SSP_CTRL0_SFTRST) ||
	    (loop < SSP_SOFT_RST_LOOP))
		loop++;

	/* Clear CLKGATE so we can wait for its assertion below. */
	SSP_WR(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_CLKGATE);

	/* Soft-reset the block. */
	SSP_WR(sc, HW_SSP_CTRL0_SET, HW_SSP_CTRL0_SFTRST);

	/* Wait until clock is in the gated state. */
	while (!(SSP_RD(sc, HW_SSP_CTRL0) & HW_SSP_CTRL0_CLKGATE));

	/* Bring block out of reset. */
	SSP_WR(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_SFTRST);

	loop = 0;
	while ((SSP_RD(sc, HW_SSP_CTRL0) & HW_SSP_CTRL0_SFTRST) ||
	    (loop < SSP_SOFT_RST_LOOP))
		loop++;

	SSP_WR(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_CLKGATE);

	/* Wait until clock is in the NON-gated state. */
	while (SSP_RD(sc, HW_SSP_CTRL0) & HW_SSP_CTRL0_CLKGATE);

	return;
}

/*
 * Initialize SSP controller to SD/MMC mode.
 */
static void
issp_init(struct issp_softc *sc)
{
	uint32_t reg;

	reg = SSP_RD(sc, HW_SSP_CTRL0);
	reg |= HW_SSP_CTRL0_ENABLE;

	/* Initial data bus width is 1-bit. */
	reg &= ~(HW_SSP_CTRL0_BUS_WIDTH);
	reg |= __SHIFTIN(BUS_WIDTH_1_BIT, HW_SSP_CTRL0_BUS_WIDTH) |
	    HW_SSP_CTRL0_WAIT_FOR_IRQ | HW_SSP_CTRL0_ENABLE;
	SSP_WR(sc, HW_SSP_CTRL0, reg);
	sc->sc_bus_width = BUS_WIDTH_1_BIT;

	/* Set data timeout. */
	reg = SSP_RD(sc, HW_SSP_TIMING);
	reg &= ~(HW_SSP_TIMING_TIMEOUT);
	reg |= __SHIFTIN(DATA_TIMEOUT, HW_SSP_TIMING_TIMEOUT);
	SSP_WR(sc, HW_SSP_TIMING, reg);

	/* Set initial clock rate to minimum. */
	issp_set_sck(sc, SSP_CLK_MIN * 1000);

	reg = SSP_RD(sc, HW_SSP_CTRL1);
	/* Enable all but SDIO IRQ's. */
	reg |= HW_SSP_CTRL1_RESP_ERR_IRQ_EN |
	    HW_SSP_CTRL1_RESP_TIMEOUT_IRQ_EN |
	    HW_SSP_CTRL1_DATA_TIMEOUT_IRQ_EN |
	    HW_SSP_CTRL1_DATA_CRC_IRQ_EN |
	    HW_SSP_CTRL1_FIFO_UNDERRUN_EN |
	    HW_SSP_CTRL1_RECV_TIMEOUT_IRQ_EN |
	    HW_SSP_CTRL1_FIFO_OVERRUN_IRQ_EN;
	reg |= HW_SSP_CTRL1_DMA_ENABLE;
	reg |= HW_SSP_CTRL1_POLARITY;
	/* Set SD/MMC mode and use use 8-bits per word. */
	reg &= ~(HW_SSP_CTRL1_WORD_LENGTH | HW_SSP_CTRL1_SSP_MODE);
	reg |= __SHIFTIN(0x7, HW_SSP_CTRL1_WORD_LENGTH) |
	    __SHIFTIN(0x3, HW_SSP_CTRL1_SSP_MODE);
	SSP_WR(sc, HW_SSP_CTRL1, reg);

	return;
}

/*
 * Set SSP_SCK clock rate to the value specified in target.
 *
 * SSP_SCK is calculated as: SSP_CLK / (CLOCK_DIVIDE * (1 + CLOCK_RATE))
 *
 * issp_set_sck finds the most suitable CLOCK_DIVIDE and CLOCK_RATE register
 * values for the target clock rate by iterating through all possible register
 * values.
 */
static uint32_t
issp_set_sck(struct issp_softc *sc, uint32_t target)
{
	uint32_t newclk, found, reg;
	uint8_t div, rate, d, r;

	found = div = rate = 0;

	for (d = 2; d < 254; d++) {
		for (r = 0; r < 255; r++) {
			newclk = SSP_CLK / (d * (1 + r));
			if (newclk == target) {
				found = newclk;
				div = d;
				rate = r;
				goto out;
			}
			if (newclk < target && newclk > found) {
				found = newclk;
				div = d;
				rate = r;
			}
		}
	}
out:
	reg = SSP_RD(sc, HW_SSP_TIMING);
	reg &= ~(HW_SSP_TIMING_CLOCK_DIVIDE | HW_SSP_TIMING_CLOCK_RATE);
	reg |= __SHIFTIN(div, HW_SSP_TIMING_CLOCK_DIVIDE) |
	    __SHIFTIN(rate, HW_SSP_TIMING_CLOCK_RATE);
	SSP_WR(sc, HW_SSP_TIMING, reg);

	return SSP_CLK / (div * (1 + rate));
}

/*
 * IRQ from DMA.
 */
static int
issp_dma_intr(void *arg)
{
	issp_softc_t sc = arg;
	unsigned int dma_err;

	dma_err = apbdma_intr_status(sc->sc_dmac, sc->dma_channel);

	if (dma_err) {
		apbdma_ack_error_intr(sc->sc_dmac, sc->dma_channel);
	} else {
		apbdma_ack_intr(sc->sc_dmac, sc->dma_channel);
	}

	mutex_enter(&sc->sc_lock);

	sc->sc_dma_error = dma_err;
	sc->sc_state = SSP_STATE_IDLE;

	/* Signal thread that interrupt was handled. */
	cv_signal(&sc->sc_intr_cv);

	mutex_exit(&sc->sc_lock);

	/* Return 1 to acknowledge IRQ. */
	return 1;
}

/*
 * IRQ from SSP block.
 *
 * When SSP receives IRQ it terminates ongoing DMA transfer by issuing DMATERM
 * signal to DMA block.
 */
static int
issp_error_intr(void *arg)
{
	issp_softc_t sc = arg;

	mutex_enter(&sc->sc_lock);

	sc->sc_irq_error =
	    SSP_RD(sc, HW_SSP_CTRL1) & HW_SSP_CTRL1_IRQ_MASK;

	issp_ack_intr(sc);

	mutex_exit(&sc->sc_lock);

	/* Return 1 to acknowledge IRQ. */
	return 1;
}

/*
 * Acknowledge SSP error IRQ.
 */
static void
issp_ack_intr(struct issp_softc *sc)
{

	/* Acknowledge all IRQ's. */
	SSP_WR(sc, HW_SSP_CTRL1_CLR, HW_SSP_CTRL1_IRQ_MASK);

	return;
}

/*
 * Set up multi block DMA transfer.
 */
static void
issp_create_dma_cmd_list_multi(issp_softc_t sc, void *dma_chain,
    struct sdmmc_command *cmd)
{
	apbdma_command_t dma_cmd;
	int blocks;
	int nblk;

	blocks = cmd->c_datalen / cmd->c_blklen;
	nblk = 0;
	dma_cmd = dma_chain;

	/* HEAD */
	apbdma_cmd_buf(&dma_cmd[nblk], cmd->c_blklen * nblk, cmd->c_dmamap);
	apbdma_cmd_chain(&dma_cmd[nblk], &dma_cmd[nblk+1], dma_chain,
	    sc->sc_dmamp);

	dma_cmd[nblk].control =
	    __SHIFTIN(cmd->c_blklen, APBDMA_CMD_XFER_COUNT) |
	    __SHIFTIN(3, APBDMA_CMD_CMDPIOWORDS) | APBDMA_CMD_HALTONTERMINATE |
	    APBDMA_CMD_CHAIN;

	if (!ISSET(cmd->c_flags, SCF_RSP_CRC)) {
		dma_cmd[nblk].pio_words[PIO_WORD_CTRL0] |=
		    HW_SSP_CTRL0_IGNORE_CRC;
	}

	dma_cmd[nblk].pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_DATA_XFER |
	    __SHIFTIN(sc->sc_bus_width, HW_SSP_CTRL0_BUS_WIDTH) |
	    HW_SSP_CTRL0_WAIT_FOR_IRQ | 
	    __SHIFTIN(cmd->c_datalen, HW_SSP_CTRL0_XFER_COUNT);

	if (ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		dma_cmd[nblk].pio_words[PIO_WORD_CTRL0] |=
		    HW_SSP_CTRL0_GET_RESP;
		if (ISSET(cmd->c_flags, SCF_RSP_136)) {
			dma_cmd[nblk].pio_words[PIO_WORD_CTRL0] |=
			    HW_SSP_CTRL0_LONG_RESP;
		}
	}

	dma_cmd[nblk].pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_ENABLE;

	dma_cmd[nblk].pio_words[PIO_WORD_CMD0] =
	    __SHIFTIN(ffs(cmd->c_blklen) - 1, HW_SSP_CMD0_BLOCK_SIZE) |
	    __SHIFTIN(blocks - 1, HW_SSP_CMD0_BLOCK_COUNT) |
	    __SHIFTIN(cmd->c_opcode, HW_SSP_CMD0_CMD);

	dma_cmd[nblk].pio_words[PIO_WORD_CMD1] = cmd->c_arg;

	if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
		dma_cmd[nblk].control |=
		    __SHIFTIN(APBDMA_CMD_DMA_WRITE, APBDMA_CMD_COMMAND);
		dma_cmd[nblk].pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_READ;
	} else {
		dma_cmd[nblk].control |=
		    __SHIFTIN(APBDMA_CMD_DMA_READ, APBDMA_CMD_COMMAND);
	}

	nblk++;

	/* BODY: Build commands for blocks between head and tail, if any. */
	for (; nblk < blocks - 1; nblk++) {

		apbdma_cmd_buf(&dma_cmd[nblk], cmd->c_blklen * nblk,
		    cmd->c_dmamap);

		apbdma_cmd_chain(&dma_cmd[nblk], &dma_cmd[nblk+1], dma_chain,
		    sc->sc_dmamp);

		dma_cmd[nblk].control =
		    __SHIFTIN(cmd->c_blklen, APBDMA_CMD_XFER_COUNT) |
		    APBDMA_CMD_HALTONTERMINATE | APBDMA_CMD_CHAIN;

		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			dma_cmd[nblk].control |=
			    __SHIFTIN(APBDMA_CMD_DMA_WRITE,
				APBDMA_CMD_COMMAND);
		} else {
			dma_cmd[nblk].control |=
			    __SHIFTIN(APBDMA_CMD_DMA_READ, APBDMA_CMD_COMMAND);
		}
	}

	/* TAIL
	 *
	 * TODO: Send CMD12/STOP with last DMA command to support
	 * SMC_CAPS_AUTO_STOP.
	 */
	apbdma_cmd_buf(&dma_cmd[nblk], cmd->c_blklen * nblk, cmd->c_dmamap);
	/* next = NULL */
	dma_cmd[nblk].control =
	    __SHIFTIN(cmd->c_blklen, APBDMA_CMD_XFER_COUNT) |
	    APBDMA_CMD_HALTONTERMINATE | APBDMA_CMD_WAIT4ENDCMD |
	    APBDMA_CMD_SEMAPHORE | APBDMA_CMD_IRQONCMPLT;

	if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
		dma_cmd[nblk].control |= __SHIFTIN(APBDMA_CMD_DMA_WRITE,
		    APBDMA_CMD_COMMAND);
	} else {
		dma_cmd[nblk].control |= __SHIFTIN(APBDMA_CMD_DMA_READ,
		    APBDMA_CMD_COMMAND);
	}

	return;
}

/*
 * Set up single block DMA transfer.
 */
static void
issp_create_dma_cmd_list_single(issp_softc_t sc, void *dma_chain,
    struct sdmmc_command *cmd)
{
	apbdma_command_t dma_cmd;

	dma_cmd = dma_chain;

	dma_cmd[0].control = __SHIFTIN(cmd->c_datalen, APBDMA_CMD_XFER_COUNT) |
	    __SHIFTIN(3, APBDMA_CMD_CMDPIOWORDS) |
	    APBDMA_CMD_HALTONTERMINATE | APBDMA_CMD_WAIT4ENDCMD |
	    APBDMA_CMD_SEMAPHORE | APBDMA_CMD_IRQONCMPLT;

	/* Transfer single block to the beginning of the DMA buffer. */
	apbdma_cmd_buf(&dma_cmd[0], 0, cmd->c_dmamap);

	if (!ISSET(cmd->c_flags, SCF_RSP_CRC)) {
		dma_cmd[0].pio_words[PIO_WORD_CTRL0] |=
		    HW_SSP_CTRL0_IGNORE_CRC;
	}

	dma_cmd[0].pio_words[PIO_WORD_CTRL0] |=
	    HW_SSP_CTRL0_DATA_XFER |
	    __SHIFTIN(sc->sc_bus_width, HW_SSP_CTRL0_BUS_WIDTH) |
	    HW_SSP_CTRL0_WAIT_FOR_IRQ |
	    __SHIFTIN(cmd->c_datalen, HW_SSP_CTRL0_XFER_COUNT);

	if (ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		dma_cmd[0].pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_GET_RESP;
		if (ISSET(cmd->c_flags, SCF_RSP_136)) {
			dma_cmd[0].pio_words[PIO_WORD_CTRL0] |=
			    HW_SSP_CTRL0_LONG_RESP;
		}
	}

	dma_cmd[0].pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_ENABLE;
	
	dma_cmd[0].pio_words[PIO_WORD_CMD0] =
	    HW_SSP_CMD0_APPEND_8CYC |
	    __SHIFTIN(cmd->c_opcode, HW_SSP_CMD0_CMD);
	dma_cmd[0].pio_words[PIO_WORD_CMD1] = cmd->c_arg;

	if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
		dma_cmd[0].control |=
		    __SHIFTIN(APBDMA_CMD_DMA_WRITE, APBDMA_CMD_COMMAND);
		dma_cmd[0].pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_READ;
	} else {
		dma_cmd[0].control |=
		    __SHIFTIN(APBDMA_CMD_DMA_READ, APBDMA_CMD_COMMAND);
	}
	
	return;
}

/*
 * Do DMA PIO (issue CMD). No block transfers.
 */
static void
issp_create_dma_cmd_list(issp_softc_t sc, void *dma_chain,
    struct sdmmc_command *cmd)
{
	apbdma_command_t dma_cmd;

	dma_cmd = dma_chain;

	dma_cmd[0].control = __SHIFTIN(3, APBDMA_CMD_CMDPIOWORDS) |
	    APBDMA_CMD_HALTONTERMINATE | APBDMA_CMD_WAIT4ENDCMD |
	    APBDMA_CMD_SEMAPHORE | APBDMA_CMD_IRQONCMPLT |
	    __SHIFTIN(APBDMA_CMD_NO_DMA_XFER, APBDMA_CMD_COMMAND);
	
	if (!ISSET(cmd->c_flags, SCF_RSP_CRC)) {
		dma_cmd[0].pio_words[PIO_WORD_CTRL0] |=
		    HW_SSP_CTRL0_IGNORE_CRC;
	}

	dma_cmd[0].pio_words[PIO_WORD_CTRL0] |=
	    __SHIFTIN(sc->sc_bus_width, HW_SSP_CTRL0_BUS_WIDTH) |
	    HW_SSP_CTRL0_WAIT_FOR_IRQ;
	
	if (ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		dma_cmd[0].pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_GET_RESP;
		if (ISSET(cmd->c_flags, SCF_RSP_136)) {
			dma_cmd[0].pio_words[PIO_WORD_CTRL0] |=
			    HW_SSP_CTRL0_LONG_RESP;
		}
	}

	dma_cmd[0].pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_ENABLE;

	dma_cmd[0].pio_words[PIO_WORD_CMD0] =
		__SHIFTIN(cmd->c_opcode, HW_SSP_CMD0_CMD);
	dma_cmd[0].pio_words[PIO_WORD_CMD1] = cmd->c_arg;

	return;
}
