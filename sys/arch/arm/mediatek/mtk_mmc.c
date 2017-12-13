/*-
 * Copyright (c) 2017 Mediatek Inc.
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

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <dev/fdt/fdtvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmcreg.h>
#include <dev/sdmmc/sdmmcvar.h>

#define MSDC_CFG		0x0
#define MSDC_IOCON		0x4
#define MSDC_PS			0x8
#define MSDC_INT		0xC
#define MSDC_INTEN		0x10
#define MSDC_FIFOCS		0x14
#define SDC_CFG			0x30
#define SDC_CMD			0x34
#define SDC_ARG			0x38
#define SDC_STS			0x3C
#define SDC_RESP0		0x40
#define SDC_RESP1		0x44
#define SDC_RESP2		0x48
#define SDC_RESP3		0x4c
#define SDC_BLK_NUM		0x50
#define MSDC_DMA_SA		0x90
#define MSDC_DMA_CTRL		0x98
#define MSDC_DMA_CFG		0x9c
#define MSDC_DMA_LEN		0xA8
#define MSDC_PATCH_BIT		0xB0
#define MSDC_PATCH_BIT1		0xB4
#define MSDC_PATCH_BIT2		0xB8
#define MSDC_PAD_TUNE		0xF0
#define MSDC_PAD_TUNE1		0xF0
#define SDC_FIFO_CFG		0x228

/* MSDC_CFG mask */
#define MSDC_CFG_MODE		(0x1 << 0)	/* RW */
#define MSDC_CFG_CKPDN		(0x1 << 1)	/* RW */
#define MSDC_CFG_RST		(0x1 << 2)	/* RW */
#define MSDC_CFG_PIO		(0x1 << 3)	/* RW */
#define MSDC_CFG_CKSTB		(0x1 << 7)	/* RW */
#define MSDC_CFG_CKDIV		(0xfff << 8)	/* RW */
#define MSDC_CFG_CKMOD		(0x3 << 20)	/* RW */

/* MSDC_PS mask */
#define MSDC_PS_CDEN		(0x1 << 0)	/* RW */

/* MSDC_INT mask */
#define MSDC_INT_MMCIRQ		(0x1 << 0)	/* W1C */
#define MSDC_INT_CDSC		(0x1 << 1)	/* W1C */
#define MSDC_INT_ACMDRDY	(0x1 << 3)	/* W1C */
#define MSDC_INT_ACMDTMO	(0x1 << 4)	/* W1C */
#define MSDC_INT_ACMDCRCERR	(0x1 << 5)	/* W1C */
#define MSDC_INT_DMAQ_EMPTY	(0x1 << 6)	/* W1C */
#define MSDC_INT_SDIOIRQ	(0x1 << 7)	/* W1C */
#define MSDC_INT_CMDRDY		(0x1 << 8)	/* W1C */
#define MSDC_INT_CMDTMP		(0x1 << 9)	/* W1C */
#define MSDC_INT_RSPCRCERR	(0x1 << 10)	/* W1C */
#define MSDC_INT_CSTA		(0x1 << 11)	/* R */
#define MSDC_INT_XFER_COMPL	(0x1 << 12)	/* W1C */
#define MSDC_INT_XFER_DONE	(0x1 << 13)	/* W1C */
#define MSDC_INT_DATTMO		(0x1 << 14)	/* W1C */
#define MSDC_INT_DATCRCERR	(0x1 << 15)	/* W1C */
#define MSDC_INT_ACMD19_DONE	(0x1 << 16)	/* W1C */
#define MSDC_INT_DMA_BDCSERR	(0x1 << 17)	/* W1C */
#define MSDC_INT_DMA_GPDCSERR	(0x1 << 18)	/* W1C */
#define MSDC_INT_DMA_PROTECT	(0x1 << 19)	/* W1C */

