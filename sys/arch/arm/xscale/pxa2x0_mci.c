/*	$NetBSD: pxa2x0_mci.c,v 1.3 2009/12/05 13:56:43 nonaka Exp $	*/
/*	$OpenBSD: pxa2x0_mmc.c,v 1.5 2009/02/23 18:09:55 miod Exp $	*/

/*
 * Copyright (c) 2007 Uwe Stuehler <uwe@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*-
 * Copyright (c) 2007-2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * MMC/SD/SDIO controller driver for Intel PXA2xx processors
 *
 * Power management is beyond control of the processor's SD/SDIO/MMC
 * block, so this driver depends on the attachment driver to provide
 * us with some callback functions via the "tag" member in our softc.
 * Bus power management calls are then dispatched to the attachment
 * driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pxa2x0_mci.c,v 1.3 2009/12/05 13:56:43 nonaka Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <machine/intr.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_dmac.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/xscale/pxa2x0_mci.h>

#ifdef PXAMCI_DEBUG
int pxamci_debug = 1;
#define DPRINTF(n,s)	do { if ((n) <= pxamci_debug) printf s; } while (0)
#else
#define DPRINTF(n,s)	do {} while (0)
#endif

#ifndef DEBUG
#define	STOPCLK_TIMO	2	/* ms */
#define	EXECCMD_TIMO	2	/* ms */
#else
#define	STOPCLK_TIMO	2	/* ms */
#define	EXECCMD_TIMO	5	/* ms */
#endif

static int	pxamci_host_reset(sdmmc_chipset_handle_t);
static uint32_t	pxamci_host_ocr(sdmmc_chipset_handle_t);
static int	pxamci_host_maxblklen(sdmmc_chipset_handle_t);
static int	pxamci_card_detect(sdmmc_chipset_handle_t);
static int	pxamci_write_protect(sdmmc_chipset_handle_t);
static int	pxamci_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	pxamci_bus_clock(sdmmc_chipset_handle_t, int);
static int	pxamci_bus_width(sdmmc_chipset_handle_t, int);
static void	pxamci_exec_command(sdmmc_chipset_handle_t,
		    struct sdmmc_command *);
static void	pxamci_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	pxamci_card_intr_ack(sdmmc_chipset_handle_t);

static struct sdmmc_chip_functions pxamci_chip_functions = {
	/* host controller reset */
	.host_reset		= pxamci_host_reset,

	/* host controller capabilities */
	.host_ocr		= pxamci_host_ocr,
	.host_maxblklen		= pxamci_host_maxblklen,

	/* card detection */
	.card_detect		= pxamci_card_detect,

	/* write protect */
	.write_protect		= pxamci_write_protect,

	/* bus power, clock frequency, width */
	.bus_power		= pxamci_bus_power,
	.bus_clock		= pxamci_bus_clock,
	.bus_width		= pxamci_bus_width,

	/* command execution */
	.exec_command		= pxamci_exec_command,

	/* card interrupt */
	.card_enable_intr	= pxamci_card_enable_intr,
	.card_intr_ack		= pxamci_card_intr_ack,
};

static int	pxamci_intr(void *);
static void	pxamci_intr_cmd(struct pxamci_softc *);
static void	pxamci_intr_data(struct pxamci_softc *);
static void	pxamci_intr_done(struct pxamci_softc *);
static void	pxamci_dmac_iintr(struct dmac_xfer *, int);
static void	pxamci_dmac_ointr(struct dmac_xfer *, int);

static void	pxamci_stop_clock(struct pxamci_softc *);

