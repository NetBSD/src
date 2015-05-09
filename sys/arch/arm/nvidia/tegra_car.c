/* $NetBSD: tegra_car.c,v 1.6 2015/05/09 18:56:51 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_car.c,v 1.6 2015/05/09 18:56:51 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_carreg.h>
#include <arm/nvidia/tegra_var.h>

static int	tegra_car_match(device_t, cfdata_t, void *);
static void	tegra_car_attach(device_t, device_t, void *);

struct tegra_car_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

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

	aprint_verbose_dev(self, "PLLX = %u Hz\n", tegra_car_pllx_rate());
	aprint_verbose_dev(self, "PLLC = %u Hz\n", tegra_car_pllc_rate());
	aprint_verbose_dev(self, "PLLU = %u Hz\n", tegra_car_pllu_rate());
	aprint_verbose_dev(self, "PLLP0 = %u Hz\n", tegra_car_pllp0_rate());
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

	rate = tegra_car_osc_rate();	
	const uint32_t base = bus_space_read_4(bst, bsh, base_reg);
	const u_int divm = __SHIFTOUT(base, divm_mask);
	const u_int divn = __SHIFTOUT(base, divn_mask);
	const u_int divp = __SHIFTOUT(base, divp_mask);

	rate = (uint64_t)tegra_car_osc_rate() * divn;

	return rate / (divm << divp);
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
		const u_int div = __SHIFTOUT(src, CAR_CLKSRC_UART_DIV) + 1;
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

	const u_int div = __SHIFTOUT(src, CAR_CLKSRC_SDMMC_DIV) + 1;

	return tegra_car_pllp0_rate() / div;
}

int
tegra_car_periph_sdmmc_set_div(u_int port, u_int div)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t src_reg, rst_reg, enb_reg;
	u_int dev_bit;
	uint32_t src;

	KASSERT(div > 0);

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

	/* update clk div */
	src = __SHIFTIN(CAR_CLKSRC_SDMMC_SRC_PLLP_OUT0,
			CAR_CLKSRC_SDMMC_SRC);
	src |= __SHIFTIN(div - 1, CAR_CLKSRC_SDMMC_DIV);
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
	const u_int stable_count = 0x33;
	const u_int active_dly_count = 0x09;
	const u_int xtal_freq_count = 0x7f;
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