/* MSDC_INTEN mask */
#define MSDC_INTEN_MMCIRQ	(0x1 << 0)	/* RW */
#define MSDC_INTEN_CDSC		(0x1 << 1)	/* RW */
#define MSDC_INTEN_ACMDRDY	(0x1 << 3)	/* RW */
#define MSDC_INTEN_ACMDTMO	(0x1 << 4)	/* RW */
#define MSDC_INTEN_ACMDCRCERR	(0x1 << 5)	/* RW */
#define MSDC_INTEN_DMAQ_EMPTY	(0x1 << 6)	/* RW */
#define MSDC_INTEN_SDIOIRQ	(0x1 << 7)	/* RW */
#define MSDC_INTEN_CMDRDY	(0x1 << 8)	/* RW */
#define MSDC_INTEN_CMDTMO	(0x1 << 9)	/* RW */
#define MSDC_INTEN_RSPCRCERR	(0x1 << 10)	/* RW */
#define MSDC_INTEN_CSTA		(0x1 << 11)	/* RW */
#define MSDC_INTEN_XFER_COMPL	(0x1 << 12)	/* RW */
#define MSDC_INTEN_XFER_DONE	(0x1 << 13)	/* RW */
#define MSDC_INTEN_DATTMO	(0x1 << 14)	/* RW */
#define MSDC_INTEN_DATCRCERR	(0x1 << 15)	/* RW */
#define MSDC_INTEN_ACMD19_DONE	(0x1 << 16)	/* RW */
#define MSDC_INTEN_DMA_BDCSERR	(0x1 << 17)	/* RW */
#define MSDC_INTEN_DMA_GPDCSERR	(0x1 << 18)	/* RW */
#define MSDC_INTEN_DMA_PROTECT	(0x1 << 19)	/* RW */

/* MSDC_FIFOCS mask */
#define MSDC_FIFOCS_CLR		(0x1 << 31)	/* RW */

/* SDC_CFG mask */
#define SDC_CFG_SDIOINTWKUP     (0x1 << 0)	/* RW */
#define SDC_CFG_INSWKUP         (0x1 << 1)	/* RW */
#define SDC_CFG_BUSWIDTH        (0x3 << 16)	/* RW */
#define SDC_CFG_SDIO            (0x1 << 19)	/* RW */
#define SDC_CFG_SDIOIDE         (0x1 << 20)	/* RW */
#define SDC_CFG_INTATGAP        (0x1 << 21)	/* RW */
#define SDC_CFG_DTOC            (0xff << 24)	/* RW */

/* SDC_STS mask */
#define SDC_STS_SDCBUSY		(0x1 << 0)	/* RU */
#define SDC_STS_CMDBUSY		(0x1 << 1)	/* RU */

/* MSDC_DMA_CTRL mask */
#define MSDC_DMA_CTRL_START     (0x1 << 0)	/* W */
#define MSDC_DMA_CTRL_STOP      (0x1 << 1)	/* W */
#define MSDC_DMA_CTRL_RESUME    (0x1 << 2)	/* W */
#define MSDC_DMA_CTRL_MODE      (0x1 << 8)	/* RW */
#define MSDC_DMA_CTRL_LASTBUF   (0x1 << 10)	/* RW */
#define MSDC_DMA_CTRL_BRUSTSZ   (0x7 << 12)	/* RW */

/* MSDC_PATCH_BIT1 mask */
#define MSDC_PATCH_BIT1_STOP_DLY  (0xf << 8)    /* RW */

/* MSDC_PATCH_BIT2 mask */
#define MSDC_PATCH_BIT2_CFGRESP   (0x1 << 15)   /* RW */
#define MSDC_PATCH_BIT2_POPENCNT  (0xF << 20)   /* RW */
#define MSDC_PATCH_BIT2_CFGCRCSTS (0x1 << 28)   /* RW */
#define MSDC_PB2_RESPWAIT         (0x3 << 2)    /* RW */
#define MSDC_PB2_RESPSTSENSEL     (0x7 << 16)   /* RW */
#define MSDC_PB2_CRCSTSENSEL      (0x7 << 29)   /* RW */


/* MSDC_PAD_TUNE mask */
#define MSDC_PAD_TUNE_DATWRDLY	(0x1f << 0)	/* RW */
#define MSDC_PAD_TUNE_DELAYEN	(0x1 << 7)	/* RW */
#define MSDC_PAD_TUNE_DATRRDLY	(0x1f << 8)	/* RW */
#define MSDC_PAD_TUNE_RD_SEL	(0x1 << 13)	/* RW */
#define MSDC_PAD_TUNE_RXDLYSEL	(0x1 << 15)	/* RW */
#define MSDC_PAD_TUNE_CMDRDLY	(0x1f << 16)	/* RW */
#define MSDC_PAD_TUNE_CMD_SEL	(0x1 << 21)	/* RW */
#define MSDC_PAD_TUNE_CMDRRDLY	(0x1f << 22)	/* RW */

