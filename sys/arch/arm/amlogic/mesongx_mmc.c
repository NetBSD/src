/* $NetBSD: mesongx_mmc.c,v 1.4 2019/03/16 12:52:47 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: mesongx_mmc.c,v 1.4 2019/03/16 12:52:47 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/bitops.h>
#include <sys/gpio.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmc_ioreg.h>

#include <dev/fdt/fdtvar.h>

#define	SD_EMMC_CLOCK			0x00
#define	 CLOCK_CFG_IRQ_SDIO_SLEEP		__BIT(25)
#define	 CLOCK_CFG_ALWAYS_ON			__BIT(24)
#define	 CLOCK_CFG_RX_DELAY			__BITS(23,20)
#define	 CLOCK_CFG_TX_DELAY			__BITS(19,16)
#define	 CLOCK_CFG_SRAM_PD			__BITS(15,14)
#define	 CLOCK_CFG_RX_PHASE			__BITS(13,12)
#define	 CLOCK_CFG_TX_PHASE			__BITS(11,10)
#define	 CLOCK_CFG_CO_PHASE			__BITS(9,8)
#define	 CLOCK_CFG_SRC				__BITS(7,6)
#define	 CLOCK_CFG_DIV				__BITS(5,0)
#define	SD_EMMC_DELAY			0x04
#define	SD_EMMC_ADJUST			0x08
#define	 ADJUST_ADJ_DELAY			__BITS(21,16)
#define	 ADJUST_CALI_RISE			__BIT(14)
#define	 ADJUST_ADJ_ENABLE			__BIT(13)
#define	 ADJUST_CALI_ENABLE			__BIT(12)
#define	 ADJUST_CALI_SEL			__BITS(11,8)
#define	SD_EMMC_CALOUT			0x10
#define	 CALOUT_CALI_SETUP			__BITS(15,8)
#define	 CALOUT_CALI_VLD			__BIT(7)
#define	 CALOUT_CALI_IDX			__BITS(5,0)
#define	SD_EMMC_START			0x40
#define	 START_DESC_ADDR			__BITS(31,2)
#define	 START_DESC_BUSY			__BIT(1)
#define	 START_DESC_INT				__BIT(0)
#define	SD_EMMC_CFG			0x44
#define	 CFG_IP_TXD_ADJ				__BITS(31,28)
#define	 CFG_ERR_ABORT				__BIT(27)
#define	 CFG_IRQ_DS				__BIT(26)
#define	 CFG_TXD_RETRY				__BIT(25)
#define	 CFG_TXD_ADD_ERR			__BIT(24)
#define	 CFG_AUTO_CLK				__BIT(23)
#define	 CFG_STOP_CLK				__BIT(22)
#define	 CFG_CMD_LOW				__BIT(21)
#define	 CFG_CHK_DS				__BIT(20)
#define	 CFG_IGNORE_OWNER			__BIT(19)
#define	 CFG_SDCLK_ALWAYS_ON			__BIT(18)
#define	 CFG_BLK_GAP_IP				__BIT(17)
#define	 CFG_OUT_FALL				__BIT(16)
#define	 CFG_RC_CC				__BITS(15,12)
#define	 CFG_RESP_TIMEOUT			__BIT(11,8)
#define	 CFG_BL_LEN				__BITS(7,4)
#define	 CFG_DC_UGT				__BIT(3)
#define	 CFG_DDR				__BIT(2)
#define	 CFG_BUS_WIDTH				__BITS(1,0)
#define	  CFG_BUS_WIDTH_1			0
#define	  CFG_BUS_WIDTH_4			1
#define	  CFG_BUS_WIDTH_8			2
#define	SD_EMMC_STATUS			0x48
#define	 STATUS_CORE_BUSY			__BIT(31)
#define	 STATUS_DESC_BUSY			__BIT(30)
#define	 STATUS_BUS_FSM				__BIT(29,26)
#define	 STATUS_DS				__BIT(25)
#define	 STATUS_CMD_I				__BIT(24)
#define	 STATUS_DAT_I				__BITS(23,16)
#define	 STATUS_IRQ_SDIO			__BIT(15)
#define	 STATUS_RESP_STATUS			__BIT(14)
#define	 STATUS_END_OF_CHAIN			__BIT(13)
#define	 STATUS_DESC_TIMEOUT			__BIT(12)
#define	 STATUS_RESP_TIMEOUT			__BIT(11)
#define	 STATUS_RESP_ERR			__BIT(10)
#define	 STATUS_DESC_ERR			__BIT(9)
#define	 STATUS_TXD_ERR				__BIT(8)
#define	 STATUS_RXD_ERR				__BITS(7,0)
#define	 STATUS_TIMEOUT				(STATUS_DESC_TIMEOUT | STATUS_RESP_TIMEOUT)
#define	 STATUS_ERROR				(STATUS_RESP_ERR | STATUS_DESC_ERR | STATUS_RXD_ERR | STATUS_TXD_ERR)
#define	SD_EMMC_IRQ_EN			0x4c
#define	 IRQ_EN_CFG_SECURE			__BIT(16)
#define	 IRQ_EN_IRQ_SDIO			__BIT(15)
#define	 IRQ_EN_RESP_STATUS			__BIT(14)
#define	 IRQ_EN_END_OF_CHAIN			__BIT(13)
#define	 IRQ_EN_DESC_TIMEOUT			__BIT(12)
#define	 IRQ_EN_RESP_TIMEOUT			__BIT(11)
#define	 IRQ_EN_RESP_ERR			__BIT(10)
#define	 IRQ_EN_DESC_ERR			__BIT(9)
#define	 IRQ_EN_TXD_ERR				__BIT(8)
#define	 IRQ_EN_RXD_ERR				__BITS(7,0)
#define	SD_EMMC_CMD_CFG			0x50
#define	SD_EMMC_CMD_ARG			0x54
#define	SD_EMMC_CMD_DAT			0x58
#define	SD_EMMC_CMD_RSP			0x5c
#define	SD_EMMC_CMD_RSP1		0x60
#define	SD_EMMC_CMD_RSP2		0x64
#define	SD_EMMC_CMD_RSP3		0x68

struct mesongx_mmc_desc {
	uint32_t		flags;
#define	MESONGX_MMC_FLAGS_OWNER		__BIT(31)
#define	MESONGX_MMC_FLAGS_ERROR		__BIT(30)
#define	MESONGX_MMC_FLAGS_CMD_INDEX	__BITS(29,24)
#define	MESONGX_MMC_FLAGS_DATA_NUM	__BIT(23)
#define	MESONGX_MMC_FLAGS_RESP_NUM	__BIT(22)
#define	MESONGX_MMC_FLAGS_RESP_128	__BIT(21)
#define	MESONGX_MMC_FLAGS_RESP_NOCRC	__BIT(20)
#define	MESONGX_MMC_FLAGS_DATA_WR	__BIT(19)
#define	MESONGX_MMC_FLAGS_DATA_IO	__BIT(18)
#define	MESONGX_MMC_FLAGS_NO_CMD	__BIT(17)
#define	MESONGX_MMC_FLAGS_NO_RESP	__BIT(16)
#define	MESONGX_MMC_FLAGS_TIMEOUT	__BITS(15,12)
#define	MESONGX_MMC_FLAGS_END_OF_CHAIN	__BIT(11)
#define	MESONGX_MMC_FLAGS_R1B		__BIT(10)
#define	MESONGX_MMC_FLAGS_BLOCK_MODE	__BIT(9)
#define	MESONGX_MMC_FLAGS_LENGTH	__BITS(8,0)
	uint32_t		arg;
	uint32_t		data;
#define	MESONGX_MMC_DATA_BIG_ENDIAN	__BIT(1)
#define	MESONGX_MMC_DATA_SRAM		__BIT(0)
	uint32_t		resp;
#define	MESONGX_MMC_RESP_SRAM		__BIT(0)
} __packed;

#define MESONGX_MMC_NDESC		256

struct mesongx_mmc_softc;

static int	mesongx_mmc_match(device_t, cfdata_t, void *);
static void	mesongx_mmc_attach(device_t, device_t, void *);
static void	mesongx_mmc_attach_i(device_t);

static int	mesongx_mmc_intr(void *);
static int	mesongx_mmc_dma_setup(struct mesongx_mmc_softc *);
static int	mesongx_mmc_dmabounce_setup(struct mesongx_mmc_softc *);

static int	mesongx_mmc_host_reset(sdmmc_chipset_handle_t);
static uint32_t	mesongx_mmc_host_ocr(sdmmc_chipset_handle_t);
static int	mesongx_mmc_host_maxblklen(sdmmc_chipset_handle_t);
static int	mesongx_mmc_card_detect(sdmmc_chipset_handle_t);
static int	mesongx_mmc_write_protect(sdmmc_chipset_handle_t);
static int	mesongx_mmc_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	mesongx_mmc_bus_clock(sdmmc_chipset_handle_t, int, bool);
static int	mesongx_mmc_bus_width(sdmmc_chipset_handle_t, int);
static int	mesongx_mmc_bus_rod(sdmmc_chipset_handle_t, int);
static int	mesongx_mmc_signal_voltage(sdmmc_chipset_handle_t, int);
static int	mesongx_mmc_execute_tuning(sdmmc_chipset_handle_t, int);
static void	mesongx_mmc_exec_command(sdmmc_chipset_handle_t,
				      struct sdmmc_command *);
static void	mesongx_mmc_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	mesongx_mmc_card_intr_ack(sdmmc_chipset_handle_t);

static struct sdmmc_chip_functions mesongx_mmc_chip_functions = {
	.host_reset = mesongx_mmc_host_reset,
	.host_ocr = mesongx_mmc_host_ocr,
	.host_maxblklen = mesongx_mmc_host_maxblklen,
	.card_detect = mesongx_mmc_card_detect,
	.write_protect = mesongx_mmc_write_protect,
	.bus_power = mesongx_mmc_bus_power,
	.bus_clock_ddr = mesongx_mmc_bus_clock,
	.bus_width = mesongx_mmc_bus_width,
	.bus_rod = mesongx_mmc_bus_rod,
	.signal_voltage = mesongx_mmc_signal_voltage,
	.execute_tuning = mesongx_mmc_execute_tuning,
	.exec_command = mesongx_mmc_exec_command,
	.card_enable_intr = mesongx_mmc_card_enable_intr,
	.card_intr_ack = mesongx_mmc_card_intr_ack,
};

struct mesongx_mmc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;
	int			sc_phandle;

	void			*sc_ih;
	kmutex_t		sc_intr_lock;
	kcondvar_t		sc_intr_cv;

	device_t		sc_sdmmc_dev;
	uint32_t		sc_host_ocr;

	struct sdmmc_command	*sc_cmd;

	bus_dma_segment_t	sc_desc_segs[1];
	int			sc_desc_nsegs;
	bus_size_t		sc_desc_size;
	bus_dmamap_t		sc_desc_map;
	int			sc_desc_ndesc;
	void			*sc_desc_desc;

	bus_dmamap_t		sc_dmabounce_map;
	void			*sc_dmabounce_buf;
	size_t			sc_dmabounce_buflen;

	struct clk		*sc_clk_core;
	struct clk		*sc_clk_clkin[2];

	struct fdtbus_reset	*sc_rst;

	struct fdtbus_gpio_pin	*sc_gpio_cd;
	int			sc_gpio_cd_inverted;
	struct fdtbus_gpio_pin	*sc_gpio_wp;
	int			sc_gpio_wp_inverted;

	struct fdtbus_regulator	*sc_reg_vmmc;
	struct fdtbus_regulator	*sc_reg_vqmmc;

	struct fdtbus_mmc_pwrseq *sc_pwrseq;

	u_int			sc_max_frequency;
	bool			sc_non_removable;
	bool			sc_broken_cd;
};

CFATTACH_DECL_NEW(mesongx_mmc, sizeof(struct mesongx_mmc_softc),
	mesongx_mmc_match, mesongx_mmc_attach, NULL, NULL);

#define MMC_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define MMC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static const struct of_compat_data compat_data[] = {
	{ "amlogic,meson-gx-mmc",	1 },
	{ "amlogic,meson-gxbb-mmc",	1 },
	{ NULL }
};

static int
mesongx_mmc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
mesongx_mmc_attach(device_t parent, device_t self, void *aux)
{
	struct mesongx_mmc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_clk_core = fdtbus_clock_get(phandle, "core");
	sc->sc_clk_clkin[0] = fdtbus_clock_get(phandle, "clkin0");
	sc->sc_clk_clkin[1] = fdtbus_clock_get(phandle, "clkin1");

	if (sc->sc_clk_core == NULL || sc->sc_clk_clkin[0] == NULL ||
	    sc->sc_clk_clkin[1] == NULL) {
		aprint_error(": couldn't get clocks\n");
		return;
	}

	sc->sc_rst = fdtbus_reset_get_index(phandle, 0);
	if (sc->sc_rst == NULL) {
		aprint_error(": couldn't get reset\n");
		return;
	}

	sc->sc_pwrseq = fdtbus_mmc_pwrseq_get(phandle);

	if (clk_enable(sc->sc_clk_core) != 0) {
		aprint_error(": couldn't enable core clock\n");
		return;
	}
	if (clk_enable(sc->sc_clk_clkin[0]) != 0 ||
	    clk_enable(sc->sc_clk_clkin[1]) != 0) {
		aprint_error(": couldn't enable clkin clocks\n");
		return;
	}

	if (fdtbus_reset_deassert(sc->sc_rst) != 0) {
		aprint_error(": couldn't de-assert reset\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_intr_cv, "gxmmcirq");

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": eMMC/SD/SDIO controller\n");

	sc->sc_reg_vmmc = fdtbus_regulator_acquire(phandle, "vmmc-supply");
	sc->sc_reg_vqmmc = fdtbus_regulator_acquire(phandle, "vqmmc-supply");

	sc->sc_gpio_cd = fdtbus_gpio_acquire(phandle, "cd-gpios",
	    GPIO_PIN_INPUT);
	sc->sc_gpio_wp = fdtbus_gpio_acquire(phandle, "wp-gpios",
	    GPIO_PIN_INPUT);

	sc->sc_gpio_cd_inverted = of_hasprop(phandle, "cd-inverted") ? 1 : 0;
	sc->sc_gpio_wp_inverted = of_hasprop(phandle, "wp-inverted") ? 1 : 0;

	sc->sc_non_removable = of_hasprop(phandle, "non-removable");
	sc->sc_broken_cd = of_hasprop(phandle, "broken-cd");

	if (of_getprop_uint32(phandle, "max-frequency", &sc->sc_max_frequency))
		sc->sc_max_frequency = 52000000;

	if (mesongx_mmc_dma_setup(sc) != 0 ||
	    mesongx_mmc_dmabounce_setup(sc) != 0) {
		aprint_error_dev(self, "failed to setup DMA\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_BIO, FDT_INTR_MPSAFE,
	    mesongx_mmc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	if (sc->sc_pwrseq)
		fdtbus_mmc_pwrseq_reset(sc->sc_pwrseq);

	config_interrupts(self, mesongx_mmc_attach_i);
}

static int
mesongx_mmc_dma_setup(struct mesongx_mmc_softc *sc)
{
	int error;

	sc->sc_desc_ndesc = MESONGX_MMC_NDESC;
	sc->sc_desc_size = sizeof(struct mesongx_mmc_desc) *
	    sc->sc_desc_ndesc;
	error = bus_dmamem_alloc(sc->sc_dmat, sc->sc_desc_size,
	    sizeof(struct mesongx_mmc_desc),
	    sc->sc_desc_size, sc->sc_desc_segs, 1,
	    &sc->sc_desc_nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;
	error = bus_dmamem_map(sc->sc_dmat, sc->sc_desc_segs,
	    sc->sc_desc_nsegs, sc->sc_desc_size,
	    &sc->sc_desc_desc, BUS_DMA_WAITOK);
	if (error)
		goto free;
	error = bus_dmamap_create(sc->sc_dmat, sc->sc_desc_size, 1,
	    sc->sc_desc_size, 0, BUS_DMA_WAITOK, &sc->sc_desc_map);
	if (error)
		goto unmap;
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_desc_map,
	    sc->sc_desc_desc, sc->sc_desc_size, NULL, BUS_DMA_WAITOK);
	if (error)
		goto destroy;
	return 0;

destroy:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_desc_map);
unmap:
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_desc_desc, sc->sc_desc_size);
free:
	bus_dmamem_free(sc->sc_dmat, sc->sc_desc_segs, sc->sc_desc_nsegs);
	return error;
}

static int
mesongx_mmc_dmabounce_setup(struct mesongx_mmc_softc *sc)
{
	bus_dma_segment_t ds[1];
	int error, rseg;

	sc->sc_dmabounce_buflen = MAXPHYS;
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
mesongx_mmc_set_clock(struct mesongx_mmc_softc *sc, u_int freq, bool ddr)
{
	int best_diff, best_sel, best_div, sel, div;
	uint32_t val;

	if (freq == 0)
		freq = SDMMC_SDCLK_400K;

	best_diff = INT_MAX;
	best_sel = 0;
	best_div = 0;

	const u_int target_rate = (freq * 1000) >> ddr;
	for (sel = 0; sel <= 1; sel++) {
		const u_int parent_rate = clk_get_rate(sc->sc_clk_clkin[sel]);
		for (div = 1; div <= 63; div++) {
			const u_int rate = parent_rate / div;
			if (rate > target_rate)
				continue;
			const int diff = target_rate - rate;
			if (diff < best_diff) {
				best_diff = diff;
				best_sel = sel;
				best_div = div;
			}
		}
	}

	if (best_diff == INT_MAX)
		return ERANGE;

	val = MMC_READ(sc, SD_EMMC_CLOCK);
	val |= CLOCK_CFG_ALWAYS_ON;
	val &= ~CLOCK_CFG_RX_PHASE;
	val |= __SHIFTIN(0, CLOCK_CFG_RX_PHASE);
	val &= ~CLOCK_CFG_TX_PHASE;
	val |= __SHIFTIN(2, CLOCK_CFG_TX_PHASE);
	val &= ~CLOCK_CFG_CO_PHASE;
	val |= __SHIFTIN(3, CLOCK_CFG_CO_PHASE);
	val &= ~CLOCK_CFG_SRC;
	val |= __SHIFTIN(best_sel, CLOCK_CFG_SRC);
	val &= ~CLOCK_CFG_DIV;
	val |= __SHIFTIN(best_div, CLOCK_CFG_DIV);
	MMC_WRITE(sc, SD_EMMC_CLOCK, val);

	return 0;
}

static void
mesongx_mmc_attach_i(device_t self)
{
	struct mesongx_mmc_softc * const sc = device_private(self);
	struct sdmmcbus_attach_args saa;
	uint32_t width;

	if (sc->sc_pwrseq)
		fdtbus_mmc_pwrseq_pre_power_on(sc->sc_pwrseq);

	mesongx_mmc_bus_clock(sc, SDMMC_SDCLK_400K, false);
	mesongx_mmc_host_reset(sc);
	mesongx_mmc_bus_width(sc, 1);

	if (sc->sc_pwrseq)
		fdtbus_mmc_pwrseq_post_power_on(sc->sc_pwrseq);

	if (of_getprop_uint32(sc->sc_phandle, "bus-width", &width) != 0)
		width = 4;

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &mesongx_mmc_chip_functions;
	saa.saa_sch = sc;
	saa.saa_dmat = sc->sc_dmat;
	saa.saa_clkmin = SDMMC_SDCLK_400K;
	saa.saa_clkmax = sc->sc_max_frequency / 1000;
	saa.saa_caps = SMC_CAPS_DMA;
#if notyet
	/* XXX causes init to die when using root on eMMC with ODROID-C2 */
	saa.saa_caps |= SMC_CAPS_MULTI_SEG_DMA;