#define CSR_READ_1(sc, reg) \
	bus_space_read_1((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define CSR_WRITE_1(sc, reg, val) \
	bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define CSR_READ_4(sc, reg) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define CSR_WRITE_4(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define CSR_SET_4(sc, reg, val) \
	CSR_WRITE_4(sc, reg, CSR_READ_4(sc, reg) | (val))
#define CSR_CLR_4(sc, reg, val) \
	CSR_WRITE_4(sc, reg, CSR_READ_4(sc, reg) & ~(val))

static void
pxamci_enable_intr(struct pxamci_softc *sc, uint32_t mask)
{
	int s;

	s = splsdmmc();
	sc->sc_imask &= ~mask;
	CSR_WRITE_4(sc, MMC_I_MASK, sc->sc_imask);
	splx(s);
}

static void
pxamci_disable_intr(struct pxamci_softc *sc, uint32_t mask)
{
	int s;

	s = splsdmmc();
	sc->sc_imask |= mask;
	CSR_WRITE_4(sc, MMC_I_MASK, sc->sc_imask);
	splx(s);
}

int
pxamci_attach_sub(device_t self, struct pxaip_attach_args *pxa)
{
	struct pxamci_softc *sc = device_private(self);
	struct sdmmcbus_attach_args saa;

	sc->sc_dev = self;

	aprint_normal(": MMC/SD Controller\n");
	aprint_naive("\n");

	/* Enable the clocks to the MMC controller. */
	pxa2x0_clkman_config(CKEN_MMC, 1);

	sc->sc_iot = pxa->pxa_iot;
	if (bus_space_map(sc->sc_iot, PXA2X0_MMC_BASE, PXA2X0_MMC_SIZE, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "couldn't map registers\n");
		goto out;
	}

	/*
	 * Establish the card detection and MMC interrupt handlers and
	 * mask all interrupts until we are prepared to handle them.
	 */
	pxamci_disable_intr(sc, MMC_I_ALL);
	sc->sc_ih = pxa2x0_intr_establish(PXA2X0_INT_MMC, IPL_SDMMC,
	    pxamci_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish MMC interrupt\n");
		goto free_map;
	}

	/*
	 * Reset the host controller and unmask normal interrupts.
	 */
	(void) pxamci_host_reset(sc);

	/* Setup bus clock */
	if (CPU_IS_PXA270) {
		sc->sc_clkmin = PXA270_MMC_CLKRT_MIN / 1000;
		sc->sc_clkmax = PXA270_MMC_CLKRT_MAX / 1000;
	} else {
		sc->sc_clkmin = PXA250_MMC_CLKRT_MIN / 1000;
		sc->sc_clkmax = PXA250_MMC_CLKRT_MAX / 1000;
	}
	sc->sc_clkbase = sc->sc_clkmin;
	pxamci_bus_clock(sc, sc->sc_clkbase);

	/* Setup max block length */
	if (CPU_IS_PXA270) {
		sc->sc_maxblklen = 2048;
	} else {
		sc->sc_maxblklen = 512;
	}

	/* Set default bus width */
	sc->sc_buswidth = 1;

	/* setting DMA */
#if 1	/* XXX */
	SET(sc->sc_caps, PMC_CAPS_NO_DMA);	/* disable DMA */
#endif
	if (!ISSET(sc->sc_caps, PMC_CAPS_NO_DMA)) {
		aprint_normal_dev(sc->sc_dev, "using DMA transfer\n");

		sc->sc_rxdr.ds_addr = PXA2X0_MMC_BASE + MMC_RXFIFO;
		sc->sc_rxdr.ds_len = 1;
		sc->sc_rxdx = pxa2x0_dmac_allocate_xfer(M_NOWAIT);
		if (sc->sc_rxdx == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't alloc rx dma xfer\n");
			goto free_intr;
		}
		sc->sc_rxdx->dx_cookie = sc;
		sc->sc_rxdx->dx_priority = DMAC_PRIORITY_NORMAL;
		sc->sc_rxdx->dx_dev_width = DMAC_DEV_WIDTH_1;
		sc->sc_rxdx->dx_burst_size = DMAC_BURST_SIZE_32;
		sc->sc_rxdx->dx_done = pxamci_dmac_iintr;
		sc->sc_rxdx->dx_peripheral = DMAC_PERIPH_MMCRX;
		sc->sc_rxdx->dx_flow = DMAC_FLOW_CTRL_SRC;
		sc->sc_rxdx->dx_loop_notify = DMAC_DONT_LOOP;
		sc->sc_rxdx->dx_desc[DMAC_DESC_SRC].xd_addr_hold = true;
		sc->sc_rxdx->dx_desc[DMAC_DESC_SRC].xd_nsegs = 1;
		sc->sc_rxdx->dx_desc[DMAC_DESC_SRC].xd_dma_segs = &sc->sc_rxdr;
		sc->sc_rxdx->dx_desc[DMAC_DESC_DST].xd_addr_hold = false;

		sc->sc_txdr.ds_addr = PXA2X0_MMC_BASE + MMC_TXFIFO;
		sc->sc_txdr.ds_len = 1;
		sc->sc_txdx = pxa2x0_dmac_allocate_xfer(M_NOWAIT);
		if (sc->sc_txdx == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't alloc tx dma xfer\n");
			goto free_xfer;
		}
		sc->sc_txdx->dx_cookie = sc;
		sc->sc_txdx->dx_priority = DMAC_PRIORITY_NORMAL;
		sc->sc_txdx->dx_dev_width = DMAC_DEV_WIDTH_1;
		sc->sc_txdx->dx_burst_size = DMAC_BURST_SIZE_32;
		sc->sc_txdx->dx_done = pxamci_dmac_ointr;
		sc->sc_txdx->dx_peripheral = DMAC_PERIPH_MMCTX;
		sc->sc_txdx->dx_flow = DMAC_FLOW_CTRL_DEST;
		sc->sc_txdx->dx_loop_notify = DMAC_DONT_LOOP;
		sc->sc_txdx->dx_desc[DMAC_DESC_DST].xd_addr_hold = true;
		sc->sc_txdx->dx_desc[DMAC_DESC_DST].xd_nsegs = 1;
		sc->sc_txdx->dx_desc[DMAC_DESC_DST].xd_dma_segs = &sc->sc_txdr;
		sc->sc_txdx->dx_desc[DMAC_DESC_SRC].xd_addr_hold = false;
	}

	/*
	 * Attach the generic SD/MMC bus driver.  (The bus driver must
	 * not invoke any chipset functions before it is attached.)
	 */
	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &pxamci_chip_functions;
	saa.saa_sch = sc;
	saa.saa_dmat = pxa->pxa_dmat;
	saa.saa_clkmin = sc->sc_clkmin;
	saa.saa_clkmax = sc->sc_clkmax;
	saa.saa_caps = 0;
	if (!ISSET(sc->sc_caps, PMC_CAPS_NO_DMA))
		SET(saa.saa_caps, SMC_CAPS_DMA);
#if notyet
	if (CPU_IS_PXA270 && ISSET(sc->sc_caps, PMC_CAPS_4BIT))
		SET(saa.saa_caps, SMC_CAPS_4BIT_MODE);
#endif

	sc->sc_sdmmc = config_found(sc->sc_dev, &saa, NULL);
	if (sc->sc_sdmmc == NULL) {
		aprint_error_dev(sc->sc_dev, "couldn't attach bus\n");
		goto free_xfer;
	}
	return 0;

free_xfer:
	if (!ISSET(sc->sc_caps, PMC_CAPS_NO_DMA)) {
		if (sc->sc_rxdx)
			pxa2x0_dmac_free_xfer(sc->sc_rxdx);
		if (sc->sc_txdx)
			pxa2x0_dmac_free_xfer(sc->sc_txdx);
	}
free_intr:
	pxa2x0_intr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;
free_map:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, PXA2X0_MMC_SIZE);
out:
	pxa2x0_clkman_config(CKEN_MMC, 0);
	return 1;
}

/*
 * Notify card attach/detach event.
 */
void
pxamci_card_detect_event(struct pxamci_softc *sc)
{

	sdmmc_needs_discover(sc->sc_sdmmc);
}

/*
 * Reset the host controller.  Called during initialization, when
 * cards are removed, upon resume, and during error recovery.
 */
static int
pxamci_host_reset(sdmmc_chipset_handle_t sch)
{
	struct pxamci_softc *sc = (struct pxamci_softc *)sch;
	int s;

	s = splsdmmc();

	CSR_WRITE_4(sc, MMC_SPI, 0);
	CSR_WRITE_4(sc, MMC_RESTO, 0x7f);
	CSR_WRITE_4(sc, MMC_I_MASK, sc->sc_imask);

	/* Make sure to initialize the card before the next command. */
	CLR(sc->sc_flags, PMF_CARDINITED);

	splx(s);

	return 0;
}

static uint32_t
pxamci_host_ocr(sdmmc_chipset_handle_t sch)
{
	struct pxamci_softc *sc = (struct pxamci_softc *)sch;
	int rv;

	if (__predict_true(sc->sc_tag.get_ocr != NULL)) {
		rv = (*sc->sc_tag.get_ocr)(sc->sc_tag.cookie);
		return rv;
	}

	DPRINTF(0,("%s: driver lacks get_ocr() function.\n",
	    device_xname(sc->sc_dev)));
	return ENXIO;
}

static int
pxamci_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	struct pxamci_softc *sc = (struct pxamci_softc *)sch;

	return sc->sc_maxblklen;
}

static int
pxamci_card_detect(sdmmc_chipset_handle_t sch)
{
	struct pxamci_softc *sc = (struct pxamci_softc *)sch;

	if (__predict_true(sc->sc_tag.card_detect != NULL)) {
		return (*sc->sc_tag.card_detect)(sc->sc_tag.cookie);
	}

	DPRINTF(0,("%s: driver lacks card_detect() function.\n",
	    device_xname(sc->sc_dev)));
	return 1;	/* always detect */
}

static int
pxamci_write_protect(sdmmc_chipset_handle_t sch)
{
	struct pxamci_softc *sc = (struct pxamci_softc *)sch;

	if (__predict_true(sc->sc_tag.write_protect != NULL)) {
		return (*sc->sc_tag.write_protect)(sc->sc_tag.cookie);
	}

	DPRINTF(0,("%s: driver lacks write_protect() function.\n",
	    device_xname(sc->sc_dev)));
	return 0;	/* non-protect */
}

/*
 * Set or change SD bus voltage and enable or disable SD bus power.
 * Return zero on success.
 */
static int
pxamci_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	struct pxamci_softc *sc = (struct pxamci_softc *)sch;

	/*
	 * Bus power management is beyond control of the SD/SDIO/MMC
	 * block of the PXA2xx processors, so we have to hand this
	 * task off to the attachment driver.
	 */
	if (__predict_true(sc->sc_tag.set_power != NULL)) {
		return (*sc->sc_tag.set_power)(sc->sc_tag.cookie, ocr);
	}

	DPRINTF(0,("%s: driver lacks set_power() function\n",
	    device_xname(sc->sc_dev)));
	return ENXIO;
}

