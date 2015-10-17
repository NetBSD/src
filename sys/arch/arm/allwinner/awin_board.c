/*	$NetBSD: awin_board.c,v 1.39 2015/10/17 15:30:14 bouyer Exp $	*/
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
#include "opt_multiprocessor.h"

#define	_ARM32_BUS_DMA_PRIVATE

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_board.c,v 1.39 2015/10/17 15:30:14 bouyer Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/kmem.h>

#include <prop/proplib.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <arm/locore.h>

#include <arm/mainbus/mainbus.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <arm/cortex/gtmr_var.h>

bus_space_handle_t awin_core_bsh;
#if defined(ALLWINNER_A80)
bus_space_handle_t awin_core2_bsh;
bus_space_handle_t awin_rcpus_bsh;
#endif

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

#define CCM_READ4(reg) bus_space_read_4(&armv7_generic_bs_tag, \
    awin_core_bsh, AWIN_CCM_OFFSET + (reg))
#define CCM_WRITE4(reg, v) bus_space_write_4(&armv7_generic_bs_tag, \
    awin_core_bsh, AWIN_CCM_OFFSET + (reg), (v))

#ifdef AWIN_CONSOLE_EARLY
#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>
#include <dev/cons.h>

static volatile uint32_t *uart_base;

static int
awin_cngetc(dev_t dv)
{
        if ((le32toh(uart_base[com_lsr]) & LSR_RXRDY) == 0)
		return -1;

	return le32toh(uart_base[com_data]) & 0xff;
}

static void
awin_cnputc(dev_t dv, int c)
{
	int timo = 150000;

        while ((le32toh(uart_base[com_lsr]) & LSR_TXRDY) == 0 && --timo > 0)
		;

	uart_base[com_data] = htole32(c & 0xff);

	timo = 150000;
        while ((le32toh(uart_base[com_lsr]) & LSR_TSRE) == 0 && --timo > 0)
		;
}

static struct consdev awin_earlycons = {
	.cn_putc = awin_cnputc,
	.cn_getc = awin_cngetc,
	.cn_pollc = nullcnpollc,
};
#endif /* AWIN_CONSOLE_EARLY */

struct awin_cpu_freq {
	uint	freq; /* this frequency in hz */
	int	mvolt; /* minimum millivolt for this freq */
	int	pll_n, pll_k, pll_m, pll_p; /* pll parameters */
	int	axi_div, ahb_div, abp0_div;
};

struct awin_cpu_freq awin_current_freq;
const struct awin_cpu_freq *awin_freqs;

#define A20_PLL1_ENTRY(n, k, m, p) \
	.freq = (((AWIN_REF_FREQ * n * (k + 1)) / (m + 1)) >> p) / 1000000, \
	.pll_n = (n), \
	.pll_k = (k), \
	.pll_m = (m), \
	.pll_p = (p)

static const struct awin_cpu_freq awin_a20_cpus_freqs[] = {
	{
		A20_PLL1_ENTRY(13, 0, 0, 0), /* 312 Mhz */
		.mvolt = 1100,
		.axi_div  = 0, /* /1, 312 Mhz */
		.ahb_div  = 1, /* /2, 156 Mhz */
		.abp0_div = 0, /* /2, 78 Mhz */
	},
	{
		A20_PLL1_ENTRY(22, 0, 0, 0), /* 528 Mhz */
		.mvolt = 1100,
		.axi_div  = 1, /* /2, 264 Mhz */
		.ahb_div  = 1, /* /2, 132 Mhz */
		.abp0_div = 0, /* /2, 66 Mhz */
	},
	{
		A20_PLL1_ENTRY(30, 0, 0, 0), /* 720 Mhz */
		.mvolt = 1200,
		.axi_div  = 1, /* /2, 360Mhz */
		.ahb_div  = 1, /* /2, 180 Mhz */
		.abp0_div = 0, /* /2, 90 Mhz */
	},
	{
		A20_PLL1_ENTRY(18, 1, 0, 0), /* 864 Mhz */
		.mvolt = 1300,
		.axi_div  = 2, /* /3, 288 Mhz */
		.ahb_div  = 1, /* /2, 144 Mhz */
		.abp0_div = 0, /* /2, 72 Mhz */
	},
	{
		A20_PLL1_ENTRY(19, 1, 0, 0), /* 912 Mhz */
		.mvolt = 1400,
		.axi_div  = 2, /* /3, 304Mhz */
		.ahb_div  = 1, /* /2, 152 Mhz */
		.abp0_div = 0, /* /2, 76 Mhz */
	},
	{
		A20_PLL1_ENTRY(20, 1, 0, 0), /* 960 Mhz */
		.mvolt = 1400,
		.axi_div  = 2, /* /3, 320Mhz */
		.ahb_div  = 1, /* /2, 160 Mhz */
		.abp0_div = 0, /* /2, 80 Mhz */
	},
	{
		A20_PLL1_ENTRY(21, 1, 0, 0), /* 1008 Mhz */
		.mvolt = 1425,
		.axi_div  = 2, /* /3, 336 Mhz */
		.ahb_div  = 1, /* /2, 168 Mhz */
		.abp0_div = 0, /* /2, 84 Mhz */
	},
	{
		.freq = 0, /* end of array */
		.mvolt = 0,
		.pll_n = 0,
		.pll_k = 0,
		.pll_m = 0,
		.pll_p = 0,
		.axi_div  = 0,
		.ahb_div  = 0,
		.abp0_div = 0,
	},
};

