/* $Id: imx23_ssp.c,v 1.2 2012/12/16 19:45:52 jkunz Exp $ */

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
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <arm/imx/imx23_sspreg.h>
#include <arm/imx/imx23var.h>

#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmcreg.h>
#include <dev/sdmmc/sdmmcvar.h>

/*
 * SD/MMC host controller driver for i.MX23.
 */

struct issp_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
	device_t sc_sdmmc;
	device_t dmac;
};

static int	issp_match(device_t, cfdata_t, void *);
static void	issp_attach(device_t, device_t, void *);
static int	issp_activate(device_t, enum devact);

static void	issp_reset(struct issp_softc *);
static void	issp_init(struct issp_softc *);
static uint32_t	issp_set_sck(struct issp_softc *, uint32_t target);

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
	0);

#define SSP_SOFT_RST_LOOP 455	/* At least 1 us ... */

#define SSP_RD(sc, reg)							\
	bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define SSP_WR(sc, reg, val)						\
	bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

#define SSP_CLK		96000000	/* CLK_SSP from PLL is 96 MHz */
#define SSP_CLK_MIN	400		/* 400 kHz */
#define SSP_CLK_MAX	48000		/* 48 MHz */

#define SSP_BUSY (HW_SSP_STATUS_CMD_BUSY |				\
			HW_SSP_STATUS_DATA_BUSY |			\
			HW_SSP_STATUS_BUSY)

#define SSP_RUN_ERR (HW_SSP_STATUS_RESP_CRC_ERR |			\
			HW_SSP_STATUS_RESP_ERR |			\
			HW_SSP_STATUS_RESP_TIMEOUT |			\
			HW_SSP_STATUS_DATA_CRC_ERR |			\
			HW_SSP_STATUS_TIMEOUT)

#define BLKIO_NONE 0
#define BLKIO_RD 1
#define BLKIO_WR 2

#define BUS_WIDTH_1_BIT 0x0
#define BUS_WIDTH_4_BIT 0x1
#define BUS_WIDTH_8_BIT 0x2

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
	static int issp_attached = 0;

	if (issp_attached)
		return;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->dmac = sc_parent->dmac;

	if (bus_space_map(sc->sc_iot,
	    aa->aa_addr, aa->aa_size, 0, &(sc->sc_hdl))) {
		aprint_error_dev(sc->sc_dev, "unable to map bus space\n");
		return;
	}

	issp_reset(sc);
	issp_init(sc);

	uint32_t issp_vers = SSP_RD(sc, HW_SSP_VERSION);
	aprint_normal(": SSP Block v%" __PRIuBIT ".%" __PRIuBIT "\n",
	    __SHIFTOUT(issp_vers, HW_SSP_VERSION_MAJOR),
	    __SHIFTOUT(issp_vers, HW_SSP_VERSION_MINOR));

	saa.saa_busname = "sdmmc";
	saa.saa_sct	= &issp_functions;
	saa.saa_spi_sct	= NULL;
	saa.saa_sch	= sc;
	saa.saa_dmat	= aa->aa_dmat;
	saa.saa_clkmin	= SSP_CLK_MIN;
	saa.saa_clkmax	= SSP_CLK_MAX;
	/* Add SMC_CAPS_DMA capability when DMA funtionality is implemented. */
	saa.saa_caps	= SMC_CAPS_4BIT_MODE | SMC_CAPS_SINGLE_ONLY;

	sc->sc_sdmmc = config_found(sc->sc_dev, &saa, NULL);
	if (sc->sc_sdmmc == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to attach sdmmc\n");
		return;
	}

	issp_attached = 1;

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
	/* SSP supports at least 3.2-3.3v */
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
	/* struct issp_softc *sc = sch;
	 *
	 * In the perfect world I'll just:
	 * 	return SSP_RD(sc, HW_SSP_STATUS) & HW_SSP_STATUS_CARD_DETECT;
	 * and call it a day.
	 *
	 * But on i.MX23 OLinuXino MAXI, SSP1_DETECT is not used for the SD
	 * card detection but SSP1_DATA3 is, as Tsvetan put it:
	 *
	 * < Tsvetan> if you want to know if SD card is inserted watch
	 * 		CD/DAT3/CS port
	 * < Tsvetan> without card there is R20 weak pulldown
	 * < Tsvetan> all cards have 40K pullup to this pin
	 * < Tsvetan> so when card is inserted you will read it high
	 *
	 * Which means I should to do something like this:
	 * 	#if BOARDTYPE == MAXI (Possibly MINI & MICRO)
	 * 		return GPIO_READ(PIN_125) & PIN_125
	 * 	#else
	 * 		return SSP_RD(sc, STATUS) & CARD_DETECT;
	 * 	#endif
	 * Until GPIO functionality is not present I am just going to */

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

	sck = issp_set_sck(sc, clock * 1000);

	/* Notify user if we didn't get exact clock rate from SSP that was
	 * requested. */
	if (sck != clock * 1000)
		aprint_normal_dev(sc->sc_dev, "requested clock %dHz, "
		    "but got %dHz\n", clock * 1000, sck);

	return 0;
}

