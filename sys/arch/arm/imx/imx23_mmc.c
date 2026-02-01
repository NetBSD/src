/* $Id: imx23_mmc.c,v 1.1 2026/02/01 11:31:28 yurix Exp $ */

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

#include <dev/fdt/fdtvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmcreg.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <arm/imx/imx23_icollreg.h>
#include <arm/imx/imx23_mmcreg.h>
#include <arm/imx/imx23var.h>
#include <arm/pic/picvar.h>

/*
 * SD/MMC host controller driver for i.MX23.
 *
 * TODO:
 *
 * - Add support for SMC_CAPS_AUTO_STOP.
 * - Uset GPIO for SD card detection.
 */

struct imx23_mmc_softc {
	device_t sc_dev;
	struct fdtbus_dma *dma_channel;
	bus_space_handle_t sc_hdl;
	bus_space_tag_t sc_iot;
	device_t sc_sdmmc;
	kmutex_t sc_lock;
	struct kcondvar sc_intr_cv;
	uint32_t sc_irq_error;
	uint8_t sc_state;
	uint8_t sc_bus_width;
	uint32_t pio_words[3];
};


static int	imx23_mmc_match(device_t, cfdata_t, void *);
static void	imx23_mmc_attach(device_t, device_t, void *);

static void	imx23_mmc_reset(struct imx23_mmc_softc *);
static void	imx23_mmc_init(struct imx23_mmc_softc *);
static uint32_t	imx23_mmc_set_sck(struct imx23_mmc_softc *, uint32_t);
static void	imx23_mmc_dma_intr(void *);
static int	imx23_mmc_error_intr(void *);
static void 	imx23_mmc_prepare_data_command(struct imx23_mmc_softc *,
			       struct fdtbus_dma_req *, struct sdmmc_command *);
static void 	imx23_mmc_prepare_command(struct imx23_mmc_softc *,
			  struct fdtbus_dma_req *, struct sdmmc_command *);

/* sdmmc(4) driver chip function prototypes. */
static int	imx23_mmc_host_reset(sdmmc_chipset_handle_t);
static uint32_t	imx23_mmc_host_ocr(sdmmc_chipset_handle_t);
static int	imx23_mmc_host_maxblklen(sdmmc_chipset_handle_t);
static int	imx23_mmc_card_detect(sdmmc_chipset_handle_t);
static int	imx23_mmc_write_protect(sdmmc_chipset_handle_t);
static int	imx23_mmc_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	imx23_mmc_bus_clock(sdmmc_chipset_handle_t, int);
static int	imx23_mmc_bus_width(sdmmc_chipset_handle_t, int);
static int	imx23_mmc_bus_rod(sdmmc_chipset_handle_t, int);
static void	imx23_mmc_exec_command(sdmmc_chipset_handle_t,
		struct sdmmc_command *);
static void	imx23_mmc_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	imx23_mmc_card_intr_ack(sdmmc_chipset_handle_t);

static struct sdmmc_chip_functions imx23_mmc_functions = {
	.host_reset	= imx23_mmc_host_reset,
	.host_ocr	= imx23_mmc_host_ocr,
	.host_maxblklen	= imx23_mmc_host_maxblklen,
	.card_detect	= imx23_mmc_card_detect,
	.write_protect	= imx23_mmc_write_protect,
	.bus_power	= imx23_mmc_bus_power,
	.bus_clock	= imx23_mmc_bus_clock,
	.bus_width	= imx23_mmc_bus_width,
	.bus_rod	= imx23_mmc_bus_rod,
	.exec_command	= imx23_mmc_exec_command,
	.card_enable_intr = imx23_mmc_card_enable_intr,
	.card_intr_ack	= imx23_mmc_card_intr_ack
};

CFATTACH_DECL_NEW(imx23mmc, sizeof(struct imx23_mmc_softc), imx23_mmc_match,
		  imx23_mmc_attach, NULL, NULL);

#define SSP_SOFT_RST_LOOP 455	/* At least 1 us ... */

#define SSP_RD(sc, reg)							\
	bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define SSP_WR(sc, reg, val)						\
	bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

