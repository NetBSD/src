/* $NetBSD: sunxi_mmc.c,v 1.17 2017/11/15 13:53:26 jmcneill Exp $ */

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

#include "opt_sunximmc.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_mmc.c,v 1.17 2017/11/15 13:53:26 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/gpio.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmc_ioreg.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_mmc.h>

#ifdef SUNXI_MMC_DEBUG
static int sunxi_mmc_debug = SUNXI_MMC_DEBUG;
#define	DPRINTF(dev, fmt, ...)						\
do {									\
	if (sunxi_mmc_debug & __BIT(device_unit(dev)))			\
		device_printf((dev), fmt, ##__VA_ARGS__);		\
} while (0)
#else
#define	DPRINTF(dev, fmt, ...)		((void)0)
#endif

enum sunxi_mmc_timing {
	SUNXI_MMC_TIMING_400K,
	SUNXI_MMC_TIMING_25M,
	SUNXI_MMC_TIMING_50M,
	SUNXI_MMC_TIMING_50M_DDR,
	SUNXI_MMC_TIMING_50M_DDR_8BIT,
};

struct sunxi_mmc_delay {
	u_int	output_phase;
	u_int	sample_phase;
};

static const struct sunxi_mmc_delay sun7i_mmc_delays[] = {
	[SUNXI_MMC_TIMING_400K]		= { 180,	180 },
	[SUNXI_MMC_TIMING_25M]		= { 180,	 75 },
	[SUNXI_MMC_TIMING_50M]		= {  90,	120 },
	[SUNXI_MMC_TIMING_50M_DDR]	= {  60,	120 },
	[SUNXI_MMC_TIMING_50M_DDR_8BIT]	= {  90,	180 },
};

static const struct sunxi_mmc_delay sun9i_mmc_delays[] = {
	[SUNXI_MMC_TIMING_400K]		= { 180,	180 },
	[SUNXI_MMC_TIMING_25M]		= { 180,	 75 },
	[SUNXI_MMC_TIMING_50M]		= { 150,	120 },
	[SUNXI_MMC_TIMING_50M_DDR]	= {  54,	 36 },
	[SUNXI_MMC_TIMING_50M_DDR_8BIT]	= {  72,	 72 },
};

#define SUNXI_MMC_NDESC		16

struct sunxi_mmc_softc;

static int	sunxi_mmc_match(device_t, cfdata_t, void *);
static void	sunxi_mmc_attach(device_t, device_t, void *);
static void	sunxi_mmc_attach_i(device_t);

static int	sunxi_mmc_intr(void *);
static int	sunxi_mmc_dmabounce_setup(struct sunxi_mmc_softc *);
static int	sunxi_mmc_idma_setup(struct sunxi_mmc_softc *);

static int	sunxi_mmc_host_reset(sdmmc_chipset_handle_t);
static uint32_t	sunxi_mmc_host_ocr(sdmmc_chipset_handle_t);
static int	sunxi_mmc_host_maxblklen(sdmmc_chipset_handle_t);
static int	sunxi_mmc_card_detect(sdmmc_chipset_handle_t);
static int	sunxi_mmc_write_protect(sdmmc_chipset_handle_t);
static int	sunxi_mmc_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	sunxi_mmc_bus_clock(sdmmc_chipset_handle_t, int, bool);
static int	sunxi_mmc_bus_width(sdmmc_chipset_handle_t, int);
static int	sunxi_mmc_bus_rod(sdmmc_chipset_handle_t, int);
static int	sunxi_mmc_signal_voltage(sdmmc_chipset_handle_t, int);
static void	sunxi_mmc_exec_command(sdmmc_chipset_handle_t,
				      struct sdmmc_command *);
static void	sunxi_mmc_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	sunxi_mmc_card_intr_ack(sdmmc_chipset_handle_t);

static struct sdmmc_chip_functions sunxi_mmc_chip_functions = {
	.host_reset = sunxi_mmc_host_reset,
	.host_ocr = sunxi_mmc_host_ocr,
	.host_maxblklen = sunxi_mmc_host_maxblklen,
	.card_detect = sunxi_mmc_card_detect,
	.write_protect = sunxi_mmc_write_protect,
	.bus_power = sunxi_mmc_bus_power,
	.bus_clock_ddr = sunxi_mmc_bus_clock,
	.bus_width = sunxi_mmc_bus_width,
	.bus_rod = sunxi_mmc_bus_rod,
	.signal_voltage = sunxi_mmc_signal_voltage,
	.exec_command = sunxi_mmc_exec_command,
	.card_enable_intr = sunxi_mmc_card_enable_intr,
	.card_intr_ack = sunxi_mmc_card_intr_ack,
};

struct sunxi_mmc_config {
	u_int idma_xferlen;
	u_int flags;
#define	SUNXI_MMC_FLAG_CALIB_REG	0x01
#define	SUNXI_MMC_FLAG_NEW_TIMINGS	0x02
#define	SUNXI_MMC_FLAG_MASK_DATA0	0x04
	const struct sunxi_mmc_delay *delays;
	uint32_t dma_ftrglevel;
};

struct sunxi_mmc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	int sc_phandle;

	void *sc_ih;
	kmutex_t sc_intr_lock;
	kcondvar_t sc_intr_cv;
	kcondvar_t sc_idst_cv;

	int sc_mmc_width;
	int sc_mmc_present;

	device_t sc_sdmmc_dev;

	struct sunxi_mmc_config *sc_config;

	bus_dma_segment_t sc_idma_segs[1];
	int sc_idma_nsegs;
	bus_size_t sc_idma_size;
	bus_dmamap_t sc_idma_map;
	int sc_idma_ndesc;
	void *sc_idma_desc;

	bus_dmamap_t sc_dmabounce_map;
	void *sc_dmabounce_buf;
	size_t sc_dmabounce_buflen;

	uint32_t sc_intr_rint;
	uint32_t sc_idma_idst;

	struct clk *sc_clk_ahb;
	struct clk *sc_clk_mmc;
	struct clk *sc_clk_output;
	struct clk *sc_clk_sample;

	struct fdtbus_reset *sc_rst_ahb;

	struct fdtbus_gpio_pin *sc_gpio_cd;
	int sc_gpio_cd_inverted;
	struct fdtbus_gpio_pin *sc_gpio_wp;
	int sc_gpio_wp_inverted;

	struct fdtbus_regulator *sc_reg_vqmmc;

	struct fdtbus_mmc_pwrseq *sc_pwrseq;

	bool sc_non_removable;
	bool sc_broken_cd;
};

