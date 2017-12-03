/* $NetBSD: rockchip_board.c,v 1.14.18.2 2017/12/03 11:35:55 jdolecek Exp $ */

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

#include "opt_rockchip.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rockchip_board.c,v 1.14.18.2 2017/12/03 11:35:55 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <arm/bootconfig.h>
#include <arm/cpufunc.h>

#include <arm/rockchip/rockchip_reg.h>
#include <arm/rockchip/rockchip_crureg.h>
#include <arm/rockchip/rockchip_var.h>

bus_space_handle_t rockchip_core0_bsh;
bus_space_handle_t rockchip_core1_bsh;

void
rockchip_bootstrap(void)
{
	int error;

	error = bus_space_map(&armv7_generic_bs_tag, ROCKCHIP_CORE0_BASE,
	    ROCKCHIP_CORE0_SIZE, 0, &rockchip_core0_bsh);
	if (error)
		panic("%s: failed to map CORE0 registers: %d", __func__, error);

	error = bus_space_map(&armv7_generic_bs_tag, ROCKCHIP_CORE1_BASE,
	    ROCKCHIP_CORE1_SIZE, 0, &rockchip_core1_bsh);
	if (error)
		panic("%s: failed to map CORE1 registers: %d", __func__, error);
}

uint32_t
rockchip_chip_id(void)
{
	static uint32_t chip_id = 0;
	char *chipver;

	if (!chip_id) {
		if (get_bootconf_option(boot_args, "chipver",
		    BOOTOPT_TYPE_STRING, &chipver) == 0) {
			return 0;
		} else if (strncmp(chipver, "300A20111111V101", 16) == 0) {
			chip_id = ROCKCHIP_CHIP_ID_RK3066;
		} else if (strncmp(chipver, "310B20121130V100", 16) == 0) {
			chip_id = ROCKCHIP_CHIP_ID_RK3188;
		} else if (strncmp(chipver, "310B20130131V101", 16) == 0) {
			chip_id = ROCKCHIP_CHIP_ID_RK3188PLUS;
		}
	}

	return chip_id;
}

const char *
rockchip_chip_name(void)
{
	uint32_t chip_id = rockchip_chip_id();

	switch (chip_id) {
	case ROCKCHIP_CHIP_ID_RK3066: return "RK3066";
	case ROCKCHIP_CHIP_ID_RK3188: return "RK3188";
	case ROCKCHIP_CHIP_ID_RK3188PLUS: return "RK3188+";
	default: return "Rockchip";
	}
}

static void
rockchip_get_cru_bsh(bus_space_handle_t *pbsh)
{
	bus_space_subregion(&armv7_generic_bs_tag, rockchip_core1_bsh,
	    ROCKCHIP_CRU_OFFSET, ROCKCHIP_CRU_SIZE, pbsh);
}

static u_int
rockchip_pll_get_rate(bus_size_t con0_reg, bus_size_t con1_reg)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t pll_con0, pll_con1;
	uint32_t nr, nf, no;

	rockchip_get_cru_bsh(&bsh);

	pll_con0 = bus_space_read_4(bst, bsh, con0_reg);
	pll_con1 = bus_space_read_4(bst, bsh, con1_reg);

	nr = __SHIFTOUT(pll_con0, CRU_PLL_CON0_CLKR) + 1;

	switch (rockchip_chip_id()) {
	case ROCKCHIP_CHIP_ID_RK3066:
	case ROCKCHIP_CHIP_ID_RK3188PLUS:
		no = __SHIFTOUT(pll_con0, CRU_PLL_CON0_CLKOD) + 1;
		nf = __SHIFTOUT(pll_con1, CRU_PLL_CON1_CLKF) + 1;
		break;
	case ROCKCHIP_CHIP_ID_RK3188:
		no = __SHIFTOUT(pll_con0, RK3188_CRU_PLL_CON0_CLKOD) + 1;
		nf = __SHIFTOUT(pll_con1, RK3188_CRU_PLL_CON1_CLKF) + 1;
		break;
	default:
		return EINVAL;
	}

#ifdef ROCKCHIP_CLOCK_DEBUG
	printf("%s: %#x %#x: nr=%d no=%d nf=%d\n", __func__,
	    (unsigned int)con0_reg, (unsigned int)con1_reg,
	    nr, no, nf);
#endif

	return ((uint64_t)ROCKCHIP_REF_FREQ * nf) / (nr * no);
}

u_int
rockchip_gpll_get_rate(void)
{
	return rockchip_pll_get_rate(CRU_GPLL_CON0_REG, CRU_GPLL_CON1_REG);
}

u_int
rockchip_cpll_get_rate(void)
{
	return rockchip_pll_get_rate(CRU_CPLL_CON0_REG, CRU_CPLL_CON1_REG);
}

u_int
rockchip_dpll_get_rate(void)
{
	return rockchip_pll_get_rate(CRU_DPLL_CON0_REG, CRU_DPLL_CON1_REG);
}