/*
 * Set or change MMCLK frequency or disable the MMC clock.
 * Return zero on success.
 */
static int
pxamci_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct pxamci_softc *sc = (struct pxamci_softc *)sch;
	int actfreq;
	int div;
	int rv = 0;
	int s;

	s = splsdmmc();

	/*
	 * Stop MMC clock before changing the frequency.
	 */
	pxamci_stop_clock(sc);

	/* Just stop the clock. */
	if (freq == 0)
		goto out;

	/*
	 * PXA27x Errata...
	 *
	 * <snip>
	 * E40. SDIO: SDIO Devices Not Working at 19.5 Mbps
	 *
	 * SD/SDIO controller can only support up to 9.75 Mbps data
	 * transfer rate for SDIO card.
	 * </snip>
	 *
	 * If we don't limit the frequency, CRC errors will be
	 * reported by the controller after we set the bus speed.
	 * XXX slow down incrementally.
	 */
	if (CPU_IS_PXA270) {
		if (freq > 9750) {
			freq = 9750;
		}
	}

	/*
	 * Pick the smallest divider that produces a frequency not
	 * more than `freq' KHz.
	 */
	actfreq = sc->sc_clkmax;
	for (div = 0; div < 7; actfreq /= 2, div++) {
		if (actfreq <= freq)
			break;
	}
	if (div == 7) {
		aprint_error_dev(sc->sc_dev,
		    "unsupported bus frequency of %d KHz\n", freq);
		rv = 1;
		goto out;
	}

	DPRINTF(1,("%s: freq = %d, actfreq = %d, div = %d\n",
	    device_xname(sc->sc_dev), freq, actfreq, div));

	sc->sc_clkbase = actfreq;
	sc->sc_clkrt = div;

	CSR_WRITE_4(sc, MMC_CLKRT, sc->sc_clkrt);
	CSR_WRITE_4(sc, MMC_STRPCL, STRPCL_START);

 out:
	splx(s);

	return rv;
}

