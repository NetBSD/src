/* $Id: imx23_ssp.c,v 1.2.2.2 2013/01/16 05:32:47 yamt Exp $ */

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
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <arm/pic/picvar.h>

#include <arm/imx/imx23_apbdma.h>
#include <arm/imx/imx23_icollreg.h>
#include <arm/imx/imx23_sspreg.h>
#include <arm/imx/imx23var.h>

#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmcreg.h>
#include <dev/sdmmc/sdmmcvar.h>

/*
 * SD/MMC host controller driver for i.MX233.
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

/* sdmmc chip function prototypes. */
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

/* Used from the above callbacks. */
static void	issp_reset(struct issp_softc *);
static void	issp_init(struct issp_softc *);
static uint32_t	issp_set_sck(struct issp_softc *, uint32_t target);

#define SSP_SOFT_RST_LOOP 455	/* At least 1 us ... */

CFATTACH_DECL3_NEW(ssp,
	sizeof(struct issp_softc),
	issp_match,
	issp_attach,
	NULL,
	issp_activate,
	NULL,
	NULL,
	0);

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

#define SSP_READ(sc, reg)						\
	bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define SSP_WRITE(sc, reg, val)						\
	bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

#define SSP_CLK			96000000 /* CLK_SSP from PLL is 96 MHz */
#define SSP_CLK_MIN		2	 /* 2 kHz */
#define SSP_CLK_MAX		48000	 /* 48 MHz */
/* SSP_CMD_TIMEOUT is calculated as (1.0/SSP_SCK)*(SSP_CMD_TIMEOUT*4096) */
#define SSP_CMD_TIMEOUT		0xffff	/* 2.8 seconds. */
#define SSP_STATUS_ERR (HW_SSP_STATUS_RESP_CRC_ERR |			\
			HW_SSP_STATUS_RESP_ERR |			\
			HW_SSP_STATUS_RESP_TIMEOUT |			\
			HW_SSP_STATUS_DATA_CRC_ERR |			\
			HW_SSP_STATUS_TIMEOUT)

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
	static int issp_attached = 0;
	struct issp_softc *sc = device_private(self);
	struct apb_softc *scp = device_private(parent);
	struct apb_attach_args *aa = aux;
	struct sdmmcbus_attach_args saa;
	
	
	if (issp_attached)
		return;

//XXX:
	if (scp == NULL)
		printf("ISSP_ATTACH: scp == NULL\n");
	if (scp->dmac == NULL)
		printf("ISSP_ATTACH: scp->dmac == NULL\n");

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->dmac = scp->dmac;

	if (bus_space_map(sc->sc_iot,
	    aa->aa_addr, aa->aa_size, 0, &(sc->sc_hdl))) {
		aprint_error_dev(sc->sc_dev, "unable to map bus space\n");
		return;
	}

	issp_reset(sc);
	issp_init(sc); 

	uint32_t issp_vers = SSP_READ(sc, HW_SSP_VERSION);
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
	saa.saa_caps	= SMC_CAPS_4BIT_MODE | SMC_CAPS_DMA;

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
	/* XXX: This value was made up. */
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
	 * 	return SSP_READ(sc, HW_SSP_STATUS) & HW_SSP_STATUS_CARD_DETECT;
	 * and call it a day.
	 *
	 * But on i.MX233 OLinuXino MAXI, SSP1_DETECT is not used for the SD
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
	 * 		return SSP_READ(sc, STATUS) & CARD_DETECT;
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
issp_bus_power(sdmmc_chipset_handle_t sch, uint32_t power)
{
	/* i.MX233 does not support setting bus power. */
	return 0;
}

static int
issp_bus_clock(sdmmc_chipset_handle_t sch, int clock)
{
	struct issp_softc *sc = sch;
	uint32_t sck;

	aprint_normal_dev(sc->sc_dev, "requested clock %d Hz", clock * 1000);
	sck = issp_set_sck(sc, clock * 1000);
	aprint_normal(", got %d Hz\n", sck);

	return 0;
}

