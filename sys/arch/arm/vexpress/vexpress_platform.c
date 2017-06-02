/* $NetBSD: vexpress_platform.c,v 1.1 2017/06/02 15:22:47 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_multiprocessor.h"
#include "opt_fdt_arm.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vexpress_platform.c,v 1.1 2017/06/02 15:22:47 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>
#include <arm/cpufunc.h>

#include <arm/fdt/arm_fdtvar.h>

#include <arm/cortex/gtmr_var.h>

#include <evbarm/dev/plcomvar.h>

#define	VEXPRESS_REF_FREQ	24000000

#define	VEXPRESS_CORE_VBASE	0xf0000000
#define	VEXPRESS_CORE_PBASE	0x10000000
#define	VEXPRESS_CORE_SIZE	0x10000000

#define	DEVMAP_ALIGN(a)	((a) & ~L1_S_OFFSET)
#define	DEVMAP_SIZE(s)	roundup2((s), L1_S_SIZE)
#define	DEVMAP_ENTRY(va, pa, sz)			\
	{						\
		.pd_va = DEVMAP_ALIGN(va),		\
		.pd_pa = DEVMAP_ALIGN(pa),		\
		.pd_size = DEVMAP_SIZE(sz),		\
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,	\
		.pd_cache = PTE_NOCACHE			\
	}
#define	DEVMAP_ENTRY_END	{ 0 }

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern struct arm32_bus_dma_tag armv7_generic_dma_tag;

#define	SYSREG_BASE		0x1c010000
#define	SYSREG_SIZE		0x1000

#define	SYS_FLAGS		0x0030
#define	SYS_FLAGSCLR		0x0034
#define	SYS_CFGDATA		0x00a0
#define	SYS_CFGCTRL		0x00a4
#define	 SYS_CFGCTRL_START	__BIT(31)
#define	 SYS_CFGCTRL_WRITE	__BIT(30)
#define	 SYS_CFGCTRL_DCC	__BITS(29,26)
#define	 SYS_CFGCTRL_FUNCTION	__BITS(25,20)
#define	  SYS_CFGCTRL_FUNCTION_SHUTDOWN	8
#define	  SYS_CFGCTRL_FUNCTION_REBOOT	9
#define	 SYS_CFGCTRL_SITE	__BITS(17,16)
#define	 SYS_CFGCTRL_POSITION	__BITS(15,12)
#define	 SYS_CFGCTRL_DEVICE	__BITS(11,0)
#define	SYS_CFGSTAT		0x00a8
#define	 SYS_CFGSTAT_ERROR	__BIT(1)
#define	 SYS_CFGSTAT_COMPLETE	__BIT(0)

static bus_space_tag_t sysreg_bst = &armv7_generic_bs_tag;
static bus_space_handle_t sysreg_bsh;

#define	SYSREG_WRITE(o, v)	\
	bus_space_write_4(sysreg_bst, sysreg_bsh, (o), (v))


static const struct pmap_devmap *
vexpress_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(VEXPRESS_CORE_VBASE,
			     VEXPRESS_CORE_PBASE,
			     VEXPRESS_CORE_SIZE),
		DEVMAP_ENTRY_END
	};	

	return devmap;
}

static void
vexpress_platform_bootstrap(void)
{
	bus_space_map(sysreg_bst, SYSREG_BASE, SYSREG_SIZE, 0,
	    &sysreg_bsh);
}

static void
vexpress_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &armv7_generic_bs_tag;
	faa->faa_a4x_bst = &armv7_generic_a4x_bs_tag;
	faa->faa_dmat = &armv7_generic_dma_tag;
}

static void
vexpress_platform_early_putchar(char c)
{
}

static void
vexpress_platform_device_register(device_t self, void *aux)
{
}

static void
vexpress_platform_reset(void)
{
	SYSREG_WRITE(SYS_CFGSTAT, 0);
	SYSREG_WRITE(SYS_CFGDATA, 0);
	SYSREG_WRITE(SYS_CFGCTRL,
	    SYS_CFGCTRL_START |
	    SYS_CFGCTRL_WRITE |
	    __SHIFTIN(SYS_CFGCTRL_FUNCTION_REBOOT,
		      SYS_CFGCTRL_FUNCTION));
}

static u_int
vexpress_platform_uart_freq(void)
{
	return VEXPRESS_REF_FREQ;
}

static const struct arm_platform vexpress_platform = {
	.devmap = vexpress_platform_devmap,
	.bootstrap = vexpress_platform_bootstrap,
	.init_attach_args = vexpress_platform_init_attach_args,
	.early_putchar = vexpress_platform_early_putchar,
	.device_register = vexpress_platform_device_register,
	.reset = vexpress_platform_reset,
	.delay = gtmr_delay,
	.uart_freq = vexpress_platform_uart_freq,
};

ARM_PLATFORM(vexpress, "arm,vexpress", &vexpress_platform);