/* SDC_FIFO_CFG mask */
#define SDC_FIFO_CFG_WRVALIDSEL	(0x1 << 24)
#define SDC_FIFO_CFG_RDVALIDSEL	(0x1 << 25)


#define MSDC_BUS_1BITS		0x0
#define MSDC_BUS_4BITS		0x1
#define MSDC_BUS_8BITS		0x2

static const uint32_t cmd_ints_mask = MSDC_INTEN_CMDRDY | MSDC_INTEN_RSPCRCERR |
			MSDC_INTEN_CMDTMO;
static const uint32_t data_ints_mask = MSDC_INTEN_XFER_COMPL | MSDC_INTEN_DATTMO |
			MSDC_INTEN_DATCRCERR;

static int mtk_mmc_match(device_t, cfdata_t, void *);
static void mtk_mmc_attach(device_t, device_t, void * );
static void mtk_mmc_attach_i(device_t);

static int mtk_mmc_host_reset(sdmmc_chipset_handle_t);
static uint32_t mtk_mmc_host_ocr(sdmmc_chipset_handle_t);
static int mtk_mmc_host_maxblklen(sdmmc_chipset_handle_t);
static int mtk_mmc_card_detect(sdmmc_chipset_handle_t);
static int mtk_mmc_write_protect(sdmmc_chipset_handle_t);
static int mtk_mmc_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int mtk_mmc_bus_clock(sdmmc_chipset_handle_t, int);
static int mtk_mmc_bus_width(sdmmc_chipset_handle_t, int);
static int mtk_mmc_bus_rod(sdmmc_chipset_handle_t, int);
static void mtk_mmc_exec_command(sdmmc_chipset_handle_t, struct sdmmc_command *);
static void mtk_mmc_card_enable_intr(sdmmc_chipset_handle_t, int);
static void mtk_mmc_card_intr_ack(sdmmc_chipset_handle_t);

static struct sdmmc_chip_functions mtk_mmc_chip_functions = {
	.host_reset = mtk_mmc_host_reset,
	.host_ocr = mtk_mmc_host_ocr,
	.host_maxblklen = mtk_mmc_host_maxblklen,
	.card_detect = mtk_mmc_card_detect,
	.write_protect = mtk_mmc_write_protect,
	.bus_power = mtk_mmc_bus_power,
	.bus_clock = mtk_mmc_bus_clock,
	.bus_width = mtk_mmc_bus_width,
	.bus_rod = mtk_mmc_bus_rod,
	.exec_command = mtk_mmc_exec_command,
	.card_enable_intr = mtk_mmc_card_enable_intr,
	.card_intr_ack = mtk_mmc_card_intr_ack,
};

struct mtk_mmc_softc {
	device_t sc_dev;		/* SD/MS Constrol device*/
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;

	void *sc_ih;			/*gic intterupt*/
	kmutex_t sc_intr_lock;
	kcondvar_t cmd_done_cv;
	kcondvar_t dma_done_cv;

	bool flag_cmd_intr;

	uint32_t sc_events;		/* mmc interrupt*/
	int src_clk_freq;		/* source clock frequency*/
	int mclk;		 /* mmc subsystem clock frequency*/
	int sclk;		 /* SD/MS bus clock frequency */
	int sc_buswidth;         /* current bus width*/

	device_t sdmmc_dev;		/* SD/MS card device*/

	bus_dmamap_t sc_dmamap;
	bus_dma_segment_t sc_segs[1];
	void *sc_dma_desc;
};

static const char * const compatible[] = {
	"mediatek,mercury-mmc",
	NULL
};

CFATTACH_DECL_NEW(mtk_mmc, sizeof(struct mtk_mmc_softc),
	mtk_mmc_match, mtk_mmc_attach, NULL, NULL);