static kmutex_t cpufreq_lock;
static int awin_freq_min, awin_freq_max;

static int awin_set_cpu_clk(uint64_t);

static int
awin_current_frequency_sysctl_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	int freq = awin_current_freq.freq;
	int old_freq = freq;
	int old_mvolt;
	struct awin_cpu_freq new_awin_freq;

	KASSERTMSG(
	    curcpu()->ci_data.cpu_cc_freq == awin_current_freq.freq * 1000000,
	    "cc_freq %"PRIu64" mpu_freq %u000000",
	    curcpu()->ci_data.cpu_cc_freq, awin_current_freq.freq);

	node.sysctl_data = &freq;

	int error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	if (freq == old_freq)
		return 0;
	KASSERT(awin_freqs != NULL);
	error = EINVAL;
	if (freq < awin_freq_min || freq > awin_freq_max)
		return error;
	for (size_t i = 0; awin_freqs[i].freq > 0; i++) {
		if (awin_freqs[i].freq == freq) {
			new_awin_freq = awin_freqs[i];
			error = 0;    
			break;
		}
	}
	if (error)
		return error;
	mutex_enter(&cpufreq_lock);
	freq = awin_current_freq.freq;
	old_mvolt = awin_current_freq.mvolt;

	if (new_awin_freq.mvolt > old_mvolt) {
		/* need to raise CPU voltage first */
		for (old_mvolt += 25; old_mvolt <= new_awin_freq.mvolt;
		    old_mvolt += 25) {
			error = awin_set_mpu_volt(old_mvolt, 0);
			if (error)
				goto end;
			kpause("cpuvolt", 0, max(mstohz(1), 1), NULL);
		}
	}
	/* adjust ppl1. This needs to adjust the other dividers as well */
	error = awin_set_cpu_clk(new_awin_freq.freq);
	if (error)
		goto end;
	if (new_awin_freq.mvolt < old_mvolt) {
		/*
		 * now we can lower voltage. Ignore error if any, because
		 * the freq has been changed.
		 */
		for (old_mvolt -= 25; old_mvolt >= new_awin_freq.mvolt;
		    old_mvolt -= 25) {
			error = awin_set_mpu_volt(old_mvolt, 0);
			if (error) {
				printf(
				   "warning: failed to lower CPU voltage: %d\n",
			    	   error);
			}
			error = 0;
			kpause("cpuvolt", 0, max(mstohz(1), 1), NULL);
		}
	}

end:
	mutex_exit(&cpufreq_lock);
	return error;
}

static int
awin_available_frequency_sysctl_helper(SYSCTLFN_ARGS)
{
	char *available_frequencies;
	int availfreq_size;
	char cur_cpu_freq[6];
	int i;

	available_frequencies = oldp;
	availfreq_size = *oldlenp;
	if (available_frequencies == NULL) {
		*oldlenp = __arraycount(awin_a20_cpus_freqs)*6;
		return 0;
	}

	available_frequencies[0] = '\0';
	for (i = 0 ; awin_freqs[i].freq > 0; i++) {
		if (awin_freqs[i].freq < awin_freq_min ||
		    awin_freqs[i].freq > awin_freq_max)
			continue;
		snprintf(cur_cpu_freq, sizeof(cur_cpu_freq), "%u",      
		    awin_freqs[i].freq);
		if (strlen(available_frequencies) > 0) {
			strlcat(available_frequencies, " ", availfreq_size);
		}
		strlcat(available_frequencies, cur_cpu_freq, availfreq_size);
	}
	return 0;
}

