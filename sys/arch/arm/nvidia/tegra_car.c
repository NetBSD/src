/* $NetBSD: tegra_car.c,v 1.27 2015/10/17 21:16:09 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_car.c,v 1.27 2015/10/17 21:16:09 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/rndpool.h>
#include <sys/rndsource.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_carreg.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_var.h>

static int	tegra_car_match(device_t, cfdata_t, void *);
static void	tegra_car_attach(device_t, device_t, void *);

struct tegra_car_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	kmutex_t		sc_intr_lock;
	kmutex_t		sc_rnd_lock;
	u_int			sc_bytes_wanted;
	void			*sc_sih;
	krndsource_t		sc_rndsource;
};

static void	tegra_car_init(struct tegra_car_softc *);
static void	tegra_car_rnd_attach(device_t);
static void	tegra_car_rnd_intr(void *);
static void	tegra_car_rnd_callback(size_t, void *);

static struct tegra_car_softc *pmc_softc = NULL;

CFATTACH_DECL_NEW(tegra_car, sizeof(struct tegra_car_softc),
	tegra_car_match, tegra_car_attach, NULL, NULL);

static int
tegra_car_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_car_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_car_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;

	sc->sc_dev = self;
	sc->sc_bst = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	KASSERT(pmc_softc == NULL);
	pmc_softc = sc;

	aprint_naive("\n");
	aprint_normal(": CAR\n");

	tegra_car_init(sc);

	aprint_verbose_dev(self, "PLLX = %u Hz\n", tegra_car_pllx_rate());
	aprint_verbose_dev(self, "PLLC = %u Hz\n", tegra_car_pllc_rate());
	aprint_verbose_dev(self, "PLLE = %u Hz\n", tegra_car_plle_rate());
	aprint_verbose_dev(self, "PLLU = %u Hz\n", tegra_car_pllu_rate());
	aprint_verbose_dev(self, "PLLP0 = %u Hz\n", tegra_car_pllp0_rate());
	aprint_verbose_dev(self, "PLLD2 = %u Hz\n", tegra_car_plld2_rate());

	config_interrupts(self, tegra_car_rnd_attach);
}

static void
tegra_car_init(struct tegra_car_softc *sc)
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	tegra_reg_set_clear(bst, bsh, CAR_PLLD2_BASE_REG,
	    __SHIFTIN(1, CAR_PLLD2_BASE_MDIV) |
	    __SHIFTIN(99, CAR_PLLD2_BASE_NDIV) |
	    __SHIFTIN(1, CAR_PLLD2_BASE_PLDIV),
	    CAR_PLLD2_BASE_REF_SRC_SEL |
	    CAR_PLLD2_BASE_PLDIV | CAR_PLLD2_BASE_NDIV | CAR_PLLD2_BASE_MDIV);
}

static void
tegra_car_rnd_attach(device_t self)
{
	struct tegra_car_softc * const sc = device_private(self);

	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SERIAL);
	mutex_init(&sc->sc_rnd_lock, MUTEX_DEFAULT, IPL_SERIAL);
	sc->sc_bytes_wanted = 0;
	sc->sc_sih = softint_establish(SOFTINT_SERIAL|SOFTINT_MPSAFE,
	    tegra_car_rnd_intr, sc);
	if (sc->sc_sih == NULL) {
		aprint_error_dev(sc->sc_dev, "couldn't establish softint\n");
		return;
	}

	rndsource_setcb(&sc->sc_rndsource, tegra_car_rnd_callback, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(sc->sc_dev),
	    RND_TYPE_RNG, RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);
}

static void
tegra_car_rnd_intr(void *priv)
{
	struct tegra_car_softc * const sc = priv;
	uint16_t buf[512];
	uint32_t cnt;

	mutex_enter(&sc->sc_intr_lock);
	while (sc->sc_bytes_wanted) {
		const u_int nbytes = MIN(sc->sc_bytes_wanted, 1024);
		for (cnt = 0; cnt < sc->sc_bytes_wanted / 2; cnt++) {
			buf[cnt] = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
			    CAR_PLL_LFSR_REG) & 0xffff;
		}
		mutex_exit(&sc->sc_intr_lock);
		mutex_enter(&sc->sc_rnd_lock);
		rnd_add_data(&sc->sc_rndsource, buf, nbytes, nbytes * NBBY);
		mutex_exit(&sc->sc_rnd_lock);
		mutex_enter(&sc->sc_intr_lock);
		sc->sc_bytes_wanted -= MIN(sc->sc_bytes_wanted, nbytes);
	}
	explicit_memset(buf, 0, sizeof(buf));
	mutex_exit(&sc->sc_intr_lock);
}

static void
tegra_car_rnd_callback(size_t bytes_wanted, void *priv)
{
	struct tegra_car_softc * const sc = priv;

	mutex_enter(&sc->sc_intr_lock);
	if (sc->sc_bytes_wanted == 0) {
		softint_schedule(sc->sc_sih);
	}
	if (bytes_wanted > (UINT_MAX - sc->sc_bytes_wanted)) {
		sc->sc_bytes_wanted = UINT_MAX;
	} else {
		sc->sc_bytes_wanted += bytes_wanted;
	}
	mutex_exit(&sc->sc_intr_lock);
}

static void
tegra_car_get_bs(bus_space_tag_t *pbst, bus_space_handle_t *pbsh)
{
	if (pmc_softc) {
		*pbst = pmc_softc->sc_bst;
		*pbsh = pmc_softc->sc_bsh;
	} else {
		*pbst = &armv7_generic_bs_tag;
		bus_space_subregion(*pbst, tegra_ppsb_bsh,
		    TEGRA_CAR_OFFSET, TEGRA_CAR_SIZE, pbsh);
	}
}

u_int
tegra_car_osc_rate(void)
{
	return TEGRA_REF_FREQ;
}

static u_int
tegra_car_pll_rate(u_int base_reg, u_int divm_mask, u_int divn_mask,
    u_int divp_mask)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint64_t rate;

	tegra_car_get_bs(&bst, &bsh);

	const uint32_t base = bus_space_read_4(bst, bsh, base_reg);
	const u_int divm = __SHIFTOUT(base, divm_mask);
	const u_int divn = __SHIFTOUT(base, divn_mask);
	const u_int divp = __SHIFTOUT(base, divp_mask);

	rate = (uint64_t)tegra_car_osc_rate() * divn;

	return rate / (divm << divp);
}

void
tegra_car_pllx_set_rate(u_int divm, u_int divn, u_int divp)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint32_t base, bp;

	tegra_car_get_bs(&bst, &bsh);

	bp = bus_space_read_4(bst, bsh, CAR_CCLKG_BURST_POLICY_REG);
	bp &= ~CAR_CCLKG_BURST_POLICY_CPU_STATE;
	bp |= __SHIFTIN(CAR_CCLKG_BURST_POLICY_CPU_STATE_IDLE,
			CAR_CCLKG_BURST_POLICY_CPU_STATE);
	bp &= ~CAR_CCLKG_BURST_POLICY_CWAKEUP_IDLE_SOURCE;
	bp |= __SHIFTIN(CAR_CCLKG_BURST_POLICY_CWAKEUP_SOURCE_CLKM,
			CAR_CCLKG_BURST_POLICY_CWAKEUP_IDLE_SOURCE);
	bus_space_write_4(bst, bsh, CAR_CCLKG_BURST_POLICY_REG, bp);

	base = bus_space_read_4(bst, bsh, CAR_PLLX_BASE_REG);
	base &= ~CAR_PLLX_BASE_DIVM;
	base &= ~CAR_PLLX_BASE_DIVN;
	base &= ~CAR_PLLX_BASE_DIVP;
	base |= __SHIFTIN(divm, CAR_PLLX_BASE_DIVM);
	base |= __SHIFTIN(divn, CAR_PLLX_BASE_DIVN);
	base |= __SHIFTIN(divp, CAR_PLLX_BASE_DIVP);
	bus_space_write_4(bst, bsh, CAR_PLLX_BASE_REG, base);

	tegra_reg_set_clear(bst, bsh, CAR_PLLX_MISC_REG,
	    CAR_PLLX_MISC_LOCK_ENABLE, 0);
	do {
		delay(2);
		base = bus_space_read_4(bst, bsh, CAR_PLLX_BASE_REG);
	} while ((base & CAR_PLLX_BASE_LOCK) == 0);
	delay(100);

	bp &= ~CAR_CCLKG_BURST_POLICY_CPU_STATE;
	bp |= __SHIFTIN(CAR_CCLKG_BURST_POLICY_CPU_STATE_RUN,
			CAR_CCLKG_BURST_POLICY_CPU_STATE);
	bp &= ~CAR_CCLKG_BURST_POLICY_CWAKEUP_IDLE_SOURCE;
	bp |= __SHIFTIN(CAR_CCLKG_BURST_POLICY_CWAKEUP_SOURCE_PLLX_OUT0_LJ,
			CAR_CCLKG_BURST_POLICY_CWAKEUP_IDLE_SOURCE);
	bus_space_write_4(bst, bsh, CAR_CCLKG_BURST_POLICY_REG, bp);
}

u_int
tegra_car_pllx_rate(void)
{
	return tegra_car_pll_rate(CAR_PLLX_BASE_REG, CAR_PLLX_BASE_DIVM,
	    CAR_PLLX_BASE_DIVN, CAR_PLLX_BASE_DIVP);
}

u_int
tegra_car_pllc_rate(void)
{
	return tegra_car_pll_rate(CAR_PLLC_BASE_REG, CAR_PLLC_BASE_DIVM,
	    CAR_PLLC_BASE_DIVN, CAR_PLLC_BASE_DIVP);
}

u_int
tegra_car_plle_rate(void)
{
	return tegra_car_pll_rate(CAR_PLLE_BASE_REG, CAR_PLLE_BASE_DIVM,
	    CAR_PLLE_BASE_DIVN, CAR_PLLE_BASE_DIVP_CML);
}

u_int
tegra_car_pllu_rate(void)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint64_t rate;

	tegra_car_get_bs(&bst, &bsh);

	rate = tegra_car_osc_rate();	
	const uint32_t base = bus_space_read_4(bst, bsh, CAR_PLLU_BASE_REG);
	const u_int divm = __SHIFTOUT(base, CAR_PLLU_BASE_DIVM);
	const u_int divn = __SHIFTOUT(base, CAR_PLLU_BASE_DIVN);
	const u_int divp = __SHIFTOUT(base, CAR_PLLU_BASE_VCO_FREQ) ? 0 : 1;

	rate = (uint64_t)tegra_car_osc_rate() * divn;

	return rate / (divm << divp);
}

u_int
tegra_car_pllp0_rate(void)
{
	return tegra_car_pll_rate(CAR_PLLP_BASE_REG, CAR_PLLP_BASE_DIVM,
	    CAR_PLLP_BASE_DIVN, CAR_PLLP_BASE_DIVP);
}

u_int
tegra_car_plld2_rate(void)
{
	return tegra_car_pll_rate(CAR_PLLD2_BASE_REG, CAR_PLLD2_BASE_MDIV,
	    CAR_PLLD2_BASE_NDIV, CAR_PLLD2_BASE_PLDIV);
}

u_int
tegra_car_uart_rate(u_int port)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t src_reg;
	u_int src_rate;

	tegra_car_get_bs(&bst, &bsh);

	switch (port) {
	case 0: src_reg = CAR_CLKSRC_UARTA_REG; break;
	case 1: src_reg = CAR_CLKSRC_UARTB_REG; break;
	case 2: src_reg = CAR_CLKSRC_UARTC_REG; break;
	case 3: src_reg = CAR_CLKSRC_UARTD_REG; break;
	default: return 0;
	}

	const uint32_t src = bus_space_read_4(bst, bsh, src_reg);
	switch (__SHIFTOUT(src, CAR_CLKSRC_UART_SRC)) {
	case 0:
		src_rate = tegra_car_pllp0_rate();
		break;
	default:
		panic("%s: unsupported src %#x", __func__, src);
	}

	if (__SHIFTOUT(src, CAR_CLKSRC_UART_DIV_ENB)) {
		const u_int div = (__SHIFTOUT(src, CAR_CLKSRC_UART_DIV) / 2) + 1;
		return src_rate / div;
	} else {
		return src_rate;
	}
}

u_int
tegra_car_periph_sdmmc_rate(u_int port)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t src_reg;

	tegra_car_get_bs(&bst, &bsh);

	switch (port) {
	case 0:	src_reg = CAR_CLKSRC_SDMMC1_REG; break;
	case 1: src_reg = CAR_CLKSRC_SDMMC2_REG; break;
	case 2: src_reg = CAR_CLKSRC_SDMMC3_REG; break;
	case 3: src_reg = CAR_CLKSRC_SDMMC4_REG; break;
	default: return 0;
	}

	const uint32_t src = bus_space_read_4(bst, bsh, src_reg);

	const u_int div = __SHIFTOUT(src, CAR_CLKSRC_SDMMC_DIV) + 2;

	return (tegra_car_pllp0_rate() * 2) / div;
}

int
tegra_car_periph_sdmmc_set_rate(u_int port, u_int rate)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t src_reg, rst_reg, enb_reg;
	u_int dev_bit;
	uint32_t src;

	KASSERT(rate > 0);

	tegra_car_get_bs(&bst, &bsh);

	switch (port) {
	case 0:
		src_reg = CAR_CLKSRC_SDMMC1_REG;
		rst_reg = CAR_RST_DEV_L_SET_REG;
		enb_reg = CAR_CLK_ENB_L_SET_REG;
		dev_bit = CAR_DEV_L_SDMMC1;
		break;
	case 1:
		src_reg = CAR_CLKSRC_SDMMC2_REG;
		rst_reg = CAR_RST_DEV_L_SET_REG;
		enb_reg = CAR_CLK_ENB_L_SET_REG;
		dev_bit = CAR_DEV_L_SDMMC2;
		break;
	case 2:
		src_reg = CAR_CLKSRC_SDMMC3_REG;
		rst_reg = CAR_RST_DEV_U_SET_REG;
		enb_reg = CAR_CLK_ENB_U_SET_REG;
		dev_bit = CAR_DEV_U_SDMMC3;
		break;
	case 3:
		src_reg = CAR_CLKSRC_SDMMC4_REG;
		rst_reg = CAR_RST_DEV_L_SET_REG;
		enb_reg = CAR_CLK_ENB_L_SET_REG;
		dev_bit = CAR_DEV_L_SDMMC4;
		break;
	default: return EINVAL;
	}

	/* enter reset */
	bus_space_write_4(bst, bsh, rst_reg, dev_bit);
	/* enable clk */
	bus_space_write_4(bst, bsh, enb_reg, dev_bit);

	const u_int div = howmany(tegra_car_pllp0_rate() * 2, rate) - 2;

	/* update clk div */
	src = __SHIFTIN(CAR_CLKSRC_SDMMC_SRC_PLLP_OUT0,
			CAR_CLKSRC_SDMMC_SRC);
	src |= __SHIFTIN(div, CAR_CLKSRC_SDMMC_DIV);
	bus_space_write_4(bst, bsh, src_reg, src);

	/* leave reset */
	bus_space_write_4(bst, bsh, rst_reg+4, dev_bit);

	return 0;
}