#endif

	sc->sc_host_ocr = MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V;

	if (of_getprop_bool(sc->sc_phandle, "cap-sd-highspeed")) {
		saa.saa_caps |= SMC_CAPS_SD_HIGHSPEED;
		sc->sc_host_ocr |= MMC_OCR_HCS;
	}
	if (of_getprop_bool(sc->sc_phandle, "cap-mmc-highspeed"))
		saa.saa_caps |= SMC_CAPS_MMC_HIGHSPEED;

	if (of_getprop_bool(sc->sc_phandle, "mmc-ddr-1_8v")) {
		saa.saa_caps |= SMC_CAPS_MMC_DDR52;
		sc->sc_host_ocr |= MMC_OCR_1_65V_1_95V;
	}
	if (of_getprop_bool(sc->sc_phandle, "mmc-hs200-1_8v")) {
		saa.saa_caps |= SMC_CAPS_MMC_HS200;
		sc->sc_host_ocr |= MMC_OCR_1_65V_1_95V;
	}

	if (width == 4)
		saa.saa_caps |= SMC_CAPS_4BIT_MODE;
	if (width == 8)
		saa.saa_caps |= SMC_CAPS_8BIT_MODE;

	if (sc->sc_gpio_cd)
		saa.saa_caps |= SMC_CAPS_POLL_CARD_DET;

	sc->sc_sdmmc_dev = config_found(self, &saa, NULL);
}

