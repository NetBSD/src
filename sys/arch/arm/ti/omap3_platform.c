/* $NetBSD: omap3_platform.c,v 1.8 2023/04/07 08:55:30 skrll Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap3_platform.c,v 1.8 2023/04/07 08:55:30 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>

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
#define	 PRM_RSTCTRL		(OMAP3_PRCM_GR_BASE + 0x50)
#define	  PRM_RSTCTRL_RST_DPLL3	__BIT(2)

#define	OMAP3_32KTIMER_BASE	0x48320000
#define	 REG_32KSYNCNT_CR	(OMAP3_32KTIMER_BASE + 0x10)

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

static void
omap3_platform_device_register(device_t self, void *aux)
{
}

static u_int
omap3_platform_uart_freq(void)
{
	return 48000000U;
}

static void
omap3_platform_reset(void)
{
	volatile uint32_t *rstctrl =
	    (volatile uint32_t *)omap3_phystovirt(PRM_RSTCTRL);

	*rstctrl |= PRM_RSTCTRL_RST_DPLL3;

	for (;;)
		__asm("wfi");
}

static void
omap3_platform_delay(u_int n)
{
	volatile uint32_t *cr =
	    (volatile uint32_t *)omap3_phystovirt(REG_32KSYNCNT_CR);
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