int
tegra_car_periph_usb_enable(u_int port)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t rst_reg, enb_reg;
	uint32_t dev_bit;

	tegra_car_get_bs(&bst, &bsh);
	switch (port) {
	case 0:
		rst_reg = CAR_RST_DEV_L_SET_REG;
		enb_reg = CAR_CLK_ENB_L_SET_REG;
		dev_bit = CAR_DEV_L_USBD;
		break;
	case 1:
		rst_reg = CAR_RST_DEV_H_SET_REG;
		enb_reg = CAR_CLK_ENB_H_SET_REG;
		dev_bit = CAR_DEV_H_USB2;
		break;
	case 2:
		rst_reg = CAR_RST_DEV_H_SET_REG;
		enb_reg = CAR_CLK_ENB_H_SET_REG;
		dev_bit = CAR_DEV_H_USB3;
		break;
	default:
		return EINVAL;
	}

	/* enter reset */
	bus_space_write_4(bst, bsh, rst_reg, dev_bit);
	/* enable clk */
	bus_space_write_4(bst, bsh, enb_reg, dev_bit);

	/* leave reset */
	bus_space_write_4(bst, bsh, rst_reg+4, dev_bit);

	return 0;
}

void
tegra_car_utmip_init(void)
{
	const u_int enable_dly_count = 0x02;
	const u_int stable_count = 0x2f;
	const u_int active_dly_count = 0x04;
	const u_int xtal_freq_count = 0x76;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	tegra_car_get_bs(&bst, &bsh);

	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG2_REG,
	    __SHIFTIN(stable_count, CAR_UTMIP_PLL_CFG2_STABLE_COUNT) |
	    __SHIFTIN(active_dly_count, CAR_UTMIP_PLL_CFG2_ACTIVE_DLY_COUNT),
	    CAR_UTMIP_PLL_CFG2_STABLE_COUNT |
	    CAR_UTMIP_PLL_CFG2_ACTIVE_DLY_COUNT);

	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG1_REG,
	    __SHIFTIN(enable_dly_count, CAR_UTMIP_PLL_CFG1_ENABLE_DLY_COUNT) |
	    __SHIFTIN(xtal_freq_count, CAR_UTMIP_PLL_CFG1_XTAL_FREQ_COUNT),
	    CAR_UTMIP_PLL_CFG1_ENABLE_DLY_COUNT |
	    CAR_UTMIP_PLL_CFG1_XTAL_FREQ_COUNT);

	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG1_REG,
	    0,
	    CAR_UTMIP_PLL_CFG1_PLLU_POWERDOWN |
	    CAR_UTMIP_PLL_CFG1_PLL_ENABLE_POWERDOWN);
}

