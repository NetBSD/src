/* $NetBSD: awin_mmc.c,v 1.15.2.2 2015/09/22 12:05:36 skrll Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: awin_mmc.c,v 1.15.2.2 2015/09/22 12:05:36 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmc_ioreg.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#define AWIN_MMC_NDESC		16
#define AWIN_MMC_DMA_FTRGLEVEL_A20	0x20070008
#define AWIN_MMC_DMA_FTRGLEVEL_A80	0x200f0010

static const struct awin_gpio_pinset awin_mmc_pinsets_a80[4] = {
	[0] = { 'F', AWIN_A80_PIO_PF_SDMMC0_FUNC, AWIN_A80_PIO_PF_SDMMC0_PINS,
		GPIO_PIN_PULLUP, 2 },
	[1] = { 'G', AWIN_A80_PIO_PG_SDMMC1_FUNC, AWIN_A80_PIO_PG_SDMMC1_PINS,
		GPIO_PIN_PULLUP, 2 },
	[2] = { 'C', AWIN_A80_PIO_PC_SDMMC2_FUNC, AWIN_A80_PIO_PC_SDMMC2_PINS,
		GPIO_PIN_PULLUP, 2 },
};

static int	awin_mmc_match(device_t, cfdata_t, void *);
static void	awin_mmc_attach(device_t, device_t, void *);
static void	awin_mmc_attach_i(device_t);

static int	awin_mmc_intr(void *);

static int	awin_mmc_host_reset(sdmmc_chipset_handle_t);
static uint32_t	awin_mmc_host_ocr(sdmmc_chipset_handle_t);
static int	awin_mmc_host_maxblklen(sdmmc_chipset_handle_t);
static int	awin_mmc_card_detect(sdmmc_chipset_handle_t);
static int	awin_mmc_write_protect(sdmmc_chipset_handle_t);
static int	awin_mmc_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	awin_mmc_bus_clock(sdmmc_chipset_handle_t, int);
static int	awin_mmc_bus_width(sdmmc_chipset_handle_t, int);
static int	awin_mmc_bus_rod(sdmmc_chipset_handle_t, int);
static void	awin_mmc_exec_command(sdmmc_chipset_handle_t,
				      struct sdmmc_command *);
static void	awin_mmc_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	awin_mmc_card_intr_ack(sdmmc_chipset_handle_t);

static struct sdmmc_chip_functions awin_mmc_chip_functions = {
	.host_reset = awin_mmc_host_reset,
	.host_ocr = awin_mmc_host_ocr,
	.host_maxblklen = awin_mmc_host_maxblklen,
	.card_detect = awin_mmc_card_detect,
	.write_protect = awin_mmc_write_protect,
	.bus_power = awin_mmc_bus_power,
	.bus_clock = awin_mmc_bus_clock,
	.bus_width = awin_mmc_bus_width,
	.bus_rod = awin_mmc_bus_rod,
	.exec_command = awin_mmc_exec_command,
	.card_enable_intr = awin_mmc_card_enable_intr,
	.card_intr_ack = awin_mmc_card_intr_ack,
};

struct awin_mmc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_clk_bsh;
	bus_dma_tag_t sc_dmat;

	bool sc_use_dma;

	void *sc_ih;
	kmutex_t sc_intr_lock;
	kcondvar_t sc_intr_cv;
	kcondvar_t sc_idst_cv;

	int sc_mmc_width;
	int sc_mmc_present;
	int sc_mmc_port;

	device_t sc_sdmmc_dev;

	uint32_t sc_fifo_reg;
	uint32_t sc_dma_ftrglevel;

	uint32_t sc_idma_xferlen;
	bus_dma_segment_t sc_idma_segs[1];
	int sc_idma_nsegs;
	bus_size_t sc_idma_size;
	bus_dmamap_t sc_idma_map;
	int sc_idma_ndesc;
	void *sc_idma_desc;

	uint32_t sc_intr_rint;
	uint32_t sc_intr_mint;
	uint32_t sc_idma_idst;

	bool sc_has_gpio_detect;
	struct awin_gpio_pindata sc_gpio_detect;	/* card detect */
	bool sc_has_gpio_wp;
	struct awin_gpio_pindata sc_gpio_wp;		/* write protect */
	bool sc_has_gpio_led;
	struct awin_gpio_pindata sc_gpio_led;		/* LED */
};