static int
issp_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct issp_softc *sc = sch;
	uint32_t reg;

	reg = SSP_RD(sc, HW_SSP_CTRL0);
	reg &= ~(HW_SSP_CTRL0_BUS_WIDTH);

	switch(width) {
	case(1):
		reg |= __SHIFTIN(BUS_WIDTH_1_BIT, HW_SSP_CTRL0_BUS_WIDTH);
		break;
	case(4):
		reg |= __SHIFTIN(BUS_WIDTH_4_BIT, HW_SSP_CTRL0_BUS_WIDTH);
		break;
	case(8):
		reg |= __SHIFTIN(BUS_WIDTH_8_BIT, HW_SSP_CTRL0_BUS_WIDTH);
		break;
	default:
		return 1;
	}

	SSP_WR(sc, HW_SSP_CTRL0, reg);

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
	struct issp_softc *sc = sch;
	uint32_t reg;
	uint32_t do_blkio;
	uint32_t i;

	do_blkio = 0;

	/* Wait until SSP done. (data I/O error + retry...) */
	while (SSP_RD(sc, HW_SSP_STATUS) & SSP_BUSY)
                       ;

	/* Set expected response type. */
	SSP_WR(sc, HW_SSP_CTRL0_CLR,
	    HW_SSP_CTRL0_GET_RESP | HW_SSP_CTRL0_LONG_RESP);

	if (ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		SSP_WR(sc, HW_SSP_CTRL0_SET, HW_SSP_CTRL0_GET_RESP);
		if (ISSET(cmd->c_flags, SCF_RSP_136))
			SSP_WR(sc, HW_SSP_CTRL0_SET, HW_SSP_CTRL0_LONG_RESP);
	}

	/* If CMD does not need CRC validation, tell it to SSP. */
	if (ISSET(cmd->c_flags, SCF_RSP_CRC))
		SSP_WR(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_IGNORE_CRC);
	else
		SSP_WR(sc, HW_SSP_CTRL0_SET, HW_SSP_CTRL0_IGNORE_CRC);

	/* Set command. */
	SSP_WR(sc, HW_SSP_CMD0_CLR, HW_SSP_CMD0_CMD);
	SSP_WR(sc, HW_SSP_CMD0_SET,
	    __SHIFTIN(cmd->c_opcode, HW_SSP_CMD0_CMD));

	/* Set command argument. */
	SSP_WR(sc, HW_SSP_CMD1, cmd->c_arg);

	/* Is data to be transferred? */
	if (cmd->c_datalen > 0 && cmd->c_data != NULL) {
		SSP_WR(sc, HW_SSP_CTRL0_SET, HW_SSP_CTRL0_DATA_XFER);
		/* Transfer XFER_COUNT of 8-bit words. */
		SSP_WR(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_XFER_COUNT);
		SSP_WR(sc, HW_SSP_CTRL0_SET,
		    __SHIFTIN(cmd->c_datalen, HW_SSP_CTRL0_XFER_COUNT));

		/* XXX: why 8CYC? Bit is never cleaned. */
		SSP_WR(sc, HW_SSP_CMD0_SET, HW_SSP_CMD0_APPEND_8CYC);

		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			/* Read mode. */
			do_blkio |= BLKIO_RD;
			SSP_WR(sc, HW_SSP_CTRL0_SET, HW_SSP_CTRL0_READ);
		} else {
			/* Write mode. */
			do_blkio |= BLKIO_WR;
			SSP_WR(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_READ);
		}
	} else {
		/* No data to be transferred. */
		SSP_WR(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_DATA_XFER);
	}

	/* Run the command. */
	SSP_WR(sc, HW_SSP_CTRL0_SET, HW_SSP_CTRL0_RUN);

	if (ISSET(do_blkio, BLKIO_RD)) {
		for (i = 0; i < cmd->c_datalen / 4; i++) {
			/* Wait until data arrives to FIFO. */
			while (SSP_RD(sc, HW_SSP_STATUS)
			    & HW_SSP_STATUS_FIFO_EMPTY) {
				/* Abort if error while waiting. */
				if (SSP_RD(sc, HW_SSP_STATUS) & SSP_RUN_ERR) {
					aprint_normal_dev(sc->sc_dev,
					    "RD_ERR: %x\n",
					    SSP_RD(sc, HW_SSP_STATUS));
					cmd->c_error = 1;
					goto pioerr;
				}
			}
			*((uint32_t *)cmd->c_data+i) = SSP_RD(sc, HW_SSP_DATA);
		}
	} else if (ISSET(do_blkio, BLKIO_WR)) {
		for (i = 0; i < (cmd->c_datalen / 4); i++) {
			while (SSP_RD(sc, HW_SSP_STATUS)
			    & HW_SSP_STATUS_FIFO_FULL) {
				/* Abort if error while waiting. */
				if (SSP_RD(sc, HW_SSP_STATUS) & SSP_RUN_ERR) {
					aprint_normal_dev(sc->sc_dev,
					    "WR_ERR: %x\n",
					    SSP_RD(sc, HW_SSP_STATUS));
					cmd->c_error = 1;
					goto pioerr;
				}
			}
			SSP_WR(sc, HW_SSP_DATA, *((uint32_t *)cmd->c_data+i));
		}
	}

	/* Wait until SSP is done. */
	while (SSP_RD(sc, HW_SSP_STATUS) & SSP_BUSY)
                       ;

	/* Check if the command ran successfully. */
	reg = SSP_RD(sc, HW_SSP_STATUS);
	if (reg & SSP_RUN_ERR)
		cmd->c_error = reg & SSP_RUN_ERR;

	/* Read response if such was requested. */
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
pioerr:
	return;
}