SYSCTL_SETUP(sysctl_awin_machdep_setup, "sysctl allwinner machdep subtree setup")
{
	const struct sysctlnode *freqnode, *node;
	int i;
	int cc_freq = curcpu()->ci_data.cpu_cc_freq / 1000000;

	if (awin_chip_id() == AWIN_CHIP_ID_A20) {
		awin_freqs = awin_a20_cpus_freqs;
		awin_freq_min = 700;
		awin_freq_max = 960;
	} else {
		return;
	}
	mutex_init(&cpufreq_lock, MUTEX_DEFAULT, IPL_NONE);

	/* locate current freq in array */
	for (i = 0; awin_freqs[i].freq > 0; i++) {
		if (awin_freqs[i].freq >= cc_freq) {
			awin_current_freq = awin_freqs[i];
			awin_current_freq.freq = cc_freq;
			break;
		}
	}
	KASSERT(awin_current_freq.mvolt > 0);

	sysctl_createv(clog, 0, NULL, &node,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, &node, &freqnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "frequency", NULL,
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &freqnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "available", NULL,
		       awin_available_frequency_sysctl_helper, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &freqnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "current", NULL,
		       awin_current_frequency_sysctl_helper, 0, NULL, 0, 
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &freqnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "min", NULL,
		       NULL, 0, &awin_freq_min, sizeof(awin_freq_min), 
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &freqnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "max", NULL,
		       NULL, 0, &awin_freq_max, sizeof(awin_freq_max), 
		       CTL_CREATE, CTL_EOL);
}

static void
awin_cpu_clk(void)
{
	struct cpu_info * const ci = curcpu();

#if defined(ALLWINNER_A80)
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	const uint32_t c0cpux = bus_space_read_4(bst, awin_core_bsh,
	    AWIN_A80_CCU_OFFSET + AWIN_A80_CCU_PLL_C0CPUX_CTRL_REG);
	const u_int p = (c0cpux & AWIN_A80_CCU_PLL_CxCPUX_OUT_EXT_DIVP) ? 4 : 1;
	const u_int n = __SHIFTOUT(c0cpux, AWIN_A80_CCU_PLL_CxCPUX_FACTOR_N);

	ci->ci_data.cpu_cc_freq = ((uint64_t)AWIN_REF_FREQ * n) / p;
#else
	u_int reg = awin_chip_id() == AWIN_CHIP_ID_A31 ?
				      AWIN_A31_CPU_AXI_CFG_REG :
				      AWIN_CPU_AHB_APB0_CFG_REG;
	const uint32_t cpu0_cfg = CCM_READ4(reg);

	switch (__SHIFTOUT(cpu0_cfg, AWIN_CPU_CLK_SRC_SEL)) {
	case AWIN_CPU_CLK_SRC_SEL_LOSC:
		ci->ci_data.cpu_cc_freq = 32768;
		break;
	case AWIN_CPU_CLK_SRC_SEL_OSC24M:
		ci->ci_data.cpu_cc_freq = AWIN_REF_FREQ;
		break;
	case AWIN_CPU_CLK_SRC_SEL_PLL1: {
		const uint32_t pll1_cfg = CCM_READ4(AWIN_PLL1_CFG_REG);
		u_int p, n, k, m;
		if (awin_chip_id() == AWIN_CHIP_ID_A31) {
			p = 0;
			n = __SHIFTOUT(pll1_cfg, AWIN_PLL_CFG_FACTOR_N) + 1;
			k = __SHIFTOUT(pll1_cfg, AWIN_PLL_CFG_FACTOR_K) + 1;
			m = __SHIFTOUT(pll1_cfg, AWIN_PLL_CFG_FACTOR_M) + 1;
		} else {
			p = __SHIFTOUT(pll1_cfg, AWIN_PLL_CFG_OUT_EXP_DIVP);
			n = __SHIFTOUT(pll1_cfg, AWIN_PLL_CFG_FACTOR_N);
			k = __SHIFTOUT(pll1_cfg, AWIN_PLL_CFG_FACTOR_K) + 1;
			m = __SHIFTOUT(pll1_cfg, AWIN_PLL_CFG_FACTOR_M) + 1;
		}
		ci->ci_data.cpu_cc_freq =
		    ((uint64_t)AWIN_REF_FREQ * (n ? n : 1) * k / m) >> p;
		break;
	}
	case AWIN_CPU_CLK_SRC_SEL_200MHZ:
		ci->ci_data.cpu_cc_freq = 200000000;
		break;
	}
#endif
}

