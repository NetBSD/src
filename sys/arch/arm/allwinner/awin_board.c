/*	$NetBSD: awin_board.c,v 1.17 2014/09/07 22:21:36 jmcneill Exp $	*/
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

#include "opt_allwinner.h"
#include "opt_arm_debug.h"

#define	_ARM32_BUS_DMA_PRIVATE

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_board.c,v 1.17 2014/09/07 22:21:36 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <prop/proplib.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <arm/locore.h>

#include <arm/mainbus/mainbus.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <arm/cortex/gtmr_var.h>

bus_space_handle_t awin_core_bsh;

struct arm32_bus_dma_tag awin_dma_tag = {
	_BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};

struct arm32_dma_range awin_coherent_dma_ranges[] = {
	[0] = {
		.dr_sysbase = AWIN_SDRAM_PBASE,
		.dr_busbase = AWIN_SDRAM_PBASE,
		.dr_flags = _BUS_DMAMAP_COHERENT,
	},
};


struct arm32_bus_dma_tag awin_coherent_dma_tag = {
	._ranges = awin_coherent_dma_ranges,
	._nranges = __arraycount(awin_coherent_dma_ranges),
	_BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};

#ifdef AWIN_CONSOLE_EARLY
#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>
#include <dev/cons.h>

static volatile uint32_t *uart_base;

static int
awin_cngetc(dev_t dv)
{
        if ((uart_base[com_lsr] & LSR_RXRDY) == 0)
		return -1;

	return uart_base[com_data] & 0xff;
}

static void
awin_cnputc(dev_t dv, int c)
{
	int timo = 150000;

        while ((uart_base[com_lsr] & LSR_TXRDY) == 0 && --timo > 0)
		;

	uart_base[com_data] = c & 0xff;

	timo = 150000;
        while ((uart_base[com_lsr] & LSR_TSRE) == 0 && --timo > 0)
		;
}

static struct consdev awin_earlycons = {
	.cn_putc = awin_cnputc,
	.cn_getc = awin_cngetc,
	.cn_pollc = nullcnpollc,
};
#endif /* AWIN_CONSOLE_EARLY */

static void
awin_cpu_clk(void)
{
	struct cpu_info * const ci = curcpu();
	const uint32_t cpu0_cfg = bus_space_read_4(&awin_bs_tag, awin_core_bsh,
	    AWIN_CCM_OFFSET + AWIN_CPU_AHB_APB0_CFG_REG);
	const u_int cpu_clk_sel = __SHIFTIN(cpu0_cfg, AWIN_CPU_CLK_SRC_SEL);
	switch (__SHIFTOUT(cpu_clk_sel, AWIN_CPU_CLK_SRC_SEL)) {
	case AWIN_CPU_CLK_SRC_SEL_LOSC:
		ci->ci_data.cpu_cc_freq = 32768;
		break;
	case AWIN_CPU_CLK_SRC_SEL_OSC24M:
		ci->ci_data.cpu_cc_freq = AWIN_REF_FREQ;
		break;
	case AWIN_CPU_CLK_SRC_SEL_PLL1: {
		const uint32_t pll1_cfg = bus_space_read_4(&awin_bs_tag,
		    awin_core_bsh, AWIN_CCM_OFFSET + AWIN_PLL1_CFG_REG);
		u_int p = __SHIFTOUT(pll1_cfg, AWIN_PLL_CFG_OUT_EXP_DIVP);
		u_int n = __SHIFTOUT(pll1_cfg, AWIN_PLL_CFG_FACTOR_N);
		u_int k = __SHIFTOUT(pll1_cfg, AWIN_PLL_CFG_FACTOR_K) + 1;
		u_int m = __SHIFTOUT(pll1_cfg, AWIN_PLL_CFG_FACTOR_M) + 1;
		ci->ci_data.cpu_cc_freq =
		    ((uint64_t)AWIN_REF_FREQ * (n ? n : 1) * k / m) >> p;
		break;
	}
	case AWIN_CPU_CLK_SRC_SEL_200MHZ:
		ci->ci_data.cpu_cc_freq = 200000000;
		break;
	}
}

