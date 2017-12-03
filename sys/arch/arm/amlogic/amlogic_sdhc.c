/* $NetBSD: amlogic_sdhc.c,v 1.12.16.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amlogic_sdhc.c,v 1.12.16.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmc_ioreg.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_sdhcreg.h>
#include <arm/amlogic/amlogic_var.h>

static int	amlogic_sdhc_match(device_t, cfdata_t, void *);
static void	amlogic_sdhc_attach(device_t, device_t, void *);
static void	amlogic_sdhc_attach_i(device_t);

static int	amlogic_sdhc_intr(void *);

struct amlogic_sdhc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;
	void			*sc_ih;

	device_t		sc_sdmmc_dev;
	kmutex_t		sc_intr_lock;
	kcondvar_t		sc_intr_cv;

	uint32_t		sc_intr_ista;

	bus_dmamap_t		sc_dmamap;
	bus_dma_segment_t	sc_segs[1];
	void			*sc_bbuf;

	int			sc_port;
	int			sc_signal_voltage;
};

CFATTACH_DECL_NEW(amlogic_sdhc, sizeof(struct amlogic_sdhc_softc),
	amlogic_sdhc_match, amlogic_sdhc_attach, NULL, NULL);

static int	amlogic_sdhc_host_reset(sdmmc_chipset_handle_t);
static uint32_t	amlogic_sdhc_host_ocr(sdmmc_chipset_handle_t);
static int	amlogic_sdhc_host_maxblklen(sdmmc_chipset_handle_t);
static int	amlogic_sdhc_card_detect(sdmmc_chipset_handle_t);
static int	amlogic_sdhc_write_protect(sdmmc_chipset_handle_t);
static int	amlogic_sdhc_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	amlogic_sdhc_bus_clock(sdmmc_chipset_handle_t, int);
static int	amlogic_sdhc_bus_width(sdmmc_chipset_handle_t, int);
static int	amlogic_sdhc_bus_rod(sdmmc_chipset_handle_t, int);
static void	amlogic_sdhc_exec_command(sdmmc_chipset_handle_t,
				     struct sdmmc_command *);
static void	amlogic_sdhc_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	amlogic_sdhc_card_intr_ack(sdmmc_chipset_handle_t);
static int	amlogic_sdhc_signal_voltage(sdmmc_chipset_handle_t, int);
static int	amlogic_sdhc_execute_tuning(sdmmc_chipset_handle_t, int);

static int	amlogic_sdhc_default_rx_phase(struct amlogic_sdhc_softc *);
static int	amlogic_sdhc_set_clock(struct amlogic_sdhc_softc *, u_int);
static int	amlogic_sdhc_wait_idle(struct amlogic_sdhc_softc *);
static int	amlogic_sdhc_wait_ista(struct amlogic_sdhc_softc *, uint32_t, int);

static void	amlogic_sdhc_dmainit(struct amlogic_sdhc_softc *);

static struct sdmmc_chip_functions amlogic_sdhc_chip_functions = {
	.host_reset = amlogic_sdhc_host_reset,
	.host_ocr = amlogic_sdhc_host_ocr,
	.host_maxblklen = amlogic_sdhc_host_maxblklen,
	.card_detect = amlogic_sdhc_card_detect,
	.write_protect = amlogic_sdhc_write_protect,
	.bus_power = amlogic_sdhc_bus_power,
	.bus_clock = amlogic_sdhc_bus_clock,
	.bus_width = amlogic_sdhc_bus_width,
	.bus_rod = amlogic_sdhc_bus_rod,
	.exec_command = amlogic_sdhc_exec_command,
	.card_enable_intr = amlogic_sdhc_card_enable_intr,
	.card_intr_ack = amlogic_sdhc_card_intr_ack,
	.signal_voltage = amlogic_sdhc_signal_voltage,
	.execute_tuning = amlogic_sdhc_execute_tuning,
};

#define SDHC_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define SDHC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define SDHC_SET_CLEAR(sc, reg, set, clr) \
	amlogic_reg_set_clear((sc)->sc_bst, (sc)->sc_bsh, (reg), (set), (clr))

static int
amlogic_sdhc_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
amlogic_sdhc_attach(device_t parent, device_t self, void *aux)
{
	struct amlogic_sdhc_softc * const sc = device_private(self);
	struct amlogicio_attach_args * const aio = aux;
	const struct amlogic_locators * const loc = &aio->aio_loc;
	prop_dictionary_t cfg = device_properties(self);
	uint32_t boot_id;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	bus_space_subregion(aio->aio_core_bst, aio->aio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_intr_cv, "sdhcintr");
	sc->sc_port = loc->loc_port;
	sc->sc_signal_voltage = SDMMC_SIGNAL_VOLTAGE_330;

	if (sc->sc_port == AMLOGICIOCF_PORT_DEFAULT) {
		if (!prop_dictionary_get_uint32(cfg, "boot_id", &boot_id)) {
			aprint_error(": no port selected\n");
			return;
		}
		/* Booted device goes on SDHC controller */
		if (boot_id == 0) {
			sc->sc_port = AMLOGIC_SDHC_PORT_C;	/* eMMC */
		} else {
			sc->sc_port = AMLOGIC_SDHC_PORT_B;	/* SD card */
		}
	}

	amlogic_sdhc_init();
	if (amlogic_sdhc_select_port(sc->sc_port) != 0) {
		aprint_error(": couldn't select port %d\n", sc->sc_port);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": SDHC controller (port %c)\n", sc->sc_port + 'A');

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_BIO, IST_EDGE,
	    amlogic_sdhc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	amlogic_sdhc_dmainit(sc);

	config_interrupts(self, amlogic_sdhc_attach_i);
}