static void
awin_set_cpu_ahb_abp0_cfg(int reg, const struct awin_cpu_freq *awin_target_freq)
{
	uint32_t value = CCM_READ4(reg);

	/* set up dividers */
	value &= ~AWIN_APB0_CLK_RATIO;
	value |= __SHIFTIN(awin_target_freq->abp0_div, AWIN_APB0_CLK_RATIO);
	value &= ~AWIN_AHB_CLK_RATIO;
	value |= __SHIFTIN(awin_target_freq->ahb_div, AWIN_AHB_CLK_RATIO);
	value &= ~AWIN_AHB_CLK_SRC_SEL;
	value |= __SHIFTIN(AWIN_AHB_CLK_SRC_SEL_AXI, AWIN_AHB_CLK_SRC_SEL);
	value &= ~AWIN_AXI_CLK_DIV_RATIO;
	value |= __SHIFTIN(awin_target_freq->axi_div, AWIN_AXI_CLK_DIV_RATIO);
	CCM_WRITE4(reg, value);
	delay(500);
	CCM_WRITE4(reg, value | AWIN_DVFS_START);
	delay(500);
	while ((CCM_READ4(reg) & AWIN_DVFS_START) != 0)
		delay(500);
}

static void
awin_set_pll1(int pll1reg, const struct awin_cpu_freq *awin_target_freq)
{
	uint32_t pll1v = CCM_READ4(pll1reg);
	int old_p = __SHIFTOUT(pll1v, AWIN_PLL_CFG_OUT_EXP_DIVP);

	KASSERT(awin_target_freq->pll_n != 0);
	if (old_p < awin_target_freq->pll_p) {
		/* first increase P, then change others */
		pll1v &=  ~AWIN_PLL_CFG_OUT_EXP_DIVP;
		pll1v |= __SHIFTIN(awin_target_freq->pll_p,
		    AWIN_PLL_CFG_OUT_EXP_DIVP);
		CCM_WRITE4(pll1reg, pll1v);
		delay(500);
		pll1v &=  ~AWIN_PLL_CFG_FACTOR_N;
		pll1v |= __SHIFTIN(awin_target_freq->pll_n,
		    AWIN_PLL_CFG_FACTOR_N);
		pll1v &=  ~AWIN_PLL_CFG_FACTOR_K;
		pll1v |= __SHIFTIN(awin_target_freq->pll_k,
		    AWIN_PLL_CFG_FACTOR_K);
		pll1v &=  ~AWIN_PLL_CFG_FACTOR_M;
		pll1v |= __SHIFTIN(awin_target_freq->pll_m,
		    AWIN_PLL_CFG_FACTOR_M);
		CCM_WRITE4(pll1reg, pll1v);
		delay(500);
	} else {
		/* set other factors, then P */
		pll1v &=  ~AWIN_PLL_CFG_FACTOR_N;
		pll1v |= __SHIFTIN(awin_target_freq->pll_n,
		    AWIN_PLL_CFG_FACTOR_N);
		pll1v &=  ~AWIN_PLL_CFG_FACTOR_K;
		pll1v |= __SHIFTIN(awin_target_freq->pll_k,
		    AWIN_PLL_CFG_FACTOR_K);
		pll1v &=  ~AWIN_PLL_CFG_FACTOR_M;
		pll1v |= __SHIFTIN(awin_target_freq->pll_m,
		    AWIN_PLL_CFG_FACTOR_M);
		CCM_WRITE4(pll1reg, pll1v);
		delay(500);
		if (old_p != awin_target_freq->pll_p) {
			pll1v &=  ~AWIN_PLL_CFG_OUT_EXP_DIVP;
			pll1v |= __SHIFTIN(awin_target_freq->pll_p,
			    AWIN_PLL_CFG_OUT_EXP_DIVP);
			CCM_WRITE4(pll1reg, pll1v);
			delay(500);
		}
	}
}

