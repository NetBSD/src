/*	$NetBSD: bcm53xx_board.c,v 1.15 2013/02/19 02:30:55 matt Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_broadcom.h"

#define	_ARM32_BUS_DMA_PRIVATE

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_board.c,v 1.15 2013/02/19 02:30:55 matt Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <prop/proplib.h>

#include <net/if.h>
#include <net/if_ether.h>

#define CRU_PRIVATE
#define DDR_PRIVATE
#define DMU_PRIVATE
#define ARMCORE_PRIVATE
#define SRAB_PRIVATE

#include <arm/cortex/a9tmr_var.h>
#include <arm/cortex/pl310_var.h>
#include <arm/mainbus/mainbus.h>

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

bus_space_tag_t bcm53xx_ioreg_bst = &bcmgen_bs_tag;
bus_space_handle_t bcm53xx_ioreg_bsh;
bus_space_tag_t bcm53xx_armcore_bst = &bcmgen_bs_tag;
bus_space_handle_t bcm53xx_armcore_bsh;

static struct cpu_softc cpu_softc;

struct arm32_dma_range bcm53xx_dma_ranges[] = {
	[0] = {
		.dr_sysbase = 0x80000000,
		.dr_busbase = 0x80000000,
		.dr_len = 0x10000000,
	}, [1] = {
		.dr_sysbase = 0x90000000,
		.dr_busbase = 0x90000000,
	},
};

struct arm32_bus_dma_tag bcm53xx_dma_tag = {
	._ranges = bcm53xx_dma_ranges,
	._nranges = __arraycount(bcm53xx_dma_ranges),
	_BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};

struct arm32_dma_range bcm53xx_coherent_dma_ranges[] = {
	[0] = {
		.dr_sysbase = 0x80000000,
		.dr_busbase = 0x80000000,
		.dr_len = 0x10000000,
		.dr_flags = _BUS_DMAMAP_COHERENT,
	}, [1] = {
		.dr_sysbase = 0x90000000,
		.dr_busbase = 0x90000000,
	},
};

struct arm32_bus_dma_tag bcm53xx_coherent_dma_tag = {
	._ranges = bcm53xx_coherent_dma_ranges,
	._nranges = __arraycount(bcm53xx_coherent_dma_ranges),
	_BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};

#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
struct arm32_bus_dma_tag bcm53xx_bounce_dma_tag = {
	._ranges = bcm53xx_coherent_dma_ranges,
	._nranges = 1,
	_BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};
#endif

#ifdef BCM53XX_CONSOLE_EARLY
#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>
#include <dev/cons.h>

static vaddr_t com_base;

static inline uint32_t
uart_read(bus_size_t o)
{
	return *(volatile uint8_t *)(com_base + o);
}

static inline void
uart_write(bus_size_t o, uint32_t v)
{
	*(volatile uint8_t *)(com_base + o) = v;
}

static int
bcm53xx_cngetc(dev_t dv)
{
        if ((uart_read(com_lsr) & LSR_RXRDY) == 0)
		return -1;

	return uart_read(com_data) & 0xff;
}

static void
bcm53xx_cnputc(dev_t dv, int c)
{
	int timo = 150000;

        while ((uart_read(com_lsr) & LSR_TXRDY) == 0 && --timo > 0)
		;

	uart_write(com_data, c);

	timo = 150000;
        while ((uart_read(com_lsr) & LSR_TSRE) == 0 && --timo > 0)
		;
}

static struct consdev bcm53xx_earlycons = {
	.cn_putc = bcm53xx_cnputc,
	.cn_getc = bcm53xx_cngetc,
	.cn_pollc = nullcnpollc,
};
#endif /* BCM53XX_CONSOLE_EARLY */