static int
mesongx_mmc_intr(void *priv)
{
	struct mesongx_mmc_softc * const sc = priv;
	struct sdmmc_command *cmd;
	int rv = 0;

	mutex_enter(&sc->sc_intr_lock);

	const uint32_t irq_en = MMC_READ(sc, SD_EMMC_IRQ_EN);
	const uint32_t status = MMC_READ(sc, SD_EMMC_STATUS) & irq_en;

	if ((status & STATUS_IRQ_SDIO) != 0) {
		rv = 1;
		sdmmc_card_intr(sc->sc_sdmmc_dev);
	}

	cmd = sc->sc_cmd;
	if (cmd == NULL) {
		device_printf(sc->sc_dev, "WARNING: IRQ with no active command, status %#x\n", status);
		goto done;
	}

	if ((status & STATUS_TIMEOUT) != 0) {
		rv = 1;
		cmd->c_error = ETIMEDOUT;
		goto done;
	}

	if ((status & STATUS_ERROR) != 0) {
		rv = 1;
		cmd->c_error = EIO;
		goto done;
	}

	if ((status & STATUS_END_OF_CHAIN) != 0 && (cmd->c_flags & SCF_ITSDONE) == 0) {
		rv = 1;
		if ((cmd->c_flags & SCF_RSP_PRESENT) != 0) {
			if (cmd->c_flags & SCF_RSP_136) {
				cmd->c_resp[0] = MMC_READ(sc, SD_EMMC_CMD_RSP);
				cmd->c_resp[1] = MMC_READ(sc, SD_EMMC_CMD_RSP1);
				cmd->c_resp[2] = MMC_READ(sc, SD_EMMC_CMD_RSP2);
				cmd->c_resp[3] = MMC_READ(sc, SD_EMMC_CMD_RSP3);
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
				cmd->c_resp[0] = MMC_READ(sc, SD_EMMC_CMD_RSP);
			}
		}
		cmd->c_flags |= SCF_ITSDONE;
		cmd->c_error = 0;
		goto done;
	}

done:
	if (rv) {
		cv_broadcast(&sc->sc_intr_cv);
		MMC_WRITE(sc, SD_EMMC_STATUS, irq_en);
	}

	mutex_exit(&sc->sc_intr_lock);

	return rv;
}