static int
awin_set_cpu_clk(uint64_t new_freq)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	u_int reg = awin_chip_id() == AWIN_CHIP_ID_A31 ?
				      AWIN_A31_CPU_AXI_CFG_REG :
				      AWIN_CPU_AHB_APB0_CFG_REG;
	int i;

	if (awin_chip_id() != AWIN_CHIP_ID_A20)
		return EOPNOTSUPP;

	/*
	 * we have to either increase or decrease frequencies in steps
	 * to repect constraints on axi, ahb and abp frequencies
	 */
	if (new_freq > awin_current_freq.freq) {
		/* locate first step above current freq */
		for (i = 0;
		    awin_freqs[i].freq <= awin_current_freq.freq;
		    i++) {
			KASSERT(awin_freqs[i].freq != 0);
		}
		/* now increase frequency */
		for (;
		    awin_freqs[i].freq <= new_freq && awin_freqs[i].freq != 0;
		    i++) {
			KASSERT(awin_freqs[i].pll_n != 0);
			/* set up dividers */
			awin_set_cpu_ahb_abp0_cfg(reg, &awin_freqs[i]);
			/* set up PLL */
			awin_set_pll1(AWIN_PLL1_CFG_REG, &awin_freqs[i]);
			awin_current_freq = awin_freqs[i];
		}
	} else if (new_freq < awin_current_freq.freq) {
		/* locate current freq */
		for (i = 0;
		    awin_freqs[i].freq < awin_current_freq.freq;
		    i++) {
			KASSERT(awin_freqs[i].freq != 0);
		}
		KASSERT(i > 0);
		/* and decrease frequency */
		for (i--; awin_freqs[i].freq >= new_freq && i >= 0; i--) {
			/* first decrease PLL */
			awin_set_pll1(AWIN_PLL1_CFG_REG, &awin_freqs[i]);
			/* set up dividers */
			awin_set_cpu_ahb_abp0_cfg(reg, &awin_freqs[i]);
			awin_current_freq = awin_freqs[i];
		}
	}
	for (CPU_INFO_FOREACH(cii, ci)) {
		ci->ci_data.cpu_cc_freq = awin_current_freq.freq * 1000000;
	}
	return 0;
}

void
awin_bootstrap(vaddr_t iobase, vaddr_t uartbase)
{
	int error;
	bus_space_tag_t bst = &armv7_generic_bs_tag;

#ifdef AWIN_CONSOLE_EARLY
	uart_base = (volatile uint32_t *)uartbase;
	cn_tab = &awin_earlycons;
	printf("Early console started\n");
#endif

	error = bus_space_map(bst, AWIN_CORE_PBASE, AWIN_CORE_SIZE,
	    0, &awin_core_bsh);
	if (error)
		panic("%s: failed to map awin %s registers: %d",
		    __func__, "io", error);
	KASSERT(awin_core_bsh == iobase);

#ifdef ALLWINNER_A80
	error = bus_space_map(bst, AWIN_A80_CORE2_PBASE, AWIN_A80_CORE2_SIZE,
	    0, &awin_core2_bsh);
	if (error)
		panic("%s: failed to map awin %s registers: %d",
		    __func__, "core2", error);
	error = bus_space_map(bst, AWIN_A80_RCPUS_PBASE, AWIN_A80_RCPUS_SIZE,
	    0, &awin_rcpus_bsh);
	if (error)
		panic("%s: failed to map awin %s registers: %d",
		    __func__, "rcpus", error);
#endif

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
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		uint32_t s[4];
		unsigned int cpuno;
		for (cpuno = 0; cpuno < 4; cpuno++) {
			s[cpuno] = bus_space_read_4(bst, awin_core_bsh,
			    AWIN_A31_CPUCFG_OFFSET +
			    AWIN_A31_CPUCFG_STATUS_REG(cpuno));
		}
		printf("%s: cpu status: 0=%#x 1=%#x 2=%#x 3=%#x\n", __func__,
		    s[0], s[1], s[2], s[3]);
	} else if (awin_chip_id() == AWIN_CHIP_ID_A20) {
		uint32_t s0 = bus_space_read_4(bst, awin_core_bsh,
		    AWIN_CPUCFG_OFFSET + AWIN_CPUCFG_CPU0_STATUS_REG);
		uint32_t s1 = bus_space_read_4(bst, awin_core_bsh,
		    AWIN_CPUCFG_OFFSET + AWIN_CPUCFG_CPU1_STATUS_REG);
		printf("%s: cpu status: 0=%#x 1=%#x\n", __func__, s0, s1);
	}
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
	psize_t memsize;

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
#ifdef VERBOSE_INIT_ARM
		printf("memprobe not supported on A31\n");