static void
amlogic_sdhc_attach_i(device_t self)
{
	struct amlogic_sdhc_softc *sc = device_private(self);
	struct sdmmcbus_attach_args saa;
	u_int pll_freq;

	pll_freq = amlogic_get_rate_fixed() / 1000 / 3;

	amlogic_sdhc_host_reset(sc);
	amlogic_sdhc_bus_width(sc, 1);

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &amlogic_sdhc_chip_functions;
	saa.saa_dmat = sc->sc_dmat;
	saa.saa_sch = sc;
	saa.saa_clkmin = 400;
	saa.saa_clkmax = pll_freq;
	/* Do not advertise DMA capabilities, we handle DMA ourselves */
	saa.saa_caps = SMC_CAPS_4BIT_MODE|
		       SMC_CAPS_SD_HIGHSPEED|
		       SMC_CAPS_MMC_HIGHSPEED|
		       SMC_CAPS_UHS_SDR50|
		       SMC_CAPS_UHS_SDR104|
		       SMC_CAPS_AUTO_STOP;

	if (sc->sc_port == AMLOGIC_SDHC_PORT_C) {
		saa.saa_caps |= SMC_CAPS_MMC_HS200;
		saa.saa_caps |= SMC_CAPS_8BIT_MODE;
	}

	sc->sc_sdmmc_dev = config_found(self, &saa, NULL);
}

static int
amlogic_sdhc_intr(void *priv)
{
	struct amlogic_sdhc_softc *sc = priv;
	uint32_t ista;

	mutex_enter(&sc->sc_intr_lock);
	ista = SDHC_READ(sc, SD_ISTA_REG);

	if (!ista) {
		mutex_exit(&sc->sc_intr_lock);
		return 0;
	}

	SDHC_WRITE(sc, SD_ISTA_REG, ista);

	sc->sc_intr_ista |= ista;
	cv_broadcast(&sc->sc_intr_cv);

	mutex_exit(&sc->sc_intr_lock);

	return 1;
}

static void
amlogic_sdhc_dmainit(struct amlogic_sdhc_softc *sc)
{
	int error, rseg;

	error = bus_dmamem_alloc(sc->sc_dmat, MAXPHYS, PAGE_SIZE, MAXPHYS,
	    sc->sc_segs, 1, &rseg, BUS_DMA_WAITOK);
	if (error) {
		device_printf(sc->sc_dev, "bus_dmamem_alloc failed: %d\n", error);
		return;
	}
	KASSERT(rseg == 1);

	error = bus_dmamem_map(sc->sc_dmat, sc->sc_segs, rseg, MAXPHYS,
	    &sc->sc_bbuf, BUS_DMA_WAITOK);
	if (error) {
		device_printf(sc->sc_dev, "bus_dmamem_map failed\n");
		return;
	}

	error = bus_dmamap_create(sc->sc_dmat, MAXPHYS, 1, MAXPHYS, 0,
	    BUS_DMA_WAITOK, &sc->sc_dmamap);
	if (error) {
		device_printf(sc->sc_dev, "bus_dmamap_create failed\n");
		return;
	}

}