CFATTACH_DECL_NEW(sunxi_mmc, sizeof(struct sunxi_mmc_softc),
	sunxi_mmc_match, sunxi_mmc_attach, NULL, NULL);

#define MMC_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define MMC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static const struct sunxi_mmc_config sun4i_a10_mmc_config = {
	.idma_xferlen = 0x2000,
	.dma_ftrglevel = 0x20070008,
	.delays = NULL,
	.flags = 0,
};

static const struct sunxi_mmc_config sun5i_a13_mmc_config = {
	.idma_xferlen = 0x10000,
	.dma_ftrglevel = 0x20070008,
	.delays = NULL,
	.flags = 0,
};

static const struct sunxi_mmc_config sun7i_a20_mmc_config = {
	.idma_xferlen = 0x2000,
	.dma_ftrglevel = 0x20070008,
	.delays = sun7i_mmc_delays,
	.flags = 0,
};

static const struct sunxi_mmc_config sun8i_a83t_emmc_config = {
	.idma_xferlen = 0x10000,
	.dma_ftrglevel = 0x20070008,
	.delays = NULL,
	.flags = SUNXI_MMC_FLAG_NEW_TIMINGS,
};

static const struct sunxi_mmc_config sun9i_a80_mmc_config = {
	.idma_xferlen = 0x10000,
	.dma_ftrglevel = 0x200f0010,
	.delays = sun9i_mmc_delays,
	.flags = 0,
};

static const struct sunxi_mmc_config sun50i_a64_mmc_config = {
	.idma_xferlen = 0x10000,
	.dma_ftrglevel = 0x20070008,
	.delays = NULL,
	.flags = SUNXI_MMC_FLAG_CALIB_REG |
		 SUNXI_MMC_FLAG_NEW_TIMINGS |
		 SUNXI_MMC_FLAG_MASK_DATA0,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun4i-a10-mmc",	(uintptr_t)&sun4i_a10_mmc_config },
	{ "allwinner,sun5i-a13-mmc",	(uintptr_t)&sun5i_a13_mmc_config },
	{ "allwinner,sun7i-a20-mmc",	(uintptr_t)&sun7i_a20_mmc_config },
	{ "allwinner,sun8i-a83t-emmc",	(uintptr_t)&sun8i_a83t_emmc_config },
	{ "allwinner,sun9i-a80-mmc",	(uintptr_t)&sun9i_a80_mmc_config },
	{ "allwinner,sun50i-a64-mmc",	(uintptr_t)&sun50i_a64_mmc_config },
	{ NULL }
};

static int
sunxi_mmc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_mmc_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_mmc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_clk_ahb = fdtbus_clock_get(phandle, "ahb");
	sc->sc_clk_mmc = fdtbus_clock_get(phandle, "mmc");
	sc->sc_clk_output = fdtbus_clock_get(phandle, "output");
	sc->sc_clk_sample = fdtbus_clock_get(phandle, "sample");