psize_t
bcm53xx_memprobe(void)
{
	bus_space_tag_t bst = bcm53xx_ioreg_bst;
	bus_space_handle_t bsh = bcm53xx_ioreg_bsh;

	/*
	 * First, let's read the magic DDR registers!
	 */
	const uint32_t v01 = bus_space_read_4(bst, bsh, DDR_BASE + DDR_CTL_01);
	const uint32_t v82 = bus_space_read_4(bst, bsh, DDR_BASE + DDR_CTL_82);
	const uint32_t v86 = bus_space_read_4(bst, bsh, DDR_BASE + DDR_CTL_86);
	const uint32_t v87 = bus_space_read_4(bst, bsh, DDR_BASE + DDR_CTL_87);

	/*
	 * Calculate chip parameters
	 * */
	const u_int rows = __SHIFTOUT(v01, CTL_01_MAX_ROW)
	    - __SHIFTOUT(v82, CTL_82_ROW_DIFF);
	const u_int cols = __SHIFTOUT(v01, CTL_01_MAX_COL)
	    - __SHIFTOUT(v82, CTL_82_COL_DIFF);
	const u_int banks_log2 = 3 - __SHIFTOUT(v82, CTL_82_BANK_DIFF);

	/*
	 * For each chip select, increase the chip count if if is enabled.
	 */
	const u_int max_chips = __SHIFTOUT(v01, CTL_01_MAX_CHIP_SEL);
	u_int cs_map = __SHIFTOUT(v86, CTL_86_CS_MAP);
	u_int chips = 0;

	for (u_int i = 0; cs_map != 0 && i < max_chips; i++, cs_map >>= 1) {
		chips += (cs_map & 1);
	}

	/* get log2(ddr width) */

	const u_int ddr_width_log2 = (v87 & CTL_87_REDUC) ? 1 : 2;

	/*
	 * Let's add up all the things that contribute to the size of a chip.
	 */
	const u_int chip_size_log2 = cols + rows + banks_log2 + ddr_width_log2;

	/*
	 * Now our memory size is simply the number of chip shifted by the
	 * log2(chip_size).
	 */
	return (psize_t) chips << chip_size_log2;
}

static inline uint32_t
bcm53xx_freq_calc(struct bcm53xx_clock_info *clk,
	uint32_t pdiv, uint32_t ndiv_int, uint32_t ndiv_frac)
{
	if (ndiv_frac == 0 && pdiv == 1)
		return ndiv_int * clk->clk_ref;

	uint64_t freq64 = ((uint64_t)ndiv_int << 30) + ndiv_frac;
	freq64 *= clk->clk_ref;
	if (pdiv > 1)
		freq64 /= pdiv;
	return (uint32_t) (freq64 >> 30);
}

static uint32_t
bcm53xx_value_wrap(uint32_t value, uint32_t mask)
{
	/*
	 * n is n except when n is 0 then n = mask + 1.
	 */
	return ((__SHIFTOUT(value, mask) - 1) &  __SHIFTOUT(mask, mask)) + 1;
}

static void
bcm53xx_genpll_clock_init(struct bcm53xx_clock_info *clk, uint32_t control5,
	uint32_t control6, uint32_t control7)
{
	const uint32_t pdiv = bcm53xx_value_wrap(control6,
	    GENPLL_CONTROL6_PDIV);
	const uint32_t ndiv_int = bcm53xx_value_wrap(control5,
	    GENPLL_CONTROL5_NDIV_INT);
	const uint32_t ndiv_frac = __SHIFTOUT(control5,
	    GENPLL_CONTROL5_NDIV_FRAC);

	clk->clk_genpll = bcm53xx_freq_calc(clk, pdiv, ndiv_int, ndiv_frac);

	const uint32_t ch0_mdiv = bcm53xx_value_wrap(control6,
	    GENPLL_CONTROL6_CH0_MDIV);
	const uint32_t ch1_mdiv = bcm53xx_value_wrap(control6,
	    GENPLL_CONTROL6_CH1_MDIV);
	const uint32_t ch2_mdiv = bcm53xx_value_wrap(control6,
	    GENPLL_CONTROL6_CH2_MDIV);
	const uint32_t ch3_mdiv = bcm53xx_value_wrap(control7,
	    GENPLL_CONTROL7_CH3_MDIV);

	clk->clk_mac = clk->clk_genpll / ch0_mdiv;	// GENPLL CH0
	clk->clk_robo = clk->clk_genpll / ch1_mdiv;	// GENPLL CH1
	clk->clk_usb2 = clk->clk_genpll / ch2_mdiv;	// GENPLL CH2
	clk->clk_iproc = clk->clk_genpll / ch3_mdiv;	// GENPLL CH3
}