static int
amlogic_sdhc_default_rx_phase(struct amlogic_sdhc_softc *sc)
{
	const u_int pll_freq = amlogic_get_rate_fixed() / 1000 / 3;
	const u_int clkc = SDHC_READ(sc, SD_CLKC_REG);
	const u_int clk_div = __SHIFTOUT(clkc, SD_CLKC_CLK_DIV);
	const u_int act_freq = pll_freq / clk_div;

	if (act_freq > 90000) {
		return 1;
	} else if (act_freq > 45000) {
		if (sc->sc_signal_voltage == SDMMC_SIGNAL_VOLTAGE_330) {
			return 15;
		} else {
			return 11;
		}
	} else if (act_freq >= 25000) {
		return 15;
	} else if (act_freq > 5000) {
		return 23;
	} else if (act_freq > 1000) {
		return 55;
	} else {
		return 1061;
	}
}

static int
amlogic_sdhc_set_clock(struct amlogic_sdhc_softc *sc, u_int freq)
{
	uint32_t clkc;
	uint32_t clk2;
	u_int pll_freq, clk_div;

	clkc = SDHC_READ(sc, SD_CLKC_REG);
	clkc &= ~SD_CLKC_TX_CLK_ENABLE;
	clkc &= ~SD_CLKC_RX_CLK_ENABLE;
	clkc &= ~SD_CLKC_SD_CLK_ENABLE;
	SDHC_WRITE(sc, SD_CLKC_REG, clkc);
	clkc &= ~SD_CLKC_MOD_CLK_ENABLE;
	SDHC_WRITE(sc, SD_CLKC_REG, clkc);

	if (freq == 0)
		return 0;

	clkc &= ~SD_CLKC_CLK_DIV;
	clkc &= ~SD_CLKC_CLK_IN_SEL;

	clkc |= __SHIFTIN(SD_CLKC_CLK_IN_SEL_FCLK_DIV3,
			  SD_CLKC_CLK_IN_SEL);

	pll_freq = amlogic_get_rate_fixed() / 1000;	/* 2.55GHz */
	pll_freq /= 3;	/* for SD_CLKC_CLK_IN_SEL_FCLK_DIV3 */
	clk_div = howmany(pll_freq, freq);

	clkc |= __SHIFTIN(clk_div - 1, SD_CLKC_CLK_DIV);

	SDHC_WRITE(sc, SD_CLKC_REG, clkc);

	clkc |= SD_CLKC_MOD_CLK_ENABLE;
	SDHC_WRITE(sc, SD_CLKC_REG, clkc);

	clkc |= SD_CLKC_TX_CLK_ENABLE;
	clkc |= SD_CLKC_RX_CLK_ENABLE;
	clkc |= SD_CLKC_SD_CLK_ENABLE;
	SDHC_WRITE(sc, SD_CLKC_REG, clkc);

	clk2 = SDHC_READ(sc, SD_CLK2_REG);
	clk2 &= ~SD_CLK2_SD_CLK_PHASE;
	clk2 |= __SHIFTIN(1, SD_CLK2_SD_CLK_PHASE);
	clk2 &= ~SD_CLK2_RX_CLK_PHASE;
	clk2 |= __SHIFTIN(amlogic_sdhc_default_rx_phase(sc),
			  SD_CLK2_RX_CLK_PHASE);
	SDHC_WRITE(sc, SD_CLK2_REG, clk2);

	return 0;
}

static int
amlogic_sdhc_wait_idle(struct amlogic_sdhc_softc *sc)
{
	int i;

	for (i = 0; i < 1000000; i++) {
		const uint32_t stat = SDHC_READ(sc, SD_STAT_REG);
		const uint32_t esta = SDHC_READ(sc, SD_ESTA_REG);
		if ((stat & SD_STAT_BUSY) == 0 &&
		    (esta & SD_ESTA_BUSY) == 0)
			return 0;
		delay(1);
	}

	return EBUSY;
}

static int
amlogic_sdhc_wait_ista(struct amlogic_sdhc_softc *sc, uint32_t mask, int timeout)
{
	int retry, error;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	if (sc->sc_intr_ista & mask)
		return 0;

	retry = timeout / hz;

	while (retry > 0) {
		error = cv_timedwait(&sc->sc_intr_cv, &sc->sc_intr_lock, hz);
		if (error && error != EWOULDBLOCK)
			return error;
		if (sc->sc_intr_ista & mask)
			return 0;
		--retry;
	}

	return ETIMEDOUT;
}