void
tegra_car_utmip_enable(u_int port)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint32_t bit = 0;

	tegra_car_get_bs(&bst, &bsh);

	switch (port) {
	case 0:	bit = CAR_UTMIP_PLL_CFG2_PD_SAMP_A_POWERDOWN; break;
	case 1:	bit = CAR_UTMIP_PLL_CFG2_PD_SAMP_B_POWERDOWN; break;
	case 2:	bit = CAR_UTMIP_PLL_CFG2_PD_SAMP_C_POWERDOWN; break;
	}

	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG2_REG, 0, bit);
}

void
tegra_car_periph_hda_enable(void)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	tegra_car_get_bs(&bst, &bsh);

	bus_space_write_4(bst, bsh, CAR_RST_DEV_V_SET_REG, CAR_DEV_V_HDA);
	bus_space_write_4(bst, bsh, CAR_CLK_ENB_V_SET_REG, CAR_DEV_V_HDA);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_V_SET_REG,
	    CAR_DEV_V_HDA2CODEC_2X);
	bus_space_write_4(bst, bsh, CAR_CLK_ENB_V_SET_REG,
	    CAR_DEV_V_HDA2CODEC_2X);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_W_SET_REG,
	    CAR_DEV_W_HDA2HDMICODEC);
	bus_space_write_4(bst, bsh, CAR_CLK_ENB_W_SET_REG,
	    CAR_DEV_W_HDA2HDMICODEC);

	/* configure HDA2CODEC_2X for 48 MHz */
	const u_int div = howmany(tegra_car_pllp0_rate() * 2, 48000000) - 2;
	bus_space_write_4(bst, bsh, CAR_CLKSRC_HDA2CODEC_2X_REG,
	    __SHIFTIN(CAR_CLKSRC_HDA2CODEC_2X_SRC_PLLP_OUT0,
		      CAR_CLKSRC_HDA2CODEC_2X_SRC) |
	    __SHIFTIN(div, CAR_CLKSRC_HDA2CODEC_2X_DIV));

	bus_space_write_4(bst, bsh, CAR_RST_DEV_V_CLR_REG, CAR_DEV_V_HDA);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_V_CLR_REG,
	    CAR_DEV_V_HDA2CODEC_2X);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_W_CLR_REG,
	    CAR_DEV_W_HDA2HDMICODEC);
}

