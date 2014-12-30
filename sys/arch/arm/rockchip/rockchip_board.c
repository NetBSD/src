/* $NetBSD: rockchip_board.c,v 1.6 2014/12/30 03:53:52 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: rockchip_board.c,v 1.6 2014/12/30 03:53:52 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <arm/bootconfig.h>

#include <arm/rockchip/rockchip_reg.h>
#include <arm/rockchip/rockchip_crureg.h>
#include <arm/rockchip/rockchip_var.h>

bus_space_handle_t rockchip_core0_bsh;
bus_space_handle_t rockchip_core1_bsh;

void
rockchip_bootstrap(void)
{
	int error;

	error = bus_space_map(&rockchip_bs_tag, ROCKCHIP_CORE0_BASE,
	    ROCKCHIP_CORE0_SIZE, 0, &rockchip_core0_bsh);
	if (error)
		panic("%s: failed to map CORE0 registers: %d", __func__, error);

	error = bus_space_map(&rockchip_bs_tag, ROCKCHIP_CORE1_BASE,
	    ROCKCHIP_CORE1_SIZE, 0, &rockchip_core1_bsh);
	if (error)
		panic("%s: failed to map CORE1 registers: %d", __func__, error);
}

bool
rockchip_is_chip(const char *chipver)
{
	const size_t chipver_len = 16;
	char *env_chipver;

	if (get_bootconf_option(boot_args, "chipver",
				BOOTOPT_TYPE_STRING, &env_chipver) == 0) {
		return false;
	}

	return strncmp(env_chipver, chipver, chipver_len) == 0;
}

static void
rockchip_get_cru_bsh(bus_space_handle_t *pbsh)
{
	bus_space_subregion(&rockchip_bs_tag, rockchip_core1_bsh,
	    ROCKCHIP_CRU_OFFSET, ROCKCHIP_CRU_SIZE, pbsh);
}

static u_int
rockchip_pll_get_rate(bus_size_t con0_reg, bus_size_t con1_reg)
{
	bus_space_tag_t bst = &rockchip_bs_tag;
	bus_space_handle_t bsh;
	uint32_t pll_con0, pll_con1;
	uint32_t nr, nf, no;

	rockchip_get_cru_bsh(&bsh);

	pll_con0 = bus_space_read_4(bst, bsh, con0_reg);
	pll_con1 = bus_space_read_4(bst, bsh, con1_reg);

	nr = __SHIFTOUT(pll_con0, CRU_PLL_CON0_CLKR) + 1;
	no = __SHIFTOUT(pll_con0, CRU_PLL_CON0_CLKOD) + 1;
	nf = __SHIFTOUT(pll_con1, CRU_PLL_CON1_CLKF) + 1;

#ifdef ROCKCHIP_CLOCK_DEBUG
	printf("%s: %#x %#x: nr=%d no=%d nf=%d\n", __func__,
	    (unsigned int)con0_reg, (unsigned int)con1_reg,
	    nr, no, nf);
#endif

	return (ROCKCHIP_REF_FREQ * nf) / (nr * no);
}

u_int
rockchip_gpll_get_rate(void)
{
	return rockchip_pll_get_rate(CRU_GPLL_CON0_REG, CRU_GPLL_CON1_REG);
}

u_int
rockchip_apll_get_rate(void)
{
	return rockchip_pll_get_rate(CRU_APLL_CON0_REG, CRU_APLL_CON1_REG);
}

static u_int
rk3188_apll_set_rate(u_int rate)
{
	bus_space_tag_t bst = &rockchip_bs_tag;
	bus_space_handle_t bsh;
	uint32_t apll_con0, apll_con1, clksel0_con, clksel1_con;
	u_int no, nr, nf, core_div, core_periph_div, core_axi_div,
	      aclk_div, hclk_div, pclk_div, ahb2apb_div;
	u_int cpu_aclk_div_con;

	rockchip_get_cru_bsh(&bsh);

#ifdef ROCKCHIP_CLOCK_DEBUG
	printf("%s: rate=%u\n", __func__, rate);
#endif

	switch (rate) {
	case 1608000000:
		nr = 1;
		nf = 67;
		no = 1;
		core_div = 1;
		core_periph_div = 8;
		core_axi_div = 4;
		aclk_div = 4;
		hclk_div = 2;
		pclk_div = 4;
		ahb2apb_div = 2;
		break;
	case 1008000000:
		nr = 1;
		nf = 42;
		no = 1;
		core_div = 1;
		core_periph_div = 8;
		core_axi_div = 3;
		aclk_div = 3;
		hclk_div = 2;
		pclk_div = 4;
		ahb2apb_div = 2;
		break;
	case 600000000:
		nr = 1;
		nf = 50;
		no = 2;
		core_div = 1;
		core_periph_div = 4;
		core_axi_div = 4;
		aclk_div = 3;
		hclk_div = 2;
		pclk_div = 4;
		ahb2apb_div = 2;
		break;
	default:
#ifdef ROCKCHIP_CLOCK_DEBUG
		printf("%s: unsupported rate %u\n", __func__, rate);
#endif
		return EINVAL;
	}

	apll_con0 = CRU_PLL_CON0_CLKR_MASK | CRU_PLL_CON0_CLKOD_MASK;
	apll_con0 |= __SHIFTIN(no - 1, CRU_PLL_CON0_CLKOD);
	apll_con0 |= __SHIFTIN(nr - 1, CRU_PLL_CON0_CLKR);

	apll_con1 = CRU_PLL_CON1_CLKF_MASK;
	apll_con1 |= __SHIFTIN(nf - 1, CRU_PLL_CON1_CLKF);

	clksel0_con = RK3188_CRU_CLKSEL_CON0_A9_CORE_DIV_CON_MASK |
		      CRU_CLKSEL_CON0_CORE_PERI_DIV_CON_MASK |
		      CRU_CLKSEL_CON0_A9_CORE_DIV_CON_MASK |
		      CRU_CLKSEL_CON0_CPU_CLK_PLL_SEL;
	clksel0_con |= __SHIFTIN(core_div - 1,
				 RK3188_CRU_CLKSEL_CON0_A9_CORE_DIV_CON);
	clksel0_con |= __SHIFTIN(ffs(core_periph_div) - 2,
				 CRU_CLKSEL_CON0_CORE_PERI_DIV_CON);
	clksel0_con |= __SHIFTIN(aclk_div - 1,
				 CRU_CLKSEL_CON0_A9_CORE_DIV_CON);

	clksel1_con = RK3188_CRU_CLKSEL_CON1_CPU_ACLK_DIV_CON_MASK |
		      CRU_CLKSEL_CON1_AHB2APB_PCLKEN_DIV_CON_MASK |
		      CRU_CLKSEL_CON1_CPU_PCLK_DIV_CON_MASK |
		      CRU_CLKSEL_CON1_CPU_HCLK_DIV_CON_MASK;
	
	switch (core_axi_div) {
	case 1:	cpu_aclk_div_con = 0; break;
	case 2: cpu_aclk_div_con = 1; break;
	case 3: cpu_aclk_div_con = 2; break;
	case 4: cpu_aclk_div_con = 3; break;
	case 8: cpu_aclk_div_con = 4; break;
	default: panic("bad core_axi_div");
	}
	clksel1_con |= __SHIFTIN(cpu_aclk_div_con,
				 RK3188_CRU_CLKSEL_CON1_CPU_ACLK_DIV_CON);
	clksel1_con |= __SHIFTIN(ffs(ahb2apb_div) - 1,
				 CRU_CLKSEL_CON1_AHB2APB_PCLKEN_DIV_CON);
	clksel1_con |= __SHIFTIN(ffs(hclk_div) - 1,
				 CRU_CLKSEL_CON1_CPU_HCLK_DIV_CON);
	clksel1_con |= __SHIFTIN(ffs(pclk_div) - 1,
				 CRU_CLKSEL_CON1_CPU_PCLK_DIV_CON);

#ifdef ROCKCHIP_CLOCK_DEBUG
	printf("before: APLL_CON0: %#x\n",
	    bus_space_read_4(bst, bsh, CRU_APLL_CON0_REG));
	printf("before: APLL_CON1: %#x\n",
	    bus_space_read_4(bst, bsh, CRU_APLL_CON1_REG));
	printf("before: CLKSEL0_CON: %#x\n",
	    bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(0)));
	printf("before: CLKSEL1_CON: %#x\n",
	    bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(1)));
#endif

	/* Change from normal to slow mode */
	bus_space_write_4(bst, bsh, CRU_MODE_CON_REG,
	    CRU_MODE_CON_APLL_WORK_MODE_MASK |
	    __SHIFTIN(CRU_MODE_CON_APLL_WORK_MODE_SLOW,
		      CRU_MODE_CON_APLL_WORK_MODE));

	/* Power down */
	bus_space_write_4(bst, bsh, CRU_APLL_CON3_REG,
	    CRU_PLL_CON3_POWER_DOWN_MASK | CRU_PLL_CON3_POWER_DOWN);

	/* Update APLL regs */
	bus_space_write_4(bst, bsh, CRU_APLL_CON0_REG, apll_con0);
	bus_space_write_4(bst, bsh, CRU_APLL_CON1_REG, apll_con1);

	/* Wait for PLL lock */
	for (volatile int i = 5000; i >= 0; i--)
		;

	/* Power up */
	bus_space_write_4(bst, bsh, CRU_APLL_CON3_REG,
	    CRU_PLL_CON3_POWER_DOWN_MASK);

	/* Update CLKSEL regs */
	bus_space_write_4(bst, bsh, CRU_CLKSEL_CON_REG(0), clksel0_con);
	bus_space_write_4(bst, bsh, CRU_CLKSEL_CON_REG(1), clksel1_con);

	for (volatile int i = 50000; i >= 0; i--)
		;

	/* Change from slow mode to normal mode */
	bus_space_write_4(bst, bsh, CRU_MODE_CON_REG,
	    CRU_MODE_CON_APLL_WORK_MODE_MASK |
	    __SHIFTIN(CRU_MODE_CON_APLL_WORK_MODE_NORMAL,
		      CRU_MODE_CON_APLL_WORK_MODE));