#define SSP_CLK		160000000	/* CLK_SSP from PLL in Hz */
#define SSP_CLK_MIN	400		/* 400 kHz */
#define SSP_CLK_MAX	48000		/* 48 MHz */

/* DATA_TIMEOUT is calculated as: * (1 / SSP_CLK) * (DATA_TIMEOUT * 4096) */
#define DATA_TIMEOUT 0x4240

#define BUS_WIDTH_1_BIT 0x0
#define BUS_WIDTH_4_BIT 0x1
#define BUS_WIDTH_8_BIT 0x2

/* Flags for sc_state. */
#define SSP_STATE_IDLE	0
#define SSP_STATE_DMA	1

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

/* Offsets of pio words in pio array */
#define PIO_WORD_CTRL0	0
#define PIO_WORD_CMD0	1
#define PIO_WORD_CMD1	2

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx23-mmc" },
	DEVICE_COMPAT_EOL
};

static int
imx23_mmc_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args *const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx23_mmc_attach(device_t parent, device_t self, void *aux)
{
	struct imx23_mmc_softc *const sc = device_private(self);
	struct fdt_attach_args *const faa = aux;
	const int phandle = faa->faa_phandle;
	struct sdmmcbus_attach_args saa;
	char intrstr[128];

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	/* map ssp control registers */
	bus_addr_t addr;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_hdl)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* Initialize lock. */
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SCHED);

	/* Condvar to wait interrupt complete. */
	cv_init(&sc->sc_intr_cv, "ssp_intr");

	/* acquire DMA channel */
	sc->dma_channel = fdtbus_dma_get(phandle,"rx-tx", imx23_mmc_dma_intr,
					 sc);
	if(sc->dma_channel == NULL) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* establish error interrupt */
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	void *ih = fdtbus_intr_establish_xname(phandle, 0, IPL_SDMMC, IST_LEVEL,
					       imx23_mmc_error_intr, sc,
					 device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish error interrupt\n");
		return;
	}

	imx23_mmc_reset(sc);
	imx23_mmc_init(sc);

	uint32_t imx23_mmc_vers = SSP_RD(sc, HW_SSP_VERSION);
	aprint_normal(": SSP Block v%" __PRIuBIT ".%" __PRIuBIT "\n",
	    __SHIFTOUT(imx23_mmc_vers, HW_SSP_VERSION_MAJOR),
	    __SHIFTOUT(imx23_mmc_vers, HW_SSP_VERSION_MINOR));

	/* Attach sdmmc to ssp bus. */
	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct	= &imx23_mmc_functions;
	saa.saa_spi_sct	= NULL;
	saa.saa_sch	= sc;
	saa.saa_dmat	= faa->faa_dmat;
	saa.saa_clkmin	= SSP_CLK_MIN;
	saa.saa_clkmax	= SSP_CLK_MAX;
	saa.saa_caps	= SMC_CAPS_DMA | SMC_CAPS_4BIT_MODE;

	sc->sc_sdmmc = config_found(sc->sc_dev, &saa, NULL, CFARGS_NONE);
	if (sc->sc_sdmmc == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to attach sdmmc\n");
		return;
	}

	return;
}

/*
 * sdmmc chip functions.
 */
static int
imx23_mmc_host_reset(sdmmc_chipset_handle_t sch)
{
	struct imx23_mmc_softc *sc = sch;
	imx23_mmc_reset(sc);
	return 0;
}

static uint32_t
imx23_mmc_host_ocr(sdmmc_chipset_handle_t sch)
{
	/* SSP supports at least 3.2 - 3.3v */
	return MMC_OCR_3_2V_3_3V;
}

static int
imx23_mmc_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 512;
}

/*
 * Called at the beginning of sdmmc_task_thread to detect the presence
 * of the SD card.
 */
static int
imx23_mmc_card_detect(sdmmc_chipset_handle_t sch)
{
	return 1; /* the olinuxino has no card detection */
}

static int
imx23_mmc_write_protect(sdmmc_chipset_handle_t sch)
{
	/* The device is not write protected. */
	return 0;
}

static int
imx23_mmc_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	/* i.MX23 SSP does not support setting bus power. */
	return 0;
}