#if notyet
	if (sc->sc_clk_ahb == NULL || sc->sc_clk_mmc == NULL ||
	    sc->sc_clk_output == NULL || sc->sc_clk_sample == NULL) {
#else
	if (sc->sc_clk_ahb == NULL || sc->sc_clk_mmc == NULL) {
#endif
		aprint_error(": couldn't get clocks\n");
		return;
	}

	sc->sc_rst_ahb = fdtbus_reset_get(phandle, "ahb");

	sc->sc_reg_vqmmc = fdtbus_regulator_acquire(phandle, "vqmmc-supply");

	sc->sc_pwrseq = fdtbus_mmc_pwrseq_get(phandle);

	if (clk_enable(sc->sc_clk_ahb) != 0 ||
	    clk_enable(sc->sc_clk_mmc) != 0) {
		aprint_error(": couldn't enable clocks\n");
		return;
	}

	if (sc->sc_rst_ahb != NULL) {
		if (fdtbus_reset_deassert(sc->sc_rst_ahb) != 0) {
			aprint_error(": couldn't de-assert resets\n");
			return;
		}
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_config = (void *)of_search_compatible(phandle, compat_data)->data;
	sc->sc_bst = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_intr_cv, "awinmmcirq");
	cv_init(&sc->sc_idst_cv, "awinmmcdma");

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": SD/MMC controller\n");

	sc->sc_gpio_cd = fdtbus_gpio_acquire(phandle, "cd-gpios",
	    GPIO_PIN_INPUT);
	sc->sc_gpio_wp = fdtbus_gpio_acquire(phandle, "wp-gpios",
	    GPIO_PIN_INPUT);

	sc->sc_gpio_cd_inverted = of_hasprop(phandle, "cd-inverted") ? 0 : 1;
	sc->sc_gpio_wp_inverted = of_hasprop(phandle, "wp-inverted") ? 0 : 1;

	sc->sc_non_removable = of_hasprop(phandle, "non-removable");
	sc->sc_broken_cd = of_hasprop(phandle, "broken-cd");

	if (sunxi_mmc_dmabounce_setup(sc) != 0 ||
	    sunxi_mmc_idma_setup(sc) != 0) {
		aprint_error_dev(self, "failed to setup DMA\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_BIO, FDT_INTR_MPSAFE,
	    sunxi_mmc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	config_interrupts(self, sunxi_mmc_attach_i);
}

static int
sunxi_mmc_dmabounce_setup(struct sunxi_mmc_softc *sc)
{
	bus_dma_segment_t ds[1];
	int error, rseg;

	sc->sc_dmabounce_buflen = sunxi_mmc_host_maxblklen(sc);
	error = bus_dmamem_alloc(sc->sc_dmat, sc->sc_dmabounce_buflen, 0,
	    sc->sc_dmabounce_buflen, ds, 1, &rseg, BUS_DMA_WAITOK);
	if (error)
		return error;
	error = bus_dmamem_map(sc->sc_dmat, ds, 1, sc->sc_dmabounce_buflen,
	    &sc->sc_dmabounce_buf, BUS_DMA_WAITOK);
	if (error)
		goto free;
	error = bus_dmamap_create(sc->sc_dmat, sc->sc_dmabounce_buflen, 1,
	    sc->sc_dmabounce_buflen, 0, BUS_DMA_WAITOK, &sc->sc_dmabounce_map);
	if (error)
		goto unmap;
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmabounce_map,
	    sc->sc_dmabounce_buf, sc->sc_dmabounce_buflen, NULL,
	    BUS_DMA_WAITOK);
	if (error)
		goto destroy;
	return 0;

destroy:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmabounce_map);
unmap:
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_dmabounce_buf,
	    sc->sc_dmabounce_buflen);
free:
	bus_dmamem_free(sc->sc_dmat, ds, rseg);
	return error;
}