CFATTACH_DECL_NEW(awin_mmc, sizeof(struct awin_mmc_softc),
	awin_mmc_match, awin_mmc_attach, NULL, NULL);

#define MMC_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define MMC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static int
awin_mmc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const int port = cf->cf_loc[AWINIOCF_PORT];

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	if (port != AWINIOCF_PORT_DEFAULT && port != loc->loc_port)
		return 0;

	return 1;
}

static int
awin_mmc_idma_setup(struct awin_mmc_softc *sc)
{
	int error;

	if (awin_chip_id() == AWIN_CHIP_ID_A10) {
		sc->sc_idma_xferlen = 0x2000;
	} else {
		sc->sc_idma_xferlen = 0x10000;
	}

	sc->sc_idma_ndesc = AWIN_MMC_NDESC;
	sc->sc_idma_size = sizeof(struct awin_mmc_idma_descriptor) *
	    sc->sc_idma_ndesc;
	error = bus_dmamem_alloc(sc->sc_dmat, sc->sc_idma_size, 0,
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
awin_mmc_attach(device_t parent, device_t self, void *aux)
{
	struct awin_mmc_softc * const sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	prop_dictionary_t cfg = device_properties(self);
	const char *pin_name;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	sc->sc_mmc_port = loc->loc_port;
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_intr_cv, "awinmmcirq");
	cv_init(&sc->sc_idst_cv, "awinmmcdma");
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	sc->sc_use_dma = true;
	prop_dictionary_get_bool(cfg, "dma", &sc->sc_use_dma);

	aprint_naive("\n");
	aprint_normal(": SD3.0 (%s)\n", sc->sc_use_dma ? "DMA" : "PIO");

	if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		if (awin_mmc_pinsets_a80[loc->loc_port].pinset_group) {
			awin_gpio_pinset_acquire(
			    &awin_mmc_pinsets_a80[loc->loc_port]);
		}

		bus_space_subregion(sc->sc_bst, aio->aio_ccm_bsh,
		    AWIN_A80_CCU_SCLK_SDMMC0_CLK_REG + (loc->loc_port * 4), 4,
		    &sc->sc_clk_bsh);
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A80_CCU_SCLK_BUS_CLK_GATING0_REG,
		    AWIN_A80_CCU_SCLK_BUS_CLK_GATING0_SD, 0);
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A80_CCU_SCLK_BUS_SOFT_RST0_REG,
		    AWIN_A80_CCU_SCLK_BUS_SOFT_RST0_SD, 0);
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_core_bsh,
		    AWIN_A80_SDMMC_COMM_OFFSET + (loc->loc_port * 4),
		    AWIN_A80_SDMMC_COMM_SDC_RESET_SW |
		    AWIN_A80_SDMMC_COMM_SDC_CLOCK_SW, 0);
		delay(1000);
	} else {
		bus_space_subregion(sc->sc_bst, aio->aio_ccm_bsh,
		    AWIN_SD0_CLK_REG + (loc->loc_port * 4), 4, &sc->sc_clk_bsh);

		awin_pll6_enable();

		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_AHB_GATING0_REG,
		    AWIN_AHB_GATING0_SDMMC0 << loc->loc_port, 0);
		if (awin_chip_id() == AWIN_CHIP_ID_A31) {
			awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
			    AWIN_A31_AHB_RESET0_REG,
			    AWIN_A31_AHB_RESET0_SD0_RST << loc->loc_port, 0);
		}
	}

	if (prop_dictionary_get_cstring_nocopy(cfg, "detect-gpio", &pin_name)) {
		if (!awin_gpio_pin_reserve(pin_name, &sc->sc_gpio_detect)) {
			aprint_error_dev(self,
			    "failed to reserve GPIO \"%s\"\n", pin_name);
		} else {
			sc->sc_has_gpio_detect = true;
		}
	}
	if (prop_dictionary_get_cstring_nocopy(cfg, "wp-gpio", &pin_name)) {
		if (!awin_gpio_pin_reserve(pin_name, &sc->sc_gpio_wp)) {
			aprint_error_dev(self,
			    "failed to reserve GPIO \"%s\"\n", pin_name);
		} else {
			sc->sc_has_gpio_wp = true;
		}
	}
	if (prop_dictionary_get_cstring_nocopy(cfg, "led-gpio", &pin_name)) {
		if (!awin_gpio_pin_reserve(pin_name, &sc->sc_gpio_led)) {
			aprint_error_dev(self,
			    "failed to reserve GPIO \"%s\"\n", pin_name);
		} else {
			sc->sc_has_gpio_led = true;
		}
	}

	switch (awin_chip_id()) {
	case AWIN_CHIP_ID_A80:
		sc->sc_fifo_reg = AWIN_A31_MMC_FIFO;
		sc->sc_dma_ftrglevel = AWIN_MMC_DMA_FTRGLEVEL_A80;
		break;
	case AWIN_CHIP_ID_A31:
		sc->sc_fifo_reg = AWIN_A31_MMC_FIFO;
		sc->sc_dma_ftrglevel = AWIN_MMC_DMA_FTRGLEVEL_A20;
		break;
	default:
		sc->sc_fifo_reg = AWIN_MMC_FIFO;
		sc->sc_dma_ftrglevel = AWIN_MMC_DMA_FTRGLEVEL_A20;
		break;
	}

	if (sc->sc_use_dma) {
		if (awin_mmc_idma_setup(sc) != 0) {
			aprint_error_dev(self, "failed to setup DMA\n");
			return;
		}
	}

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_BIO, IST_LEVEL,
	    awin_mmc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting at irq %d\n", loc->loc_intr);

	config_interrupts(self, awin_mmc_attach_i);
}