static int
mesongx_mmc_host_reset(sdmmc_chipset_handle_t sch)
{
	struct mesongx_mmc_softc * const sc = sch;
	uint32_t val;

	MMC_WRITE(sc, SD_EMMC_START, 0);

	val = MMC_READ(sc, SD_EMMC_CFG);
	val &= ~CFG_RC_CC;
	val |= __SHIFTIN(ilog2(16), CFG_RC_CC);
	val |= CFG_SDCLK_ALWAYS_ON;
	MMC_WRITE(sc, SD_EMMC_CFG, val);

	return 0;
}

static uint32_t
mesongx_mmc_host_ocr(sdmmc_chipset_handle_t sch)
{
	struct mesongx_mmc_softc * const sc = sch;

	return sc->sc_host_ocr;
}

static int
mesongx_mmc_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 512;
}

static int
mesongx_mmc_card_detect(sdmmc_chipset_handle_t sch)
{
	struct mesongx_mmc_softc * const sc = sch;
	int val;

	if (sc->sc_non_removable || sc->sc_broken_cd) {
		/*
		 * Non-removable or broken card detect flag set in
		 * DT, assume always present
		 */
		return 1;
	} else if (sc->sc_gpio_cd != NULL) {
		val = fdtbus_gpio_read(sc->sc_gpio_cd);
		if (sc->sc_gpio_cd_inverted)
			val = !val;
		return val;
	} else {
		return 1;
	}
}