#ifdef ROCKCHIP_CLOCK_DEBUG
	printf("after: APLL_CON0: %#x\n",
	    bus_space_read_4(bst, bsh, CRU_APLL_CON0_REG));
	printf("after: APLL_CON1: %#x\n",
	    bus_space_read_4(bst, bsh, CRU_APLL_CON1_REG));
	printf("after: CLKSEL0_CON: %#x\n",
	    bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(0)));
	printf("after: CLKSEL1_CON: %#x\n",
	    bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(1)));
#endif

	return 0;
}

u_int
rockchip_apll_set_rate(u_int rate)
{
	if (rockchip_is_chip(ROCKCHIP_CHIPVER_RK3188) ||
	    rockchip_is_chip(ROCKCHIP_CHIPVER_RK3188PLUS)) {
		return rk3188_apll_set_rate(rate);
	}

	return ENODEV;
}

u_int
rockchip_cpu_get_rate(void)
{
	bus_space_tag_t bst = &rockchip_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con0;
	uint32_t a9_core_div_con;
	u_int rate;

	rockchip_get_cru_bsh(&bsh);

	clksel_con0 = bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(0));
	if (clksel_con0 & CRU_CLKSEL_CON0_CPU_CLK_PLL_SEL) {
		rate = rockchip_gpll_get_rate();
	} else {
		rate = rockchip_apll_get_rate();
	}

	if (rockchip_is_chip(ROCKCHIP_CHIPVER_RK3066)) {
		a9_core_div_con = __SHIFTOUT(clksel_con0,
				     CRU_CLKSEL_CON0_A9_CORE_DIV_CON);
	} else {
		a9_core_div_con = __SHIFTOUT(clksel_con0,
				     RK3188_CRU_CLKSEL_CON0_A9_CORE_DIV_CON);
	}