static int
awin_mmc_set_clock(struct awin_mmc_softc *sc, u_int freq)
{
	uint32_t odly, sdly, clksrc, n, m, clk;
	u_int osc24m_freq = AWIN_REF_FREQ / 1000;
	u_int pll_freq;

	if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		pll_freq = awin_periph0_get_rate() / 1000;
	} else {
		pll_freq = awin_pll6_get_rate() / 1000;
	}

#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "freq = %d, pll_freq = %d\n",
	    freq, pll_freq);
#endif

	if (freq <= 400) {
		odly = 0;
		sdly = 0;
		clksrc = AWIN_SD_CLK_SRC_SEL_OSC24M;
		n = 2;
		m = ((osc24m_freq / (1 << n)) / freq) - 1;
	} else if (freq <= 25000) {
		odly = 0;
		sdly = 5;
		clksrc = AWIN_SD_CLK_SRC_SEL_PLL6;
		n = awin_chip_id() == AWIN_CHIP_ID_A80 ? 2 : 0;
		m = ((pll_freq / freq) / (1 << n)) - 1;
	} else if (freq <= 50000) {
		odly = awin_chip_id() == AWIN_CHIP_ID_A80 ? 5 : 3;
		sdly = awin_chip_id() == AWIN_CHIP_ID_A80 ? 4 : 5;
		clksrc = AWIN_SD_CLK_SRC_SEL_PLL6;
		n = awin_chip_id() == AWIN_CHIP_ID_A80 ? 2 : 0;
		m = ((pll_freq / freq) / (1 << n)) - 1;
	} else {
		/* UHS speeds not implemented yet */
		return EIO;
	}

	clk = bus_space_read_4(sc->sc_bst, sc->sc_clk_bsh, 0);
	clk &= ~AWIN_SD_CLK_SRC_SEL;
	clk |= __SHIFTIN(clksrc, AWIN_SD_CLK_SRC_SEL);
	clk &= ~AWIN_SD_CLK_DIV_RATIO_N;
	clk |= __SHIFTIN(n, AWIN_SD_CLK_DIV_RATIO_N);
	clk &= ~AWIN_SD_CLK_DIV_RATIO_M;
	clk |= __SHIFTIN(m, AWIN_SD_CLK_DIV_RATIO_M);
	clk &= ~AWIN_SD_CLK_OUTPUT_PHASE_CTR;
	clk |= __SHIFTIN(odly, AWIN_SD_CLK_OUTPUT_PHASE_CTR);
	clk &= ~AWIN_SD_CLK_PHASE_CTR;
	clk |= __SHIFTIN(sdly, AWIN_SD_CLK_PHASE_CTR);
	clk |= AWIN_PLL_CFG_ENABLE;
	bus_space_write_4(sc->sc_bst, sc->sc_clk_bsh, 0, clk);

	return 0;
}