void
tegra_car_periph_sata_enable(void)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	tegra_car_get_bs(&bst, &bsh);

	/* Assert resets */
	bus_space_write_4(bst, bsh, CAR_RST_DEV_V_SET_REG, CAR_DEV_V_SATA);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_W_SET_REG, CAR_DEV_W_SATACOLD);

	/* Disable software control of SATA PLL */
	tegra_reg_set_clear(bst, bsh, CAR_SATA_PLL_CFG0_REG,
	    0, CAR_SATA_PLL_CFG0_PADPLL_RESET_SWCTL);

	/* Set SATA_OOB clock source to PLLP, 204MHz */
	const u_int sataoob_div = 2;
	bus_space_write_4(bst, bsh, CAR_CLKSRC_SATA_OOB_REG,
	    __SHIFTIN(CAR_CLKSRC_SATA_OOB_SRC_PLLP_OUT0,
		      CAR_CLKSRC_SATA_OOB_SRC) |
	    __SHIFTIN((sataoob_div - 1) * 2, CAR_CLKSRC_SATA_OOB_DIV));

	/* Set SATA clock source to PLLP, 102MHz */
	const u_int sata_div = 4;
	bus_space_write_4(bst, bsh, CAR_CLKSRC_SATA_REG,
	    CAR_CLKSRC_SATA_AUX_CLK_ENB |
	    __SHIFTIN(CAR_CLKSRC_SATA_SRC_PLLP_OUT0,
		      CAR_CLKSRC_SATA_SRC) |
	    __SHIFTIN((sata_div - 1) * 2, CAR_CLKSRC_SATA_DIV));

	/* Ungate SAX partition in the PMC */
	tegra_pmc_power(PMC_PARTID_SAX, true);
	delay(20);

	/* Remove clamping from SAX partition in the PMC */
	tegra_pmc_remove_clamping(PMC_PARTID_SAX);
	delay(20);

	/* De-assert reset to SATA PADPLL */
	tegra_reg_set_clear(bst, bsh, CAR_SATA_PLL_CFG0_REG,
	    0, CAR_SATA_PLL_CFG0_PADPLL_RESET_OVERRIDE_VALUE);
	delay(15);

	/* Enable CML clock for SATA */
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_AUX_REG,
	    CAR_PLLE_AUX_CML1_OEN, 0);

	/* Turn on the clocks to SATA and de-assert resets */
	bus_space_write_4(bst, bsh, CAR_CLK_ENB_W_SET_REG, CAR_DEV_W_SATACOLD);
	bus_space_write_4(bst, bsh, CAR_CLK_ENB_V_SET_REG, CAR_DEV_V_SATA);
	bus_space_write_4(bst, bsh, CAR_CLK_ENB_V_SET_REG, CAR_DEV_V_SATA_OOB);

	bus_space_write_4(bst, bsh, CAR_RST_DEV_W_CLR_REG, CAR_DEV_W_SATACOLD);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_V_CLR_REG, CAR_DEV_V_SATA);
}

