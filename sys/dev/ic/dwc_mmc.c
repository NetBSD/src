/* $NetBSD: dwc_mmc.c,v 1.11.6.1 2018/06/25 07:25:50 pgoyette Exp $ */

/*-
 * Copyright (c) 2014-2017 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dwc_mmc.c,v 1.11.6.1 2018/06/25 07:25:50 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmc_ioreg.h>

#include <dev/ic/dwc_mmc_reg.h>
#include <dev/ic/dwc_mmc_var.h>

#define DWC_MMC_NDESC		64

static int	dwc_mmc_host_reset(sdmmc_chipset_handle_t);
static uint32_t	dwc_mmc_host_ocr(sdmmc_chipset_handle_t);
static int	dwc_mmc_host_maxblklen(sdmmc_chipset_handle_t);
static int	dwc_mmc_card_detect(sdmmc_chipset_handle_t);
static int	dwc_mmc_write_protect(sdmmc_chipset_handle_t);
static int	dwc_mmc_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	dwc_mmc_bus_clock(sdmmc_chipset_handle_t, int);
static int	dwc_mmc_bus_width(sdmmc_chipset_handle_t, int);
static int	dwc_mmc_bus_rod(sdmmc_chipset_handle_t, int);
static void	dwc_mmc_exec_command(sdmmc_chipset_handle_t,
				      struct sdmmc_command *);
static void	dwc_mmc_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	dwc_mmc_card_intr_ack(sdmmc_chipset_handle_t);

static struct sdmmc_chip_functions dwc_mmc_chip_functions = {
	.host_reset = dwc_mmc_host_reset,
	.host_ocr = dwc_mmc_host_ocr,
	.host_maxblklen = dwc_mmc_host_maxblklen,
	.card_detect = dwc_mmc_card_detect,
	.write_protect = dwc_mmc_write_protect,
	.bus_power = dwc_mmc_bus_power,
	.bus_clock = dwc_mmc_bus_clock,
	.bus_width = dwc_mmc_bus_width,
	.bus_rod = dwc_mmc_bus_rod,
	.exec_command = dwc_mmc_exec_command,
	.card_enable_intr = dwc_mmc_card_enable_intr,
	.card_intr_ack = dwc_mmc_card_intr_ack,
};

#define MMC_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define MMC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static void
dwc_mmc_dump_regs(struct dwc_mmc_softc *sc)
{
	device_printf(sc->sc_dev, "device registers:\n");
	for (u_int off = 0x00; off < 0x100; off += 16) {
		device_printf(sc->sc_dev, "xxxxxx%02x: %08x %08x %08x %08x\n",
		    off,
		    MMC_READ(sc, off + 0), MMC_READ(sc, off + 4),
		    MMC_READ(sc, off + 8), MMC_READ(sc, off + 12));
	}
}

static int
dwc_mmc_idma_setup(struct dwc_mmc_softc *sc)
{
	int error;

	sc->sc_idma_xferlen = 0x1000;

	sc->sc_idma_ndesc = DWC_MMC_NDESC;
	sc->sc_idma_size = sizeof(struct dwc_mmc_idma_desc) *
	    sc->sc_idma_ndesc;
	error = bus_dmamem_alloc(sc->sc_dmat, sc->sc_idma_size, 8,
	    sc->sc_idma_size, sc->sc_idma_segs, 1,
	    &sc->sc_idma_nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;
	error = bus_dmamem_map(sc->sc_dmat, sc->sc_idma_segs,
	    sc->sc_idma_nsegs, sc->sc_idma_size,
	    &sc->sc_idma_desc, BUS_DMA_WAITOK);
	if (error)
		goto free;
	error = bus_dmamap_create(sc->sc_dmat, sc->sc_idma_size, 1,
	    sc->sc_idma_size, 0, BUS_DMA_WAITOK, &sc->sc_idma_map);
	if (error)
		goto unmap;
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_idma_map,
	    sc->sc_idma_desc, sc->sc_idma_size, NULL, BUS_DMA_WAITOK);
	if (error)
		goto destroy;
	return 0;

destroy:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_idma_map);
unmap:
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_idma_desc, sc->sc_idma_size);
free:
	bus_dmamem_free(sc->sc_dmat, sc->sc_idma_segs, sc->sc_idma_nsegs);
	return error;
}

static void
dwc_mmc_attach_i(device_t self)
{
	struct dwc_mmc_softc *sc = device_private(self);
	struct sdmmcbus_attach_args saa;

	dwc_mmc_host_reset(sc);
	dwc_mmc_bus_width(sc, 1);

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &dwc_mmc_chip_functions;
	saa.saa_sch = sc;
	saa.saa_clkmin = 400;
	saa.saa_clkmax = sc->sc_clock_freq / 1000;
	saa.saa_caps = SMC_CAPS_4BIT_MODE|
		       SMC_CAPS_8BIT_MODE|
		       SMC_CAPS_SD_HIGHSPEED|
		       SMC_CAPS_MMC_HIGHSPEED|
		       SMC_CAPS_AUTO_STOP;
	if (ISSET(sc->sc_flags, DWC_MMC_F_DMA)) {
		saa.saa_dmat = sc->sc_dmat;
		saa.saa_caps |= SMC_CAPS_DMA |
				SMC_CAPS_MULTI_SEG_DMA;
	}
	if (sc->sc_card_detect)
		saa.saa_caps |= SMC_CAPS_POLL_CARD_DET;

	sc->sc_sdmmc_dev = config_found(self, &saa, NULL);
}

static int
dwc_mmc_wait_rint(struct dwc_mmc_softc *sc, uint32_t mask, int timeout)
{
	const bool use_dma = ISSET(sc->sc_flags, DWC_MMC_F_DMA);
	int retry, error;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	if (sc->sc_intr_rint & mask)
		return 0;

	retry = timeout / hz;

	while (retry > 0) {
		if (use_dma) {
			error = cv_timedwait(&sc->sc_intr_cv,
			    &sc->sc_intr_lock, hz);
			if (error && error != EWOULDBLOCK)
				return error;
			if (sc->sc_intr_rint & mask)
				return 0;
		} else {
			sc->sc_intr_rint |= MMC_READ(sc, DWC_MMC_RINT);
			if (sc->sc_intr_rint & mask)
				return 0;
			delay(1000);
		}
		--retry;
	}

	return ETIMEDOUT;
}

static void
dwc_mmc_led(struct dwc_mmc_softc *sc, int on)
{
	if (sc->sc_set_led)
		sc->sc_set_led(sc, on);
}

static int
dwc_mmc_host_reset(sdmmc_chipset_handle_t sch)
{
	struct dwc_mmc_softc *sc = sch;
	uint32_t fifoth, ctrl;
	int retry = 1000;

#ifdef DWC_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "host reset\n");
#endif

	MMC_WRITE(sc, DWC_MMC_PWREN, 1);

	MMC_WRITE(sc, DWC_MMC_GCTRL,
	    MMC_READ(sc, DWC_MMC_GCTRL) | DWC_MMC_GCTRL_RESET);
	while (--retry > 0) {
		if (!(MMC_READ(sc, DWC_MMC_GCTRL) & DWC_MMC_GCTRL_RESET))
			break;
		delay(100);
	}

	MMC_WRITE(sc, DWC_MMC_CLKSRC, 0);

	MMC_WRITE(sc, DWC_MMC_TIMEOUT, 0xffffffff);

	MMC_WRITE(sc, DWC_MMC_IMASK,
	    DWC_MMC_INT_CMD_DONE | DWC_MMC_INT_ERROR |
	    DWC_MMC_INT_DATA_OVER | DWC_MMC_INT_AUTO_CMD_DONE);

	const uint32_t rx_wmark = (sc->sc_fifo_depth / 2) - 1;
	const uint32_t tx_wmark = sc->sc_fifo_depth / 2;
	fifoth = __SHIFTIN(DWC_MMC_FIFOTH_DMA_MULTIPLE_TXN_SIZE_16,
			   DWC_MMC_FIFOTH_DMA_MULTIPLE_TXN_SIZE);
	fifoth |= __SHIFTIN(rx_wmark, DWC_MMC_FIFOTH_RX_WMARK);
	fifoth |= __SHIFTIN(tx_wmark, DWC_MMC_FIFOTH_TX_WMARK);
	MMC_WRITE(sc, DWC_MMC_FIFOTH, fifoth);

	MMC_WRITE(sc, DWC_MMC_UHS, 0);

	ctrl = MMC_READ(sc, DWC_MMC_GCTRL);
	ctrl |= DWC_MMC_GCTRL_INTEN;
	ctrl |= DWC_MMC_GCTRL_DMAEN;
	ctrl |= DWC_MMC_GCTRL_SEND_AUTO_STOP_CCSD;
	ctrl |= DWC_MMC_GCTRL_USE_INTERNAL_DMAC;
	MMC_WRITE(sc, DWC_MMC_GCTRL, ctrl);

	return 0;
}

static uint32_t
dwc_mmc_host_ocr(sdmmc_chipset_handle_t sch)
{
	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V | MMC_OCR_HCS;
}

static int
dwc_mmc_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 32768;
}

static int
dwc_mmc_card_detect(sdmmc_chipset_handle_t sch)
{
	struct dwc_mmc_softc *sc = sch;
	int v = 0, i;

	if (!sc->sc_card_detect)
		return 1;	/* no card detect pin, assume present */

	for (i = 0; i < 5; i++) {
		v += sc->sc_card_detect(sc);
		delay(1000);
	}
	if (v == 5)
		sc->sc_mmc_present = 0;
	else if (v == 0)
		sc->sc_mmc_present = 1;
	return sc->sc_mmc_present;
}