void
awin_bootstrap(vaddr_t iobase, vaddr_t uartbase)
{
	int error;

#ifdef AWIN_CONSOLE_EARLY
	uart_base = (volatile uint32_t *)uartbase;
	cn_tab = &awin_earlycons;
	printf("Early console started\n");
#endif

	error = bus_space_map(&awin_bs_tag, AWIN_CORE_PBASE,
	    AWIN_CORE_SIZE, 0, &awin_core_bsh);
	if (error)
		panic("%s: failed to map a[12]0 %s registers: %d",
		    __func__, "io", error);
	KASSERT(awin_core_bsh == iobase);

#ifdef VERBOSE_INIT_ARM
	printf("CPU Speed is");
#endif
	awin_cpu_clk();
#ifdef VERBOSE_INIT_ARM
	printf(" %"PRIu64"\n", curcpu()->ci_data.cpu_cc_freq);
#endif

#ifdef VERBOSE_INIT_ARM
	printf("Determining GPIO configuration");
#endif
	awin_gpio_init();
#ifdef VERBOSE_INIT_ARM
	printf("\n");
#endif

#ifdef VERBOSE_INIT_ARM
	uint32_t s0 = bus_space_read_4(&awin_bs_tag, awin_core_bsh,
	    AWIN_CPUCFG_OFFSET + AWIN_CPUCFG_CPU0_STATUS_REG);
	uint32_t s1 = bus_space_read_4(&awin_bs_tag, awin_core_bsh,
	    AWIN_CPUCFG_OFFSET + AWIN_CPUCFG_CPU1_STATUS_REG);
	printf("%s: cpu status: 0=%#x 1=%#x\n", __func__, s0, s1);
#endif

#if !defined(MULTIPROCESSOR) && defined(VERBOSE_INIT_ARM)
	u_int arm_cpu_max;
#endif
#if defined(MULTIPROCESSOR) || defined(VERBOSE_INIT_ARM)
	arm_cpu_max = 1 + __SHIFTOUT(armreg_l2ctrl_read(), L2CTRL_NUMCPU);
#endif
#ifdef VERBOSE_INIT_ARM
	printf("%s: %d cpus present\n", __func__, arm_cpu_max);
#endif
#if defined(MULTIPROCESSOR)
	for (size_t i = 1; i < arm_cpu_max; i++) {
		if ((arm_cpu_hatched & (1 << i)) == 0) {
			printf("%s: warning: cpu%zu failed to hatch\n",
			    __func__, i);
		}
	}
#endif
}

void
awin_dma_bootstrap(psize_t psize)
{
	awin_coherent_dma_ranges[0].dr_len = psize;
}

#ifdef MULTIPROCESSOR
void
awin_cpu_hatch(struct cpu_info *ci)
{
	gtmr_init_cpu_clock(ci);
}
#endif

psize_t
awin_memprobe(void)
{
	const uint32_t dcr = bus_space_read_4(&awin_bs_tag, awin_core_bsh,
	    AWIN_DRAM_OFFSET + AWIN_DRAM_DCR_REG);

	psize_t memsize = __SHIFTOUT(dcr, AWIN_DRAM_DCR_IO_WIDTH);
	memsize <<= __SHIFTOUT(dcr, AWIN_DRAM_DCR_CHIP_DENSITY) + 28 - 3;
#ifdef VERBOSE_INIT_ARM
	printf("sdram_config = %#x, memsize = %uMB\n", dcr,
	    (u_int)(memsize >> 20));
#endif
	return memsize;
}