int
tegra_car_periph_i2c_enable(u_int port, u_int rate)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t rst_reg, enb_reg, clksrc_reg;
	uint32_t dev_bit;

	tegra_car_get_bs(&bst, &bsh);

	switch (port) {
	case 0:
		rst_reg = CAR_RST_DEV_L_SET_REG;
		enb_reg = CAR_CLK_ENB_L_SET_REG;
		dev_bit = CAR_DEV_L_I2C1;
		clksrc_reg = CAR_CLKSRC_I2C1_REG;
		break;
	case 1:
		rst_reg = CAR_RST_DEV_H_SET_REG;
		enb_reg = CAR_CLK_ENB_H_SET_REG;
		dev_bit = CAR_DEV_H_I2C2;
		clksrc_reg = CAR_CLKSRC_I2C2_REG;
		break;
	case 2:
		rst_reg = CAR_RST_DEV_U_SET_REG;
		enb_reg = CAR_CLK_ENB_U_SET_REG;
		dev_bit = CAR_DEV_U_I2C3;
		clksrc_reg = CAR_CLKSRC_I2C3_REG;
		break;
	case 3:
		rst_reg = CAR_RST_DEV_V_SET_REG;
		enb_reg = CAR_CLK_ENB_V_SET_REG;
		dev_bit = CAR_DEV_V_I2C4;
		clksrc_reg = CAR_CLKSRC_I2C4_REG;
		break;
	case 4:
		rst_reg = CAR_RST_DEV_H_SET_REG;
		enb_reg = CAR_CLK_ENB_H_SET_REG;
		dev_bit = CAR_DEV_H_I2C5;
		clksrc_reg = CAR_CLKSRC_I2C5_REG;
		break;
	case 5:
		rst_reg = CAR_RST_DEV_X_SET_REG;
		enb_reg = CAR_CLK_ENB_X_SET_REG;
		dev_bit = CAR_DEV_X_I2C6;
		clksrc_reg = CAR_CLKSRC_I2C6_REG;
		break;
	default:
		return EINVAL;
	}

	/* Enter reset, enable clock */
	bus_space_write_4(bst, bsh, rst_reg, dev_bit);
	bus_space_write_4(bst, bsh, enb_reg, dev_bit);

	/* Set clock source to PLLP */
	const u_int div = howmany(tegra_car_pllp0_rate() / 1000, rate / 1000);
	bus_space_write_4(bst, bsh, clksrc_reg,
	    __SHIFTIN(CAR_CLKSRC_I2C_SRC_PLLP_OUT0, CAR_CLKSRC_I2C_SRC) |
	    __SHIFTIN(div - 1, CAR_CLKSRC_I2C_DIV));

	/* Leave reset */
	bus_space_write_4(bst, bsh, rst_reg+4, dev_bit);

	return 0;
}