static void
bcm53xx_lcpll_clock_init(struct bcm53xx_clock_info *clk, uint32_t control1,
	uint32_t control2)
{
	const uint32_t pdiv = bcm53xx_value_wrap(control1,
	    LCPLL_CONTROL1_PDIV);
	const uint32_t ndiv_int = bcm53xx_value_wrap(control1,
	    LCPLL_CONTROL1_NDIV_INT);
	const uint32_t ndiv_frac = __SHIFTOUT(control1,
	    LCPLL_CONTROL1_NDIV_FRAC);

	clk->clk_lcpll = bcm53xx_freq_calc(clk, pdiv, ndiv_int, ndiv_frac);

	const uint32_t ch0_mdiv = bcm53xx_value_wrap(control2,
	    LCPLL_CONTROL2_CH0_MDIV);
	const uint32_t ch1_mdiv = bcm53xx_value_wrap(control2,
	    LCPLL_CONTROL2_CH1_MDIV);
	const uint32_t ch2_mdiv = bcm53xx_value_wrap(control2,
	    LCPLL_CONTROL2_CH2_MDIV);
	const uint32_t ch3_mdiv = bcm53xx_value_wrap(control2,
	    LCPLL_CONTROL2_CH3_MDIV);

	clk->clk_pcie_ref = clk->clk_lcpll / ch0_mdiv;	// LCPLL CH0
	clk->clk_sdio = clk->clk_lcpll / ch1_mdiv;	// LCPLL CH1
	clk->clk_ddr_ref = clk->clk_lcpll / ch2_mdiv;	// LCPLL CH2
	clk->clk_axi = clk->clk_lcpll / ch3_mdiv;	// LCPLL CH3
}

static void
bcm53xx_usb_clock_init(struct bcm53xx_clock_info *clk, uint32_t usb2_control)
{
	const uint32_t pdiv = bcm53xx_value_wrap(usb2_control,
	    USB2_CONTROL_PDIV);
	const uint32_t ndiv = bcm53xx_value_wrap(usb2_control,
	    USB2_CONTROL_NDIV_INT);

	uint32_t usb_ref = (clk->clk_usb2 / pdiv) * ndiv; 
	if (usb_ref != USB2_REF_CLK) {
		/*
		 * USB Reference Clock isn't 1.92GHz.  So we need to modify
		 * USB2_CONTROL to produce it.
		 */
		uint32_t new_ndiv = (USB2_REF_CLK / clk->clk_usb2) * pdiv;
		usb2_control &= ~USB2_CONTROL_NDIV_INT;
		usb2_control |= __SHIFTIN(new_ndiv, USB2_CONTROL_NDIV_INT);

		// Allow Clocks to be modified
		bus_space_write_4(bcm53xx_ioreg_bst, bcm53xx_ioreg_bsh,
		    CRU_BASE + CRU_CLKSET_KEY, CRU_CLKSET_KEY_MAGIC);

		// Update USB2 clock generator
		bus_space_write_4(bcm53xx_ioreg_bst, bcm53xx_ioreg_bsh,
		    CRU_BASE + CRU_USB2_CONTROL, usb2_control);

		// Prevent Clock modification
		bus_space_write_4(bcm53xx_ioreg_bst, bcm53xx_ioreg_bsh,
		    CRU_BASE + CRU_CLKSET_KEY, 0);

		usb_ref = (clk->clk_usb2 / pdiv) * new_ndiv; 
	}

	clk->clk_usb_ref = usb_ref;
}


static void
bcm53xx_clock_init(struct bcm53xx_clock_info *clk)
{
	clk->clk_ref = BCM53XX_REF_CLK;
	clk->clk_sys = 8*clk->clk_ref;
}

/*
 * F(ddr) = ((1 / pdiv) * ndiv * CH2) / (post_div * 2)
 */
static void 
bcm53xx_get_ddr_freq(struct bcm53xx_clock_info *clk, uint32_t pll_status,
    uint32_t pll_dividers)
{
	const bool clocking_4x = (pll_status & PLL_STATUS_CLOCKING_4X) != 0;
	u_int post_div = __SHIFTOUT(pll_dividers, PLL_DIVIDERS_POST_DIV);
	u_int pdiv = __SHIFTOUT(pll_dividers, PLL_DIVIDERS_PDIV);
	u_int ndiv = __SHIFTOUT(pll_dividers, PLL_DIVIDERS_NDIV);

	pdiv = ((pdiv - (clocking_4x ? 1 : 5)) & 7) + 1;

	clk->clk_ddr_mhz = __SHIFTOUT(pll_status, PLL_STATUS_MHZ);
	clk->clk_ddr = (clk->clk_ddr_ref / pdiv) * ndiv / (2 + post_div);
}