static int
amlogic_sdhc_host_reset(sdmmc_chipset_handle_t sch)
{
	struct amlogic_sdhc_softc *sc = sch;
	uint32_t enhc;

	SDHC_WRITE(sc, SD_SRST_REG,
	    SD_SRST_MAIN_CTRL | SD_SRST_TX_FIFO | SD_SRST_RX_FIFO |
	    SD_SRST_DPHY_TX | SD_SRST_DPHY_RX | SD_SRST_DMA_IF);

	delay(50);

	SDHC_WRITE(sc, SD_SRST_REG, 0);

	delay(10);

	SDHC_WRITE(sc, SD_CNTL_REG,
	    __SHIFTIN(0x7, SD_CNTL_TX_ENDIAN_CTRL) |
	    __SHIFTIN(0x7, SD_CNTL_RX_ENDIAN_CTRL) |
	    __SHIFTIN(0xf, SD_CNTL_RX_PERIOD) |
	    __SHIFTIN(0x7f, SD_CNTL_RX_TIMEOUT));

	SDHC_WRITE(sc, SD_CLKC_REG,
	    SDHC_READ(sc, SD_CLKC_REG) & ~SD_CLKC_MEM_PWR);

	SDHC_WRITE(sc, SD_PDMA_REG,
	    __SHIFTIN(7, SD_PDMA_TX_BURST_LEN) |
	    __SHIFTIN(49, SD_PDMA_TXFIFO_THRESHOLD) |
	    __SHIFTIN(15, SD_PDMA_RX_BURST_LEN) |
	    __SHIFTIN(7, SD_PDMA_RXFIFO_THRESHOLD) |
	    SD_PDMA_DMA_URGENT);

	SDHC_WRITE(sc, SD_MISC_REG,
	    __SHIFTIN(7, SD_MISC_TXSTART_THRESHOLD) |
	    __SHIFTIN(5, SD_MISC_WCRC_ERR_PATTERN) |
	    __SHIFTIN(2, SD_MISC_WCRC_OK_PATTERN));

	enhc = SDHC_READ(sc, SD_ENHC_REG);
	enhc &= ~SD_ENHC_RXFIFO_THRESHOLD;
	enhc |= __SHIFTIN(63, SD_ENHC_RXFIFO_THRESHOLD);
	enhc &= ~SD_ENHC_DMA_RX_RESP;
	enhc |= SD_ENHC_DMA_TX_RESP;
	enhc &= ~SD_ENHC_SDIO_IRQ_PERIOD;
	enhc |= __SHIFTIN(12, SD_ENHC_SDIO_IRQ_PERIOD);
	enhc &= ~SD_ENHC_RX_TIMEOUT;
	enhc |= __SHIFTIN(0xff, SD_ENHC_RX_TIMEOUT);
	SDHC_WRITE(sc, SD_ENHC_REG, enhc);

	SDHC_WRITE(sc, SD_ICTL_REG, 0);
	SDHC_WRITE(sc, SD_ISTA_REG, SD_INT_CLEAR);

	return 0;
}

static uint32_t
amlogic_sdhc_host_ocr(sdmmc_chipset_handle_t sch)
{
	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V |
	       MMC_OCR_HCS | MMC_OCR_S18A;
}

static int
amlogic_sdhc_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 512;
}

static int
amlogic_sdhc_card_detect(sdmmc_chipset_handle_t sch)
{
	struct amlogic_sdhc_softc *sc = sch;

	return amlogic_sdhc_is_card_present(sc->sc_port);
}

static int
amlogic_sdhc_write_protect(sdmmc_chipset_handle_t sch)
{
	return 0;
}

static int
amlogic_sdhc_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	return 0;
}

static int
amlogic_sdhc_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct amlogic_sdhc_softc *sc = sch;

	return amlogic_sdhc_set_clock(sc, freq);
}