static int
sunxi_mmc_idma_setup(struct sunxi_mmc_softc *sc)
{
	int error;

	sc->sc_idma_ndesc = SUNXI_MMC_NDESC;
	sc->sc_idma_size = sizeof(struct sunxi_mmc_idma_descriptor) *
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

static int
sunxi_mmc_set_clock(struct sunxi_mmc_softc *sc, u_int freq, bool ddr)
{
	const struct sunxi_mmc_delay *delays;
	int error, timing;

	if (freq <= 400) {
		timing = SUNXI_MMC_TIMING_400K;
	} else if (freq <= 25000) {
		timing = SUNXI_MMC_TIMING_25M;
	} else if (freq <= 52000) {
		if (ddr) {
			timing = sc->sc_mmc_width == 8 ?
			    SUNXI_MMC_TIMING_50M_DDR_8BIT :
			    SUNXI_MMC_TIMING_50M_DDR;
		} else {
			timing = SUNXI_MMC_TIMING_50M;
		}
	} else
		return EINVAL;

	error = clk_set_rate(sc->sc_clk_mmc, (freq * 1000) << ddr);
	if (error != 0)
		return error;

	if (sc->sc_config->delays == NULL)
		return 0;

	delays = &sc->sc_config->delays[timing];

	if (sc->sc_clk_sample) {
		error = clk_set_rate(sc->sc_clk_sample, delays->sample_phase);
		if (error != 0)
			return error;
	}
	if (sc->sc_clk_output) {
		error = clk_set_rate(sc->sc_clk_output, delays->output_phase);
		if (error != 0)
			return error;
	}

	return 0;
}

static void
sunxi_mmc_attach_i(device_t self)
{
	struct sunxi_mmc_softc *sc = device_private(self);
	struct sdmmcbus_attach_args saa;
	uint32_t width;

	if (sc->sc_pwrseq)
		fdtbus_mmc_pwrseq_pre_power_on(sc->sc_pwrseq);

	sunxi_mmc_host_reset(sc);
	sunxi_mmc_bus_width(sc, 1);
	sunxi_mmc_set_clock(sc, 400, false);

	if (sc->sc_pwrseq)
		fdtbus_mmc_pwrseq_post_power_on(sc->sc_pwrseq);

	if (of_getprop_uint32(sc->sc_phandle, "bus-width", &width) != 0)
		width = 4;

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &sunxi_mmc_chip_functions;
	saa.saa_sch = sc;
	saa.saa_dmat = sc->sc_dmat;
	saa.saa_clkmin = 400;
	saa.saa_clkmax = 52000;
	saa.saa_caps = SMC_CAPS_DMA |
		       SMC_CAPS_MULTI_SEG_DMA |
		       SMC_CAPS_AUTO_STOP |
		       SMC_CAPS_SD_HIGHSPEED |
		       SMC_CAPS_MMC_HIGHSPEED |
		       SMC_CAPS_MMC_DDR52 |
		       SMC_CAPS_POLLING;
	if (width == 4)
		saa.saa_caps |= SMC_CAPS_4BIT_MODE;
	if (width == 8)
		saa.saa_caps |= SMC_CAPS_8BIT_MODE;

	if (sc->sc_gpio_cd)
		saa.saa_caps |= SMC_CAPS_POLL_CARD_DET;

	sc->sc_sdmmc_dev = config_found(self, &saa, NULL);
}

static int
sunxi_mmc_intr(void *priv)
{
	struct sunxi_mmc_softc *sc = priv;
	uint32_t idst, rint;

	mutex_enter(&sc->sc_intr_lock);
	idst = MMC_READ(sc, SUNXI_MMC_IDST);
	rint = MMC_READ(sc, SUNXI_MMC_RINT);
	if (!idst && !rint) {
		mutex_exit(&sc->sc_intr_lock);
		return 0;
	}
	MMC_WRITE(sc, SUNXI_MMC_IDST, idst);
	MMC_WRITE(sc, SUNXI_MMC_RINT, rint);

	DPRINTF(sc->sc_dev, "mmc intr idst=%08X rint=%08X\n",
	    idst, rint);

	if (idst != 0) {
		sc->sc_idma_idst |= idst;
		cv_broadcast(&sc->sc_idst_cv);
	}

	if ((rint & ~SUNXI_MMC_INT_SDIO_INT) != 0) {
		sc->sc_intr_rint |= (rint & ~SUNXI_MMC_INT_SDIO_INT);
		cv_broadcast(&sc->sc_intr_cv);
	}

	if ((rint & SUNXI_MMC_INT_SDIO_INT) != 0) {
		sdmmc_card_intr(sc->sc_sdmmc_dev);
	}

	mutex_exit(&sc->sc_intr_lock);

	return 1;
}

static int
sunxi_mmc_wait_rint(struct sunxi_mmc_softc *sc, uint32_t mask,
    int timeout, bool poll)
{
	int retry;
	int error;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	if (sc->sc_intr_rint & mask)
		return 0;

	if (poll)
		retry = timeout / hz * 1000;
	else
		retry = timeout / hz;

	while (retry > 0) {
		if (poll) {
			sc->sc_intr_rint |= MMC_READ(sc, SUNXI_MMC_RINT);
		} else {
			error = cv_timedwait(&sc->sc_intr_cv,
			    &sc->sc_intr_lock, hz);
			if (error && error != EWOULDBLOCK)
				return error;
		}
		if (sc->sc_intr_rint & mask)
			return 0;
		if (poll)
			delay(1000);	
		--retry;
	}

	return ETIMEDOUT;
}

static int
sunxi_mmc_host_reset(sdmmc_chipset_handle_t sch)
{
	struct sunxi_mmc_softc *sc = sch;
	int retry = 1000;

	DPRINTF(sc->sc_dev, "host reset\n");

	MMC_WRITE(sc, SUNXI_MMC_GCTRL,
	    MMC_READ(sc, SUNXI_MMC_GCTRL) | SUNXI_MMC_GCTRL_RESET);
	while (--retry > 0) {
		if (!(MMC_READ(sc, SUNXI_MMC_GCTRL) & SUNXI_MMC_GCTRL_RESET))
			break;
		delay(100);
	}

	MMC_WRITE(sc, SUNXI_MMC_TIMEOUT, 0xffffffff);

	MMC_WRITE(sc, SUNXI_MMC_IMASK,
	    SUNXI_MMC_INT_CMD_DONE | SUNXI_MMC_INT_ERROR |
	    SUNXI_MMC_INT_DATA_OVER | SUNXI_MMC_INT_AUTO_CMD_DONE);

	MMC_WRITE(sc, SUNXI_MMC_GCTRL,
	    MMC_READ(sc, SUNXI_MMC_GCTRL) | SUNXI_MMC_GCTRL_INTEN);

	return 0;
}

static uint32_t
sunxi_mmc_host_ocr(sdmmc_chipset_handle_t sch)
{
	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V | MMC_OCR_HCS;
}

static int
sunxi_mmc_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 8192;
}