static int msdc_irq(void *);
static void msdc_init_hw(struct mtk_mmc_softc * const);
static int msdc_dma_init(struct mtk_mmc_softc *);

static void msdc_writel(struct mtk_mmc_softc *sc, uint32_t reg, uint32_t val)
{
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val));
}

static uint32_t msdc_readl(struct mtk_mmc_softc *sc, uint32_t reg)
{
	return bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg));
}

static void sdr_set_bits(struct mtk_mmc_softc *sc, uint32_t reg, uint32_t bs)
{
	uint32_t val = msdc_readl(sc, reg);
	val |= bs;
	msdc_writel(sc, reg, val);
}

static void sdr_clr_bits(struct mtk_mmc_softc *sc, uint32_t reg, uint32_t bs)
{
	uint32_t val = msdc_readl(sc, reg);
	val &= ~bs;
	msdc_writel(sc, reg, val);
}

static void msdc_set_field(struct mtk_mmc_softc *sc, uint32_t reg,
		uint32_t field, uint32_t val)
{
	uint32_t tv = msdc_readl(sc, reg);
	tv &= ~field;
	tv |= __SHIFTIN(val, field);
	msdc_writel(sc, reg, tv);
}

static int mtk_mmc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void mtk_mmc_attach(device_t parent, device_t self, void *aux)
{
	struct mtk_mmc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	char intrstr[128];
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->cmd_done_cv, "mtkmmcirq");
	cv_init(&sc->dma_done_cv, "mtkmmcdma");

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, addr, size, 0, &bsh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}
	sc->sc_bsh = bsh;
	sc->src_clk_freq = 200000;

	msdc_init_hw(sc);

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(faa->faa_phandle, 0, IPL_BIO,
		FDT_INTR_MPSAFE, msdc_irq, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
			intrstr);
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	msdc_dma_init(sc);

	config_interrupts(self, mtk_mmc_attach_i);
}

static void mtk_mmc_attach_i(device_t self)
{
	struct mtk_mmc_softc *sc = device_private(self);
	struct sdmmcbus_attach_args saa;

	mtk_mmc_bus_width(sc, 1);
	mtk_mmc_bus_clock(sc, 400);

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &mtk_mmc_chip_functions;
	saa.saa_sch = sc;
	saa.saa_clkmin = 400;
	saa.saa_clkmax = 50000;
	saa.saa_caps = SMC_CAPS_4BIT_MODE | SMC_CAPS_8BIT_MODE |
		SMC_CAPS_SD_HIGHSPEED | SMC_CAPS_MMC_HIGHSPEED;
	saa.saa_caps |= SMC_CAPS_DMA; /*only use DMA mode in kernel*/
	saa.saa_dmat = sc->sc_dmat;
	sc->sdmmc_dev = config_found(self, &saa, NULL);
}

static void msdc_init_hw(struct mtk_mmc_softc *const sc)
{
	uint32_t val;

	sdr_set_bits(sc, MSDC_CFG, MSDC_CFG_MODE);

	/* Reset */
	mtk_mmc_host_reset(sc);

	/* Disable card detection */
	sdr_clr_bits(sc, MSDC_PS, MSDC_PS_CDEN);

	/* Disable and clear all interrupts */
	 msdc_writel(sc, MSDC_INTEN, 0);
	 val = msdc_readl(sc, MSDC_INT);
	 msdc_writel(sc, MSDC_INT, val);

	 msdc_writel(sc, MSDC_PAD_TUNE, 0);
	 msdc_writel(sc, MSDC_IOCON, 0);
	 msdc_writel(sc, MSDC_PATCH_BIT, 0x403c0006);
	 msdc_writel(sc, MSDC_PATCH_BIT1, 0xFFA04309);

	 /* Stop clock delay*/
	 msdc_set_field(sc, MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_STOP_DLY, 6);
	 msdc_set_field(sc, MSDC_PATCH_BIT2, MSDC_PATCH_BIT2_POPENCNT, 0);
	 sdr_clr_bits(sc, SDC_FIFO_CFG, SDC_FIFO_CFG_WRVALIDSEL);
	 sdr_clr_bits(sc, SDC_FIFO_CFG, SDC_FIFO_CFG_RDVALIDSEL);

	 /*use aync fifo, then no need tune internal delay */
	 sdr_clr_bits(sc, MSDC_PATCH_BIT2, MSDC_PATCH_BIT2_CFGRESP);
	 sdr_set_bits(sc, MSDC_PATCH_BIT2, MSDC_PATCH_BIT2_CFGCRCSTS);
	 msdc_set_field(sc, MSDC_PATCH_BIT2, MSDC_PB2_RESPWAIT, 3);
	 msdc_set_field(sc, MSDC_PATCH_BIT2, MSDC_PB2_RESPSTSENSEL, 2);
	 msdc_set_field(sc, MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL, 2);

	 /* use data tune */
	 sdr_set_bits(sc, MSDC_PAD_TUNE, MSDC_PAD_TUNE_RD_SEL | MSDC_PAD_TUNE_CMD_SEL);

	 /* Disable detect SDIO device interrupt function */
	 sdr_clr_bits(sc, SDC_CFG, SDC_CFG_SDIOIDE);

	 /*Configure to default data timeout*/
	 msdc_set_field(sc, SDC_CFG, SDC_CFG_DTOC, 3);
}

