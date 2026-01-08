/* $NetBSD: omap3_platform.c,v 1.11 2026/01/08 00:26:38 christos Exp $ */

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

#include "opt_soc.h"
#include "opt_console.h"

#include "arml2cc.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap3_platform.c,v 1.11 2026/01/08 00:26:38 christos Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_platform.h>

#include <arm/fdt/arm_fdtvar.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>
#include <arm/cpufunc.h>

#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>

#include <evbarm/fdt/platform.h>
#include <evbarm/fdt/machdep.h>

#include <net/if_ether.h>

#include <libfdt.h>

#include <arm/cortex/pl310_var.h>
#include <arm/cortex/scu_reg.h>

#include <arm/ti/smc.h>

#define	OMAP3_L4_CORE_VBASE	KERNEL_IO_VBASE
#define	OMAP3_L4_CORE_PBASE	0x48000000
#define	OMAP3_L4_CORE_SIZE	0x00100000

#define	OMAP3_L4_WKUP_VBASE	(OMAP3_L4_CORE_VBASE + OMAP3_L4_CORE_SIZE)
#define	OMAP3_L4_WKUP_PBASE	0x48300000
#define	OMAP3_L4_WKUP_SIZE	0x00100000

#define	OMAP3_L4_PER_VBASE	(OMAP3_L4_WKUP_VBASE + OMAP3_L4_WKUP_SIZE)
#define	OMAP3_L4_PER_PBASE	0x49000000
#define	OMAP3_L4_PER_SIZE	0x00100000

#define	OMAP3_PRCM_BASE		0x48306000
#define	OMAP3_PRCM_GR_BASE	(OMAP3_PRCM_BASE + 0x1200)
#define	 OMAP3_PRM_RSTCTRL	(OMAP3_PRCM_GR_BASE + 0x50)
#define	  OMAP3_PRM_RSTCTRL_RST_DPLL3	__BIT(2)

#define	OMAP3_32KTIMER_BASE	0x48320000
#define	 OMAP3_REG_32KSYNCNT_CR	(OMAP3_32KTIMER_BASE + 0x10)

#define	OMAP4_L4_PER_VBASE	KERNEL_IO_VBASE
#define	OMAP4_L4_PER_PBASE	0x48000000
#define	OMAP4_L4_PER_SIZE	0x01000000

#define	OMAP4_L4_ABE_VBASE	(OMAP4_L4_PER_VBASE + OMAP4_L4_PER_SIZE)
#define	OMAP4_L4_ABE_PBASE	0x49000000
#define	OMAP4_L4_ABE_SIZE	0x01000000

#define	OMAP4_L4_CFG_VBASE	(OMAP4_L4_ABE_VBASE + OMAP4_L4_ABE_SIZE)
#define	OMAP4_L4_CFG_PBASE	0x4a000000
#define	OMAP4_L4_CFG_SIZE	0x01000000

#define	OMAP4_SCU_BASE		0x48240000
#define	OMAP4_PL310_BASE	0x48242000
#define	OMAP4_WUGEN_BASE	0x48281000
#define	 OMAP4_AUX_CORE_BOOT0	0x800
#define	 OMAP4_AUX_CORE_BOOT1	0x804

#define	OMAP4_CM1_BASE		0x4a004000
#define	OMAP4_CKGEN_CM1_BASE	(OMAP4_CM1_BASE + 0x100)
#define	 OMAP4_CLKSEL_DPLL_MPU	(OMAP4_CKGEN_CM1_BASE + 0x6c)
#define	 OMAP4_DIV_M2_DPLL_MPU	(OMAP4_CKGEN_CM1_BASE + 0x70)

#define	OMAP4_PRM_BASE		0x4a306000
#define	OMAP4_CKGEN_PRM_BASE	(OMAP4_PRM_BASE + 0x100)
#define	 OMAP4_SYS_CLKSEL	(OMAP4_CKGEN_PRM_BASE + 0x10)
#define	  OMAP4_SYS_CLKSEL_CLKIN	__BITS(2,0)
#define	OMAP4_DEVICE_PRM_BASE	(OMAP4_PRM_BASE + 0x1b00)
#define	 OMAP4_PRM_RSTCTRL	OMAP4_DEVICE_PRM_BASE
#define	  OMAP4_RST_GLOBAL_COLD		__BIT(1)