static void
issp_card_enable_intr(sdmmc_chipset_handle_t sch, int irq)
{
	struct issp_softc *sc = sch;

	aprint_normal_dev(sc->sc_dev,
	    "issp_card_enable_intr NOT IMPLEMENTED!\n");

	return;
}

static void
issp_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	struct issp_softc *sc = sch;

	aprint_normal_dev(sc->sc_dev, "issp_card_intr_ack NOT IMPLEMENTED!\n");

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
 * DATA_TIMEOUT is calculated as:
 * (1 / SSP_CLK) * (DATA_TIMEOUT * 4096)
 */
#define DATA_TIMEOUT 0x4240	/* 723ms */

/*
 * Initialize SSP controller to SD/MMC mode.
 */
static void
issp_init(struct issp_softc *sc)
{
	uint32_t reg;

	/* Initial data bus width is 1-bit. */
	reg = SSP_RD(sc, HW_SSP_CTRL0);
	reg &= ~(HW_SSP_CTRL0_BUS_WIDTH);
	reg |= __SHIFTIN(BUS_WIDTH_1_BIT, HW_SSP_CTRL0_BUS_WIDTH) |
	    HW_SSP_CTRL0_WAIT_FOR_IRQ | HW_SSP_CTRL0_ENABLE;
	SSP_WR(sc, HW_SSP_CTRL0, reg);

	/* Set data timeout. */
	reg = SSP_RD(sc, HW_SSP_TIMING);
	reg &= ~(HW_SSP_TIMING_TIMEOUT);
	reg |= __SHIFTIN(DATA_TIMEOUT, HW_SSP_TIMING_TIMEOUT);

	/* Set initial clock rate to minimum. */
	issp_set_sck(sc, SSP_CLK_MIN * 1000);

	SSP_WR(sc, HW_SSP_TIMING, reg);
	/* Enable SD/MMC mode and use use 8-bits per word. */
	reg = SSP_RD(sc, HW_SSP_CTRL1);
	reg &= ~(HW_SSP_CTRL1_WORD_LENGTH | HW_SSP_CTRL1_SSP_MODE);
	reg |= HW_SSP_CTRL1_POLARITY |
	    __SHIFTIN(0x7, HW_SSP_CTRL1_WORD_LENGTH) |
	    __SHIFTIN(0x3, HW_SSP_CTRL1_SSP_MODE);
	SSP_WR(sc, HW_SSP_CTRL1, reg);

	return;
}

/*
 * Set SSP_SCK clock rate to the value specified in target.
 *
 * SSP_SCK is calculated as: SSP_CLK / (CLOCK_DIVIDE * (1 + CLOCK_RATE))
 *
 * issp_set_sck find the most suitable CLOCK_DIVIDE and CLOCK_RATE register
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