static int
dwc_mmc_write_protect(sdmmc_chipset_handle_t sch)
{
	struct dwc_mmc_softc *sc = sch;

	if (!sc->sc_write_protect)
		return 0;	/* no write protect pin, assume rw */

	return sc->sc_write_protect(sc);
}

static int
dwc_mmc_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	return 0;
}

static int
dwc_mmc_update_clock(struct dwc_mmc_softc *sc)
{
	uint32_t cmd;
	int retry;

#ifdef DWC_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "update clock\n");
#endif

	cmd = DWC_MMC_CMD_START |
	      DWC_MMC_CMD_UPCLK_ONLY |
	      DWC_MMC_CMD_WAIT_PRE_OVER;
	if (ISSET(sc->sc_flags, DWC_MMC_F_USE_HOLD_REG))
		cmd |= DWC_MMC_CMD_USE_HOLD_REG;
	MMC_WRITE(sc, DWC_MMC_ARG, 0);
	MMC_WRITE(sc, DWC_MMC_CMD, cmd);
	retry = 0xfffff;
	while (--retry > 0) {
		if (!(MMC_READ(sc, DWC_MMC_CMD) & DWC_MMC_CMD_START))
			break;
		delay(10);
	}

	if (retry == 0) {
		aprint_error_dev(sc->sc_dev, "timeout updating clock\n");
#ifdef DWC_MMC_DEBUG
		device_printf(sc->sc_dev, "GCTRL: 0x%08x\n",
		    MMC_READ(sc, DWC_MMC_GCTRL));
		device_printf(sc->sc_dev, "CLKENA: 0x%08x\n",
		    MMC_READ(sc, DWC_MMC_CLKENA));
		device_printf(sc->sc_dev, "CLKDIV: 0x%08x\n",
		    MMC_READ(sc, DWC_MMC_CLKDIV));
		device_printf(sc->sc_dev, "TIMEOUT: 0x%08x\n",
		    MMC_READ(sc, DWC_MMC_TIMEOUT));
		device_printf(sc->sc_dev, "WIDTH: 0x%08x\n",
		    MMC_READ(sc, DWC_MMC_WIDTH));
		device_printf(sc->sc_dev, "CMD: 0x%08x\n",
		    MMC_READ(sc, DWC_MMC_CMD));
		device_printf(sc->sc_dev, "MINT: 0x%08x\n",
		    MMC_READ(sc, DWC_MMC_MINT));
		device_printf(sc->sc_dev, "RINT: 0x%08x\n",
		    MMC_READ(sc, DWC_MMC_RINT));
		device_printf(sc->sc_dev, "STATUS: 0x%08x\n",
		    MMC_READ(sc, DWC_MMC_STATUS));
#endif
		return ETIMEDOUT;
	}

	return 0;
}