static int
pxamci_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct pxamci_softc *sc = (struct pxamci_softc *)sch;
	int rv = 0;
	int s;

	s = splsdmmc();

	switch (width) {
	case 1:
		break;
	case 4:
		if (CPU_IS_PXA270)
			break;
		/*FALLTHROUGH*/
	default:
		DPRINTF(0,("%s: unsupported bus width (%d)\n",
		    device_xname(sc->sc_dev), width));
		rv = 1;
		goto out;
	}

	sc->sc_buswidth = width;

 out:
	splx(s);

	return rv;
}

static void
pxamci_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct pxamci_softc *sc = (struct pxamci_softc *)sch;
	uint32_t cmdat;
	int error;
	int timo;
	int s;

	DPRINTF(1,("%s: start cmd %d arg=%#x data=%p dlen=%d flags=%#x\n"
	    "proc=%p \"%s\"\n", device_xname(sc->sc_dev), cmd->c_opcode,
	    cmd->c_arg, cmd->c_data, cmd->c_datalen, cmd->c_flags));

	s = splsdmmc();

	/* Stop the bus clock (MMCLK). [15.8.3] */
	pxamci_stop_clock(sc);

	/* Set the command and argument. */
	CSR_WRITE_4(sc, MMC_CMD, cmd->c_opcode & CMD_MASK);
	CSR_WRITE_4(sc, MMC_ARGH, (cmd->c_arg >> 16) & ARGH_MASK);
	CSR_WRITE_4(sc, MMC_ARGL, cmd->c_arg & ARGL_MASK);

	/* Response type */
	if (!ISSET(cmd->c_flags, SCF_RSP_PRESENT))
		cmdat = CMDAT_RESPONSE_FORMAT_NO;
	else if (ISSET(cmd->c_flags, SCF_RSP_136))
		cmdat = CMDAT_RESPONSE_FORMAT_R2;
	else if (!ISSET(cmd->c_flags, SCF_RSP_CRC))
		cmdat = CMDAT_RESPONSE_FORMAT_R3;
	else
		cmdat = CMDAT_RESPONSE_FORMAT_R1;

	if (ISSET(cmd->c_flags, SCF_RSP_BSY))
		cmdat |= CMDAT_BUSY;
	if (!ISSET(cmd->c_flags, SCF_CMD_READ))
		cmdat |= CMDAT_WRITE;
	if (sc->sc_buswidth == 4)
		cmdat |= CMDAT_SD_4DAT;

	/* Fragment the data into proper blocks. */
	if (cmd->c_datalen > 0) {
		int blklen = MIN(cmd->c_datalen, cmd->c_blklen);
		int numblk = cmd->c_datalen / blklen;

		if (cmd->c_datalen % blklen > 0) {
			/* XXX: Split this command. (1.7.4) */
			aprint_error_dev(sc->sc_dev,
			    "data not a multiple of %u bytes\n", blklen);
			cmd->c_error = EINVAL;
			goto out;
		}

		/* Check limit imposed by block count. */
		if (numblk > NOB_MASK) {
			aprint_error_dev(sc->sc_dev, "too much data\n");
			cmd->c_error = EINVAL;
			goto out;
		}

		CSR_WRITE_4(sc, MMC_BLKLEN, blklen);
		CSR_WRITE_4(sc, MMC_NOB, numblk);
		CSR_WRITE_4(sc, MMC_RDTO, RDTO_MASK);

		cmdat |= CMDAT_DATA_EN;

		/* setting DMA */
		if (!ISSET(sc->sc_caps, PMC_CAPS_NO_DMA)) {
			struct dmac_xfer_desc *dx_desc;

			cmdat |= CMDAT_MMC_DMA_EN;

			if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
				dx_desc = &sc->sc_rxdx->dx_desc[DMAC_DESC_DST];
				dx_desc->xd_nsegs = cmd->c_dmamap->dm_nsegs;
				dx_desc->xd_dma_segs = cmd->c_dmamap->dm_segs;
				error = pxa2x0_dmac_start_xfer(sc->sc_rxdx);
			} else {
				dx_desc = &sc->sc_txdx->dx_desc[DMAC_DESC_SRC];
				dx_desc->xd_nsegs = cmd->c_dmamap->dm_nsegs;
				dx_desc->xd_dma_segs = cmd->c_dmamap->dm_segs;
				/* workaround for erratum #91 */
				error = 0;
				if (!CPU_IS_PXA270) {
					error =
					    pxa2x0_dmac_start_xfer(sc->sc_txdx);
				}
			}
			if (error) {
				aprint_error_dev(sc->sc_dev,
				    "couldn't start dma xfer. (error=%d)\n",
				    error);
				cmd->c_error = EIO;
				goto err;
			}
		} else {
			cmd->c_resid = cmd->c_datalen;
			cmd->c_buf = cmd->c_data;

			pxamci_enable_intr(sc, MMC_I_RXFIFO_RD_REQ
					       | MMC_I_TXFIFO_WR_REQ
					       | MMC_I_DAT_ERR);
		}
	}

	sc->sc_cmd = cmd;

	/*
	 * "After reset, the MMC card must be initialized by sending
	 * 80 clocks to it on the MMCLK signal." [15.4.3.2]
	 */
	if (!ISSET(sc->sc_flags, PMF_CARDINITED)) {
		DPRINTF(1,("%s: first command\n", device_xname(sc->sc_dev)));
		cmdat |= CMDAT_INIT;
		SET(sc->sc_flags, PMF_CARDINITED);
	}

	/* Begin the transfer and start the bus clock. */
	CSR_WRITE_4(sc, MMC_CMDAT, cmdat);
	CSR_WRITE_4(sc, MMC_CLKRT, sc->sc_clkrt);
	CSR_WRITE_4(sc, MMC_STRPCL, STRPCL_START);

	/* Wait for it to complete */
	pxamci_enable_intr(sc, MMC_I_END_CMD_RES|MMC_I_RES_ERR);
	for (timo = EXECCMD_TIMO; (sc->sc_cmd == cmd) && (timo > 0); timo--) {
		tsleep(sc, PWAIT, "mmcmd", hz);
	}

	/* If it completed in time, SCF_ITSDONE is already set. */
	if (sc->sc_cmd == cmd) {
		cmd->c_error = ETIMEDOUT;
err:
		SET(cmd->c_flags, SCF_ITSDONE);
		sc->sc_cmd = NULL;
		goto out;
	}