static int
sunxi_mmc_card_detect(sdmmc_chipset_handle_t sch)
{
	struct sunxi_mmc_softc *sc = sch;

	if (sc->sc_non_removable || sc->sc_broken_cd) {
		/*
		 * Non-removable or broken card detect flag set in
		 * DT, assume always present
		 */
		return 1;
	} else if (sc->sc_gpio_cd != NULL) {
		/* Use card detect GPIO */
		int v = 0, i;
		for (i = 0; i < 5; i++) {
			v += (fdtbus_gpio_read(sc->sc_gpio_cd) ^
			    sc->sc_gpio_cd_inverted);
			delay(1000);
		}
		if (v == 5)
			sc->sc_mmc_present = 0;
		else if (v == 0)
			sc->sc_mmc_present = 1;
		return sc->sc_mmc_present;
	} else {
		/* Use CARD_PRESENT field of SD_STATUS register */
		const uint32_t present = MMC_READ(sc, SUNXI_MMC_STATUS) &
		    SUNXI_MMC_STATUS_CARD_PRESENT;
		return present != 0;
	}
}

static int
sunxi_mmc_write_protect(sdmmc_chipset_handle_t sch)
{
	struct sunxi_mmc_softc *sc = sch;

	if (sc->sc_gpio_wp == NULL) {
		return 0;	/* no write protect pin, assume rw */
	} else {
		return fdtbus_gpio_read(sc->sc_gpio_wp) ^
		    sc->sc_gpio_wp_inverted;
	}
}

static int
sunxi_mmc_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	return 0;
}

static int
sunxi_mmc_update_clock(struct sunxi_mmc_softc *sc)
{
	uint32_t cmd;
	int retry;

	DPRINTF(sc->sc_dev, "update clock\n");

	cmd = SUNXI_MMC_CMD_START |
	      SUNXI_MMC_CMD_UPCLK_ONLY |
	      SUNXI_MMC_CMD_WAIT_PRE_OVER;
	MMC_WRITE(sc, SUNXI_MMC_CMD, cmd);
	retry = 0xfffff;
	while (--retry > 0) {
		if (!(MMC_READ(sc, SUNXI_MMC_CMD) & SUNXI_MMC_CMD_START))
			break;
		delay(10);
	}

	if (retry == 0) {
		aprint_error_dev(sc->sc_dev, "timeout updating clock\n");
		DPRINTF(sc->sc_dev, "GCTRL: 0x%08x\n",
		    MMC_READ(sc, SUNXI_MMC_GCTRL));
		DPRINTF(sc->sc_dev, "CLKCR: 0x%08x\n",
		    MMC_READ(sc, SUNXI_MMC_CLKCR));
		DPRINTF(sc->sc_dev, "TIMEOUT: 0x%08x\n",
		    MMC_READ(sc, SUNXI_MMC_TIMEOUT));
		DPRINTF(sc->sc_dev, "WIDTH: 0x%08x\n",
		    MMC_READ(sc, SUNXI_MMC_WIDTH));
		DPRINTF(sc->sc_dev, "CMD: 0x%08x\n",
		    MMC_READ(sc, SUNXI_MMC_CMD));
		DPRINTF(sc->sc_dev, "MINT: 0x%08x\n",
		    MMC_READ(sc, SUNXI_MMC_MINT));
		DPRINTF(sc->sc_dev, "RINT: 0x%08x\n",
		    MMC_READ(sc, SUNXI_MMC_RINT));
		DPRINTF(sc->sc_dev, "STATUS: 0x%08x\n",
		    MMC_READ(sc, SUNXI_MMC_STATUS));
		return ETIMEDOUT;
	}

	return 0;
}

static int
sunxi_mmc_bus_clock(sdmmc_chipset_handle_t sch, int freq, bool ddr)
{
	struct sunxi_mmc_softc *sc = sch;
	uint32_t clkcr, gctrl, ntsr;
	const u_int flags = sc->sc_config->flags;

	clkcr = MMC_READ(sc, SUNXI_MMC_CLKCR);
	if (clkcr & SUNXI_MMC_CLKCR_CARDCLKON) {
		clkcr &= ~SUNXI_MMC_CLKCR_CARDCLKON;
		if (flags & SUNXI_MMC_CLKCR_MASK_DATA0)
			clkcr |= SUNXI_MMC_CLKCR_MASK_DATA0;
		MMC_WRITE(sc, SUNXI_MMC_CLKCR, clkcr);
		if (sunxi_mmc_update_clock(sc) != 0)
			return 1;
		if (flags & SUNXI_MMC_CLKCR_MASK_DATA0) {
			clkcr = MMC_READ(sc, SUNXI_MMC_CLKCR);
			clkcr &= ~SUNXI_MMC_CLKCR_MASK_DATA0;
			MMC_WRITE(sc, SUNXI_MMC_CLKCR, clkcr);
		}
	}

	if (freq) {

		clkcr &= ~SUNXI_MMC_CLKCR_DIV;
		clkcr |= __SHIFTIN(ddr, SUNXI_MMC_CLKCR_DIV);
		MMC_WRITE(sc, SUNXI_MMC_CLKCR, clkcr);

		if (flags & SUNXI_MMC_FLAG_NEW_TIMINGS) {
			ntsr = MMC_READ(sc, SUNXI_MMC_NTSR);
			ntsr |= SUNXI_MMC_NTSR_MODE_SELECT;
			MMC_WRITE(sc, SUNXI_MMC_NTSR, ntsr);
		}

		if (flags & SUNXI_MMC_FLAG_CALIB_REG)
			MMC_WRITE(sc, SUNXI_MMC_SAMP_DL, SUNXI_MMC_SAMP_DL_SW_EN);

		if (sunxi_mmc_update_clock(sc) != 0)
			return 1;

		gctrl = MMC_READ(sc, SUNXI_MMC_GCTRL);
		if (ddr)
			gctrl |= SUNXI_MMC_GCTRL_DDR_MODE;
		else
			gctrl &= ~SUNXI_MMC_GCTRL_DDR_MODE;
		MMC_WRITE(sc, SUNXI_MMC_GCTRL, gctrl);

		if (sunxi_mmc_set_clock(sc, freq, ddr) != 0)
			return 1;

		clkcr |= SUNXI_MMC_CLKCR_CARDCLKON;
		if (flags & SUNXI_MMC_CLKCR_MASK_DATA0)
			clkcr |= SUNXI_MMC_CLKCR_MASK_DATA0;
		MMC_WRITE(sc, SUNXI_MMC_CLKCR, clkcr);
		if (sunxi_mmc_update_clock(sc) != 0)
			return 1;
		if (flags & SUNXI_MMC_CLKCR_MASK_DATA0) {
			clkcr = MMC_READ(sc, SUNXI_MMC_CLKCR);
			clkcr &= ~SUNXI_MMC_CLKCR_MASK_DATA0;
			MMC_WRITE(sc, SUNXI_MMC_CLKCR, clkcr);
		}
	}

	return 0;
}