void
tegra_car_periph_cec_enable(void)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	tegra_car_get_bs(&bst, &bsh);

	bus_space_write_4(bst, bsh, CAR_CLK_ENB_W_SET_REG, CAR_DEV_W_CEC);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_W_CLR_REG, CAR_DEV_W_CEC);
}

void
tegra_car_hdmi_enable(u_int rate)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint32_t base;
	int retry = 10000;

	tegra_car_get_bs(&bst, &bsh);

	/* Enter reset, enable clock */
	bus_space_write_4(bst, bsh, CAR_RST_DEV_H_SET_REG, CAR_DEV_H_HDMI);
	bus_space_write_4(bst, bsh, CAR_CLK_ENB_H_SET_REG, CAR_DEV_H_HDMI);

	/* Change IDDQ from 1 to 0 */
	tegra_reg_set_clear(bst, bsh, CAR_PLLD2_BASE_REG,
	    0, CAR_PLLD2_BASE_IDDQ);
	delay(2);
	/* Enable lock */
	tegra_reg_set_clear(bst, bsh, CAR_PLLD2_MISC_REG,
	    CAR_PLLD2_MISC_LOCK_ENABLE, 0);
	/* Enable PLLD2 */
	tegra_reg_set_clear(bst, bsh, CAR_PLLD2_BASE_REG,
	    CAR_PLLD2_BASE_ENABLE, 0);

	/* Wait for lock */
	do {
		delay(2);
		base = bus_space_read_4(bst, bsh, CAR_PLLD2_BASE_REG);
	} while ((base & CAR_PLLD2_BASE_LOCK) == 0 && --retry > 0);
	delay(100);
	if (retry == 0) {
		printf("WARNING: timeout waiting for PLLD2 lock\n");
	}

	/* Set clock source to PLLD2 */
	const u_int div = (tegra_car_plld2_rate() * 2) / rate - 2;
	bus_space_write_4(bst, bsh, CAR_CLKSRC_HDMI_REG,
	    __SHIFTIN(CAR_CLKSRC_HDMI_SRC_PLLD2_OUT0, CAR_CLKSRC_HDMI_SRC) |
	    __SHIFTIN(div, CAR_CLKSRC_HDMI_DIV));

	/* Leave reset */
	bus_space_write_4(bst, bsh, CAR_RST_DEV_H_CLR_REG, CAR_DEV_H_HDMI);
}