out:
	splx(s);

	DPRINTF(1,("%s: cmd %d done (flags=%08x error=%d)\n",
	  device_xname(sc->sc_dev), cmd->c_opcode, cmd->c_flags, cmd->c_error));
}

static void
pxamci_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
	struct pxamci_softc *sc = (struct pxamci_softc *)sch;

	if (enable) {
		pxamci_enable_intr(sc, MMC_I_SDIO_INT);
	} else {
		pxamci_disable_intr(sc, MMC_I_SDIO_INT);
	}
}

static void
pxamci_card_intr_ack(sdmmc_chipset_handle_t sch)
{

	/* Nothing to do */
}

static void
pxamci_stop_clock(struct pxamci_softc *sc)
{
	int timo = STOPCLK_TIMO;

	if (ISSET(CSR_READ_4(sc, MMC_STAT), STAT_CLK_EN)) {
		CSR_CLR_4(sc, MMC_I_MASK, MMC_I_CLK_IS_OFF);
		CSR_WRITE_4(sc, MMC_STRPCL, STRPCL_STOP);
		while (ISSET(CSR_READ_4(sc, MMC_STAT), STAT_CLK_EN)
		    && (timo-- > 0)) {
			tsleep(sc, PWAIT, "mmclk", hz);
		}
	}
	if (timo == 0)
		aprint_error_dev(sc->sc_dev, "clock stop timeout\n");
}