#ifdef ROCKCHIP_CLOCK_DEBUG
	printf("%s: clksel_con0=%#x\n", __func__, clksel_con0);
#endif

	return rate / (a9_core_div_con + 1);
}

u_int
rockchip_a9periph_get_rate(void)
{
	bus_space_tag_t bst = &rockchip_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con0;
	uint32_t core_peri_div_con;
	u_int rate;

	rockchip_get_cru_bsh(&bsh);

	clksel_con0 = bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(0));
	rate = rockchip_cpu_get_rate();
	core_peri_div_con = __SHIFTOUT(clksel_con0,
				       CRU_CLKSEL_CON0_CORE_PERI_DIV_CON);

	return rate / ((1 << core_peri_div_con) * 2);
}

u_int
rockchip_ahb_get_rate(void)
{
	bus_space_tag_t bst = &rockchip_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con10;
	uint32_t hclk_div, aclk_div;

	rockchip_get_cru_bsh(&bsh);

	clksel_con10 = bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(10));

	hclk_div = __SHIFTOUT(clksel_con10,
			      CRU_CLKSEL_CON10_PERI_HCLK_DIV_CON) + 1;
	aclk_div = 1 << __SHIFTOUT(clksel_con10,
				   CRU_CLKSEL_CON10_PERI_ACLK_DIV_CON);

	return rockchip_gpll_get_rate() / (hclk_div * aclk_div);
}

u_int
rockchip_mmc0_get_rate(void)
{
	bus_space_tag_t bst = &rockchip_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con11;
	uint32_t mmc0_div_con;

	rockchip_get_cru_bsh(&bsh);

	clksel_con11 = bus_space_read_4(bst, bsh, CRU_CLKSEL_CON_REG(11));

	mmc0_div_con = __SHIFTOUT(clksel_con11,
				  CRU_CLKSEL_CON11_MMC0_DIV_CON);

	return rockchip_ahb_get_rate() / (mmc0_div_con + 1);
}

u_int
rockchip_mmc0_set_div(u_int div)
{
	bus_space_tag_t bst = &rockchip_bs_tag;
	bus_space_handle_t bsh;
	uint32_t clksel_con11;

	if (div == 0 || div > 0x40)
		return EINVAL;

	rockchip_get_cru_bsh(&bsh);

	clksel_con11 = CRU_CLKSEL_CON11_MMC0_PLL_SEL_MASK |
		       CRU_CLKSEL_CON11_MMC0_DIV_CON_MASK;
	clksel_con11 |= CRU_CLKSEL_CON11_MMC0_PLL_SEL;	/* GPLL */
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