static int msdc_dma_init(struct mtk_mmc_softc *sc)
{
	int error, rseg;

	error = bus_dmamem_alloc(sc->sc_dmat, MAXPHYS, PAGE_SIZE, MAXPHYS,
	    sc->sc_segs, 1, &rseg, BUS_DMA_WAITOK);
	if (error) {
		device_printf(sc->sc_dev, "bus_dmamem_alloc failed: %d\n", error);
		return error;
	}

	KASSERT(rseg == 1);

	error = bus_dmamem_map(sc->sc_dmat, sc->sc_segs, rseg, MAXPHYS,
	    &sc->sc_dma_desc, BUS_DMA_WAITOK);
	if (error) {
		device_printf(sc->sc_dev, "bus_dmamem_map failed\n");
		goto free;
	}

	error = bus_dmamap_create(sc->sc_dmat, MAXPHYS, 1, MAXPHYS, 0,
	    BUS_DMA_WAITOK, &sc->sc_dmamap);
	if (error) {
		device_printf(sc->sc_dev, "bus_dmamap_create failed\n");
		goto unmap;
	}

	return 0;
unmap:
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_dma_desc, MAXPHYS);
free:
	bus_dmamem_free(sc->sc_dmat, sc->sc_segs, rseg);
	return error;
}

static int msdc_irq(void *priv)
{
	struct mtk_mmc_softc *sc = priv;
	uint32_t events_valid;
	uint32_t events, event_mask;

	mutex_enter(&sc->sc_intr_lock);
	events = msdc_readl(sc, MSDC_INT);
	event_mask = msdc_readl(sc, MSDC_INTEN);

	events_valid = events & event_mask;

	/*clear interrupts */
	msdc_writel(sc, MSDC_INT, events);

	if (!events_valid) {
		mutex_exit(&sc->sc_intr_lock);
		return 0;
	}

	sc->sc_events |= events_valid;
	if (events_valid & cmd_ints_mask) {
		sc->flag_cmd_intr = true;
		cv_broadcast(&sc->cmd_done_cv);
	}
	else if (events_valid & data_ints_mask) {
		cv_broadcast(&sc->dma_done_cv);
	}
	mutex_exit(&sc->sc_intr_lock);

	return 1;
}

static int mtk_mmc_host_reset(sdmmc_chipset_handle_t sch)
{
	struct mtk_mmc_softc *sc = sch;
	uint32_t val;

	sdr_set_bits(sc, MSDC_CFG, MSDC_CFG_RST);
	while (msdc_readl(sc, MSDC_CFG) & MSDC_CFG_RST)
		DELAY(1);

	sdr_set_bits(sc, MSDC_FIFOCS, MSDC_FIFOCS_CLR);
	while (msdc_readl(sc, MSDC_FIFOCS) & MSDC_FIFOCS_CLR)
		DELAY(1);

	val = msdc_readl(sc, MSDC_INT);
	msdc_writel(sc, MSDC_INT, val);

	return 0;
}

static uint32_t mtk_mmc_host_ocr(sdmmc_chipset_handle_t sch)
{
	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V;
}