/*
 * CPU_CLK = (1 / pdiv) * (ndiv_int + (ndiv_frac / 0x40000000)) x F(ref)
 */
static void
bcm53xx_get_cpu_freq(struct bcm53xx_clock_info *clk,
	uint32_t pllarma, uint32_t pllarmb, uint32_t policy)
{
	policy = __SHIFTOUT(policy, CLK_POLICY_FREQ_POLICY2);

	if (policy == CLK_POLICY_REF_CLK) {
		clk->clk_cpu = clk->clk_ref;
		clk->clk_apb = clk->clk_cpu;
		return;
	}

	if (policy == CLK_POLICY_SYS_CLK) {
		clk->clk_cpu = clk->clk_sys;
		clk->clk_apb = clk->clk_cpu / 4;
		return;
	}
		
	const u_int pdiv = bcm53xx_value_wrap(pllarma, CLK_PLLARMA_PDIV);
	const u_int ndiv_int = bcm53xx_value_wrap(pllarma, CLK_PLLARMA_NDIV_INT);
	const u_int ndiv_frac = __SHIFTOUT(pllarmb, CLK_PLLARMB_NDIV_FRAC);
	// const u_int apb_clk_div = __SHIFTOUT(apb_clk_div, CLK_APB_DIV_VALUE)+1;

	const u_int cpu_div = (policy == CLK_POLICY_ARM_PLL_CH0) ? 4 : 2;

	clk->clk_cpu = bcm53xx_freq_calc(clk, pdiv, ndiv_int, ndiv_frac) / cpu_div;
	clk->clk_apb = clk->clk_cpu / 4;
}

struct bcm53xx_chip_state {
	uint32_t bcs_lcpll_control1;
	uint32_t bcs_lcpll_control2;

	uint32_t bcs_genpll_control5;
	uint32_t bcs_genpll_control6;
	uint32_t bcs_genpll_control7;

	uint32_t bcs_usb2_control;

	uint32_t bcs_ddr_phy_ctl_pll_status;
	uint32_t bcs_ddr_phy_ctl_pll_dividers;

	uint32_t bcs_armcore_clk_policy;
	uint32_t bcs_armcore_clk_pllarma;
	uint32_t bcs_armcore_clk_pllarmb;
};

static void
bcm53xx_get_chip_ioreg_state(struct bcm53xx_chip_state *bcs,
	bus_space_tag_t bst, bus_space_handle_t bsh)
{
	bcs->bcs_lcpll_control1 = bus_space_read_4(bst, bsh,
	    DMU_BASE + DMU_LCPLL_CONTROL1);
	bcs->bcs_lcpll_control2 = bus_space_read_4(bst, bsh,
	    DMU_BASE + DMU_LCPLL_CONTROL2);

	bcs->bcs_genpll_control5 = bus_space_read_4(bst, bsh,
	    CRU_BASE + CRU_GENPLL_CONTROL5);
	bcs->bcs_genpll_control6 = bus_space_read_4(bst, bsh,
	    CRU_BASE + CRU_GENPLL_CONTROL6);
	bcs->bcs_genpll_control7 = bus_space_read_4(bst, bsh,
	    CRU_BASE + CRU_GENPLL_CONTROL7);

	bcs->bcs_usb2_control = bus_space_read_4(bst, bsh,
	    CRU_BASE + CRU_USB2_CONTROL);

	bcs->bcs_ddr_phy_ctl_pll_status = bus_space_read_4(bst, bsh,
	    DDR_BASE + DDR_PHY_CTL_PLL_STATUS);
	bcs->bcs_ddr_phy_ctl_pll_dividers = bus_space_read_4(bst, bsh,
	    DDR_BASE + DDR_PHY_CTL_PLL_DIVIDERS);
}

static void
bcm53xx_get_chip_armcore_state(struct bcm53xx_chip_state *bcs,
	bus_space_tag_t bst, bus_space_handle_t bsh)
{
	bcs->bcs_armcore_clk_policy = bus_space_read_4(bst, bsh,
	    ARMCORE_CLK_POLICY_FREQ);
	bcs->bcs_armcore_clk_pllarma = bus_space_read_4(bst, bsh,
	    ARMCORE_CLK_PLLARMA);
	bcs->bcs_armcore_clk_pllarmb = bus_space_read_4(bst, bsh,
	    ARMCORE_CLK_PLLARMB);
}