int
tegra_car_dc_enable(u_int port)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t src_reg;
	uint32_t dev_bit;
	u_int partid;

	tegra_car_get_bs(&bst, &bsh);

	switch (port) {
	case 0:
		dev_bit = CAR_DEV_L_DISP1;
		src_reg = CAR_CLKSRC_DISP1_REG;
		partid = PMC_PARTID_DIS;
		break;
	case 1:
		dev_bit = CAR_DEV_L_DISP2;
		src_reg = CAR_CLKSRC_DISP2_REG;
		partid = PMC_PARTID_DISB;
		break;
	default:
		return EINVAL;
	}

	/* Enter reset, enable clock */
	bus_space_write_4(bst, bsh, CAR_RST_DEV_L_SET_REG, dev_bit);
	bus_space_write_4(bst, bsh, CAR_CLK_ENB_L_SET_REG, dev_bit);

	/* Turn on power to display partition */
	tegra_pmc_power(partid, true);
	tegra_pmc_remove_clamping(partid);

	/* Select PLLD2 for clock source */
	bus_space_write_4(bst, bsh, src_reg,
	    __SHIFTIN(CAR_CLKSRC_DISP_SRC_PLLD2_OUT0,
		      CAR_CLKSRC_DISP_SRC));

	/* Leave reset */
	bus_space_write_4(bst, bsh, CAR_RST_DEV_L_CLR_REG, dev_bit);

	return 0;
}