static void
awin_mmc_attach_i(device_t self)
{
	struct awin_mmc_softc *sc = device_private(self);
	struct sdmmcbus_attach_args saa;

	awin_mmc_host_reset(sc);
	awin_mmc_bus_width(sc, 1);
	awin_mmc_set_clock(sc, 400);

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &awin_mmc_chip_functions;
	saa.saa_sch = sc;
	saa.saa_clkmin = 400;
	saa.saa_clkmax = awin_chip_id() == AWIN_CHIP_ID_A80 ? 48000 : 50000;
	saa.saa_caps = SMC_CAPS_4BIT_MODE|
		       SMC_CAPS_8BIT_MODE|
		       SMC_CAPS_SD_HIGHSPEED|
		       SMC_CAPS_MMC_HIGHSPEED|
		       SMC_CAPS_AUTO_STOP;
	if (sc->sc_use_dma) {
		saa.saa_dmat = sc->sc_dmat;
		saa.saa_caps |= SMC_CAPS_DMA|
				SMC_CAPS_MULTI_SEG_DMA;
	}
	if (sc->sc_has_gpio_detect) {
		saa.saa_caps |= SMC_CAPS_POLL_CARD_DET;
	}

	sc->sc_sdmmc_dev = config_found(self, &saa, NULL);
}

static int
awin_mmc_intr(void *priv)
{
	struct awin_mmc_softc *sc = priv;
	uint32_t idst, rint, mint;

	mutex_enter(&sc->sc_intr_lock);
	idst = MMC_READ(sc, AWIN_MMC_IDST);
	rint = MMC_READ(sc, AWIN_MMC_RINT);
	mint = MMC_READ(sc, AWIN_MMC_MINT);
	if (!idst && !rint && !mint) {
		mutex_exit(&sc->sc_intr_lock);
		return 0;
	}
	MMC_WRITE(sc, AWIN_MMC_IDST, idst);
	MMC_WRITE(sc, AWIN_MMC_RINT, rint);
	MMC_WRITE(sc, AWIN_MMC_MINT, mint);

#ifdef AWIN_MMC_DEBUG
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

static int
awin_mmc_wait_rint(struct awin_mmc_softc *sc, uint32_t mask, int timeout)
{
	int retry;
	int error;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	if (sc->sc_intr_rint & mask)
		return 0;

	retry = sc->sc_use_dma ? (timeout / hz) : 10000;

	while (retry > 0) {
		if (sc->sc_use_dma) {
			error = cv_timedwait(&sc->sc_intr_cv,
			    &sc->sc_intr_lock, hz);
			if (error && error != EWOULDBLOCK)
				return error;
			if (sc->sc_intr_rint & mask)
				return 0;
		} else {
			sc->sc_intr_rint |= MMC_READ(sc, AWIN_MMC_RINT);
			if (sc->sc_intr_rint & mask)
				return 0;
			delay(1000);
		}
		--retry;
	}

	return ETIMEDOUT;
}

static void
awin_mmc_led(struct awin_mmc_softc *sc, int on)
{
	if (!sc->sc_has_gpio_led)
		return;
	awin_gpio_pindata_write(&sc->sc_gpio_led, on);
}

static int
awin_mmc_host_reset(sdmmc_chipset_handle_t sch)
{
	struct awin_mmc_softc *sc = sch;
	int retry = 1000;

#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "host reset\n");
#endif

	if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		if (sc->sc_mmc_port == 2 || sc->sc_mmc_port == 3) {
			MMC_WRITE(sc, AWIN_MMC_HWRST, 0);
			delay(10);
			MMC_WRITE(sc, AWIN_MMC_HWRST, 1);
			delay(300);
		}
	}

	MMC_WRITE(sc, AWIN_MMC_GCTRL,
	    MMC_READ(sc, AWIN_MMC_GCTRL) | AWIN_MMC_GCTRL_RESET);
	while (--retry > 0) {
		if (!(MMC_READ(sc, AWIN_MMC_GCTRL) & AWIN_MMC_GCTRL_RESET))
			break;
		delay(100);
	}

	MMC_WRITE(sc, AWIN_MMC_TIMEOUT, 0xffffffff);

	MMC_WRITE(sc, AWIN_MMC_IMASK,
	    AWIN_MMC_INT_CMD_DONE | AWIN_MMC_INT_ERROR |
	    AWIN_MMC_INT_DATA_OVER | AWIN_MMC_INT_AUTO_CMD_DONE);

	MMC_WRITE(sc, AWIN_MMC_GCTRL,
	    MMC_READ(sc, AWIN_MMC_GCTRL) | AWIN_MMC_GCTRL_INTEN);


	return 0;
}