void
bcm53xx_cpu_softc_init(struct cpu_info *ci)
{
	struct cpu_softc * const cpu = ci->ci_softc;

	cpu->cpu_ioreg_bst = bcm53xx_ioreg_bst;
	cpu->cpu_ioreg_bsh = bcm53xx_ioreg_bsh;

	cpu->cpu_armcore_bst = bcm53xx_armcore_bst;
	cpu->cpu_armcore_bsh = bcm53xx_armcore_bsh;
}

void
bcm53xx_print_clocks(void)
{
#if defined(VERBOSE_ARM_INIT)
	const struct bcm53xx_clock_info * const clk = &cpu_softc.cpu_clk;
	printf("ref clk =	%u (%#x)\n", clk->clk_ref, clk->clk_ref);
	printf("sys clk =	%u (%#x)\n", clk->clk_sys, clk->clk_sys);
	printf("lcpll clk =	%u (%#x)\n", clk->clk_lcpll, clk->clk_lcpll);
	printf("pcie ref clk =	%u (%#x) [CH0]\n", clk->clk_pcie_ref, clk->clk_pcie_ref);
	printf("sdio clk =	%u (%#x) [CH1]\n", clk->clk_sdio, clk->clk_sdio);
	printf("ddr ref clk =	%u (%#x) [CH2]\n", clk->clk_ddr_ref, clk->clk_ddr_ref);
	printf("axi clk =	%u (%#x) [CH3]\n", clk->clk_axi, clk->clk_axi);
	printf("genpll clk =	%u (%#x)\n", clk->clk_genpll, clk->clk_genpll);
	printf("mac clk =	%u (%#x) [CH0]\n", clk->clk_mac, clk->clk_mac);
	printf("robo clk =	%u (%#x) [CH1]\n", clk->clk_robo, clk->clk_robo);
	printf("usb2 clk =	%u (%#x) [CH2]\n", clk->clk_usb2, clk->clk_usb2);
	printf("iproc clk =	%u (%#x) [CH3]\n", clk->clk_iproc, clk->clk_iproc);
	printf("ddr clk =	%u (%#x)\n", clk->clk_ddr, clk->clk_ddr);
	printf("ddr mhz =	%u (%#x)\n", clk->clk_ddr_mhz, clk->clk_ddr_mhz);
	printf("cpu clk =	%u (%#x)\n", clk->clk_cpu, clk->clk_cpu);
	printf("apb clk =	%u (%#x)\n", clk->clk_apb, clk->clk_apb);
	printf("usb ref clk =	%u (%#x)\n", clk->clk_usb_ref, clk->clk_usb_ref);
#endif
}

void
bcm53xx_bootstrap(vaddr_t iobase)
{
	struct bcm53xx_chip_state bcs;
	int error;

#ifdef BCM53XX_CONSOLE_EARLY
	com_base = iobase + CCA_UART0_BASE;
	cn_tab = &bcm53xx_earlycons;
#endif

	bcm53xx_ioreg_bsh = (bus_space_handle_t) iobase;
	error = bus_space_map(bcm53xx_ioreg_bst, BCM53XX_IOREG_PBASE,
	    BCM53XX_IOREG_SIZE, 0, &bcm53xx_ioreg_bsh);
	if (error)
		panic("%s: failed to map BCM53xx %s registers: %d",
		    __func__, "io", error);

	bcm53xx_armcore_bsh = (bus_space_handle_t) iobase + BCM53XX_IOREG_SIZE;
	error = bus_space_map(bcm53xx_armcore_bst, BCM53XX_ARMCORE_PBASE,
	    BCM53XX_ARMCORE_SIZE, 0, &bcm53xx_armcore_bsh);
	if (error)
		panic("%s: failed to map BCM53xx %s registers: %d",
		    __func__, "armcore", error);

	curcpu()->ci_softc = &cpu_softc;

	bcm53xx_get_chip_ioreg_state(&bcs, bcm53xx_ioreg_bst, bcm53xx_ioreg_bsh);
	bcm53xx_get_chip_armcore_state(&bcs, bcm53xx_armcore_bst, bcm53xx_armcore_bsh);

	struct bcm53xx_clock_info * const clk = &cpu_softc.cpu_clk;

	bcm53xx_clock_init(clk);
	bcm53xx_lcpll_clock_init(clk, bcs.bcs_lcpll_control1,
	    bcs.bcs_lcpll_control2);
	bcm53xx_genpll_clock_init(clk, bcs.bcs_genpll_control5,
	    bcs.bcs_genpll_control6, bcs.bcs_genpll_control7);
	bcm53xx_usb_clock_init(clk, bcs.bcs_usb2_control);
	bcm53xx_get_ddr_freq(clk, bcs.bcs_ddr_phy_ctl_pll_status,
	    bcs.bcs_ddr_phy_ctl_pll_dividers);
	bcm53xx_get_cpu_freq(clk, bcs.bcs_armcore_clk_pllarma,
	    bcs.bcs_armcore_clk_pllarmb, bcs.bcs_armcore_clk_policy);

	curcpu()->ci_data.cpu_cc_freq = clk->clk_cpu;

	arml2cc_init(bcm53xx_armcore_bst, bcm53xx_armcore_bsh, ARMCORE_L2C_BASE);
}

