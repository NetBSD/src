/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: ar7100.c,v 1.2.30.1 2015/09/22 12:05:46 skrll Exp $");

#define	__INTR_PRIVATE
#include "pci.h"
#include "opt_memsize.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/cpu.h>

#include <prop/proplib.h>

#include <mips/cpuregs.h>
#include <mips/atheros/include/platform.h>
#include <mips/atheros/include/ar9344reg.h>

#define	AR7100_BASE_FREQ	(40*1000*1000)

static const char * const ar7100_cpu_intrnames[] = {
	[AR7100_CPU_IRQ_PCI] =		"irq 0 (pci)",
	[ARCHIP_CPU_IRQ_USB] =		"irq 1 (usb)",
	[ARCHIP_CPU_IRQ_GMAC0] =	"irq 2 (gmac0)",
	[ARCHIP_CPU_IRQ_GMAC1] =	"irq 3 (gmac1)",
	[ARCHIP_CPU_IRQ_MISC] =		"irq 4 (misc)",
	[ARCHIP_CPU_IRQ_TIMER] =	"irq 5 (timer)",
};

static const char * const ar7100_misc_intrnames[] = {
	[AR7100_MISC_IRQ_TIMER] =	"irq 0 (timer)",
	[AR7100_MISC_IRQ_ERROR] =	"irq 1 (error)",
	[AR7100_MISC_IRQ_GPIO] =	"irq 2 (gpio)",
	[AR7100_MISC_IRQ_UART0] =	"irq 3 (uart)",
	[AR7100_MISC_IRQ_WDOG] =	"irq 4 (wdog)",
	[AR7100_MISC_IRQ_PC] =	       	"irq 5 (pc)",
	[AR7100_MISC_IRQ_OHCI] =	"irq 6 (ochi)",
	[AR7100_MISC_IRQ_DMA] =		"irq 7 (dma)",
};

static const struct ipl_sr_map	ar7100_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTBIO] =		MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK,
	[IPL_SOFTSERIAL] =	MIPS_SOFT_INT_MASK,
	[IPL_BIO] =		MIPS_SOFT_INT_MASK |
				MIPS_INT_MASK_0 |	/* PCI */
				MIPS_INT_MASK_1,	/* USB */
	[IPL_NET] =		MIPS_SOFT_INT_MASK |
				MIPS_INT_MASK_0 |	/* PCI */
				MIPS_INT_MASK_1	|	/* USB */
				MIPS_INT_MASK_2 |	/* GMAC0 */
				MIPS_INT_MASK_3,	/* GMAC1 */
	[IPL_SERIAL] =		MIPS_SOFT_INT_MASK |
				MIPS_INT_MASK_0 |	/* PCIE RC */
				MIPS_INT_MASK_1	|	/* USB */
				MIPS_INT_MASK_2 |	/* GMAC0 */
				MIPS_INT_MASK_3 |	/* GMAC1 */
				MIPS_INT_MASK_4,	/* MISC (UART0/1) */

	[IPL_SCHED] =		MIPS_INT_MASK,		/* EVERYTHING */
	[IPL_DDB] =		MIPS_INT_MASK,		/* EVERYTHING */
	[IPL_HIGH] =		MIPS_INT_MASK,		/* EVERYTHING */
    },
};

static void
ar7100_intr_init(void)
{
	atheros_intr_init();
}

static uint32_t
ar7100_get_memsize(void)
{
#ifdef MEMSIZE
	return MEMSIZE;
#else
	return 128*1024*1024;
#endif
}

static void
ar7100_wdog_reload(uint32_t period)
{

	if (period == 0) {
		PUTRESETREG(ARCHIP_RESET_WDOG_CTL, ARCHIP_WDOG_CTL_IGNORE);
		PUTRESETREG(ARCHIP_RESET_WDOG_TIMER, 0);
	} else {
		PUTRESETREG(ARCHIP_RESET_WDOG_TIMER, period);
		PUTRESETREG(ARCHIP_RESET_WDOG_CTL, ARCHIP_WDOG_CTL_RESET);
	}
}

static void
ar7100_bus_init(void)
{
	printf("resetctl = %#x\n", GETRESETREG(AR7100_RESET_RESETCTL));
#if NPCI > 0 && 0
	REGVAL(ARCHIP_DDR_BASE + AR7100_PCI_WINDOW_0) = 0x10000000;
	REGVAL(ARCHIP_DDR_BASE + AR7100_PCI_WINDOW_1) = 0x11000000;
	REGVAL(ARCHIP_DDR_BASE + AR7100_PCI_WINDOW_2) = 0x12000000;
	REGVAL(ARCHIP_DDR_BASE + AR7100_PCI_WINDOW_3) = 0x13000000;
	REGVAL(ARCHIP_DDR_BASE + AR7100_PCI_WINDOW_4) = 0x14000000;
	REGVAL(ARCHIP_DDR_BASE + AR7100_PCI_WINDOW_5) = 0x15000000;
	REGVAL(ARCHIP_DDR_BASE + AR7100_PCI_WINDOW_6) = 0x16000000;
	REGVAL(ARCHIP_DDR_BASE + AR7100_PCI_WINDOW_7) = 0x07000000;
#endif
}

static void
ar7100_reset(void)
{
	PUTRESETREG(AR7100_RESET_RESETCTL, ARCHIP_RESETCTL_FULL_CHIP_RESET);
}