static int
dwc_mmc_set_clock(struct dwc_mmc_softc *sc, u_int freq)
{
	const u_int pll_freq = sc->sc_clock_freq / 1000;
	u_int clk_div;

	if (freq != pll_freq)
		clk_div = howmany(pll_freq, freq);
	else
		clk_div = 0;

	MMC_WRITE(sc, DWC_MMC_CLKDIV, clk_div);

	return dwc_mmc_update_clock(sc);
}

static int
dwc_mmc_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct dwc_mmc_softc *sc = sch;
	uint32_t clkena;

	MMC_WRITE(sc, DWC_MMC_CLKSRC, 0);
	MMC_WRITE(sc, DWC_MMC_CLKENA, 0);
	if (dwc_mmc_update_clock(sc) != 0)
		return 1;

	if (freq) {
		if (sc->sc_bus_clock && sc->sc_bus_clock(sc, freq) != 0)
			return 1;

		if (dwc_mmc_set_clock(sc, freq) != 0)
			return 1;

		clkena = DWC_MMC_CLKENA_CARDCLKON;
		MMC_WRITE(sc, DWC_MMC_CLKENA, clkena);
		if (dwc_mmc_update_clock(sc) != 0)
			return 1;
	}

	delay(1000);

	return 0;
}

static int
dwc_mmc_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct dwc_mmc_softc *sc = sch;