static int
imx23_mmc_bus_clock(sdmmc_chipset_handle_t sch, int clock)
{
	struct imx23_mmc_softc *sc = sch;
	uint32_t sck;

	if (clock < SSP_CLK_MIN)
		sck = imx23_mmc_set_sck(sc, SSP_CLK_MIN * 1000);
	else
		sck = imx23_mmc_set_sck(sc, clock * 1000);

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
imx23_mmc_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct imx23_mmc_softc *sc = sch;

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
imx23_mmc_bus_rod(sdmmc_chipset_handle_t sch, int rod)
{
	/* Go to data transfer mode. */
	return 0;
}

static void
imx23_mmc_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct imx23_mmc_softc *sc = sch;
	struct fdtbus_dma_req req;

	/* SSP does not support over 64k transfer size. */
	if (cmd->c_data != NULL && cmd->c_datalen > MAX_TRANSFER_SIZE) {
		aprint_error_dev(sc->sc_dev, "transfer size over %d: %d\n",
		    MAX_TRANSFER_SIZE, cmd->c_datalen);
		cmd->c_error = ENODEV;
		return;
	}

	mutex_enter(&sc->sc_lock);

	/* Setup DMA command chain.*/
	if (cmd->c_data != NULL && cmd->c_datalen) {
		/* command with data */
		imx23_mmc_prepare_data_command(sc, &req, cmd);
	} else {
		/* Only command, no data. */
		imx23_mmc_prepare_command(sc, &req, cmd);
	}


	sc->sc_state = SSP_STATE_DMA;
	sc->sc_irq_error = 0;
	cmd->c_error = 0;

	/* Run DMA */
	if(fdtbus_dma_transfer(sc->dma_channel, &req)) {
		aprint_error_dev(sc->sc_dev, "dma transfer error\n");
		goto out;
	}

	/* Wait DMA to complete. */
	while (sc->sc_state == SSP_STATE_DMA)
		cv_wait(&sc->sc_intr_cv, &sc->sc_lock);

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

	/* Check response from the card if such was requested. */
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

out:
	mutex_exit(&sc->sc_lock);

	return;
}

static void
imx23_mmc_card_enable_intr(sdmmc_chipset_handle_t sch, int irq)
{
	struct imx23_mmc_softc *sc = sch;
	aprint_error_dev(sc->sc_dev, "issp_card_enable_intr not implemented\n");
	return;
}

static void
imx23_mmc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	struct imx23_mmc_softc *sc = sch;
	aprint_error_dev(sc->sc_dev, "issp_card_intr_ack not implemented\n");
	return;
}

/*
 * Reset the SSP block.
 *
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
imx23_mmc_reset(struct imx23_mmc_softc *sc)
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
imx23_mmc_init(struct imx23_mmc_softc *sc)
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
	imx23_mmc_set_sck(sc, SSP_CLK_MIN * 1000);

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
 * imx23_mmc_set_sck finds the most suitable CLOCK_DIVIDE and CLOCK_RATE
 * register values for the target clock rate by iterating through all possible
 * register values.
 */
static uint32_t
imx23_mmc_set_sck(struct imx23_mmc_softc *sc, uint32_t target)
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
static void
imx23_mmc_dma_intr(void *arg)
{
	struct imx23_mmc_softc *sc = arg;

	mutex_enter(&sc->sc_lock);
	
	sc->sc_state = SSP_STATE_IDLE;

	/* Signal thread that interrupt was handled. */
	cv_signal(&sc->sc_intr_cv);

	mutex_exit(&sc->sc_lock);
}

/*
 * IRQ from SSP block.
 *
 * When SSP receives IRQ it terminates ongoing DMA transfer by issuing DMATERM
 * signal to DMA block.
 */
static int
imx23_mmc_error_intr(void *arg)
{
	struct imx23_mmc_softc *sc = arg;

	mutex_enter(&sc->sc_lock);

	sc->sc_irq_error =
	    SSP_RD(sc, HW_SSP_CTRL1) & HW_SSP_CTRL1_IRQ_MASK;

	/* Acknowledge all IRQ's. */
	SSP_WR(sc, HW_SSP_CTRL1_CLR, HW_SSP_CTRL1_IRQ_MASK);

	mutex_exit(&sc->sc_lock);

	/* Return 1 to acknowledge IRQ. */
	return 1;
}