void
bcm53xx_dma_bootstrap(psize_t memsize)
{
	if (memsize <= 256*1024*1024) {
		bcm53xx_dma_ranges[0].dr_len = memsize;
		bcm53xx_coherent_dma_ranges[0].dr_len = memsize;
		bcm53xx_dma_tag._nranges = 1;
		bcm53xx_coherent_dma_tag._nranges = 1;
	} else {
		/*
		 * By setting up two ranges, bus_dmamem_alloc will always
		 * try to allocate from range 0 first resulting in allocations
		 * below 256MB which for PCI and GMAC are coherent.
		 */
		bcm53xx_dma_ranges[1].dr_len = memsize - 0x10000000;
		bcm53xx_coherent_dma_ranges[1].dr_len = memsize - 0x10000000;
	}
	KASSERT(bcm53xx_dma_tag._ranges[0].dr_flags == 0);
	KASSERT(bcm53xx_coherent_dma_tag._ranges[0].dr_flags == _BUS_DMAMAP_COHERENT);
#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
	KASSERT(bcm53xx_bounce_dma_tag._ranges[0].dr_flags == _BUS_DMAMAP_COHERENT);
#endif
}

#ifdef MULTIPROCESSOR
void
bcm53xx_cpu_hatch(struct cpu_info *ci)
{
	a9tmr_init_cpu_clock(ci);
}
#endif

void
bcm53xx_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	if (device_is_a(self, "armperiph")
	    && device_is_a(device_parent(self), "mainbus")) {
		/*
		 * XXX KLUDGE ALERT XXX
		 * The iot mainbus supplies is completely wrong since it scales
		 * addresses by 2.  The simpliest remedy is to replace with our
		 * bus space used for the armcore regisers (which armperiph uses). 
		 */
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = bcm53xx_armcore_bst;
		return;
	}
 
	/*
	 * We need to tell the A9 Global/Watchdog Timer
	 * what frequency it runs at.
	 */
	if (device_is_a(self, "a9tmr") || device_is_a(self, "a9wdt")) {
		/*
		 * This clock always runs at (arm_clk div 2) and only goes
		 * to timers that are part of the A9 MP core subsystem.
		 */
                prop_dictionary_set_uint32(dict, "frequency",
		    cpu_softc.cpu_clk.clk_cpu / 2);
		return;
	}

	if (device_is_a(self, "bcmeth")) {
		const struct bcmccb_attach_args * const ccbaa = aux;
		const uint8_t enaddr[ETHER_ADDR_LEN] = {
			0x00, 0x01, 0x02, 0x03, 0x04,
			0x05 + 2 * ccbaa->ccbaa_loc.loc_port,
		};
		prop_data_t pd = prop_data_create_data(enaddr, ETHER_ADDR_LEN);
		KASSERT(pd != NULL);
		if (prop_dictionary_set(device_properties(self), "mac-address", pd) == false) {
			printf("WARNING: Unable to set mac-address property for %s\n", device_xname(self));
		}
		prop_object_release(pd);
	}
}

static kmutex_t srab_lock __cacheline_aligned;