static int
amlogic_sdhc_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct amlogic_sdhc_softc *sc = sch;
	uint32_t cntl;

	cntl = SDHC_READ(sc, SD_CNTL_REG);
	cntl &= ~SD_CNTL_DAT_TYPE;
	switch (width) {
	case 1:
		cntl |= __SHIFTIN(0, SD_CNTL_DAT_TYPE);
		break;
	case 4:
		cntl |= __SHIFTIN(1, SD_CNTL_DAT_TYPE);
		break;
	case 8:
		cntl |= __SHIFTIN(2, SD_CNTL_DAT_TYPE);
		break;
	default:
		return EINVAL;
	}

	SDHC_WRITE(sc, SD_CNTL_REG, cntl);

	return 0;
}

static int
amlogic_sdhc_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	return ENOTSUP;
}

static void
amlogic_sdhc_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct amlogic_sdhc_softc *sc = sch;
	uint32_t cmdval = 0, cntl, srst, pdma, ictl;
	bool use_bbuf = false;
	int i;

	KASSERT(cmd->c_blklen <= 512);

	mutex_enter(&sc->sc_intr_lock);

	/* Filter SDIO commands */
	switch (cmd->c_opcode) {
	case SD_IO_SEND_OP_COND:
	case SD_IO_RW_DIRECT:
	case SD_IO_RW_EXTENDED:
		cmd->c_error = EINVAL;
		goto done;
	}

	if (cmd->c_opcode == MMC_STOP_TRANSMISSION)
		cmdval |= SD_SEND_DATA_STOP;
	if (cmd->c_flags & SCF_RSP_PRESENT)
		cmdval |= SD_SEND_COMMAND_HAS_RESP;
	if (cmd->c_flags & SCF_RSP_136) {
		cmdval |= SD_SEND_RESPONSE_LENGTH;
		cmdval |= SD_SEND_RESPONSE_NO_CRC;
	}
	if ((cmd->c_flags & SCF_RSP_CRC) == 0)
		cmdval |= SD_SEND_RESPONSE_NO_CRC;

	SDHC_WRITE(sc, SD_ICTL_REG, 0);
	SDHC_WRITE(sc, SD_ISTA_REG, SD_INT_CLEAR);
	sc->sc_intr_ista = 0;

	ictl = SD_INT_ERROR;

	cntl = SDHC_READ(sc, SD_CNTL_REG);
	cntl &= ~SD_CNTL_PACK_LEN;
	if (cmd->c_datalen > 0) {
		unsigned int nblks;

		cmdval |= SD_SEND_COMMAND_HAS_DATA;
		if (!ISSET(cmd->c_flags, SCF_CMD_READ)) {
			cmdval |= SD_SEND_DATA_DIRECTION;
		}

		nblks = cmd->c_datalen / cmd->c_blklen;
		if (nblks == 0 || (cmd->c_datalen % cmd->c_blklen) != 0)
			++nblks;

		cntl |= __SHIFTIN(cmd->c_blklen & 0x1ff, SD_CNTL_PACK_LEN);
				    
		cmdval |= __SHIFTIN(nblks - 1, SD_SEND_TOTAL_PACK);

		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			ictl |= SD_INT_DATA_COMPLETE;
		} else {
			ictl |= SD_INT_DMA_DONE;
		}
	} else {
		ictl |= SD_INT_RESP_COMPLETE;
	}

	SDHC_WRITE(sc, SD_ICTL_REG, ictl);

	SDHC_WRITE(sc, SD_CNTL_REG, cntl);

	pdma = SDHC_READ(sc, SD_PDMA_REG);
	if (cmd->c_datalen > 0) {
		pdma |= SD_PDMA_DMA_MODE;
	} else {
		pdma &= ~SD_PDMA_DMA_MODE;
	}
	SDHC_WRITE(sc, SD_PDMA_REG, pdma);

	SDHC_WRITE(sc, SD_ARGU_REG, cmd->c_arg);

	cmd->c_error = amlogic_sdhc_wait_idle(sc);
	if (cmd->c_error) {
		goto done;
	}

	if (cmd->c_datalen > 0) {
		cmd->c_error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap,
		    sc->sc_bbuf, MAXPHYS, NULL, BUS_DMA_WAITOK);
		if (cmd->c_error) {
			device_printf(sc->sc_dev, "bus_dmamap_load failed\n");
			goto done;
		}
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
			    MAXPHYS, BUS_DMASYNC_PREREAD);
		} else {
			memcpy(sc->sc_bbuf, cmd->c_data, cmd->c_datalen);
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
			    MAXPHYS, BUS_DMASYNC_PREWRITE);
		}
		SDHC_WRITE(sc, SD_ADDR_REG, sc->sc_dmamap->dm_segs[0].ds_addr);
		use_bbuf = true;
	}

	cmd->c_resid = cmd->c_datalen;
	SDHC_WRITE(sc, SD_SEND_REG, cmdval | cmd->c_opcode);

	if (cmd->c_datalen > 0) {
		uint32_t wbit = ISSET(cmd->c_flags, SCF_CMD_READ) ?
		    SD_INT_DATA_COMPLETE : SD_INT_DMA_DONE;
		cmd->c_error = amlogic_sdhc_wait_ista(sc,
		    SD_INT_ERROR | wbit, hz * 10);
		if (cmd->c_error == 0 &&
		    (sc->sc_intr_ista & SD_INT_ERROR)) {
			cmd->c_error = ETIMEDOUT;
		}
		if (cmd->c_error) {
			goto done;
		}
	} else {
		cmd->c_error = amlogic_sdhc_wait_ista(sc,
		    SD_INT_ERROR | SD_INT_RESP_COMPLETE, hz * 10);
		if (cmd->c_error == 0 && (sc->sc_intr_ista & SD_INT_ERROR)) {
			if (sc->sc_intr_ista & SD_INT_TIMEOUT) {
				cmd->c_error = ETIMEDOUT;
			} else {
				cmd->c_error = EIO;
			}
		}
		if (cmd->c_error) {
			goto done;
		}
	}

	SDHC_WRITE(sc, SD_ISTA_REG, sc->sc_intr_ista);

	if (cmd->c_flags & SCF_RSP_PRESENT) {
		pdma = SDHC_READ(sc, SD_PDMA_REG);
		pdma &= ~SD_PDMA_DMA_MODE;
		if (cmd->c_flags & SCF_RSP_136) {
			for (i = 4; i >= 1; i--) {
				pdma &= ~SD_PDMA_PIO_RDRESP;
				pdma |= __SHIFTIN(i, SD_PDMA_PIO_RDRESP);
				SDHC_WRITE(sc, SD_PDMA_REG, pdma);
				cmd->c_resp[i - 1] = SDHC_READ(sc, SD_ARGU_REG);
				
			}
			if (cmd->c_flags & SCF_RSP_CRC) {
				cmd->c_resp[0] = (cmd->c_resp[0] >> 8) |
				    (cmd->c_resp[1] << 24);
				cmd->c_resp[1] = (cmd->c_resp[1] >> 8) |
				    (cmd->c_resp[2] << 24);
				cmd->c_resp[2] = (cmd->c_resp[2] >> 8) |
				    (cmd->c_resp[3] << 24);
				cmd->c_resp[3] = (cmd->c_resp[3] >> 8);
			}
		} else {
			pdma &= ~SD_PDMA_PIO_RDRESP;
			pdma |= __SHIFTIN(0, SD_PDMA_PIO_RDRESP);
			SDHC_WRITE(sc, SD_PDMA_REG, pdma);
			cmd->c_resp[0] = SDHC_READ(sc, SD_ARGU_REG);
		}
	}