static void
ar7100_get_freqs(struct arfreqs *freqs)
{
	const uint32_t pll = GETPLLREG(ARCHIP_PLL_CPU_PLL_CONFIG);
	const uint32_t ref_freq = AR7100_BASE_FREQ;

	const uint32_t pll_freq =
	    ref_freq * (__SHIFTOUT(pll, AR7100_PLL_PLL_FB) + 1);

	const uint32_t cpu_freq =
	    pll_freq / (__SHIFTOUT(pll, AR7100_CPU_PLL_CPU_DIV_SEL) + 1);

	const uint32_t ddr_freq =
	    pll_freq / (__SHIFTOUT(pll, AR7100_CPU_PLL_DDR_DIV_SEL) + 1);

	const uint32_t bus_freq =
	    cpu_freq / ((__SHIFTOUT(pll, AR7100_CPU_PLL_AHB_DIV) + 1) * 2);

	freqs->freq_bus = bus_freq;
	freqs->freq_cpu = cpu_freq;
	freqs->freq_mem = ddr_freq;
	freqs->freq_pll = pll_freq;
	freqs->freq_ref = ref_freq;
}

static void
ar7100_device_register(device_t self, void *aux)
{
	if (device_is_a(self, "com")) {
		prop_number_t pn =
		    prop_number_create_unsigned_integer(atheros_get_bus_freq());
		KASSERT(pn != NULL);
		prop_dictionary_set(device_properties(self), "frequency", pn);
		prop_object_release(pn);
	}
}

static int
ar7100_enable_device(const struct atheros_device *adv)
{
#if 0
	if (adv->adv_mask && ((adv->adv_mask & info->config) == 0)) {
		return -1;
	}
#endif
	if (adv->adv_reset) {
		/* put device into reset */
		PUTRESETREG(AR7100_RESET_RESETCTL,
		    GETRESETREG(AR7100_RESET_RESETCTL) | adv->adv_reset);

		delay(15000);	/* XXX: tsleep? */

		/* take it out of reset */
		PUTRESETREG(AR7100_RESET_RESETCTL,
		    GETRESETREG(AR7100_RESET_RESETCTL) & ~adv->adv_reset);

		delay(25);
	}
	if (adv->adv_enable) {
		panic("%s: %s: enable not supported!", __func__, adv->adv_name);
#if 0
		PUTRESETREG(AR7100_RESET_ENABLE,
		    GETRESETREG(AR7100_RESET_ENABLE) | adv->adv_enable);
#endif
	}
	return 0;
}

static const struct atheros_device ar7100_devices[] = {
	{
		.adv_name = "com",
		.adv_addr = AR7100_UART_BASE,
		.adv_size = 0x1000,
		.adv_cirq = ARCHIP_CPU_IRQ_MISC,
		.adv_mirq = AR7100_MISC_IRQ_UART0,
	}, {
		.adv_name = "arpci",
		.adv_addr = AR7100_PCI_BASE,
		.adv_size = 0x2000,
		.adv_cirq = AR7100_CPU_IRQ_PCI,
		.adv_mirq = -1,
		.adv_reset = AR7100_RESETCTL_PCI_CORE_RESET,
	}, {
		.adv_name = "ohci",
		.adv_addr = AR7100_OHCI_BASE,
		.adv_size = 0x2000,
		.adv_cirq = ARCHIP_CPU_IRQ_USB,
		.adv_mirq = -1,
	}, {
		.adv_name = "ehci",
		.adv_addr = AR7100_EHCI_BASE,
		.adv_size = 0x2000,
		.adv_cirq = ARCHIP_CPU_IRQ_USB,
		.adv_mirq = -1,
	}, {
		.adv_name = "age",
		.adv_addr = AR7100_GMAC0_BASE,
		.adv_size = 0x2000,
		.adv_cirq = ARCHIP_CPU_IRQ_GMAC0,
		.adv_mirq = -1,
	}, {
		.adv_name = "age",
		.adv_addr = AR7100_GMAC1_BASE,
		.adv_size = 0x2000,
		.adv_cirq = ARCHIP_CPU_IRQ_GMAC1,
		.adv_mirq = -1,
	}, {
		.adv_name = NULL,
	}
};

const struct atheros_platformsw ar7100_platformsw = {
	.apsw_intrsw = &atheros_intrsw,
	.apsw_intr_init = ar7100_intr_init,
	.apsw_cpu_intrnames = ar7100_cpu_intrnames,
	.apsw_misc_intrnames = ar7100_misc_intrnames,
	.apsw_cpu_nintrs = __arraycount(ar7100_cpu_intrnames),
	.apsw_misc_nintrs = __arraycount(ar7100_misc_intrnames),
	.apsw_cpuirq_misc = ARCHIP_CPU_IRQ_MISC,
	.apsw_ipl_sr_map = &ar7100_ipl_sr_map,

	.apsw_revision_id_addr = ARCHIP_RESET_BASE + ARCHIP_RESET_REVISION,
	.apsw_uart0_base = AR7100_UART_BASE,
	.apsw_misc_intstat = ARCHIP_RESET_BASE + ARCHIP_RESET_MISC_INTSTAT,
	.apsw_misc_intmask = ARCHIP_RESET_BASE + ARCHIP_RESET_MISC_INTMASK,

	/*
	 * CPU specific routines.
	 */
	.apsw_get_memsize = ar7100_get_memsize,
	.apsw_wdog_reload = ar7100_wdog_reload,
	.apsw_bus_init = ar7100_bus_init,
	.apsw_reset = ar7100_reset,

	.apsw_get_freqs = ar7100_get_freqs,
	.apsw_device_register = ar7100_device_register,
	.apsw_enable_device = ar7100_enable_device,
	.apsw_devices = ar7100_devices,
};