static int
sunxi_mmc_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct sunxi_mmc_softc *sc = sch;

	DPRINTF(sc->sc_dev, "width = %d\n", width);

	switch (width) {
	case 1:
		MMC_WRITE(sc, SUNXI_MMC_WIDTH, SUNXI_MMC_WIDTH_1);
		break;
	case 4:
		MMC_WRITE(sc, SUNXI_MMC_WIDTH, SUNXI_MMC_WIDTH_4);
		break;
	case 8:
		MMC_WRITE(sc, SUNXI_MMC_WIDTH, SUNXI_MMC_WIDTH_8);
		break;
	default:
		return 1;
	}

	sc->sc_mmc_width = width;
	
	return 0;
}

static int
sunxi_mmc_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	return -1;
}

static int
sunxi_mmc_signal_voltage(sdmmc_chipset_handle_t sch, int signal_voltage)
{
	struct sunxi_mmc_softc *sc = sch;
	u_int uvol;
	int error;

	if (sc->sc_reg_vqmmc == NULL)
		return 0;

	switch (signal_voltage) {
	case SDMMC_SIGNAL_VOLTAGE_330:
		uvol = 3300000;
		break;
	case SDMMC_SIGNAL_VOLTAGE_180:
		uvol = 1800000;
		break;
	default:
		return EINVAL;
	}

	error = fdtbus_regulator_set_voltage(sc->sc_reg_vqmmc, uvol, uvol);
	if (error != 0)
		return error;

	return fdtbus_regulator_enable(sc->sc_reg_vqmmc);
}