#ifdef DWC_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "width = %d\n", width);
#endif

	switch (width) {
	case 1:
		MMC_WRITE(sc, DWC_MMC_WIDTH, DWC_MMC_WIDTH_1);
		break;
	case 4:
		MMC_WRITE(sc, DWC_MMC_WIDTH, DWC_MMC_WIDTH_4);
		break;
	case 8:
		MMC_WRITE(sc, DWC_MMC_WIDTH, DWC_MMC_WIDTH_8);
		break;
	default:
		return 1;
	}

	sc->sc_mmc_width = width;
	
	return 0;
}

static int
dwc_mmc_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	return -1;
}


static int
dwc_mmc_pio_wait(struct dwc_mmc_softc *sc, struct sdmmc_command *cmd)
{
	int retry = 0xfffff;
	uint32_t bit = (cmd->c_flags & SCF_CMD_READ) ?
	    DWC_MMC_STATUS_FIFO_EMPTY : DWC_MMC_STATUS_FIFO_FULL;

	while (--retry > 0) {
		uint32_t status = MMC_READ(sc, DWC_MMC_STATUS);
		if (!(status & bit))
			return 0;
		delay(10);
	}

	return ETIMEDOUT;
}

static int
dwc_mmc_pio_transfer(struct dwc_mmc_softc *sc, struct sdmmc_command *cmd)
{
	uint32_t *datap = (uint32_t *)cmd->c_data;
	int i;

	for (i = 0; i < (cmd->c_resid >> 2); i++) {
		if (dwc_mmc_pio_wait(sc, cmd))
			return ETIMEDOUT;
		if (cmd->c_flags & SCF_CMD_READ) {
			datap[i] = MMC_READ(sc, sc->sc_fifo_reg);
		} else {
			MMC_WRITE(sc, sc->sc_fifo_reg, datap[i]);
		}
	}

	return 0;
}