#endif
		memsize = 0;
	} else {
		const uint32_t dcr = bus_space_read_4(&armv7_generic_bs_tag,
		    awin_core_bsh,
		    AWIN_DRAM_OFFSET + AWIN_DRAM_DCR_REG);

		memsize = (__SHIFTOUT(dcr, AWIN_DRAM_DCR_BUS_WIDTH) + 1)
		   / __SHIFTOUT(dcr, AWIN_DRAM_DCR_IO_WIDTH);
		memsize *= 1 << (__SHIFTOUT(dcr, AWIN_DRAM_DCR_CHIP_DENSITY)
		   + 28 - 3);
#ifdef VERBOSE_INIT_ARM
		printf("sdram_config = %#x, memsize = %uMB\n", dcr,
		    (u_int)(memsize >> 20));
#endif
	}
	return memsize;
}

uint16_t
awin_chip_id(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
#if defined(ALLWINNER_A80)
	bus_space_handle_t bsh = awin_core2_bsh;
#else
	bus_space_handle_t bsh = awin_core_bsh;
#endif
	static uint16_t chip_id = 0;
	uint32_t ver;

	if (!chip_id) {
		ver = bus_space_read_4(bst, bsh,
		    AWIN_SRAM_OFFSET + AWIN_SRAM_VER_REG);
		ver |= AWIN_SRAM_VER_R_EN;
		bus_space_write_4(bst, bsh,
		    AWIN_SRAM_OFFSET + AWIN_SRAM_VER_REG, ver);
		ver = bus_space_read_4(bst, bsh,
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
	case AWIN_CHIP_ID_A80: return "A80";
	default: return "unknown chip";
	}
}

void
awin_pll6_enable(void)
{
	KASSERT(awin_chip_id() != AWIN_CHIP_ID_A80);

	/*
	 * SATA needs PLL6 to be a 100MHz clock.
	 */
	const uint32_t ocfg = CCM_READ4(AWIN_PLL6_CFG_REG);

	/*
	 * Output freq is 24MHz * n * k / m / 6.
	 * To get to 100MHz, k & m must be equal and n must be 25.
	 */
	uint32_t ncfg = ocfg;
	ncfg &= ~(AWIN_PLL_CFG_BYPASS);
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		ncfg &= ~(AWIN_PLL_CFG_FACTOR_N|AWIN_PLL_CFG_FACTOR_K);
		ncfg |= __SHIFTIN(1, AWIN_PLL_CFG_FACTOR_K);
		ncfg |= __SHIFTIN(24, AWIN_PLL_CFG_FACTOR_N);
	} else {
		const u_int k = __SHIFTOUT(ocfg, AWIN_PLL_CFG_FACTOR_K);
		ncfg &= ~(AWIN_PLL_CFG_FACTOR_M|AWIN_PLL_CFG_FACTOR_N);
		ncfg |= __SHIFTIN(k, AWIN_PLL_CFG_FACTOR_M);
		ncfg |= __SHIFTIN(25, AWIN_PLL_CFG_FACTOR_N);
		ncfg |= AWIN_PLL6_CFG_SATA_CLK_EN;
	}
	ncfg |= AWIN_PLL_CFG_ENABLE;
	if (ncfg != ocfg) {
		CCM_WRITE4(AWIN_PLL6_CFG_REG, ncfg);

		if (awin_chip_id() == AWIN_CHIP_ID_A31) {
			do {
				ncfg = CCM_READ4(AWIN_PLL6_CFG_REG);
			} while ((ncfg & AWIN_A31_PLL6_CFG_LOCK) == 0);
		}
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
	/*
  	 * AC (at 48kHz) needs PLL2 to be 24576000 Hz
  	 */
	const uint32_t ocfg = CCM_READ4(AWIN_PLL2_CFG_REG);

	uint32_t ncfg = ocfg;

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		ncfg &= ~(AWIN_A31_PLL2_CFG_PREVDIV_M|
			  AWIN_A31_PLL2_CFG_FACTOR_N|
			  AWIN_A31_PLL2_CFG_POSTDIV_P);
		ncfg |= __SHIFTIN(20, AWIN_A31_PLL2_CFG_PREVDIV_M);
		ncfg |= __SHIFTIN(85, AWIN_A31_PLL2_CFG_FACTOR_N);
		ncfg |= __SHIFTIN(3, AWIN_A31_PLL2_CFG_POSTDIV_P);
		ncfg |= AWIN_PLL_CFG_ENABLE;
	} else {
		ncfg &= ~(AWIN_PLL2_CFG_PREVDIV|
			  AWIN_PLL2_CFG_FACTOR_N|
			  AWIN_PLL2_CFG_POSTDIV);
		ncfg |= __SHIFTIN(21, AWIN_PLL2_CFG_PREVDIV);
		ncfg |= __SHIFTIN(86, AWIN_PLL2_CFG_FACTOR_N);
		ncfg |= __SHIFTIN(4, AWIN_PLL2_CFG_POSTDIV);
		ncfg |= AWIN_PLL_CFG_ENABLE;
	}

	if (ncfg != ocfg) {
		CCM_WRITE4(AWIN_PLL2_CFG_REG, ncfg);

		if (awin_chip_id() == AWIN_CHIP_ID_A31) {
			do {
				ncfg = CCM_READ4(AWIN_PLL2_CFG_REG);
			} while ((ncfg & AWIN_A31_PLL2_CFG_LOCK) == 0);
		}
	}
}