static int mtk_mmc_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 2048;
}

static int mtk_mmc_card_detect(sdmmc_chipset_handle_t sch)
{
	return 1;
}

static int mtk_mmc_write_protect(sdmmc_chipset_handle_t sch)
{
	return 0;
}

static int mtk_mmc_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	return 0;
}

static int mtk_mmc_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct mtk_mmc_softc *sc = sch;
	uint32_t val = msdc_readl(sc, SDC_CFG);
	val &= ~SDC_CFG_BUSWIDTH;

	switch(width) {
	case 1:
		val |= MSDC_BUS_1BITS;
		break;
	case 4:
		val |= MSDC_BUS_4BITS;
		break;
	case 8:
		val |= MSDC_BUS_8BITS;
		break;
	default:
		return 1;
	}

	sc->sc_buswidth = width;
	msdc_set_field(sc, SDC_CFG, SDC_CFG_BUSWIDTH, val);
	aprint_normal_dev(sc->sc_dev, "width = %d\n", width);

	return 0;
}

static int mtk_mmc_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct mtk_mmc_softc *sc = sch;

	uint32_t mode;
	uint32_t flags;
	uint32_t div;
	uint32_t sclk;

	if(sc->mclk == freq)
		return 0;

	if(freq <= 0) {
		sc->mclk = 0;
		sdr_clr_bits(sc, MSDC_CFG, MSDC_CFG_CKPDN);
		return 1;
	}

	flags = msdc_readl(sc, MSDC_INTEN);
	sdr_clr_bits(sc, MSDC_INTEN, flags);

	if(freq >= sc->src_clk_freq) {
		mode = 0x1;	/* no divisor */
		div = 0;
		sclk = sc->src_clk_freq;
	} else {
		mode = 0x0;	/* use divisor*/
		if (freq >= (sc->src_clk_freq >> 1)) {
			div = 0;
			sclk = sc->src_clk_freq >> 1; /*sclk = clk/2 */
		} else {
			div = (sc->src_clk_freq + ((freq << 2)-1))/(freq << 2);
			sclk = (sc->src_clk_freq >> 2) / div;
		}
	}

	sdr_clr_bits(sc, MSDC_CFG, MSDC_CFG_CKPDN);

	/*before change clock mode, should gate source clock*/
	//clk_disable_unprepare(sc->src_clk);
	msdc_set_field(sc, MSDC_CFG, MSDC_CFG_CKMOD | MSDC_CFG_CKDIV,
			(mode << 12) | (div % 0xfff));
	//clk_prepare_enable(sc->src_clk);

	while(!(msdc_readl(sc, MSDC_CFG) & MSDC_CFG_CKSTB))
		DELAY(1);

	sdr_set_bits(sc, MSDC_CFG, MSDC_CFG_CKPDN);
	sc->sclk = sclk;
	sc->mclk = freq;
	sdr_set_bits(sc, MSDC_INTEN, flags);
	aprint_normal_dev(sc->sc_dev, "sclk = %d\n", sclk);

	return 0;
}

static int mtk_mmc_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	return -1;
}

#define mmc_response_type(cmd) ((cmd)->c_flags & (SCF_RSP_PRESENT|SCF_RSP_136|SCF_RSP_CRC|SCF_RSP_BSY|SCF_RSP_IDX))
#define mmc_cmd_type(cmd)	((cmd)->c_flags & SCF_CMD_MASK)
static uint32_t mtk_mmc_cmd_find_resp(struct mtk_mmc_softc *sc, struct sdmmc_command *cmd)
{
	uint32_t resp;

	switch (mmc_response_type(cmd)) {
	case SCF_RSP_R1:
		resp = 0x1;
		break;
	case SCF_RSP_R1B:
		resp = 0x7;
		break;
	case SCF_RSP_R2:
		resp = 0x2;
		break;
	case SCF_RSP_R3:
		resp = 0x3;
		break;
	case SCF_RSP_R0:
	default:
		resp = 0x0;
		break;
	}
	return resp;
}