done:
	if (use_bbuf) {
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
			    MAXPHYS, BUS_DMASYNC_POSTREAD);
		} else {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
			    MAXPHYS, BUS_DMASYNC_POSTWRITE);
		}
		bus_dmamap_unload(sc->sc_dmat, sc->sc_dmamap);
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			memcpy(cmd->c_data, sc->sc_bbuf, cmd->c_datalen);
		}
	}

	cmd->c_flags |= SCF_ITSDONE;

	SDHC_WRITE(sc, SD_ISTA_REG, SD_INT_CLEAR);
	SDHC_WRITE(sc, SD_ICTL_REG, 0);

	srst = SDHC_READ(sc, SD_SRST_REG);
	srst |= (SD_SRST_TX_FIFO | SD_SRST_RX_FIFO);
	SDHC_WRITE(sc, SD_SRST_REG, srst);

	mutex_exit(&sc->sc_intr_lock);
}

static void
amlogic_sdhc_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
}

static void
amlogic_sdhc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
}

static int
amlogic_sdhc_signal_voltage(sdmmc_chipset_handle_t sch, int signal_voltage)
{
	struct amlogic_sdhc_softc *sc = sch;

	switch (signal_voltage) {
	case SDMMC_SIGNAL_VOLTAGE_330:
		amlogic_sdhc_set_voltage(sc->sc_port, AMLOGIC_SDHC_VOL_330);
		break;
	case SDMMC_SIGNAL_VOLTAGE_180:
		amlogic_sdhc_set_voltage(sc->sc_port, AMLOGIC_SDHC_VOL_180);
		break;
	default:
		return EINVAL;
	}

	sc->sc_signal_voltage = signal_voltage;
	return 0;
}