static int
mesongx_mmc_write_protect(sdmmc_chipset_handle_t sch)
{
	struct mesongx_mmc_softc * const sc = sch;
	int val;

	if (sc->sc_gpio_wp != NULL) {
		val = fdtbus_gpio_read(sc->sc_gpio_wp);
		if (sc->sc_gpio_wp_inverted)
			val = !val;
		return val;
	}

	return 0;
}

static int
mesongx_mmc_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	return 0;
}

static int
mesongx_mmc_bus_clock(sdmmc_chipset_handle_t sch, int freq, bool ddr)
{
	struct mesongx_mmc_softc * const sc = sch;
	uint32_t val;
	int error;

	error = mesongx_mmc_set_clock(sc, freq, ddr);
	if (error != 0)
		return error;

	val = MMC_READ(sc, SD_EMMC_CFG);
	if (ddr)
		val |= CFG_DDR; 
	else
		val &= ~CFG_DDR;
	MMC_WRITE(sc, SD_EMMC_CFG, val);

	return 0;
}

static int
mesongx_mmc_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct mesongx_mmc_softc *sc = sch;
	uint32_t val;

	val = MMC_READ(sc, SD_EMMC_CFG);
	val &= ~CFG_BUS_WIDTH;

	switch (width) {
	case 1:
		val |= __SHIFTIN(CFG_BUS_WIDTH_1, CFG_BUS_WIDTH);
		break;
	case 4:
		val |= __SHIFTIN(CFG_BUS_WIDTH_4, CFG_BUS_WIDTH);
		break;
	case 8:
		val |= __SHIFTIN(CFG_BUS_WIDTH_8, CFG_BUS_WIDTH);
		break;
	default:
		return EINVAL;
	}

	MMC_WRITE(sc, SD_EMMC_CFG, val);

	return 0;
}