uint16_t
awin_chip_id(void)
{
	static uint16_t chip_id = 0;
	uint32_t ver;

	if (!chip_id) {
		ver = bus_space_read_4(&awin_bs_tag, awin_core_bsh,
		    AWIN_SRAM_OFFSET + AWIN_SRAM_VER_REG);
		ver |= AWIN_SRAM_VER_R_EN;
		bus_space_write_4(&awin_bs_tag, awin_core_bsh,
		    AWIN_SRAM_OFFSET + AWIN_SRAM_VER_REG, ver);
		ver = bus_space_read_4(&awin_bs_tag, awin_core_bsh,
		    AWIN_SRAM_OFFSET + AWIN_SRAM_VER_REG);

		chip_id = __SHIFTOUT(ver, AWIN_SRAM_VER_KEY_FIELD);
	}

	return chip_id;
}

const char *
awin_chip_name(void)
{
	uint16_t chip_id = awin_chip_id();

	switch (chip_id) {
	case AWIN_CHIP_ID_A10: return "A10";
	case AWIN_CHIP_ID_A13: return "A13";
	case AWIN_CHIP_ID_A20: return "A20";
	case AWIN_CHIP_ID_A23: return "A23";
	case AWIN_CHIP_ID_A31: return "A31";
	default: return "unknown chip";
	}
}

void
awin_pll6_enable(void)
{
	bus_space_tag_t bst = &awin_bs_tag;
	bus_space_handle_t bsh = awin_core_bsh;

	/*
	 * SATA needs PLL6 to be a 100MHz clock.
	 */
	const uint32_t ocfg = bus_space_read_4(bst, bsh,
	    AWIN_CCM_OFFSET + AWIN_PLL6_CFG_REG);
	const u_int k = __SHIFTOUT(ocfg, AWIN_PLL_CFG_FACTOR_K);

	/*
	 * Output freq is 24MHz * n * k / m / 6.
	 * To get to 100MHz, k & m must be equal and n must be 25.
	 */
	uint32_t ncfg = ocfg;
	ncfg &= ~(AWIN_PLL_CFG_FACTOR_M|AWIN_PLL_CFG_FACTOR_N);
	ncfg &= ~(AWIN_PLL_CFG_BYPASS);
	ncfg |= __SHIFTIN(k, AWIN_PLL_CFG_FACTOR_M);
	ncfg |= __SHIFTIN(25, AWIN_PLL_CFG_FACTOR_N);
	ncfg |= AWIN_PLL_CFG_ENABLE | AWIN_PLL6_CFG_SATA_CLK_EN;
	if (ncfg != ocfg) {
		bus_space_write_4(bst, bsh,
		    AWIN_CCM_OFFSET + AWIN_PLL6_CFG_REG, ncfg);
	}
#if 0
	printf(" [pll6=%#x->%#x:n=%ju k=%ju m=%ju] ",
	    ocfg, ncfg,
	    __SHIFTOUT(ncfg, AWIN_PLL_CFG_FACTOR_N),
	    __SHIFTOUT(ncfg, AWIN_PLL_CFG_FACTOR_K),
	    __SHIFTOUT(ncfg, AWIN_PLL_CFG_FACTOR_M));
#endif
}

void
awin_pll2_enable(void)
{
	bus_space_tag_t bst = &awin_bs_tag;
	bus_space_handle_t bsh = awin_core_bsh;

	/*
  	 * AC (at 48kHz) needs PLL2 to be 24576000 Hz
  	 */
	const uint32_t ocfg = bus_space_read_4(bst, bsh,
	    AWIN_CCM_OFFSET + AWIN_PLL2_CFG_REG);

	uint32_t ncfg = ocfg;
	ncfg &= ~(AWIN_PLL2_CFG_PREVDIV|AWIN_PLL2_CFG_FACTOR_N|AWIN_PLL2_CFG_POSTDIV);
	ncfg |= __SHIFTIN(21, AWIN_PLL2_CFG_PREVDIV);
	ncfg |= __SHIFTIN(86, AWIN_PLL2_CFG_FACTOR_N);
	ncfg |= __SHIFTIN(4, AWIN_PLL2_CFG_POSTDIV);
	ncfg |= AWIN_PLL_CFG_ENABLE;
	if (ncfg != ocfg) {
		bus_space_write_4(bst, bsh,
		    AWIN_CCM_OFFSET + AWIN_PLL2_CFG_REG, ncfg);
	}
}