void
bcm53xx_srab_init(void)
{
	mutex_init(&srab_lock, MUTEX_DEFAULT, IPL_VM);

	bcm53xx_srab_write_4(0x0079, 0x90);	// reset switch 
	for (u_int port = 0; port < 8; port++) {        
		/* per port control: no stp */
		bcm53xx_srab_write_4(port, 0x00);
	}
	bcm53xx_srab_write_4(0x0008, 0x1c);	// IMP port (enab UC/MC/BC)
	bcm53xx_srab_write_4(0x000e, 0xbb);	// IMP port force-link 1G
	bcm53xx_srab_write_4(0x005d, 0x7b);	// port5 force-link 1G
	bcm53xx_srab_write_4(0x005f, 0x7b);	// port7 force-link 1G
	bcm53xx_srab_write_4(0x000b, 0x7);	// management mode
	bcm53xx_srab_write_4(0x0203, 0x0);	// disable BRCM tag
	bcm53xx_srab_write_4(0x0200, 0x80);	// enable IMP=port8
}

static inline void
bcm53xx_srab_busywait(bus_space_tag_t bst, bus_space_handle_t bsh)
{
	while (bus_space_read_4(bst, bsh, SRAB_BASE + SRAB_CMDSTAT) & SRA_GORDYN) {
		delay(10);
	}
}

uint32_t
bcm53xx_srab_read_4(u_int pageoffset)
{
	bus_space_tag_t bst = bcm53xx_ioreg_bst;
	bus_space_handle_t bsh = bcm53xx_ioreg_bsh;
	uint32_t rv;

	mutex_spin_enter(&srab_lock);

	bcm53xx_srab_busywait(bst, bsh);
	bus_space_write_4(bst, bsh, SRAB_BASE + SRAB_CMDSTAT,
	    __SHIFTIN(pageoffset, SRA_PAGEOFFSET) | SRA_GORDYN);
	bcm53xx_srab_busywait(bst, bsh);
	rv = bus_space_read_4(bst, bsh, SRAB_BASE + SRAB_RDL);

	mutex_spin_exit(&srab_lock);
	return rv;
}

uint64_t
bcm53xx_srab_read_8(u_int pageoffset)
{
	bus_space_tag_t bst = bcm53xx_ioreg_bst;
	bus_space_handle_t bsh = bcm53xx_ioreg_bsh;
	uint64_t rv;

	mutex_spin_enter(&srab_lock);

	bcm53xx_srab_busywait(bst, bsh);
	bus_space_write_4(bst, bsh, SRAB_BASE + SRAB_CMDSTAT,
	    __SHIFTIN(pageoffset, SRA_PAGEOFFSET) | SRA_GORDYN);
	bcm53xx_srab_busywait(bst, bsh);
	rv = bus_space_read_4(bst, bsh, SRAB_BASE + SRAB_RDH);
	rv <<= 32;
	rv |= bus_space_read_4(bst, bsh, SRAB_BASE + SRAB_RDL);

	mutex_spin_exit(&srab_lock);
	return rv;
}

void
bcm53xx_srab_write_4(u_int pageoffset, uint32_t val)
{
	bus_space_tag_t bst = bcm53xx_ioreg_bst;
	bus_space_handle_t bsh = bcm53xx_ioreg_bsh;

	mutex_spin_enter(&srab_lock);

	bcm53xx_srab_busywait(bst, bsh);
	bus_space_write_4(bst, bsh, SRAB_BASE + SRAB_WDL, val);
	bus_space_write_4(bst, bsh, SRAB_BASE + SRAB_CMDSTAT,
	    __SHIFTIN(pageoffset, SRA_PAGEOFFSET) | SRA_WRITE | SRA_GORDYN);
	bcm53xx_srab_busywait(bst, bsh);

	mutex_spin_exit(&srab_lock);
}

void
bcm53xx_srab_write_8(u_int pageoffset, uint64_t val)
{
	bus_space_tag_t bst = bcm53xx_ioreg_bst;
	bus_space_handle_t bsh = bcm53xx_ioreg_bsh;

	mutex_spin_enter(&srab_lock);

	bcm53xx_srab_busywait(bst, bsh);
	bus_space_write_4(bst, bsh, SRAB_BASE + SRAB_WDL, val);
	bus_space_write_4(bst, bsh, SRAB_BASE + SRAB_WDH, val >> 32);
	bus_space_write_4(bst, bsh, SRAB_BASE + SRAB_CMDSTAT,
	    __SHIFTIN(pageoffset, SRA_PAGEOFFSET) | SRA_WRITE | SRA_GORDYN);
	bcm53xx_srab_busywait(bst, bsh);
	mutex_spin_exit(&srab_lock);
}
