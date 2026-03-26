/* $NetBSD $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yuri Honegger.
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

/*
 * Platform code for the TI AM18XX family of SOCs (AM1808, AM1810). In linux
 * land and in the device trees, this platform is sometimes also called DA830
 * and DA850 (Davinci 8XX) because their silicon has a lot in common.
 */

#include "opt_console.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD $");

#include <uvm/uvm_extern.h>

#include <dev/fdt/fdt_platform.h>
#include <dev/fdt/fdtvar.h>
#include <dev/ic/comreg.h>

#include <arch/evbarm/fdt/platform.h>
#include <arm/fdt/arm_fdtvar.h>

#define AM18XX_IO_VBASE KERNEL_IO_VBASE
#define AM18XX_IO_PBASE 0x01c00000
#define AM18XX_IO_SIZE 0x00400000
#define AM18XX_INTC_VBASE (AM18XX_IO_VBASE + AM18XX_IO_SIZE)
#define AM18XX_INTC_PBASE 0xfffee000
#define AM18XX_INTC_SIZE 0x2000

#define AM18XX_TIMER1_BASE 0x01C21000
#define AM18XX_TIMER1_SIZE 0x1000
#define AM18XX_TIMER1_TIM12 0x10
#define AM18XX_TIMER1_TIM34 0x14
#define AM18XX_TIMER1_PRD12 0x18
#define AM18XX_TIMER1_PRD34 0x1C
#define AM18XX_TIMER1_TCR 0x20
#define AM18XX_TIMER1_TGCR 0x24
#define AM18XX_TIMER1_WDTCR 0x28

#define AM18XX_TIMER_TCR_ENAMODE12_CONTINUOUS 0x80
#define AM18XX_TIMER_TGCR_TIMMODE32_UNCHAINED 0x4
#define AM18XX_TIMER_TGCR_TIMMODE64_WATCHDOG 0x8
#define AM18XX_TIMER_TGCR_TIM12EN 1
#define AM18XX_TIMER_TGCR_TIM34EN 2
#define AM18XX_TIMER_WDTCR_WDEN 1
#define AM18XX_TIMER_WDTCR_KEY0 0xa5c60000
#define AM18XX_TIMER_WDTCR_KEY1 0xda7e0000


void am18xx_platform_early_putchar(char);

extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern struct bus_space arm_generic_bs_tag;

void __noasan
am18xx_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA (CONSADDR - AM18XX_IO_PBASE + KERNEL_IO_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p()
					  ? (volatile uint32_t *)CONSADDR_VA
					  : (volatile uint32_t *)CONSADDR;

	while ((le32toh(uartaddr[com_lsr]) & LSR_TXRDY) == 0)
		continue;

	uartaddr[com_data] = htole32(c);
#endif
}


static const struct pmap_devmap *
am18xx_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		/* input/output registers */
		DEVMAP_ENTRY(AM18XX_IO_VBASE,
			     AM18XX_IO_PBASE,
			     AM18XX_IO_SIZE),
		/* interrupt controller */
		DEVMAP_ENTRY(AM18XX_INTC_VBASE,
			     AM18XX_INTC_PBASE,
			     AM18XX_INTC_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
am18xx_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &arm_generic_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

static void
am18xx_platform_delay(u_int n)
{
	/* Use Timer1 for delay. Timer0 is used*/
	static bus_space_tag_t bst = &arm_generic_bs_tag;
	static bus_space_handle_t bsh = 0;

	if (bsh == 0) {
		/* map Timer1 */
		bus_space_map(bst, AM18XX_TIMER1_BASE, AM18XX_TIMER1_SIZE,
			      0, &bsh);

		/* disable counter to allow changing mode */
		bus_space_write_4(bst, bsh, AM18XX_TIMER1_TCR, 0);
		/* set mode to 32-bit unchained */
		bus_space_write_4(bst, bsh, AM18XX_TIMER1_TGCR,
				  AM18XX_TIMER_TGCR_TIMMODE32_UNCHAINED |
				  AM18XX_TIMER_TGCR_TIM12EN);
		/* load period registers with maximum period */
		bus_space_write_4(bst, bsh, AM18XX_TIMER1_PRD12, 0xFFFFFFFF);
		/* enable timer */
		bus_space_write_4(bst, bsh, AM18XX_TIMER1_TCR,
				  AM18XX_TIMER_TCR_ENAMODE12_CONTINUOUS);

	}

	/*
	 * The counter is driven by PLL0_AUXCLK, which is taken from OSCIN.
	 * On the EV3, that is 24MHz.
	 *
	 * n is in microseconds (us)
	 *
	 * TODO: The frequency is board-dependent and we should get it from the
	 * device tree.
	 */
	long ticks = n * (24000000 / 1000000);

	uint32_t prev, cur;
	prev = bus_space_read_4(bst, bsh, AM18XX_TIMER1_TIM12);
	while (ticks > 0) {
		cur = bus_space_read_4(bst, bsh, AM18XX_TIMER1_TIM12);
		if (cur >= prev)
			ticks -= (cur - prev);
		else
			ticks -= (UINT32_MAX - cur + prev);
		prev = cur;
	}
}

/*
 * To reset the AM1808, you have to crash the watchdog.
 */
static void
am18xx_platform_reset(void)
{
	/* map TIMER1 */
	bus_space_tag_t bst = &arm_generic_bs_tag;
	bus_space_handle_t bsh = 0;
	bus_space_map(bst, AM18XX_TIMER1_BASE, AM18XX_TIMER1_SIZE, 0, &bsh);

	/* disable counter to allow changing mode */
	bus_space_write_4(bst, bsh, AM18XX_TIMER1_TCR, 0);
	/* set mode to watchdog unchained */
	bus_space_write_4(bst, bsh, AM18XX_TIMER1_TGCR,
			  AM18XX_TIMER_TGCR_TIMMODE64_WATCHDOG |
			  AM18XX_TIMER_TGCR_TIM12EN |
			  AM18XX_TIMER_TGCR_TIM34EN);
	/* set counter and reload registers */
	bus_space_write_4(bst, bsh, AM18XX_TIMER1_TIM12, 0);
	bus_space_write_4(bst, bsh, AM18XX_TIMER1_TIM34, 0);
	bus_space_write_4(bst, bsh, AM18XX_TIMER1_PRD12, 0);
	bus_space_write_4(bst, bsh, AM18XX_TIMER1_PRD34, 0);

	/* execute the watchdog enable sequence */
	bus_space_write_4(bst, bsh, AM18XX_TIMER1_WDTCR,
			  AM18XX_TIMER_WDTCR_KEY0 | AM18XX_TIMER_WDTCR_WDEN);
	bus_space_write_4(bst, bsh, AM18XX_TIMER1_WDTCR,
			  AM18XX_TIMER_WDTCR_KEY1 | AM18XX_TIMER_WDTCR_WDEN);
	/* trigger a reset by writing an invalid value to WDTCR*/
	bus_space_write_4(bst, bsh, AM18XX_TIMER1_WDTCR, 0xaffe);

	/* NOTREACHED */
}

static const struct fdt_platform am18xx_platform = {
	.fp_devmap = am18xx_platform_devmap,
	.fp_bootstrap = arm_fdt_cpu_bootstrap,
	.fp_init_attach_args = am18xx_platform_init_attach_args,
	.fp_uart_freq = NULL,
	.fp_delay = am18xx_platform_delay,
	.fp_reset = am18xx_platform_reset,
};

/* The device tree name for the AM1808 is da850 (davinci 850) */
FDT_PLATFORM(am18xx, "ti,da850", &am18xx_platform);