static uint32_t mtk_mmc_cmd_prepare_raw_cmd(struct mtk_mmc_softc *sc, struct sdmmc_command *cmd)
{
	/* rawcmd :
	   vol_swth << 30 | autocmd << 28 | blklen << 16 | go_irq << 15 |
	   stop << 14 | rw << 13 | dtype << 11 | rsptyp << 7 | break < 6 | opcode
	 */
	uint32_t nblks;
	uint32_t opcode = cmd->c_opcode;
	uint32_t resp = mtk_mmc_cmd_find_resp(sc, cmd);
	uint32_t rawcmd = (opcode & 0x3f) | ((resp & 0x7) << 7);

	if (opcode == MMC_STOP_TRANSMISSION)
		rawcmd |= (0x1 << 14);
	else if(opcode == SD_APP_SEND_SCR ||
			(opcode == SD_APP_SD_STATUS && mmc_cmd_type(cmd) == SCF_CMD_ADTC) ||
			(opcode ==MMC_SEND_EXT_CSD && mmc_cmd_type(cmd)== SCF_CMD_ADTC))
		rawcmd |= (0x1 << 11);

	if(cmd->c_datalen) {
		nblks = cmd->c_datalen/cmd->c_blklen;
		rawcmd |= ((cmd->c_blklen & 0xFFF) << 16);
		if(opcode == MMC_WRITE_BLOCK_SINGLE || opcode == MMC_WRITE_BLOCK_MULTIPLE)
			rawcmd |= (0x1 << 13);
		if(nblks > 1)
			rawcmd |= (0x2 << 11);
		else
			rawcmd |= (0x1 << 11);
		/*Always use dma mode*/
		sdr_clr_bits(sc, MSDC_CFG, MSDC_CFG_PIO);

		msdc_writel(sc, SDC_BLK_NUM, nblks);
	}
	return rawcmd;
}

static int msdc_cmd_is_ready(struct mtk_mmc_softc *sc, struct sdmmc_command *cmd)
{
	int i;
	for (i = 0; i < 20000; i++)
	{
		if (!(msdc_readl(sc, SDC_STS) & SDC_STS_CMDBUSY))
			return 0;
		delay(1);
	}
	return EBUSY;
}

static int msdc_wait_intr(struct mtk_mmc_softc *sc, uint32_t mask, int timeout)
{
	int retry, error;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	if(sc->sc_events & mask)
		return 0;
	retry = timeout / hz;

	while (retry > 0) {
		if(sc->flag_cmd_intr)
			error = cv_timedwait(&sc->dma_done_cv, &sc->sc_intr_lock, hz);
		else
			error = cv_timedwait(&sc->cmd_done_cv, &sc->sc_intr_lock, hz);
		if (error && error != EWOULDBLOCK)
			return error;
		if(sc->sc_events & mask)
			return 0;
		--retry;
	}

	return ETIMEDOUT;
}