void
tegra_car_host1x_enable(void)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	tegra_car_get_bs(&bst, &bsh);

	/* Enter reset, enable clock */
	bus_space_write_4(bst, bsh, CAR_RST_DEV_L_SET_REG, CAR_DEV_L_HOST1X);
	bus_space_write_4(bst, bsh, CAR_CLK_ENB_L_SET_REG, CAR_DEV_L_HOST1X);

	/* Select PLLP for clock source, 408 MHz */
	bus_space_write_4(bst, bsh, CAR_CLKSRC_HOST1X_REG,
	    __SHIFTIN(CAR_CLKSRC_HOST1X_SRC_PLLP_OUT0,
		      CAR_CLKSRC_HOST1X_SRC) |
	    __SHIFTIN(0, CAR_CLKSRC_HOST1X_CLK_DIVISOR));

	delay(2);

	/* Leave reset */
	bus_space_write_4(bst, bsh, CAR_RST_DEV_L_CLR_REG, CAR_DEV_L_HOST1X);
}

void
tegra_car_wdt_enable(u_int timer, bool enable)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint32_t enable_bits;

	KASSERT(timer == 1 || timer == 2);

	tegra_car_get_bs(&bst, &bsh);

	enable_bits = enable ?
	    (CAR_RST_SOURCE_WDT_EN|CAR_RST_SOURCE_WDT_SYS_RST_EN) : 0;

	tegra_reg_set_clear(bst, bsh, CAR_RST_SOURCE_REG,
	    __SHIFTIN(timer - 1, CAR_RST_SOURCE_WDT_SEL) |
	    enable_bits,
	    CAR_RST_SOURCE_WDT_SYS_RST_EN |
	    CAR_RST_SOURCE_WDT_SEL |
	    CAR_RST_SOURCE_WDT_EN);
}

void
tegra_car_gpu_enable(void)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	tegra_car_get_bs(&bst, &bsh);

	/* Enter reset, enable clock */
	bus_space_write_4(bst, bsh, CAR_RST_DEV_X_SET_REG, CAR_DEV_X_GPU);
	bus_space_write_4(bst, bsh, CAR_CLK_ENB_X_SET_REG, CAR_DEV_X_GPU);

	/* Set PLLP_OUT5 to 204MHz */
	const u_int rate = 204000000;
	const u_int div = howmany(tegra_car_pllp0_rate() * 2, rate) - 2;
	tegra_reg_set_clear(bst, bsh, CAR_PLLP_OUTC_REG,
	    __SHIFTIN(div, CAR_PLLP_OUTC_OUT5_RATIO) |
	    CAR_PLLP_OUTC_OUT5_CLKEN,
	    CAR_PLLP_OUTC_OUT5_RATIO);
	delay(20);

	/* Remove clamping from 3D partition in the PMC */
	tegra_pmc_remove_clamping(PMC_PARTID_TD);
	delay(20);

	/* Leave reset */
	bus_space_write_4(bst, bsh, CAR_RST_DEV_X_CLR_REG, CAR_DEV_X_GPU);
}