static int
mesongx_mmc_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	return -1;
}

static int
mesongx_mmc_signal_voltage(sdmmc_chipset_handle_t sch, int signal_voltage)
{
	struct mesongx_mmc_softc *sc = sch;
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

	error = fdtbus_regulator_supports_voltage(sc->sc_reg_vqmmc, uvol, uvol);
	if (error != 0)
		return 0;

	error = fdtbus_regulator_set_voltage(sc->sc_reg_vqmmc, uvol, uvol);
	if (error != 0)
		return error;

	return fdtbus_regulator_enable(sc->sc_reg_vqmmc);
}

static int
mesongx_mmc_execute_tuning(sdmmc_chipset_handle_t sch, int timing)
{
	switch (timing) {
	case SDMMC_TIMING_MMC_HS200:
		break;
	default:
		return EINVAL;
	}

	return 0;
}

static int
mesongx_mmc_dma_prepare(struct mesongx_mmc_softc *sc, struct sdmmc_command *cmd, uint32_t cmdflags)
{
	struct mesongx_mmc_desc *dma = sc->sc_desc_desc;
	bus_dmamap_t map = cmd->c_dmamap;
	u_int xferlen, blen, resid;
	bus_size_t off;
	uint32_t flags;
	int desc, seg;

	if (cmd->c_blklen > 512) {
		device_printf(sc->sc_dev, "block length %d not supported\n", cmd->c_blklen);
		return EINVAL;
	}

	for (seg = 0; seg < map->dm_nsegs; seg++) {
		if (map->dm_segs[seg].ds_len % cmd->c_blklen != 0) {
			/* Force DMA bounce for unaligned transfers */
			map = NULL;
			break;
		}
	}

	if (map == NULL) {
		map = sc->sc_dmabounce_map;
		cmd->c_flags |= SCF_NEED_BOUNCE;

		if ((cmd->c_flags & SCF_CMD_READ) != 0) {
			memset(sc->sc_dmabounce_buf, 0, cmd->c_datalen);
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmabounce_map,
			    0, cmd->c_datalen, BUS_DMASYNC_PREREAD);
		} else {
			memcpy(sc->sc_dmabounce_buf, cmd->c_data, cmd->c_datalen);
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmabounce_map,
			    0, cmd->c_datalen, BUS_DMASYNC_PREWRITE);
		}
	}

	desc = 0;
	for (seg = 0; seg < map->dm_nsegs; seg++) {
		bus_addr_t paddr = map->dm_segs[seg].ds_addr;
		bus_size_t len = map->dm_segs[seg].ds_len;
		resid = uimin(len, cmd->c_resid);
		off = 0;
		while (resid > 0) {
			if (desc == sc->sc_desc_ndesc)
				break;

			flags = cmdflags;

			if (resid >= cmd->c_blklen) {
				xferlen = (resid / cmd->c_blklen) * cmd->c_blklen;
				blen = xferlen / cmd->c_blklen;
				flags |= MESONGX_MMC_FLAGS_BLOCK_MODE;
			} else {
				blen = xferlen = resid;
			}
			KASSERT(xferlen > 0);
			KASSERT(blen <= 512);

			flags |= __SHIFTIN(blen % 512, MESONGX_MMC_FLAGS_LENGTH);
			if (desc > 0)
				flags |= MESONGX_MMC_FLAGS_NO_CMD;
			if (cmd->c_resid == xferlen)
				flags |= MESONGX_MMC_FLAGS_END_OF_CHAIN;

			dma[desc].flags = htole32(flags);
			dma[desc].arg = htole32(cmd->c_arg);
			dma[desc].data = htole32(paddr + off);
			dma[desc].resp = 0;

			cmd->c_resid -= xferlen;
			resid -= xferlen;
			off += xferlen;

			if (cmd->c_resid == 0)
				break;

			++desc;
		}
	}
	if (desc == sc->sc_desc_ndesc) {
		device_printf(sc->sc_dev,
		    "not enough descriptors for %d byte transfer (%d segs)!\n",
		    cmd->c_datalen, map->dm_nsegs);
		return EIO;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_desc_map, 0,
	    sc->sc_desc_size, BUS_DMASYNC_PREWRITE);

	return 0;
}