void
awin_pll3_enable(void)
{
	/*
	 * HDMI needs PLL3 to be 29700000 Hz
	 */
	const uint32_t ocfg = CCM_READ4(AWIN_PLL3_CFG_REG);

	uint32_t ncfg = ocfg;

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		ncfg &= ~AWIN_A31_PLL3_CFG_MODE;
		ncfg &= ~AWIN_A31_PLL3_CFG_MODE_SEL;
		ncfg |= AWIN_A31_PLL3_CFG_FRAC_CLK_OUT;
		ncfg |= AWIN_PLL_CFG_ENABLE;
	} else {
		ncfg &= ~AWIN_PLL3_MODE_SEL;
		ncfg |= AWIN_PLL3_FRAC_SET;
		ncfg |= AWIN_PLL_CFG_ENABLE;
	}

	if (ncfg != ocfg) {
		CCM_WRITE4(AWIN_PLL3_CFG_REG, ncfg);

		if (awin_chip_id() == AWIN_CHIP_ID_A31) {
			do {
				ncfg = CCM_READ4(AWIN_PLL3_CFG_REG);
			} while ((ncfg & AWIN_A31_PLL3_CFG_LOCK) == 0);
		}
	}
}

void
awin_pll7_enable(void)
{
	/*
	 * HDMI needs PLL7 to be 29700000 Hz
	 */
	const uint32_t ocfg = CCM_READ4(AWIN_PLL7_CFG_REG);

	uint32_t ncfg = ocfg;

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		ncfg &= ~AWIN_A31_PLL7_CFG_MODE;
		ncfg &= ~AWIN_A31_PLL7_CFG_MODE_SEL;
		ncfg |= AWIN_A31_PLL7_CFG_FRAC_CLK_OUT;
		ncfg |= AWIN_PLL_CFG_ENABLE;
	} else {
		ncfg &= ~AWIN_PLL7_MODE_SEL;
		ncfg |= AWIN_PLL7_FRAC_SET;
		ncfg |= AWIN_PLL_CFG_ENABLE;
	}

	if (ncfg != ocfg) {
		CCM_WRITE4(AWIN_PLL7_CFG_REG, ncfg);

		if (awin_chip_id() == AWIN_CHIP_ID_A31) {
			do {
				ncfg = CCM_READ4(AWIN_PLL7_CFG_REG);
			} while ((ncfg & AWIN_A31_PLL7_CFG_LOCK) == 0);
		}
	}
}