u_int
rockchip_apll_get_rate(void)
{
	return rockchip_pll_get_rate(CRU_APLL_CON0_REG, CRU_APLL_CON1_REG);
}

u_int
rockchip_cpu_get_rate(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con0;
	u_int a9_core_div;

	rockchip_get_cru_bsh(&bsh);

	clksel_con0 = bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(0));
	switch (rockchip_chip_id()) {
	case ROCKCHIP_CHIP_ID_RK3066:
		a9_core_div = __SHIFTOUT(clksel_con0,
					 CRU_CLKSEL_CON0_A9_CORE_DIV_CON) + 1;
		break;
	case ROCKCHIP_CHIP_ID_RK3188:
	case ROCKCHIP_CHIP_ID_RK3188PLUS:
		a9_core_div = __SHIFTOUT(clksel_con0,
					 RK3188_CRU_CLKSEL_CON0_A9_CORE_DIV_CON) + 1;
		break;
	default:
		return EINVAL;
	}

#ifdef ROCKCHIP_CLOCK_DEBUG
	printf("%s: clksel_con0=%#x\n", __func__, clksel_con0);
#endif

	if (clksel_con0 & CRU_CLKSEL_CON0_CPU_CLK_PLL_SEL) {
		return rockchip_gpll_get_rate() / a9_core_div;
	} else {
		return rockchip_apll_get_rate() / a9_core_div;
	}
}

u_int
rockchip_a9periph_get_rate(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con0;
	u_int core_peri_div;

	rockchip_get_cru_bsh(&bsh);

	clksel_con0 = bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(0));
	core_peri_div = 1 << (__SHIFTOUT(clksel_con0,
					 CRU_CLKSEL_CON0_CORE_PERI_DIV_CON) + 1);

	return rockchip_cpu_get_rate() / core_peri_div;
}

u_int
rockchip_pclk_cpu_get_rate(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con1;
	u_int aclk_div, core_axi_div, pclk_div;

	rockchip_get_cru_bsh(&bsh);

	clksel_con1 = bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(1));
	switch (rockchip_chip_id()) {
	case ROCKCHIP_CHIP_ID_RK3066:
		aclk_div = __SHIFTOUT(clksel_con1,
				      CRU_CLKSEL_CON1_CPU_ACLK_DIV_CON);
		break;
	case ROCKCHIP_CHIP_ID_RK3188:
	case ROCKCHIP_CHIP_ID_RK3188PLUS:
		aclk_div = __SHIFTOUT(clksel_con1,
				      RK3188_CRU_CLKSEL_CON1_CPU_ACLK_DIV_CON);
		break;
	default:
		return EINVAL;
	}
	switch (aclk_div) {
	case 0: core_axi_div = 1; break;
	case 1: core_axi_div = 2; break;
	case 2: core_axi_div = 3; break;
	case 3: core_axi_div = 4; break;
	case 4: core_axi_div = 8; break;
	default: return EINVAL;
	}
	pclk_div = 1 << __SHIFTOUT(clksel_con1,
				   CRU_CLKSEL_CON1_CPU_PCLK_DIV_CON);

	return rockchip_cpu_get_rate() / (core_axi_div * pclk_div);
}

u_int
rockchip_ahb_get_rate(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con10;
	uint32_t hclk_div, aclk_div;
	u_int rate;

	rockchip_get_cru_bsh(&bsh);

	clksel_con10 = bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(10));
	switch (rockchip_chip_id()) {
	case ROCKCHIP_CHIP_ID_RK3066:
		if (clksel_con10 & CRU_CLKSEL_CON10_PERI_PLL_SEL) {
			rate = rockchip_cpll_get_rate();
		} else {
			rate = rockchip_gpll_get_rate();
		}
		break;
	case ROCKCHIP_CHIP_ID_RK3188:
	case ROCKCHIP_CHIP_ID_RK3188PLUS:
		if (clksel_con10 & CRU_CLKSEL_CON10_PERI_PLL_SEL) {
			rate = rockchip_gpll_get_rate();
		} else {
			rate = rockchip_cpll_get_rate();
		}
		break;
	default:
		return EINVAL;
	}
	aclk_div = __SHIFTOUT(clksel_con10,
			      CRU_CLKSEL_CON10_PERI_ACLK_DIV_CON) + 1;
	hclk_div = 1 << __SHIFTOUT(clksel_con10,
				   CRU_CLKSEL_CON10_PERI_HCLK_DIV_CON);

	if (hclk_div > 4)
		return EINVAL;

	return rate / (aclk_div * hclk_div);
}