static int
amlogic_sdhc_execute_tuning(sdmmc_chipset_handle_t sch, int timing)
{
	static const uint8_t tuning_blk_8bit[] = {
		0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
		0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
		0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
		0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
		0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,  
		0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
		0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
		0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
		0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
		0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
		0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
		0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
		0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
		0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
		0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
		0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
	};
	static const uint8_t tuning_blk_4bit[] = {
		0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
		0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
		0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
		0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
		0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
		0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
		0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
		0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
	};

	struct amlogic_sdhc_softc *sc = sch;
	struct sdmmc_command cmd;
	uint8_t data[sizeof(tuning_blk_8bit)];
	const uint8_t *tblk;
	size_t tsize;
	struct window_s {
		int start;
		u_int size;
	} best = { .start = -1, .size = 0 },
	  curr = { .start = -1, .size = 0 },
	  wrap = { .start =  0, .size = 0 };
	u_int ph, rx_phase, clk_div;
	int opcode;

	switch (timing) {
	case SDMMC_TIMING_MMC_HS200:
		tblk = tuning_blk_8bit;
		tsize = sizeof(tuning_blk_8bit);
		opcode = MMC_SEND_TUNING_BLOCK_HS200;
		break;
	case SDMMC_TIMING_UHS_SDR50:
	case SDMMC_TIMING_UHS_SDR104:
		tblk = tuning_blk_4bit;
		tsize = sizeof(tuning_blk_4bit);
		opcode = MMC_SEND_TUNING_BLOCK;
		break;
	default:
		return EINVAL;
	}

	const uint32_t clkc = SDHC_READ(sc, SD_CLKC_REG);
	clk_div = __SHIFTOUT(clkc, SD_CLKC_CLK_DIV);

	for (ph = 0; ph <= clk_div; ph++) {
		SDHC_SET_CLEAR(sc, SD_CLK2_REG,
		    __SHIFTIN(ph, SD_CLK2_RX_CLK_PHASE), SD_CLK2_RX_CLK_PHASE);
		delay(10);

		u_int nmatch = 0;
#define NUMTRIES 10
		for (u_int i = 0; i < NUMTRIES; i++) {
			memset(data, 0, tsize);
			memset(&cmd, 0, sizeof(cmd));
			cmd.c_data = data;
			cmd.c_datalen = cmd.c_blklen = tsize;
			cmd.c_opcode = opcode;
			cmd.c_arg = 0;
			cmd.c_flags = SCF_CMD_ADTC | SCF_CMD_READ | SCF_RSP_R1;
			amlogic_sdhc_exec_command(sc, &cmd);
			if (cmd.c_error == 0 && memcmp(data, tblk, tsize) == 0)
				nmatch++;
		}
		if (nmatch == NUMTRIES) {	/* good phase value */
			if (wrap.start == 0)
				wrap.size++;
			if (curr.start == -1)
				curr.start = ph;
			curr.size++;
		} else {
			wrap.start = -1;
			if (curr.start != -1) {	/* end of current window */
				if (best.start == -1 || best.size < curr.size)
					best = curr;
				curr = (struct window_s)
				    { .start = -1, .size = 0 };
			}
		}
#undef NUMTRIES
	}

	if (curr.start != -1) {	/* the current window wraps around */
		curr.size += wrap.size;
		if (curr.size > ph)
			curr.size = ph;
		if (best.start == -1 || best.size < curr.size)
			best = curr;
	}

	if (best.start == -1) {	/* no window - use default rx_phase */
		rx_phase = amlogic_sdhc_default_rx_phase(sc);
	} else {
		rx_phase = best.start + best.size / 2;
		if (rx_phase >= ph)
			rx_phase -= ph;
	}

	SDHC_SET_CLEAR(sc, SD_CLK2_REG,
	    __SHIFTIN(rx_phase, SD_CLK2_RX_CLK_PHASE), SD_CLK2_RX_CLK_PHASE);

	return 0;
}