void
awin_pll3_set_rate(uint32_t rate)
{
	const uint32_t ocfg = CCM_READ4(AWIN_PLL3_CFG_REG);

	uint32_t ncfg = ocfg;
	if (rate == 0) {
		ncfg &= ~AWIN_PLL_CFG_ENABLE;
	} else {
		if (awin_chip_id() == AWIN_CHIP_ID_A31) {
			unsigned int m = 8;
			unsigned int n = rate / (AWIN_REF_FREQ / m);
			ncfg |= AWIN_A31_PLL3_CFG_MODE_SEL;
			ncfg &= ~AWIN_A31_PLL3_CFG_FACTOR_N;
			ncfg |= __SHIFTIN(n - 1, AWIN_A31_PLL3_CFG_FACTOR_N);
			ncfg &= ~AWIN_A31_PLL3_CFG_PREDIV_M;
			ncfg |= __SHIFTIN(m - 1, AWIN_A31_PLL3_CFG_PREDIV_M);
		} else {
			unsigned int m = rate / 3000000;
			ncfg |= AWIN_PLL3_MODE_SEL;
			ncfg &= ~AWIN_PLL3_FACTOR_M;
			ncfg |= __SHIFTIN(m, AWIN_PLL3_FACTOR_M);
		}
		ncfg |= AWIN_PLL_CFG_ENABLE;
	}

	if (ncfg != ocfg) {
		CCM_WRITE4(AWIN_PLL3_CFG_REG, ncfg);

		if (awin_chip_id() == AWIN_CHIP_ID_A31) {
			do {
				ncfg = CCM_READ4(AWIN_PLL3_CFG_REG);
			} while ((ncfg & AWIN_A31_PLL3_CFG_LOCK) == 0);
		}
	}
}

uint32_t
awin_pll5x_get_rate(void)
{
	unsigned int n, k, p;

	KASSERT(awin_chip_id() != AWIN_CHIP_ID_A31);
	KASSERT(awin_chip_id() != AWIN_CHIP_ID_A80);

	const uint32_t cfg = CCM_READ4(AWIN_PLL5_CFG_REG);

	n = __SHIFTOUT(cfg, AWIN_PLL_CFG_FACTOR_N);
	k = __SHIFTOUT(cfg, AWIN_PLL_CFG_FACTOR_K) + 1;
	p = __SHIFTOUT(cfg, AWIN_PLL5_OUT_EXT_DIV_P);

	return (AWIN_REF_FREQ * n * k) >> p;
}

uint32_t
awin_pll6_get_rate(void)
{
	unsigned int n, k, m;

	KASSERT(awin_chip_id() != AWIN_CHIP_ID_A80);

	const uint32_t cfg = CCM_READ4(AWIN_PLL6_CFG_REG);

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		n = __SHIFTOUT(cfg, AWIN_PLL_CFG_FACTOR_N) + 1;
		k = __SHIFTOUT(cfg, AWIN_PLL_CFG_FACTOR_K) + 1;
		m = 2;
	} else {
		n = __SHIFTOUT(cfg, AWIN_PLL_CFG_FACTOR_N);
		k = __SHIFTOUT(cfg, AWIN_PLL_CFG_FACTOR_K) + 1;
		m = 2;
	}

	return (AWIN_REF_FREQ * n * k) / m;
}

uint32_t
awin_periph0_get_rate(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh = awin_core_bsh;
	unsigned int n, idiv, odiv;

	KASSERT(awin_chip_id() == AWIN_CHIP_ID_A80);

	const uint32_t cfg = bus_space_read_4(bst, bsh,
	    AWIN_A80_CCU_OFFSET + AWIN_A80_CCU_PLL_PERIPH0_CTRL_REG);

	n = __SHIFTOUT(cfg, AWIN_A80_CCU_PLL_PERIPH0_FACTOR_N);
	idiv = __SHIFTOUT(cfg, AWIN_A80_CCU_PLL_PERIPH0_INPUT_DIV) + 1;
	odiv = __SHIFTOUT(cfg, AWIN_A80_CCU_PLL_PERIPH0_OUTPUT_DIV) + 1;

	return ((AWIN_REF_FREQ * n) / idiv) / odiv;
}