static uint32_t
awin_mmc_host_ocr(sdmmc_chipset_handle_t sch)
{
	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V | MMC_OCR_HCS;
}

static int
awin_mmc_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 8192;
}

static int
awin_mmc_card_detect(sdmmc_chipset_handle_t sch)
{
	struct awin_mmc_softc *sc = sch;

	if (sc->sc_has_gpio_detect == false) {
		return 1;	/* no card detect pin, assume present */
	} else {
		int v = 0, i;
		for (i = 0; i < 5; i++) {
			v += awin_gpio_pindata_read(&sc->sc_gpio_detect);
			delay(1000);
		}
		if (v == 5)
			sc->sc_mmc_present = 0;
		else if (v == 0)
			sc->sc_mmc_present = 1;
		return sc->sc_mmc_present;
	}
}

static int
awin_mmc_write_protect(sdmmc_chipset_handle_t sch)
{
	struct awin_mmc_softc *sc = sch;

	if (sc->sc_has_gpio_wp == false) {
		return 0;	/* no write protect pin, assume rw */
	} else {
		return awin_gpio_pindata_read(&sc->sc_gpio_wp);
	}
}

static int
awin_mmc_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	return 0;
}

static int
awin_mmc_update_clock(struct awin_mmc_softc *sc)
{
	uint32_t cmd;
	int retry;

#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "update clock\n");
#endif

	cmd = AWIN_MMC_CMD_START |
	      AWIN_MMC_CMD_UPCLK_ONLY |
	      AWIN_MMC_CMD_WAIT_PRE_OVER;
	MMC_WRITE(sc, AWIN_MMC_CMD, cmd);
	retry = 0xfffff;
	while (--retry > 0) {
		if (!(MMC_READ(sc, AWIN_MMC_CMD) & AWIN_MMC_CMD_START))
			break;
		delay(10);
	}

	if (retry == 0) {
		aprint_error_dev(sc->sc_dev, "timeout updating clock\n");
#ifdef AWIN_MMC_DEBUG
		device_printf(sc->sc_dev, "GCTRL: 0x%08x\n",
		    MMC_READ(sc, AWIN_MMC_GCTRL));
		device_printf(sc->sc_dev, "CLKCR: 0x%08x\n",
		    MMC_READ(sc, AWIN_MMC_CLKCR));
		device_printf(sc->sc_dev, "TIMEOUT: 0x%08x\n",
		    MMC_READ(sc, AWIN_MMC_TIMEOUT));
		device_printf(sc->sc_dev, "WIDTH: 0x%08x\n",
		    MMC_READ(sc, AWIN_MMC_WIDTH));
		device_printf(sc->sc_dev, "CMD: 0x%08x\n",
		    MMC_READ(sc, AWIN_MMC_CMD));
		device_printf(sc->sc_dev, "MINT: 0x%08x\n",
		    MMC_READ(sc, AWIN_MMC_MINT));
		device_printf(sc->sc_dev, "RINT: 0x%08x\n",
		    MMC_READ(sc, AWIN_MMC_RINT));
		device_printf(sc->sc_dev, "STATUS: 0x%08x\n",
		    MMC_READ(sc, AWIN_MMC_STATUS));
#endif
		return ETIMEDOUT;
	}

	return 0;
}