static int
dwc_mmc_dma_prepare(struct dwc_mmc_softc *sc, struct sdmmc_command *cmd)
{
	struct dwc_mmc_idma_desc *dma = sc->sc_idma_desc;
	bus_addr_t desc_paddr = sc->sc_idma_map->dm_segs[0].ds_addr;
	bus_size_t off;
	int desc, resid, seg;
	uint32_t val;

	desc = 0;
	for (seg = 0; seg < cmd->c_dmamap->dm_nsegs; seg++) {
		bus_addr_t paddr = cmd->c_dmamap->dm_segs[seg].ds_addr;
		bus_size_t len = cmd->c_dmamap->dm_segs[seg].ds_len;
		resid = min(len, cmd->c_resid);
		off = 0;
		while (resid > 0) {
			if (desc == sc->sc_idma_ndesc)
				break;
			len = min(sc->sc_idma_xferlen, resid);
			dma[desc].dma_buf_size = htole32(len);
			dma[desc].dma_buf_addr = htole32(paddr + off);
			dma[desc].dma_config = htole32(
			    DWC_MMC_IDMA_CONFIG_OWN);
			cmd->c_resid -= len;
			resid -= len;
			off += len;
			dma[desc].dma_next = htole32(
			    desc_paddr + ((desc+1) *
			    sizeof(struct dwc_mmc_idma_desc)));
			if (desc == 0) {
				dma[desc].dma_config |= htole32(
				    DWC_MMC_IDMA_CONFIG_FD);
			}
			if (cmd->c_resid == 0) {
				dma[desc].dma_config |= htole32(
				    DWC_MMC_IDMA_CONFIG_LD);
			} else {
				dma[desc].dma_config |=
				    htole32(DWC_MMC_IDMA_CONFIG_CH|
					    DWC_MMC_IDMA_CONFIG_DIC);
			}
			++desc;
		}
	}
	if (desc == sc->sc_idma_ndesc) {
		aprint_error_dev(sc->sc_dev,
		    "not enough descriptors for %d byte transfer!\n",
		    cmd->c_datalen);
		return EIO;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_idma_map, 0,
	    sc->sc_idma_size, BUS_DMASYNC_PREWRITE);

	sc->sc_idma_idst = 0;

	val = MMC_READ(sc, DWC_MMC_GCTRL);
	val |= DWC_MMC_GCTRL_DMAEN;
	val |= DWC_MMC_GCTRL_INTEN;
	MMC_WRITE(sc, DWC_MMC_GCTRL, val);
	val |= DWC_MMC_GCTRL_DMARESET;
	MMC_WRITE(sc, DWC_MMC_GCTRL, val);
	MMC_WRITE(sc, DWC_MMC_DMAC, DWC_MMC_DMAC_SOFTRESET);
	MMC_WRITE(sc, DWC_MMC_DMAC,
	    DWC_MMC_DMAC_IDMA_ON|DWC_MMC_DMAC_FIX_BURST);
	val = MMC_READ(sc, DWC_MMC_IDIE);
	val &= ~(DWC_MMC_IDST_RECEIVE_INT|DWC_MMC_IDST_TRANSMIT_INT);
	if (cmd->c_flags & SCF_CMD_READ)
		val |= DWC_MMC_IDST_RECEIVE_INT;
	else
		val |= DWC_MMC_IDST_TRANSMIT_INT;
	MMC_WRITE(sc, DWC_MMC_IDIE, val);
	MMC_WRITE(sc, DWC_MMC_DLBA, desc_paddr);

	return 0;
}

static void
dwc_mmc_dma_complete(struct dwc_mmc_softc *sc)
{
	bus_dmamap_sync(sc->sc_dmat, sc->sc_idma_map, 0,
	    sc->sc_idma_size, BUS_DMASYNC_POSTWRITE);
}

static void
dwc_mmc_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct dwc_mmc_softc *sc = sch;
	uint32_t cmdval = DWC_MMC_CMD_START;
	int retry = 0xfffff;

#ifdef DWC_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev,
	    "opcode %d flags 0x%x data %p datalen %d blklen %d\n",
	    cmd->c_opcode, cmd->c_flags, cmd->c_data, cmd->c_datalen,
	    cmd->c_blklen);
#endif

	mutex_enter(&sc->sc_intr_lock);

	do {
		const uint32_t status = MMC_READ(sc, DWC_MMC_STATUS);
		if ((status & DWC_MMC_STATUS_CARD_DATA_BUSY) == 0)
			break;
		delay(10);
	} while (--retry > 0);
	if (retry == 0) {
		aprint_error_dev(sc->sc_dev, "timeout waiting for data busy\n");
		cmd->c_error = ETIMEDOUT;
		goto done;
	}

	MMC_WRITE(sc, DWC_MMC_IDST, 0xffffffff);
	MMC_WRITE(sc, DWC_MMC_RINT, 0xffffffff);

	if (ISSET(sc->sc_flags, DWC_MMC_F_USE_HOLD_REG))
		cmdval |= DWC_MMC_CMD_USE_HOLD_REG;

	if (cmd->c_opcode == 0)
		cmdval |= DWC_MMC_CMD_SEND_INIT_SEQ;
	if (cmd->c_flags & SCF_RSP_PRESENT)
		cmdval |= DWC_MMC_CMD_RSP_EXP;
	if (cmd->c_flags & SCF_RSP_136)
		cmdval |= DWC_MMC_CMD_LONG_RSP;
	if (cmd->c_flags & SCF_RSP_CRC)
		cmdval |= DWC_MMC_CMD_CHECK_RSP_CRC;

	if (cmd->c_datalen > 0) {
		unsigned int nblks;

		cmdval |= DWC_MMC_CMD_DATA_EXP | DWC_MMC_CMD_WAIT_PRE_OVER;
		if (!ISSET(cmd->c_flags, SCF_CMD_READ)) {
			cmdval |= DWC_MMC_CMD_WRITE;
		}

		nblks = cmd->c_datalen / cmd->c_blklen;
		if (nblks == 0 || (cmd->c_datalen % cmd->c_blklen) != 0)
			++nblks;

		if (nblks > 1) {
			cmdval |= DWC_MMC_CMD_SEND_AUTO_STOP;
		}

		MMC_WRITE(sc, DWC_MMC_BLKSZ, cmd->c_blklen);
		MMC_WRITE(sc, DWC_MMC_BYTECNT, nblks * cmd->c_blklen);
	}

	sc->sc_intr_rint = 0;

	MMC_WRITE(sc, DWC_MMC_ARG, cmd->c_arg);