static void mtk_mmc_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct mtk_mmc_softc *sc = sch;
	uint32_t rawcmd;
	bool use_bbuf = false;

	mutex_enter(&sc->sc_intr_lock);

	//aprint_error_dev(sc->sc_dev, "cmd:%d, cmd->c_arg=%08X\n", cmd->c_opcode,cmd->c_arg);

	cmd->c_error = 0;

	rawcmd = mtk_mmc_cmd_prepare_raw_cmd(sc, cmd);

	sdr_set_bits(sc, MSDC_INTEN, cmd_ints_mask);

	sc->sc_events = 0;
	sc->flag_cmd_intr = false;
	msdc_writel(sc, SDC_ARG, cmd->c_arg);

	cmd->c_error =msdc_cmd_is_ready(sc, cmd);

	if (cmd->c_error)
		goto done;

	if (cmd->c_datalen > 0) {
		cmd->c_error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap,
				sc->sc_dma_desc, MAXPHYS, NULL, BUS_DMA_WAITOK);
		if (cmd->c_error) {
			aprint_error_dev(sc->sc_dev, "bus_dmamap_load failed\n");
			goto done;
		}
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
					MAXPHYS, BUS_DMASYNC_PREREAD);
		} else {
			memcpy(sc->sc_dma_desc, cmd->c_data, cmd->c_datalen);
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
					MAXPHYS, BUS_DMASYNC_PREWRITE);
		}
		msdc_writel(sc, MSDC_DMA_SA, sc->sc_dmamap->dm_segs[0].ds_addr);
		sdr_set_bits(sc, MSDC_DMA_CTRL, MSDC_DMA_CTRL_LASTBUF);
		msdc_writel(sc, MSDC_DMA_LEN, cmd->c_datalen);
		sdr_clr_bits(sc, MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE);
		use_bbuf = true;
	}

	cmd->c_resid = cmd->c_datalen;
	msdc_writel(sc, SDC_CMD, rawcmd);

	cmd->c_error = msdc_wait_intr(sc, cmd_ints_mask, 5 * hz);

	if(cmd->c_error == 0) {
		if(sc->sc_events & MSDC_INTEN_RSPCRCERR)
			cmd->c_error = EIO;
		else if(sc->sc_events & MSDC_INTEN_CMDTMO)
			cmd->c_error = ETIMEDOUT;
	}

	sdr_clr_bits(sc, MSDC_INTEN, cmd_ints_mask);

	if(cmd->c_error) {
		aprint_error_dev(sc->sc_dev, "cmd=%d, args=%08X, cmd_error=%d\n",
				cmd->c_opcode, cmd->c_arg, cmd->c_error);
		goto done;
	}

	if(cmd->c_datalen > 0) {
		sdr_set_bits(sc, MSDC_INTEN, data_ints_mask);
		msdc_set_field(sc, MSDC_DMA_CTRL, MSDC_DMA_CTRL_START, 1);
		cmd->c_error = msdc_wait_intr(sc, data_ints_mask, 10 * hz);
		if(cmd->c_error == 0) {
			if (sc->sc_events & MSDC_INTEN_DATCRCERR)
				cmd->c_error = EIO;
			else if (sc->sc_events & MSDC_INTEN_DATTMO)
				cmd->c_error = ETIMEDOUT;
		}

		sdr_clr_bits(sc, MSDC_INTEN, data_ints_mask);
		if(cmd->c_error) {
			aprint_error_dev(sc->sc_dev,"cmd=%d, args=%08X, data_error=%d\n",
					cmd->c_opcode, cmd->c_arg, cmd->c_error);
			goto done;
		}
	}

	if(cmd->c_flags & SCF_RSP_PRESENT) {
		if (cmd->c_flags & SCF_RSP_136) {
			cmd->c_resp[0] = msdc_readl(sc, SDC_RESP0);
			cmd->c_resp[1] = msdc_readl(sc, SDC_RESP1);
			cmd->c_resp[2] = msdc_readl(sc, SDC_RESP2);
			cmd->c_resp[3] = msdc_readl(sc, SDC_RESP3);
			if(cmd->c_flags & SCF_RSP_CRC) {
				cmd->c_resp[0] = (cmd->c_resp[0] >> 8) | (cmd->c_resp[1] << 24);
				cmd->c_resp[0] = (cmd->c_resp[0] >> 8) | (cmd->c_resp[1] << 24);
				cmd->c_resp[1] = (cmd->c_resp[1] >> 8) | (cmd->c_resp[2] << 24);
				cmd->c_resp[2] = (cmd->c_resp[2] >> 8) | (cmd->c_resp[3] << 24);
				cmd->c_resp[3] = (cmd->c_resp[3] >> 8);
			}
		} else {
			cmd->c_resp[0] = msdc_readl(sc, SDC_RESP0);
		}
	}

done:
	if(use_bbuf) {
		if(ISSET(cmd->c_flags, SCF_CMD_READ)) {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
					MAXPHYS, BUS_DMASYNC_POSTREAD);
		} else {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
					MAXPHYS, BUS_DMASYNC_POSTWRITE);
		}
		bus_dmamap_unload(sc->sc_dmat, sc->sc_dmamap);
		if(ISSET(cmd->c_flags, SCF_CMD_READ)) {
			memcpy(cmd->c_data, sc->sc_dma_desc, cmd->c_datalen);
		}
	}
	cmd->c_flags |= SCF_ITSDONE;
	mtk_mmc_host_reset(sc);
	mutex_exit(&sc->sc_intr_lock);
}

static void mtk_mmc_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
	/*Nothing*/
}

static void mtk_mmc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	/*Nothing*/
}