#define	OMAP4_32KTIMER_BASE	0x4a304000
#define	 OMAP4_REG_32KSYNCNT_CR	(OMAP4_32KTIMER_BASE + 0x10)

#define	OMAP4_DPLL_MULT		__BITS(18,8)
#define	OMAP4_DPLL_DIV		__BITS(6,0)
#define	OMAP4_DPLL_CLKOUT_DIV	__BITS(4,0)
#define	OMAP4_SYS_CLKSEL_FREQS	{ 0, 12000, 0, 16800, 19200, 26000, 0, 38400 }

static inline vaddr_t
omap3_phystovirt(paddr_t pa)
{
	if (pa >= OMAP3_L4_CORE_PBASE &&
	    pa < OMAP3_L4_CORE_PBASE + OMAP3_L4_CORE_SIZE)
		return (pa - OMAP3_L4_CORE_PBASE) + OMAP3_L4_CORE_VBASE;

	if (pa >= OMAP3_L4_WKUP_PBASE &&
	    pa < OMAP3_L4_WKUP_PBASE + OMAP3_L4_WKUP_SIZE)
		return (pa - OMAP3_L4_WKUP_PBASE) + OMAP3_L4_WKUP_VBASE;

	if (pa >= OMAP3_L4_PER_PBASE &&
	    pa < OMAP3_L4_PER_PBASE + OMAP3_L4_PER_SIZE)
		return (pa - OMAP3_L4_PER_PBASE) + OMAP3_L4_PER_VBASE;

	panic("%s: pa %#x not in devmap", __func__, (uint32_t)pa);
}

#define	OMAP3_PHYSTOVIRT(pa)	\
	(((pa) - OMAP3_L4_CORE_VBASE) + OMAP3_L4_CORE_PBASE)

static inline vaddr_t
omap4_phystovirt(paddr_t pa)
{
	if (pa >= OMAP4_L4_PER_PBASE &&
	    pa < OMAP4_L4_PER_PBASE + OMAP4_L4_PER_SIZE)
		return (pa - OMAP4_L4_PER_PBASE) + OMAP4_L4_PER_VBASE;

	if (pa >= OMAP4_L4_ABE_PBASE &&
	    pa < OMAP4_L4_ABE_PBASE + OMAP4_L4_ABE_SIZE)
		return (pa - OMAP4_L4_ABE_PBASE) + OMAP4_L4_ABE_VBASE;

	if (pa >= OMAP4_L4_CFG_PBASE &&
	    pa < OMAP4_L4_CFG_PBASE + OMAP4_L4_CFG_SIZE)
		return (pa - OMAP4_L4_CFG_PBASE) + OMAP4_L4_CFG_VBASE;

	panic("%s: pa %#x not in devmap", __func__, (uint32_t)pa);
}

extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern struct bus_space arm_generic_bs_tag;