#ifdef DWC_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "cmdval = %08x\n", cmdval);
#endif

	if (cmd->c_datalen > 0) {
		cmd->c_resid = cmd->c_datalen;
		dwc_mmc_led(sc, 0);
		if (ISSET(sc->sc_flags, DWC_MMC_F_DMA)) {
			cmd->c_error = dwc_mmc_dma_prepare(sc, cmd);
			MMC_WRITE(sc, DWC_MMC_CMD, cmdval | cmd->c_opcode);
			MMC_WRITE(sc, DWC_MMC_PLDMND, 1);
			if (cmd->c_error == 0) {
				const uint32_t idst_mask =
				    DWC_MMC_IDST_ERROR | DWC_MMC_IDST_COMPLETE;
				retry = 10;
				while ((sc->sc_idma_idst & idst_mask) == 0) {
					if (retry == 0) {
						cmd->c_error = ETIMEDOUT;
						break;
					}
					cv_timedwait(&sc->sc_idst_cv,
					    &sc->sc_intr_lock, hz);
				}
			}
			dwc_mmc_dma_complete(sc);
			if (sc->sc_idma_idst & DWC_MMC_IDST_ERROR) {
				cmd->c_error = EIO;
			} else if (!(sc->sc_idma_idst & DWC_MMC_IDST_COMPLETE)) {
				cmd->c_error = ETIMEDOUT;
			}
		} else {
			mutex_exit(&sc->sc_intr_lock);
			MMC_WRITE(sc, DWC_MMC_CMD, cmdval | cmd->c_opcode);
			cmd->c_error = dwc_mmc_pio_transfer(sc, cmd);
			mutex_enter(&sc->sc_intr_lock);
		}
		dwc_mmc_led(sc, 1);
		if (cmd->c_error) {
#ifdef DWC_MMC_DEBUG
			aprint_error_dev(sc->sc_dev,
			    "xfer failed, error %d\n", cmd->c_error);
#endif
			goto done;
		}
	} else {
		MMC_WRITE(sc, DWC_MMC_CMD, cmdval | cmd->c_opcode);
	}

	cmd->c_error = dwc_mmc_wait_rint(sc,
	    DWC_MMC_INT_ERROR|DWC_MMC_INT_CMD_DONE, hz * 10);
	if (cmd->c_error == 0 && (sc->sc_intr_rint & DWC_MMC_INT_ERROR)) {
		if (sc->sc_intr_rint & DWC_MMC_INT_RESP_TIMEOUT) {
			cmd->c_error = ETIMEDOUT;
		} else {
			cmd->c_error = EIO;
		}
	}
	if (cmd->c_error) {
#ifdef DWC_MMC_DEBUG
		aprint_error_dev(sc->sc_dev,
		    "cmd failed, error %d\n", cmd->c_error);
#endif
		goto done;
	}

	if (cmd->c_datalen > 0) {
		cmd->c_error = dwc_mmc_wait_rint(sc,
		    DWC_MMC_INT_ERROR|
		    DWC_MMC_INT_AUTO_CMD_DONE|
		    DWC_MMC_INT_DATA_OVER,
		    hz*10);
		if (cmd->c_error == 0 &&
		    (sc->sc_intr_rint & DWC_MMC_INT_ERROR)) {
			cmd->c_error = ETIMEDOUT;
		}
		if (cmd->c_error) {
#ifdef DWC_MMC_DEBUG
			aprint_error_dev(sc->sc_dev,
			    "data timeout, rint = %08x\n",
			    sc->sc_intr_rint);
#endif
			dwc_mmc_dump_regs(sc);
			cmd->c_error = ETIMEDOUT;
			goto done;
		}
	}

	if (cmd->c_flags & SCF_RSP_PRESENT) {
		if (cmd->c_flags & SCF_RSP_136) {
			cmd->c_resp[0] = MMC_READ(sc, DWC_MMC_RESP0);
			cmd->c_resp[1] = MMC_READ(sc, DWC_MMC_RESP1);
			cmd->c_resp[2] = MMC_READ(sc, DWC_MMC_RESP2);
			cmd->c_resp[3] = MMC_READ(sc, DWC_MMC_RESP3);
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
			cmd->c_resp[0] = MMC_READ(sc, DWC_MMC_RESP0);
		}
	}