u_int
rockchip_apb_get_rate(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con10;
	uint32_t pclk_div, aclk_div;
	u_int rate;

	rockchip_get_cru_bsh(&bsh);

	clksel_con10 = bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(10));

	switch (rockchip_chip_id()) {
	case ROCKCHIP_CHIP_ID_RK3066:
		if (clksel_con10 & CRU_CLKSEL_CON10_PERI_PLL_SEL) {
			rate = rockchip_cpll_get_rate();
		} else {
			rate = rockchip_gpll_get_rate();
		}
		break;
	case ROCKCHIP_CHIP_ID_RK3188:
	case ROCKCHIP_CHIP_ID_RK3188PLUS:
		if (clksel_con10 & CRU_CLKSEL_CON10_PERI_PLL_SEL) {
			rate = rockchip_gpll_get_rate();
		} else {
			rate = rockchip_cpll_get_rate();
		}
		break;
	default:
		return EINVAL;
	}
	aclk_div = __SHIFTOUT(clksel_con10,
			      CRU_CLKSEL_CON10_PERI_ACLK_DIV_CON) + 1;
	pclk_div = 1 << __SHIFTOUT(clksel_con10,
				   CRU_CLKSEL_CON10_PERI_PCLK_DIV_CON);

	return rate / (aclk_div * pclk_div);
}

u_int
rockchip_mmc0_get_rate(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con11;
	u_int mmc0_div;

	rockchip_get_cru_bsh(&bsh);

	clksel_con11 = bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(11));

	mmc0_div = __SHIFTOUT(clksel_con11,
			      CRU_CLKSEL_CON11_MMC0_DIV_CON) + 1;

	return rockchip_ahb_get_rate() / mmc0_div;
}

u_int
rockchip_mmc0_set_div(u_int div)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con11;

	if (div == 0 || div > 0x40)
		return EINVAL;

	rockchip_get_cru_bsh(&bsh);

	clksel_con11 = CRU_CLKSEL_CON11_MMC0_DIV_CON_MASK;
	clksel_con11 |= __SHIFTIN(div - 1, CRU_CLKSEL_CON11_MMC0_DIV_CON);

#ifdef ROCKCHIP_CLOCK_DEBUG
	const u_int old_rate = rockchip_mmc0_get_rate();
#endif

	bus_space_write_4(bst, bsh, CRU_CLKSEL_CON_REG(11), clksel_con11);

#ifdef ROCKCHIP_CLOCK_DEBUG
	const u_int new_rate = rockchip_mmc0_get_rate();

	printf("%s: update %u Hz -> %u Hz\n", __func__, old_rate, new_rate);

	const uint32_t clkgate2 = bus_space_read_4(bst, bsh,
	    CRU_CLKGATE_CON_REG(2));
	printf("%s: clkgate2 = %#x\n", __func__, clkgate2);
#endif

	return 0;
}

u_int
rockchip_i2c_get_rate(u_int port)
{
	if (port == 0 || port == 1) {
		return rockchip_pclk_cpu_get_rate();
	} else {
		return rockchip_apb_get_rate();
	}
}

u_int
rockchip_mac_get_rate(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con21;
	u_int mac_div;

	rockchip_get_cru_bsh(&bsh);

	clksel_con21 = bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(21));
	mac_div = __SHIFTOUT(clksel_con21,
			     CRU_CLKSEL_CON21_MAC_DIV_CON) + 1;

	if (clksel_con21 & CRU_CLKSEL_CON21_MAC_PLL_SEL) {
		return rockchip_dpll_get_rate() / mac_div;
	} else {
		return rockchip_gpll_get_rate() / mac_div;
	}
}

u_int
rockchip_mac_set_rate(u_int rate)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con21;
	u_int dpll_rate, gpll_rate;
	u_int div;

	rockchip_get_cru_bsh(&bsh);

	dpll_rate = rockchip_dpll_get_rate();
	gpll_rate = rockchip_gpll_get_rate();

	clksel_con21 = CRU_CLKSEL_CON21_MAC_DIV_CON_MASK |
		       CRU_CLKSEL_CON21_RMII_EXTCLK_SEL_MASK |
		       CRU_CLKSEL_CON21_MAC_PLL_SEL_MASK;
	if (dpll_rate % rate == 0) {
		clksel_con21 |= CRU_CLKSEL_CON21_MAC_PLL_SEL;
		div = howmany(dpll_rate, rate);
	} else {
		div = howmany(gpll_rate, rate);
	}
	clksel_con21 |= __SHIFTIN(div - 1, CRU_CLKSEL_CON21_MAC_DIV_CON);

#ifdef ROCKCHIP_CLOCK_DEBUG
	const u_int old_rate = rockchip_mac_get_rate();
#endif

	bus_space_write_4(bst, bsh, CRU_CLKSEL_CON_REG(21), clksel_con21);

#ifdef ROCKCHIP_CLOCK_DEBUG
	const u_int new_rate = rockchip_mac_get_rate();

	printf("%s: update %u Hz -> %u Hz\n", __func__, old_rate, new_rate);
#endif

	return 0;
}