static int
issp_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	/* Return error if other than 4-bit width is requested. */
	return width - 4;
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

	/* Set excepted response type. */
	SSP_WRITE(sc, HW_SSP_CTRL0_CLR,
	    HW_SSP_CTRL0_GET_RESP | HW_SSP_CTRL0_LONG_RESP);

	if (ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		SSP_WRITE(sc, HW_SSP_CTRL0_SET, HW_SSP_CTRL0_GET_RESP);
		if (ISSET(cmd->c_flags, SCF_RSP_136))
			SSP_WRITE(sc, HW_SSP_CTRL0_SET, HW_SSP_CTRL0_LONG_RESP);
	}

	/* If CMD does not need CRC validation, tell it to SSP. */
	if (ISSET(cmd->c_flags, SCF_RSP_CRC))
		SSP_WRITE(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_IGNORE_CRC);
	else
		SSP_WRITE(sc, HW_SSP_CTRL0_SET, HW_SSP_CTRL0_IGNORE_CRC);

	/* Set command. */
	SSP_WRITE(sc, HW_SSP_CMD0_CLR, HW_SSP_CMD0_CMD);
	SSP_WRITE(sc, HW_SSP_CMD0_SET,
	    __SHIFTIN(cmd->c_opcode, HW_SSP_CMD0_CMD));

	/* Set command argument. */
	SSP_WRITE(sc, HW_SSP_CMD1, cmd->c_arg);

	/* Run the command. */
	SSP_WRITE(sc, HW_SSP_CTRL0_SET, HW_SSP_CTRL0_RUN);

	/* Wait until SSP has processed the command. */
	while (SSP_READ(sc, HW_SSP_STATUS) &
	    (HW_SSP_STATUS_CMD_BUSY | HW_SSP_STATUS_BUSY))
			;

	/* Check if the command ran without errors. */
	reg = SSP_READ(sc, HW_SSP_STATUS);
	
	if (reg & SSP_STATUS_ERR)
		cmd->c_error = reg & SSP_STATUS_ERR;

	/* Read response if such was requested. */
	if (ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		cmd->c_resp[0] = SSP_READ(sc, HW_SSP_SDRESP0);
		if (ISSET(cmd->c_flags, SCF_RSP_136)) {
		    cmd->c_resp[1] = SSP_READ(sc, HW_SSP_SDRESP1);
		    cmd->c_resp[2] = SSP_READ(sc, HW_SSP_SDRESP2);
		    cmd->c_resp[3] = SSP_READ(sc, HW_SSP_SDRESP3);
		}
	}

/*
	apbdma_dmamem_alloc()
	apbdma_do_dma()
	wait_until_done()
*/
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
 * Inspired by i.MX233 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
issp_reset(struct issp_softc *sc)
{
	unsigned int loop;

	/* Prepare for soft-reset by making sure that SFTRST is not currently
	 * asserted. Also clear CLKGATE so we can wait for its assertion below.
	 */
	SSP_WRITE(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_SFTRST);

	/* Wait at least a microsecond for SFTRST to deassert. */
	loop = 0;
	while ((SSP_READ(sc, HW_SSP_CTRL0) & HW_SSP_CTRL0_SFTRST) ||
	    (loop < SSP_SOFT_RST_LOOP))
		loop++;

	/* Clear CLKGATE so we can wait for its assertion below. */
	SSP_WRITE(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_CLKGATE);

	/* Soft-reset the block. */
	SSP_WRITE(sc, HW_SSP_CTRL0_SET, HW_SSP_CTRL0_SFTRST);

	/* Wait until clock is in the gated state. */
	while (!(SSP_READ(sc, HW_SSP_CTRL0) & HW_SSP_CTRL0_CLKGATE));

	/* Bring block out of reset. */
	SSP_WRITE(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_SFTRST);

	loop = 0;
	while ((SSP_READ(sc, HW_SSP_CTRL0) & HW_SSP_CTRL0_SFTRST) ||
	    (loop < SSP_SOFT_RST_LOOP))
		loop++;

	SSP_WRITE(sc, HW_SSP_CTRL0_CLR, HW_SSP_CTRL0_CLKGATE);
	
	/* Wait until clock is in the NON-gated state. */
	while (SSP_READ(sc, HW_SSP_CTRL0) & HW_SSP_CTRL0_CLKGATE);

	return;
}

/*
 * Initialize common options.
 */
static void
issp_init(struct issp_softc *sc)
{
	uint32_t reg;

	/* Initialize SD/MMC controller. */
	reg = SSP_READ(sc, HW_SSP_CTRL0);
	reg &= ~(HW_SSP_CTRL0_BUS_WIDTH);
	reg |= __SHIFTIN(0x1, HW_SSP_CTRL0_BUS_WIDTH) | HW_SSP_CTRL0_ENABLE;
	SSP_WRITE(sc, HW_SSP_CTRL0, reg);

	reg = SSP_READ(sc, HW_SSP_CTRL1);
	reg &= ~(HW_SSP_CTRL1_WORD_LENGTH | HW_SSP_CTRL1_SSP_MODE);
	reg |= HW_SSP_CTRL1_POLARITY |
	    __SHIFTIN(0x7, HW_SSP_CTRL1_WORD_LENGTH) |
	    __SHIFTIN(0x3, HW_SSP_CTRL1_SSP_MODE);
	SSP_WRITE(sc, HW_SSP_CTRL1, reg);

	/* Set command timeout. */
	reg = SSP_READ(sc, HW_SSP_TIMING);
	reg &= ~(HW_SSP_TIMING_TIMEOUT);
	reg |= __SHIFTIN(SSP_CMD_TIMEOUT, HW_SSP_TIMING_TIMEOUT);
	SSP_WRITE(sc, HW_SSP_TIMING, reg);

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
	reg = SSP_READ(sc, HW_SSP_TIMING);
	reg &= ~(HW_SSP_TIMING_CLOCK_DIVIDE | HW_SSP_TIMING_CLOCK_RATE);
	reg |= __SHIFTIN(div, HW_SSP_TIMING_CLOCK_DIVIDE) |
	    __SHIFTIN(rate, HW_SSP_TIMING_CLOCK_RATE);
	SSP_WRITE(sc, HW_SSP_TIMING, reg);

	return SSP_CLK / (d * (1 + r));
}