done:
	cmd->c_flags |= SCF_ITSDONE;
	mutex_exit(&sc->sc_intr_lock);

	if (cmd->c_error) {
#ifdef DWC_MMC_DEBUG
		aprint_error_dev(sc->sc_dev, "i/o error %d\n", cmd->c_error);
#endif
		MMC_WRITE(sc, DWC_MMC_GCTRL,
		    MMC_READ(sc, DWC_MMC_GCTRL) |
		      DWC_MMC_GCTRL_DMARESET | DWC_MMC_GCTRL_FIFORESET);
		for (retry = 0; retry < 1000; retry++) {
			if (!(MMC_READ(sc, DWC_MMC_GCTRL) & DWC_MMC_GCTRL_RESET))
				break;
			delay(10);
		}
		dwc_mmc_update_clock(sc);
	}

	if (!ISSET(sc->sc_flags, DWC_MMC_F_DMA)) {
		MMC_WRITE(sc, DWC_MMC_GCTRL,
		    MMC_READ(sc, DWC_MMC_GCTRL) | DWC_MMC_GCTRL_FIFORESET);
	}
}

static void
dwc_mmc_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
}

static void
dwc_mmc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
}

int
dwc_mmc_init(struct dwc_mmc_softc *sc)
{
	uint32_t val;

	if (sc->sc_fifo_reg == 0) {
		val = MMC_READ(sc, DWC_MMC_VERID);
		const u_int id = __SHIFTOUT(val, DWC_MMC_VERID_ID);

		if (id < DWC_MMC_VERID_240A)
			sc->sc_fifo_reg = 0x100;
		else
			sc->sc_fifo_reg = 0x200;
	}

	if (sc->sc_fifo_depth == 0) {
		val = MMC_READ(sc, DWC_MMC_FIFOTH);
		sc->sc_fifo_depth = __SHIFTOUT(val, DWC_MMC_FIFOTH_RX_WMARK) + 1;
	}

	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_intr_cv, "dwcmmcirq");
	cv_init(&sc->sc_idst_cv, "dwcmmcdma");

	const bool use_dma = ISSET(sc->sc_flags, DWC_MMC_F_DMA);

	aprint_debug_dev(sc->sc_dev, "using %s for transfers\n",
	    use_dma ? "DMA" : "PIO");

	if (use_dma && dwc_mmc_idma_setup(sc) != 0) {
		aprint_error_dev(sc->sc_dev, "failed to setup DMA\n");
		return ENOMEM;
	}

	config_interrupts(sc->sc_dev, dwc_mmc_attach_i);

	return 0;
}

int
dwc_mmc_intr(void *priv)
{
	struct dwc_mmc_softc *sc = priv;
	uint32_t idst, rint, mint;

	mutex_enter(&sc->sc_intr_lock);
	idst = MMC_READ(sc, DWC_MMC_IDST);
	rint = MMC_READ(sc, DWC_MMC_RINT);
	mint = MMC_READ(sc, DWC_MMC_MINT);
	if (!idst && !rint && !mint) {
		mutex_exit(&sc->sc_intr_lock);
		return 0;
	}
	MMC_WRITE(sc, DWC_MMC_IDST, idst);
	MMC_WRITE(sc, DWC_MMC_RINT, rint);
	MMC_WRITE(sc, DWC_MMC_MINT, mint);

#ifdef DWC_MMC_DEBUG
	device_printf(sc->sc_dev, "mmc intr idst=%08X rint=%08X mint=%08X\n",
	    idst, rint, mint);
#endif

	if (idst) {
		sc->sc_idma_idst |= idst;
		cv_broadcast(&sc->sc_idst_cv);
	}

	if (rint) {
		sc->sc_intr_rint |= rint;
		cv_broadcast(&sc->sc_intr_cv);
	}

	mutex_exit(&sc->sc_intr_lock);

	return 1;
}