static const struct pmap_devmap *
omap3_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(OMAP3_L4_CORE_VBASE,
			     OMAP3_L4_CORE_PBASE,
			     OMAP3_L4_CORE_SIZE),
		DEVMAP_ENTRY(OMAP3_L4_WKUP_VBASE,
			     OMAP3_L4_WKUP_PBASE,
			     OMAP3_L4_WKUP_SIZE),
		DEVMAP_ENTRY(OMAP3_L4_PER_VBASE,
			     OMAP3_L4_PER_PBASE,
			     OMAP3_L4_PER_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static const struct pmap_devmap *
omap4_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(OMAP4_L4_PER_VBASE,
			     OMAP4_L4_PER_PBASE,
			     OMAP4_L4_PER_SIZE),
		DEVMAP_ENTRY(OMAP4_L4_ABE_VBASE,
			     OMAP4_L4_ABE_PBASE,
			     OMAP4_L4_ABE_SIZE),
		DEVMAP_ENTRY(OMAP4_L4_CFG_VBASE,
			     OMAP4_L4_CFG_PBASE,
			     OMAP4_L4_CFG_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
omap3_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &arm_generic_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

void omap3_platform_early_putchar(char);

void __noasan
omap3_platform_early_putchar(char c)
{
#ifdef CONSADDR
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)omap3_phystovirt(CONSADDR):
	    (volatile uint32_t *)CONSADDR;

	while ((le32toh(uartaddr[com_lsr]) & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = htole32(c);
#endif
}

void omap4_platform_early_putchar(char);

void __noasan
omap4_platform_early_putchar(char c)
{
#ifdef CONSADDR
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)omap4_phystovirt(CONSADDR):
	    (volatile uint32_t *)CONSADDR;

	while ((le32toh(uartaddr[com_lsr]) & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = htole32(c);
#endif
}

static void
omap3_platform_device_register(device_t self, void *aux)
{
}

static u_int
omap3_platform_uart_freq(void)
{
	return 48000000U;
}

#if NARML2CC > 0
static void
omap4_enable_ll2c(bool enable)
{
	omap_smc(enable ? 1 : 0, OMAP4_CMD_L2X0_CTRL);
}
#endif

static void
omap4_platform_bootstrap(void)
{
	uint32_t sys_clksel, sys_clk, dpll1, dpll2, m, n, m2;
	u_int clksel;

	static const uint32_t clksel_freqs[] = OMAP4_SYS_CLKSEL_FREQS;
	sys_clksel = *(volatile uint32_t *)omap4_phystovirt(OMAP4_SYS_CLKSEL);
	clksel = __SHIFTOUT(sys_clksel, OMAP4_SYS_CLKSEL_CLKIN);
	sys_clk = clksel_freqs[clksel];
	dpll1 = *(volatile uint32_t *)omap4_phystovirt(OMAP4_CLKSEL_DPLL_MPU);
	dpll2 = *(volatile uint32_t *)omap4_phystovirt(OMAP4_DIV_M2_DPLL_MPU);
	m = __SHIFTOUT(dpll1, OMAP4_DPLL_MULT);
	n = __SHIFTOUT(dpll1, OMAP4_DPLL_DIV);
	m2 = __SHIFTOUT(dpll2, OMAP4_DPLL_CLKOUT_DIV);

	curcpu()->ci_data.cpu_cc_freq = ((sys_clk * 2 * m) / ((n + 1) * m2)) * 1000 / 2;
#if NARML2CC > 0
	bus_space_tag_t bst = &arm_generic_bs_tag;
	const bus_space_handle_t pl310_bsh = OMAP4_PL310_BASE
	    + OMAP4_L4_PER_VBASE - OMAP4_L4_PER_PBASE;
	arml2cc_get_cacheinfo(bst, pl310_bsh, 0);
	arml2cc_set_enable_func(omap4_enable_ll2c);
#endif
	arm_fdt_cpu_bootstrap();
}

static void
omap3_platform_reset(void)
{
	volatile uint32_t *rstctrl =
	    (volatile uint32_t *)omap3_phystovirt(OMAP3_PRM_RSTCTRL);

	*rstctrl |= OMAP3_PRM_RSTCTRL_RST_DPLL3;

	for (;;)
		__asm("wfi");
}

static void
omap4_platform_reset(void)
{
	volatile uint32_t *rstctrl =
	    (volatile uint32_t *)omap4_phystovirt(OMAP4_PRM_RSTCTRL);

	*rstctrl |= OMAP4_RST_GLOBAL_COLD;

	for (;;)
		__asm("wfi");
}

static void
omap3_platform_delay(u_int n)
{
	volatile uint32_t *cr =
	    (volatile uint32_t *)omap3_phystovirt(OMAP3_REG_32KSYNCNT_CR);
	uint32_t cur, prev;

	long ticks = howmany(n * 32768, 1000000);
	prev = *cr;
	while (ticks > 0) {
		cur = *cr;
		if (cur >= prev)
			ticks -= (cur - prev);
		else
			ticks -= (UINT32_MAX - cur + prev);
		prev = cur;
	}
}

static void
omap4_platform_delay(u_int n)
{
	volatile uint32_t *cr =
	    (volatile uint32_t *)omap4_phystovirt(OMAP4_REG_32KSYNCNT_CR);
	uint32_t cur, prev;

	long ticks = howmany(n * 32768, 1000000);
	prev = *cr;
	while (ticks > 0) {
		cur = *cr;
		if (cur >= prev)
			ticks -= (cur - prev);
		else
			ticks -= (UINT32_MAX - cur + prev);
		prev = cur;
	}
}

#ifdef MULTIPROCESSOR
static int
omap4_platform_mpstart(void)
{
	bus_space_tag_t bst = &arm_generic_bs_tag;
	uint32_t val;

	const bus_space_handle_t scu_bsh = OMAP4_SCU_BASE
	    + OMAP4_L4_PER_VBASE - OMAP4_L4_PER_PBASE;

	/*
	 * Invalidate all SCU cache tags. That is, for all cores (0-3)
	 */
	bus_space_write_4(bst, scu_bsh, SCU_INV_ALL_REG, 0xffff);

	val = bus_space_read_4(bst, scu_bsh, SCU_DIAG_CONTROL);
	val |= SCU_DIAG_DISABLE_MIGBIT;
	bus_space_write_4(bst, scu_bsh, SCU_DIAG_CONTROL, val);

	val = bus_space_read_4(bst, scu_bsh, SCU_CTL);
	val |= SCU_CTL_SCU_ENA;
	bus_space_write_4(bst, scu_bsh, SCU_CTL, val);

	armv7_dcache_wbinv_all();

	const bus_space_handle_t wugen_bsh = OMAP4_WUGEN_BASE
	    + OMAP4_L4_PER_VBASE - OMAP4_L4_PER_PBASE;
	const paddr_t mpstart = KERN_VTOPHYS((vaddr_t)cpu_mpstart);

	bus_space_write_4(bst, wugen_bsh, OMAP4_AUX_CORE_BOOT1, mpstart);

	for (size_t i = 1; i < arm_cpu_max; i++) {
		val = bus_space_read_4(bst, wugen_bsh, OMAP4_AUX_CORE_BOOT0);
		val |= __SHIFTIN(0xf, i * 4);
		bus_space_write_4(bst, wugen_bsh, OMAP4_AUX_CORE_BOOT0, val);
	}

	dsb(sy);
	sev();

	u_int hatched = 0;
	for (u_int cpuindex = 1; cpuindex < arm_cpu_max; cpuindex++) {
		/* Wait for AP to start */
		u_int i;
		for (i = 1500000; i > 0; i--) {
			if (cpu_hatched_p(cpuindex)) {
				hatched |= __BIT(cpuindex);
				break;
			}
		}

		if (i == 0) {
			aprint_error("cpu%d: WARNING: AP failed to start\n", cpuindex);
			return EIO;
		}
	}

	return 0;
}
#endif

static const struct fdt_platform omap3_platform = {
	.fp_devmap = omap3_platform_devmap,
	.fp_bootstrap = arm_fdt_cpu_bootstrap,
	.fp_init_attach_args = omap3_platform_init_attach_args,
	.fp_device_register = omap3_platform_device_register,
	.fp_reset = omap3_platform_reset,
	.fp_delay = omap3_platform_delay,
	.fp_uart_freq = omap3_platform_uart_freq,
};

FDT_PLATFORM(omap3, "ti,omap3", &omap3_platform);

static const struct fdt_platform omap4_platform = {
	.fp_devmap = omap4_platform_devmap,
	.fp_bootstrap = omap4_platform_bootstrap,
	.fp_init_attach_args = omap3_platform_init_attach_args,
	.fp_device_register = omap3_platform_device_register,
	.fp_reset = omap4_platform_reset,
	.fp_delay = omap4_platform_delay,
	.fp_uart_freq = omap3_platform_uart_freq,
#ifdef MULTIPROCESSOR
	.fp_mpstart = omap4_platform_mpstart,
#endif
};

FDT_PLATFORM(omap4, "ti,omap4", &omap4_platform);