static int
awin_mmc_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct awin_mmc_softc *sc = sch;
	uint32_t clkcr;

	clkcr = MMC_READ(sc, AWIN_MMC_CLKCR);
	if (clkcr & AWIN_MMC_CLKCR_CARDCLKON) {
		clkcr &= ~AWIN_MMC_CLKCR_CARDCLKON;
		MMC_WRITE(sc, AWIN_MMC_CLKCR, clkcr);
		if (awin_mmc_update_clock(sc) != 0)
			return 1;
	}

	if (freq) {

		clkcr &= ~AWIN_MMC_CLKCR_DIV;
		MMC_WRITE(sc, AWIN_MMC_CLKCR, clkcr);
		if (awin_mmc_update_clock(sc) != 0)
			return 1;

		if (awin_mmc_set_clock(sc, freq) != 0)
			return 1;

		clkcr |= AWIN_MMC_CLKCR_CARDCLKON;
		MMC_WRITE(sc, AWIN_MMC_CLKCR, clkcr);
		if (awin_mmc_update_clock(sc) != 0)
			return 1;
	}

	return 0;
}

static int
awin_mmc_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct awin_mmc_softc *sc = sch;

#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "width = %d\n", width);
#endif

	switch (width) {
	case 1:
		MMC_WRITE(sc, AWIN_MMC_WIDTH, AWIN_MMC_WIDTH_1);
		break;
	case 4:
		MMC_WRITE(sc, AWIN_MMC_WIDTH, AWIN_MMC_WIDTH_4);
		break;
	case 8:
		MMC_WRITE(sc, AWIN_MMC_WIDTH, AWIN_MMC_WIDTH_8);
		break;
	default:
		return 1;
	}

	sc->sc_mmc_width = width;

	return 0;
}

static int
awin_mmc_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	return -1;
}


static int
awin_mmc_pio_wait(struct awin_mmc_softc *sc, struct sdmmc_command *cmd)
{
	int retry = 0xfffff;
	uint32_t bit = (cmd->c_flags & SCF_CMD_READ) ?
	    AWIN_MMC_STATUS_FIFO_EMPTY : AWIN_MMC_STATUS_FIFO_FULL;

	while (--retry > 0) {
		uint32_t status = MMC_READ(sc, AWIN_MMC_STATUS);
		if (!(status & bit))
			return 0;
		delay(10);
	}

	return ETIMEDOUT;
}