static int
sunxi_mmc_dma_prepare(struct sunxi_mmc_softc *sc, struct sdmmc_command *cmd)
{
	struct sunxi_mmc_idma_descriptor *dma = sc->sc_idma_desc;
	bus_addr_t desc_paddr = sc->sc_idma_map->dm_segs[0].ds_addr;
	bus_dmamap_t map;
	bus_size_t off;
	int desc, resid, seg;
	uint32_t val;

	/*
	 * If the command includes a dma map use it, otherwise we need to
	 * bounce. This can happen for SDIO IO_RW_EXTENDED (CMD53) commands.
	 */
	if (cmd->c_dmamap) {
		map = cmd->c_dmamap;
	} else {
		if (cmd->c_datalen > sc->sc_dmabounce_buflen)
			return E2BIG;
		map = sc->sc_dmabounce_map;

		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			memset(sc->sc_dmabounce_buf, 0, cmd->c_datalen);
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmabounce_map,
			    0, cmd->c_datalen, BUS_DMASYNC_PREREAD);
		} else {
			memcpy(sc->sc_dmabounce_buf, cmd->c_data,
			    cmd->c_datalen);
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmabounce_map,
			    0, cmd->c_datalen, BUS_DMASYNC_PREWRITE);
		}
	}

	desc = 0;
	for (seg = 0; seg < map->dm_nsegs; seg++) {
		bus_addr_t paddr = map->dm_segs[seg].ds_addr;
		bus_size_t len = map->dm_segs[seg].ds_len;
		resid = min(len, cmd->c_resid);
		off = 0;
		while (resid > 0) {
			if (desc == sc->sc_idma_ndesc)
				break;
			len = min(sc->sc_config->idma_xferlen, resid);
			dma[desc].dma_buf_size = htole32(len);
			dma[desc].dma_buf_addr = htole32(paddr + off);
			dma[desc].dma_config = htole32(SUNXI_MMC_IDMA_CONFIG_CH |
					       SUNXI_MMC_IDMA_CONFIG_OWN);
			cmd->c_resid -= len;
			resid -= len;
			off += len;
			if (desc == 0) {
				dma[desc].dma_config |= htole32(SUNXI_MMC_IDMA_CONFIG_FD);
			}
			if (cmd->c_resid == 0) {
				dma[desc].dma_config |= htole32(SUNXI_MMC_IDMA_CONFIG_LD);
				dma[desc].dma_config |= htole32(SUNXI_MMC_IDMA_CONFIG_ER);
				dma[desc].dma_next = 0;
			} else {
				dma[desc].dma_config |=
				    htole32(SUNXI_MMC_IDMA_CONFIG_DIC);
				dma[desc].dma_next = htole32(
				    desc_paddr + ((desc+1) *
				    sizeof(struct sunxi_mmc_idma_descriptor)));
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

	val = MMC_READ(sc, SUNXI_MMC_GCTRL);
	val |= SUNXI_MMC_GCTRL_DMAEN;
	val |= SUNXI_MMC_GCTRL_INTEN;
	MMC_WRITE(sc, SUNXI_MMC_GCTRL, val);
	val |= SUNXI_MMC_GCTRL_DMARESET;
	MMC_WRITE(sc, SUNXI_MMC_GCTRL, val);
	MMC_WRITE(sc, SUNXI_MMC_DMAC, SUNXI_MMC_DMAC_SOFTRESET);
	MMC_WRITE(sc, SUNXI_MMC_DMAC,
	    SUNXI_MMC_DMAC_IDMA_ON|SUNXI_MMC_DMAC_FIX_BURST);
	val = MMC_READ(sc, SUNXI_MMC_IDIE);
	val &= ~(SUNXI_MMC_IDST_RECEIVE_INT|SUNXI_MMC_IDST_TRANSMIT_INT);
	if (ISSET(cmd->c_flags, SCF_CMD_READ))
		val |= SUNXI_MMC_IDST_RECEIVE_INT;
	else
		val |= SUNXI_MMC_IDST_TRANSMIT_INT;
	MMC_WRITE(sc, SUNXI_MMC_IDIE, val);
	MMC_WRITE(sc, SUNXI_MMC_DLBA, desc_paddr);
	MMC_WRITE(sc, SUNXI_MMC_FTRGLEVEL, sc->sc_config->dma_ftrglevel);

	return 0;
}

static void
sunxi_mmc_dma_complete(struct sunxi_mmc_softc *sc, struct sdmmc_command *cmd)
{
	bus_dmamap_sync(sc->sc_dmat, sc->sc_idma_map, 0,
	    sc->sc_idma_size, BUS_DMASYNC_POSTWRITE);

	if (cmd->c_dmamap == NULL) {
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmabounce_map,
			    0, cmd->c_datalen, BUS_DMASYNC_POSTREAD);
			memcpy(cmd->c_data, sc->sc_dmabounce_buf,
			    cmd->c_datalen);
		} else {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmabounce_map,
			    0, cmd->c_datalen, BUS_DMASYNC_POSTWRITE);
		}
	}
}