static void
mesongx_mmc_dma_complete(struct mesongx_mmc_softc *sc, struct sdmmc_command *cmd)
{
	bus_dmamap_sync(sc->sc_dmat, sc->sc_desc_map, 0,
	    sc->sc_desc_size, BUS_DMASYNC_POSTWRITE);

	if ((cmd->c_flags & SCF_NEED_BOUNCE) != 0) {
		if ((cmd->c_flags & SCF_CMD_READ) != 0) {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmabounce_map,
			    0, cmd->c_datalen, BUS_DMASYNC_POSTREAD);
			memcpy(cmd->c_data, sc->sc_dmabounce_buf, cmd->c_datalen);
		} else {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmabounce_map,
			    0, cmd->c_datalen, BUS_DMASYNC_POSTWRITE);
		}
	}
}

static void
mesongx_mmc_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct mesongx_mmc_softc *sc = sch;
	uint32_t cmdflags, val;
	int error;

	const uint32_t irq_mask = IRQ_EN_RESP_STATUS |
				  IRQ_EN_END_OF_CHAIN |
				  IRQ_EN_DESC_TIMEOUT |
				  IRQ_EN_RESP_TIMEOUT |
				  IRQ_EN_RESP_ERR |
				  IRQ_EN_DESC_ERR |
				  IRQ_EN_TXD_ERR |
				  IRQ_EN_RXD_ERR;

	mutex_enter(&sc->sc_intr_lock);

	while (sc->sc_cmd != NULL)
		cv_wait(&sc->sc_intr_cv, &sc->sc_intr_lock);
	sc->sc_cmd = cmd;

	MMC_WRITE(sc, SD_EMMC_START, 0);
	MMC_WRITE(sc, SD_EMMC_STATUS, MMC_READ(sc, SD_EMMC_STATUS));

	val = MMC_READ(sc, SD_EMMC_IRQ_EN);
	MMC_WRITE(sc, SD_EMMC_IRQ_EN, val | irq_mask);

	cmdflags = MESONGX_MMC_FLAGS_OWNER;
	cmdflags |= __SHIFTIN(12, MESONGX_MMC_FLAGS_TIMEOUT);	/* 2^12 = 4096 ms timeout */
	cmdflags |= __SHIFTIN(cmd->c_opcode, MESONGX_MMC_FLAGS_CMD_INDEX);

	if ((cmd->c_flags & SCF_RSP_PRESENT) == 0) {
		cmdflags |= MESONGX_MMC_FLAGS_NO_RESP;
	} else {
		cmdflags |= MESONGX_MMC_FLAGS_RESP_NUM;
		if ((cmd->c_flags & SCF_RSP_136) != 0)
			cmdflags |= MESONGX_MMC_FLAGS_RESP_128;
		if ((cmd->c_flags & SCF_RSP_CRC) == 0)
			cmdflags |= MESONGX_MMC_FLAGS_RESP_NOCRC;
		if ((cmd->c_flags & SCF_RSP_MASK) == SCF_RSP_R1B)
			cmdflags |= MESONGX_MMC_FLAGS_R1B;
	}

	if (cmd->c_datalen > 0) {
		cmdflags |= MESONGX_MMC_FLAGS_DATA_IO;
		if ((cmd->c_flags & SCF_CMD_READ) == 0)
			cmdflags |= MESONGX_MMC_FLAGS_DATA_WR;

		val = MMC_READ(sc, SD_EMMC_CFG);
		val &= ~CFG_BL_LEN;
		val |= __SHIFTIN(ilog2(cmd->c_blklen), CFG_BL_LEN);
		MMC_WRITE(sc, SD_EMMC_CFG, val);

		cmd->c_resid = cmd->c_datalen;
		cmd->c_error = mesongx_mmc_dma_prepare(sc, cmd, cmdflags);
		if (cmd->c_error != 0)
			goto done;

		const bus_addr_t desc_paddr = sc->sc_desc_map->dm_segs[0].ds_addr;
		MMC_WRITE(sc, SD_EMMC_START, desc_paddr | START_DESC_BUSY);	/* starts transfer */
	} else {
		MMC_WRITE(sc, SD_EMMC_CMD_CFG, cmdflags | MESONGX_MMC_FLAGS_END_OF_CHAIN);
		MMC_WRITE(sc, SD_EMMC_CMD_DAT, 0);
		MMC_WRITE(sc, SD_EMMC_CMD_ARG, cmd->c_arg);			/* starts transfer */
	}

	struct bintime timeout = { .sec = 5, .frac = 0 };
	const struct bintime epsilon = { .sec = 1, .frac = 0 };

	while ((cmd->c_flags & SCF_ITSDONE) == 0 && cmd->c_error == 0) {
		error = cv_timedwaitbt(&sc->sc_intr_cv, &sc->sc_intr_lock, &timeout, &epsilon);
		if (error != 0) {	
			cmd->c_error = error;
			goto done;
		}
	}

	if (cmd->c_error == 0 && cmd->c_datalen > 0)
		mesongx_mmc_dma_complete(sc, cmd);