/*
 * SD/MMC controller interrput handler
 */
static int
pxamci_intr(void *arg)
{
	struct pxamci_softc *sc = arg;
	int status;
#ifdef PXAMCI_DEBUG
	int ostatus;

	ostatus =
#endif
	status = CSR_READ_4(sc, MMC_I_REG) & ~CSR_READ_4(sc, MMC_I_MASK);
	DPRINTF(9,("%s: intr status = %08x\n", device_xname(sc->sc_dev),
	    status));

	/*
	 * Notify the process waiting in pxamci_clock_stop() when
	 * the clock has really stopped.
	 */
	if (ISSET(status, MMC_I_CLK_IS_OFF)) {
		DPRINTF(2,("%s: clock is now off\n", device_xname(sc->sc_dev)));
		wakeup(sc);
		pxamci_disable_intr(sc, MMC_I_CLK_IS_OFF);
		CLR(status, MMC_I_CLK_IS_OFF);
	}

	if (sc->sc_cmd == NULL)
		goto end;

	if (ISSET(status, MMC_I_RES_ERR)) {
		DPRINTF(9, ("%s: handling MMC_I_RES_ERR\n",
		    device_xname(sc->sc_dev)));
		pxamci_disable_intr(sc, MMC_I_RES_ERR);
		CLR(status, MMC_I_RES_ERR|MMC_I_END_CMD_RES);
		if (!ISSET(sc->sc_caps, PMC_CAPS_NO_DMA)
		 && (sc->sc_cmd->c_datalen > 0)) {
			if (ISSET(sc->sc_cmd->c_flags, SCF_CMD_READ)) {
				pxa2x0_dmac_abort_xfer(sc->sc_rxdx);
			} else {
				pxa2x0_dmac_abort_xfer(sc->sc_txdx);
			}
		}
		sc->sc_cmd->c_error = ENOEXEC;
		pxamci_intr_done(sc);
		goto end;
	}

	if (ISSET(status, MMC_I_END_CMD_RES)) {
		DPRINTF(9,("%s: handling MMC_I_END_CMD_RES\n",
		    device_xname(sc->sc_dev)));
		pxamci_intr_cmd(sc);
		pxamci_disable_intr(sc, MMC_I_END_CMD_RES);
		CLR(status, MMC_I_END_CMD_RES);
		/* ignore programming done condition */
		if (ISSET(status, MMC_I_PRG_DONE)) {
			pxamci_disable_intr(sc, MMC_I_PRG_DONE);
			CLR(status, MMC_I_PRG_DONE);
		}
		if (sc->sc_cmd == NULL)
			goto end;
	}

	if (ISSET(status, MMC_I_DAT_ERR)) {
		DPRINTF(9, ("%s: handling MMC_I_DAT_ERR\n",
		    device_xname(sc->sc_dev)));
		sc->sc_cmd->c_error = EIO;
		pxamci_intr_done(sc);
		pxamci_disable_intr(sc, MMC_I_DAT_ERR);
		CLR(status, MMC_I_DAT_ERR);
		if (!ISSET(sc->sc_caps, PMC_CAPS_NO_DMA)) {
			if (ISSET(sc->sc_cmd->c_flags, SCF_CMD_READ)) {
				pxa2x0_dmac_abort_xfer(sc->sc_rxdx);
			} else {
				pxa2x0_dmac_abort_xfer(sc->sc_txdx);
			}
		}
		/* ignore transmission done condition */
		if (ISSET(status, MMC_I_DATA_TRAN_DONE)) {
			pxamci_disable_intr(sc, MMC_I_DATA_TRAN_DONE);
			CLR(status, MMC_I_DATA_TRAN_DONE);
		}
		goto end;
	}

	if (ISSET(status, MMC_I_DATA_TRAN_DONE)) {
		DPRINTF(9,("%s: handling MMC_I_DATA_TRAN_DONE\n",
		    device_xname(sc->sc_dev)));
		pxamci_intr_done(sc);
		pxamci_disable_intr(sc, MMC_I_DATA_TRAN_DONE);
		CLR(status, MMC_I_DATA_TRAN_DONE);
	}

	if (ISSET(status, MMC_I_TXFIFO_WR_REQ|MMC_I_RXFIFO_RD_REQ)) {
		DPRINTF(9,("%s: handling MMC_I_xxFIFO_xx_REQ\n",
		    device_xname(sc->sc_dev)));
		pxamci_intr_data(sc);
		CLR(status, MMC_I_TXFIFO_WR_REQ|MMC_I_RXFIFO_RD_REQ);
	}

	if (ISSET(status, STAT_SDIO_INT)) {
		DPRINTF(9,("%s: handling STAT_SDIO_INT\n",
		    device_xname(sc->sc_dev)));
		sdmmc_card_intr(sc->sc_sdmmc);
		CLR(status, STAT_SDIO_INT);
	}

end:
	/* Avoid further unhandled interrupts. */
	if (status != 0) {
		pxamci_disable_intr(sc, status);
#ifdef PXAMCI_DEBUG
		aprint_error_dev(sc->sc_dev,
		    "unhandled interrupt 0x%x out of 0x%x\n", status, ostatus);
#endif
	}
	return 1;
}