static void
sunxi_mmc_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct sunxi_mmc_softc *sc = sch;
	uint32_t cmdval = SUNXI_MMC_CMD_START;
	const bool poll = (cmd->c_flags & SCF_POLL) != 0;
	int retry;

	DPRINTF(sc->sc_dev,
	    "opcode %d flags 0x%x data %p datalen %d blklen %d poll %d\n",
	    cmd->c_opcode, cmd->c_flags, cmd->c_data, cmd->c_datalen,
	    cmd->c_blklen, poll);

	mutex_enter(&sc->sc_intr_lock);

	if (cmd->c_opcode == 0)
		cmdval |= SUNXI_MMC_CMD_SEND_INIT_SEQ;
	if (cmd->c_flags & SCF_RSP_PRESENT)
		cmdval |= SUNXI_MMC_CMD_RSP_EXP;
	if (cmd->c_flags & SCF_RSP_136)
		cmdval |= SUNXI_MMC_CMD_LONG_RSP;
	if (cmd->c_flags & SCF_RSP_CRC)
		cmdval |= SUNXI_MMC_CMD_CHECK_RSP_CRC;

	if (cmd->c_datalen > 0) {
		unsigned int nblks;

		cmdval |= SUNXI_MMC_CMD_DATA_EXP | SUNXI_MMC_CMD_WAIT_PRE_OVER;
		if (!ISSET(cmd->c_flags, SCF_CMD_READ)) {
			cmdval |= SUNXI_MMC_CMD_WRITE;
		}

		nblks = cmd->c_datalen / cmd->c_blklen;
		if (nblks == 0 || (cmd->c_datalen % cmd->c_blklen) != 0)
			++nblks;

		if (nblks > 1) {
			cmdval |= SUNXI_MMC_CMD_SEND_AUTO_STOP;
		}

		MMC_WRITE(sc, SUNXI_MMC_BLKSZ, cmd->c_blklen);
		MMC_WRITE(sc, SUNXI_MMC_BYTECNT, nblks * cmd->c_blklen);
	}

	sc->sc_intr_rint = 0;

	MMC_WRITE(sc, SUNXI_MMC_A12A,
	    (cmdval & SUNXI_MMC_CMD_SEND_AUTO_STOP) ? 0 : 0xffff);

	MMC_WRITE(sc, SUNXI_MMC_ARG, cmd->c_arg);

	DPRINTF(sc->sc_dev, "cmdval = %08x\n", cmdval);

	if (cmd->c_datalen == 0) {
		MMC_WRITE(sc, SUNXI_MMC_CMD, cmdval | cmd->c_opcode);
	} else {
		cmd->c_resid = cmd->c_datalen;
		cmd->c_error = sunxi_mmc_dma_prepare(sc, cmd);
		MMC_WRITE(sc, SUNXI_MMC_CMD, cmdval | cmd->c_opcode);
		if (cmd->c_error == 0) {
			const uint32_t idst_mask =
			    SUNXI_MMC_IDST_ERROR | SUNXI_MMC_IDST_COMPLETE;
			retry = 10;
			while ((sc->sc_idma_idst & idst_mask) == 0) {
				if (retry-- == 0) {
					cmd->c_error = ETIMEDOUT;
					break;
				}
				cv_timedwait(&sc->sc_idst_cv,
				    &sc->sc_intr_lock, hz);
			}
		}
		sunxi_mmc_dma_complete(sc, cmd);
		if (sc->sc_idma_idst & SUNXI_MMC_IDST_ERROR) {
			cmd->c_error = EIO;
		} else if (!(sc->sc_idma_idst & SUNXI_MMC_IDST_COMPLETE)) {
			cmd->c_error = ETIMEDOUT;
		}
		if (cmd->c_error) {
			DPRINTF(sc->sc_dev,
			    "xfer failed, error %d\n", cmd->c_error);
			goto done;
		}
	}

	cmd->c_error = sunxi_mmc_wait_rint(sc,
	    SUNXI_MMC_INT_ERROR|SUNXI_MMC_INT_CMD_DONE, hz * 10, poll);
	if (cmd->c_error == 0 && (sc->sc_intr_rint & SUNXI_MMC_INT_ERROR)) {
		if (sc->sc_intr_rint & SUNXI_MMC_INT_RESP_TIMEOUT) {
			cmd->c_error = ETIMEDOUT;
		} else {
			cmd->c_error = EIO;
		}
	}
	if (cmd->c_error) {
		DPRINTF(sc->sc_dev,
		    "cmd failed, error %d\n", cmd->c_error);
		goto done;
	}
		
	if (cmd->c_datalen > 0) {
		cmd->c_error = sunxi_mmc_wait_rint(sc,
		    SUNXI_MMC_INT_ERROR|
		    SUNXI_MMC_INT_AUTO_CMD_DONE|
		    SUNXI_MMC_INT_DATA_OVER,
		    hz*10, poll);
		if (cmd->c_error == 0 &&
		    (sc->sc_intr_rint & SUNXI_MMC_INT_ERROR)) {
			cmd->c_error = ETIMEDOUT;
		}
		if (cmd->c_error) {
			DPRINTF(sc->sc_dev,
			    "data timeout, rint = %08x\n",
			    sc->sc_intr_rint);
			cmd->c_error = ETIMEDOUT;
			goto done;
		}
	}

	if (cmd->c_flags & SCF_RSP_PRESENT) {
		if (cmd->c_flags & SCF_RSP_136) {
			cmd->c_resp[0] = MMC_READ(sc, SUNXI_MMC_RESP0);
			cmd->c_resp[1] = MMC_READ(sc, SUNXI_MMC_RESP1);
			cmd->c_resp[2] = MMC_READ(sc, SUNXI_MMC_RESP2);
			cmd->c_resp[3] = MMC_READ(sc, SUNXI_MMC_RESP3);
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
			cmd->c_resp[0] = MMC_READ(sc, SUNXI_MMC_RESP0);
		}
	}

done:
	cmd->c_flags |= SCF_ITSDONE;
	mutex_exit(&sc->sc_intr_lock);

	if (cmd->c_error) {
		DPRINTF(sc->sc_dev, "i/o error %d\n", cmd->c_error);
		MMC_WRITE(sc, SUNXI_MMC_GCTRL,
		    MMC_READ(sc, SUNXI_MMC_GCTRL) |
		      SUNXI_MMC_GCTRL_DMARESET | SUNXI_MMC_GCTRL_FIFORESET);
		for (retry = 0; retry < 1000; retry++) {
			if (!(MMC_READ(sc, SUNXI_MMC_GCTRL) & SUNXI_MMC_GCTRL_RESET))
				break;
			delay(10);
		}
		sunxi_mmc_update_clock(sc);
	}

	MMC_WRITE(sc, SUNXI_MMC_GCTRL,
	    MMC_READ(sc, SUNXI_MMC_GCTRL) | SUNXI_MMC_GCTRL_FIFORESET);
}

static void
sunxi_mmc_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
	struct sunxi_mmc_softc *sc = sch;
	uint32_t imask;

	imask = MMC_READ(sc, SUNXI_MMC_IMASK);
	if (enable)
		imask |= SUNXI_MMC_INT_SDIO_INT;
	else
		imask &= ~SUNXI_MMC_INT_SDIO_INT;
	MMC_WRITE(sc, SUNXI_MMC_IMASK, imask);
}

static void
sunxi_mmc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	struct sunxi_mmc_softc *sc = sch;

	MMC_WRITE(sc, SUNXI_MMC_RINT, SUNXI_MMC_INT_SDIO_INT);
}