done:
	MMC_WRITE(sc, SD_EMMC_START, 0);

	val = MMC_READ(sc, SD_EMMC_IRQ_EN);
	MMC_WRITE(sc, SD_EMMC_IRQ_EN, val & ~irq_mask);

	sc->sc_cmd = NULL;
	cv_broadcast(&sc->sc_intr_cv);

#ifdef MESONGX_MMC_DEBUG
	if (cmd->c_error != 0) {
		for (u_int reg = 0x00; reg < 0x100; reg += 0x10) {
			device_printf(sc->sc_dev, "      %02x: %08x %08x %08x %08x\n", reg,
			    MMC_READ(sc, reg + 0),
			    MMC_READ(sc, reg + 4),
			    MMC_READ(sc, reg + 8),
			    MMC_READ(sc, reg + 12));
		}
	}
#endif

	mutex_exit(&sc->sc_intr_lock);
}

static void
mesongx_mmc_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
	struct mesongx_mmc_softc * const sc = sch;
	uint32_t val;

	mutex_enter(&sc->sc_intr_lock);

	val = MMC_READ(sc, SD_EMMC_IRQ_EN);
	MMC_WRITE(sc, SD_EMMC_IRQ_EN, val | IRQ_EN_IRQ_SDIO);

	mutex_exit(&sc->sc_intr_lock);
}

static void
mesongx_mmc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	struct mesongx_mmc_softc *sc = sch;

	MMC_WRITE(sc, SD_EMMC_STATUS, STATUS_IRQ_SDIO);
}