static void
pxamci_intr_cmd(struct pxamci_softc *sc)
{
	struct sdmmc_command *cmd = sc->sc_cmd;
	uint32_t status;
	int error;
	int i;

	KASSERT(sc->sc_cmd != NULL);

#define STAT_ERR	(STAT_READ_TIME_OUT \
			 | STAT_TIMEOUT_RESPONSE \
			 | STAT_CRC_WRITE_ERROR \
			 | STAT_CRC_READ_ERROR \
			 | STAT_SPI_READ_ERROR_TOKEN)

	if (ISSET(cmd->c_flags, SCF_RSP_136)) {
		for (i = 3; i >= 0; i--) {
			uint32_t h = CSR_READ_4(sc, MMC_RES) & 0xffff;
			uint32_t l = CSR_READ_4(sc, MMC_RES) & 0xffff;
			cmd->c_resp[i] = (h << 16) | l;
		}
		cmd->c_error = 0;
	} else if (ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		/*
		 * Grrr... The processor manual is not clear about
		 * the layout of the response FIFO.  It just states
		 * that the FIFO is 16 bits wide, has a depth of 8,
		 * and that the CRC is not copied into the FIFO.
		 *
		 * A 16-bit word in the FIFO is filled from highest
		 * to lowest bit as the response comes in.  The two
		 * start bits and the 6 command index bits are thus
		 * stored in the upper 8 bits of the first 16-bit
		 * word that we read back from the FIFO.
		 *
		 * Since the sdmmc(4) framework expects the host
		 * controller to discard the first 8 bits of the
		 * response, what we must do is discard the upper
		 * byte of the first 16-bit word.
		 */
		uint32_t h = CSR_READ_4(sc, MMC_RES) & 0xffff;
		uint32_t m = CSR_READ_4(sc, MMC_RES) & 0xffff;
		uint32_t l = CSR_READ_4(sc, MMC_RES) & 0xffff;
		cmd->c_resp[0] = (h << 24) | (m << 8) | (l >> 8);
		for (i = 1; i < 4; i++)
			cmd->c_resp[i] = 0;
		cmd->c_error = 0;
	}

	status = CSR_READ_4(sc, MMC_STAT);

	if (!ISSET(cmd->c_flags, SCF_RSP_PRESENT))
		CLR(status, STAT_TIMEOUT_RESPONSE);

	/* XXX only for R6, not for R2 */
	if (!ISSET(cmd->c_flags, SCF_RSP_IDX))
		CLR(status, STAT_RES_CRC_ERR);

	if (ISSET(status, STAT_TIMEOUT_RESPONSE))
		cmd->c_error = ETIMEDOUT;
	else if (ISSET(status, STAT_RES_CRC_ERR)
	      && ISSET(cmd->c_flags, SCF_RSP_CRC)
	      && CPU_IS_PXA270) {
		/* workaround for erratum #42 */
		if (ISSET(cmd->c_flags, SCF_RSP_136)
		 && (cmd->c_resp[0] & 0x80000000U)) {
			DPRINTF(1,("%s: ignore CRC error\n",
			    device_xname(sc->sc_dev)));
		} else
			cmd->c_error = EIO;
	} else if (ISSET(status, STAT_ERR))
		cmd->c_error = EIO;

	if (cmd->c_error == 0 && cmd->c_datalen > 0) {
		/* workaround for erratum #91 */
		if (!ISSET(sc->sc_caps, PMC_CAPS_NO_DMA)
		 && CPU_IS_PXA270
		 && !ISSET(cmd->c_flags, SCF_CMD_READ)) {
			error = pxa2x0_dmac_start_xfer(sc->sc_txdx);
			if (error) {
				aprint_error_dev(sc->sc_dev,
				    "couldn't start dma xfer. (error=%d)\n",
				    error);
				cmd->c_error = EIO;
				pxamci_intr_done(sc);
				return;
			}
			pxamci_enable_intr(sc,
			    MMC_I_DATA_TRAN_DONE|MMC_I_DAT_ERR);
		}
	} else {
		pxamci_intr_done(sc);
	}
}