/*
 * Set up a dma transfer for a block with data.
 */
static void
imx23_mmc_prepare_data_command(struct imx23_mmc_softc *sc,
	struct fdtbus_dma_req *req, struct sdmmc_command *cmd)
{
	int block_count = cmd->c_datalen / cmd->c_blklen;

	/* prepare DMA request */
	req->dreq_segs = cmd->c_dmamap->dm_segs;
	req->dreq_nsegs = cmd->c_dmamap->dm_nsegs;
	req->dreq_block_irq = 1;
	req->dreq_block_multi = 0;
	req->dreq_datalen = 3;
	req->dreq_data = sc->pio_words;

	/* prepare CTRL0 register*/
	sc->pio_words[PIO_WORD_CTRL0] =
	    HW_SSP_CTRL0_DATA_XFER |
	    __SHIFTIN(sc->sc_bus_width, HW_SSP_CTRL0_BUS_WIDTH) |
	    HW_SSP_CTRL0_WAIT_FOR_IRQ |
	    __SHIFTIN(cmd->c_datalen, HW_SSP_CTRL0_XFER_COUNT) |
	    HW_SSP_CTRL0_ENABLE;
	if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
		req->dreq_dir = FDT_DMA_READ;
		sc->pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_READ;
	} else {
		req->dreq_dir = FDT_DMA_WRITE;
	}
	if (!ISSET(cmd->c_flags, SCF_RSP_CRC)) {
		sc->pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_IGNORE_CRC;
	}
	if (ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		sc->pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_GET_RESP;
		if (ISSET(cmd->c_flags, SCF_RSP_136)) {
			sc->pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_LONG_RESP;
		}
	}

	/* prepare CMD0 register */
	sc->pio_words[PIO_WORD_CMD0] =
	    HW_SSP_CMD0_APPEND_8CYC |
	    __SHIFTIN(ffs(cmd->c_blklen) - 1, HW_SSP_CMD0_BLOCK_SIZE) |
	    __SHIFTIN(block_count - 1, HW_SSP_CMD0_BLOCK_COUNT) |
	    __SHIFTIN(cmd->c_opcode, HW_SSP_CMD0_CMD);

	/* prepare CMD1 register */
	sc->pio_words[PIO_WORD_CMD1] = cmd->c_arg;
}

/*
 * Setup a dma transfer for a command without data (PIO only)
 */
static void
imx23_mmc_prepare_command(struct imx23_mmc_softc *sc,
	struct fdtbus_dma_req *req, struct sdmmc_command *cmd)
{
	/* prepare DMA */
	req->dreq_nsegs = 0;
	req->dreq_block_irq = 1;
	req->dreq_block_multi = 0;
	req->dreq_dir = FDT_DMA_NO_XFER;
	req->dreq_datalen = 3;
	req->dreq_data = sc->pio_words;

	/* prepare CTRL0 register*/
	sc->pio_words[PIO_WORD_CTRL0] =
	    __SHIFTIN(sc->sc_bus_width, HW_SSP_CTRL0_BUS_WIDTH) |
	    HW_SSP_CTRL0_WAIT_FOR_IRQ | HW_SSP_CTRL0_ENABLE;
	if (!ISSET(cmd->c_flags, SCF_RSP_CRC)) {
		sc->pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_IGNORE_CRC;
	}
	if (ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		sc->pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_GET_RESP;
		if (ISSET(cmd->c_flags, SCF_RSP_136)) {
			sc->pio_words[PIO_WORD_CTRL0] |= HW_SSP_CTRL0_LONG_RESP;
		}
	}

	/* prepare CMD0 register */
	sc->pio_words[PIO_WORD_CMD0] =
	    __SHIFTIN(cmd->c_opcode, HW_SSP_CMD0_CMD);

	/* prepare CMD1 register */
	sc->pio_words[PIO_WORD_CMD1] = cmd->c_arg;
}