static int
awin_mmc_pio_transfer(struct awin_mmc_softc *sc, struct sdmmc_command *cmd)
{
	uint32_t *datap = (uint32_t *)cmd->c_data;
	int i;

	for (i = 0; i < (cmd->c_resid >> 2); i++) {
		if (awin_mmc_pio_wait(sc, cmd))
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
awin_mmc_dma_prepare(struct awin_mmc_softc *sc, struct sdmmc_command *cmd)
{
	struct awin_mmc_idma_descriptor *dma = sc->sc_idma_desc;
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
			dma[desc].dma_config = htole32(AWIN_MMC_IDMA_CONFIG_CH |
					       AWIN_MMC_IDMA_CONFIG_OWN);
			cmd->c_resid -= len;
			resid -= len;
			off += len;
			if (desc == 0) {
				dma[desc].dma_config |= htole32(AWIN_MMC_IDMA_CONFIG_FD);
			}
			if (cmd->c_resid == 0) {
				dma[desc].dma_config |= htole32(AWIN_MMC_IDMA_CONFIG_LD);
				dma[desc].dma_config |= htole32(AWIN_MMC_IDMA_CONFIG_ER);
				dma[desc].dma_next = 0;
			} else {
				dma[desc].dma_config |=
				    htole32(AWIN_MMC_IDMA_CONFIG_DIC);
				dma[desc].dma_next = htole32(
				    desc_paddr + ((desc+1) *
				    sizeof(struct awin_mmc_idma_descriptor)));
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

	val = MMC_READ(sc, AWIN_MMC_GCTRL);
	val |= AWIN_MMC_GCTRL_DMAEN;
	val |= AWIN_MMC_GCTRL_INTEN;
	MMC_WRITE(sc, AWIN_MMC_GCTRL, val);
	val |= AWIN_MMC_GCTRL_DMARESET;
	MMC_WRITE(sc, AWIN_MMC_GCTRL, val);
	MMC_WRITE(sc, AWIN_MMC_DMAC, AWIN_MMC_DMAC_SOFTRESET);
	MMC_WRITE(sc, AWIN_MMC_DMAC,
	    AWIN_MMC_DMAC_IDMA_ON|AWIN_MMC_DMAC_FIX_BURST);
	val = MMC_READ(sc, AWIN_MMC_IDIE);
	val &= ~(AWIN_MMC_IDST_RECEIVE_INT|AWIN_MMC_IDST_TRANSMIT_INT);
	if (cmd->c_flags & SCF_CMD_READ)
		val |= AWIN_MMC_IDST_RECEIVE_INT;
	else
		val |= AWIN_MMC_IDST_TRANSMIT_INT;
	MMC_WRITE(sc, AWIN_MMC_IDIE, val);
	MMC_WRITE(sc, AWIN_MMC_DLBA, desc_paddr);
	MMC_WRITE(sc, AWIN_MMC_FTRGLEVEL, sc->sc_dma_ftrglevel);

	return 0;
}

static void
awin_mmc_dma_complete(struct awin_mmc_softc *sc)
{
	bus_dmamap_sync(sc->sc_dmat, sc->sc_idma_map, 0,
	    sc->sc_idma_size, BUS_DMASYNC_POSTWRITE);
}

static void
awin_mmc_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct awin_mmc_softc *sc = sch;
	uint32_t cmdval = AWIN_MMC_CMD_START;

#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev,
	    "opcode %d flags 0x%x data %p datalen %d blklen %d\n",
	    cmd->c_opcode, cmd->c_flags, cmd->c_data, cmd->c_datalen,
	    cmd->c_blklen);
#endif

	mutex_enter(&sc->sc_intr_lock);

	if (cmd->c_opcode == 0)
		cmdval |= AWIN_MMC_CMD_SEND_INIT_SEQ;
	if (cmd->c_flags & SCF_RSP_PRESENT)
		cmdval |= AWIN_MMC_CMD_RSP_EXP;
	if (cmd->c_flags & SCF_RSP_136)
		cmdval |= AWIN_MMC_CMD_LONG_RSP;
	if (cmd->c_flags & SCF_RSP_CRC)
		cmdval |= AWIN_MMC_CMD_CHECK_RSP_CRC;

	if (cmd->c_datalen > 0) {
		unsigned int nblks;

		cmdval |= AWIN_MMC_CMD_DATA_EXP | AWIN_MMC_CMD_WAIT_PRE_OVER;
		if (!ISSET(cmd->c_flags, SCF_CMD_READ)) {
			cmdval |= AWIN_MMC_CMD_WRITE;
		}

		nblks = cmd->c_datalen / cmd->c_blklen;
		if (nblks == 0 || (cmd->c_datalen % cmd->c_blklen) != 0)
			++nblks;

		if (nblks > 1) {
			cmdval |= AWIN_MMC_CMD_SEND_AUTO_STOP;
		}

		MMC_WRITE(sc, AWIN_MMC_BLKSZ, cmd->c_blklen);
		MMC_WRITE(sc, AWIN_MMC_BYTECNT, nblks * cmd->c_blklen);
	}

	sc->sc_intr_rint = 0;

	if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		MMC_WRITE(sc, AWIN_MMC_A12A,
		    (cmdval & AWIN_MMC_CMD_SEND_AUTO_STOP) ? 0 : 0xffff);
	}

	MMC_WRITE(sc, AWIN_MMC_ARG, cmd->c_arg);

#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "cmdval = %08x\n", cmdval);
#endif

	if (cmd->c_datalen == 0) {
		MMC_WRITE(sc, AWIN_MMC_CMD, cmdval | cmd->c_opcode);
	} else {
		cmd->c_resid = cmd->c_datalen;
		awin_mmc_led(sc, 0);
		if (sc->sc_use_dma) {
			cmd->c_error = awin_mmc_dma_prepare(sc, cmd);
			MMC_WRITE(sc, AWIN_MMC_CMD, cmdval | cmd->c_opcode);
			if (cmd->c_error == 0) {
				cmd->c_error = cv_timedwait(&sc->sc_idst_cv,
				    &sc->sc_intr_lock, hz*10);
			}
			awin_mmc_dma_complete(sc);
			if (sc->sc_idma_idst & AWIN_MMC_IDST_ERROR) {
				cmd->c_error = EIO;
			} else if (!(sc->sc_idma_idst & AWIN_MMC_IDST_COMPLETE)) {
				cmd->c_error = ETIMEDOUT;
			}
		} else {
			mutex_exit(&sc->sc_intr_lock);
			MMC_WRITE(sc, AWIN_MMC_CMD, cmdval | cmd->c_opcode);
			cmd->c_error = awin_mmc_pio_transfer(sc, cmd);
			mutex_enter(&sc->sc_intr_lock);
		}
		awin_mmc_led(sc, 1);
		if (cmd->c_error) {
#ifdef AWIN_MMC_DEBUG
			aprint_error_dev(sc->sc_dev,
			    "xfer failed, error %d\n", cmd->c_error);
#endif
			goto done;
		}
	}

	cmd->c_error = awin_mmc_wait_rint(sc,
	    AWIN_MMC_INT_ERROR|AWIN_MMC_INT_CMD_DONE, hz * 10);
	if (cmd->c_error == 0 && (sc->sc_intr_rint & AWIN_MMC_INT_ERROR)) {
		if (sc->sc_intr_rint & AWIN_MMC_INT_RESP_TIMEOUT) {
			cmd->c_error = ETIMEDOUT;
		} else {
			cmd->c_error = EIO;
		}
	}
	if (cmd->c_error) {
#ifdef AWIN_MMC_DEBUG
		aprint_error_dev(sc->sc_dev,
		    "cmd failed, error %d\n", cmd->c_error);
#endif
		goto done;
	}

	if (cmd->c_datalen > 0) {
		cmd->c_error = awin_mmc_wait_rint(sc,
		    AWIN_MMC_INT_ERROR|
		    AWIN_MMC_INT_AUTO_CMD_DONE|
		    AWIN_MMC_INT_DATA_OVER,
		    hz*10);
		if (cmd->c_error == 0 &&
		    (sc->sc_intr_rint & AWIN_MMC_INT_ERROR)) {
			cmd->c_error = ETIMEDOUT;
		}
		if (cmd->c_error) {
#ifdef AWIN_MMC_DEBUG
			aprint_error_dev(sc->sc_dev,
			    "data timeout, rint = %08x\n",
			    sc->sc_intr_rint);
#endif
			cmd->c_error = ETIMEDOUT;
			goto done;
		}
	}

	if (cmd->c_flags & SCF_RSP_PRESENT) {
		if (cmd->c_flags & SCF_RSP_136) {
			cmd->c_resp[0] = MMC_READ(sc, AWIN_MMC_RESP0);
			cmd->c_resp[1] = MMC_READ(sc, AWIN_MMC_RESP1);
			cmd->c_resp[2] = MMC_READ(sc, AWIN_MMC_RESP2);
			cmd->c_resp[3] = MMC_READ(sc, AWIN_MMC_RESP3);
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
			cmd->c_resp[0] = MMC_READ(sc, AWIN_MMC_RESP0);
		}
	}

done:
	cmd->c_flags |= SCF_ITSDONE;
	mutex_exit(&sc->sc_intr_lock);

	if (cmd->c_error) {
#ifdef AWIN_MMC_DEBUG
		aprint_error_dev(sc->sc_dev, "i/o error %d\n", cmd->c_error);
#endif
		MMC_WRITE(sc, AWIN_MMC_GCTRL,
		    MMC_READ(sc, AWIN_MMC_GCTRL) |
		      AWIN_MMC_GCTRL_DMARESET | AWIN_MMC_GCTRL_FIFORESET);
		for (int retry = 0; retry < 1000; retry++) {
			if (!(MMC_READ(sc, AWIN_MMC_GCTRL) & AWIN_MMC_GCTRL_RESET))
				break;
			delay(10);
		}
		awin_mmc_update_clock(sc);
	}

	if (!sc->sc_use_dma) {
		MMC_WRITE(sc, AWIN_MMC_GCTRL,
		    MMC_READ(sc, AWIN_MMC_GCTRL) | AWIN_MMC_GCTRL_FIFORESET);
	}
}

static void
awin_mmc_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
}

static void
awin_mmc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
}