static void
pxamci_intr_data(struct pxamci_softc *sc)
{
	struct sdmmc_command *cmd = sc->sc_cmd;
	int intr;
	int n;

	DPRINTF(1,("%s: pxamci_intr_data: cmd = %p, resid = %d\n",
	    device_xname(sc->sc_dev), cmd, cmd->c_resid));

	n = MIN(32, cmd->c_resid);
	cmd->c_resid -= n;

	if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
		intr = MMC_I_RXFIFO_RD_REQ;
		while (n-- > 0)
			*cmd->c_buf++ = CSR_READ_1(sc, MMC_RXFIFO);
	} else {
		int short_xfer = n < 32;

		intr = MMC_I_TXFIFO_WR_REQ;
		while (n-- > 0)
			CSR_WRITE_1(sc, MMC_TXFIFO, *cmd->c_buf++);
		if (short_xfer)
			CSR_WRITE_4(sc, MMC_PRTBUF, 1);
	}

	if (cmd->c_resid > 0) {
		pxamci_enable_intr(sc, intr);
	} else {
		pxamci_disable_intr(sc, intr);
		pxamci_enable_intr(sc, MMC_I_DATA_TRAN_DONE);
	}
}

/*
 * Wake up the process sleeping in pxamci_exec_command().
 */
static void
pxamci_intr_done(struct pxamci_softc *sc)
{

	DPRINTF(1,("%s: pxamci_intr_done: mmc status = %#x\n",
	    device_xname(sc->sc_dev), CSR_READ_4(sc, MMC_STAT)));

	pxamci_disable_intr(sc, MMC_I_TXFIFO_WR_REQ|MMC_I_RXFIFO_RD_REQ|
	    MMC_I_DATA_TRAN_DONE|MMC_I_END_CMD_RES|MMC_I_RES_ERR|MMC_I_DAT_ERR);
	SET(sc->sc_cmd->c_flags, SCF_ITSDONE);
	sc->sc_cmd = NULL;
	wakeup(sc);
}

static void
pxamci_dmac_iintr(struct dmac_xfer *dx, int status)
{
	struct pxamci_softc *sc = dx->dx_cookie;

	if (status) {
		aprint_error_dev(sc->sc_dev, "pxamci_dmac_iintr: "
		    "non-zero completion status %d\n", status);
	}
}

static void
pxamci_dmac_ointr(struct dmac_xfer *dx, int status)
{
	struct pxamci_softc *sc = dx->dx_cookie;

	if (status) {
		aprint_error_dev(sc->sc_dev, "pxamci_dmac_ointr: "
		    "non-zero completion status %d\n", status);
	}
}
