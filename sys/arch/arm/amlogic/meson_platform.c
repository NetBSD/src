/* $NetBSD: meson_platform.c,v 1.5 2019/02/25 19:30:17 jmcneill Exp $ */

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
#include "opt_multiprocessor.h"
#include "opt_console.h"

#include "arml2cc.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: meson_platform.c,v 1.5 2019/02/25 19:30:17 jmcneill Exp $");

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

#include <arm/cortex/a9tmr_var.h>
#include <arm/cortex/gtmr_var.h>
#include <arm/cortex/pl310_var.h>
#include <arm/cortex/scu_reg.h>

#include <arm/amlogic/meson_uart.h>

#include <evbarm/fdt/platform.h>
#include <evbarm/fdt/machdep.h>

#include <net/if_ether.h>

#include <libfdt.h>

#define	MESON_CORE_APB3_VBASE	KERNEL_IO_VBASE
#define	MESON_CORE_APB3_PBASE	0xc0000000
#define	MESON_CORE_APB3_SIZE	0x01400000

#define MESON_CBUS_OFFSET	0x01100000

#define	MESON_WATCHDOG_BASE	0xc1109900
#define	MESON_WATCHDOG_SIZE	0x8
#define	 MESON_WATCHDOG_TC	0x00
#define	  WATCHDOG_TC_CPUS	__BITS(27,24)
#define	  WATCHDOG_TC_ENABLE	__BIT(19)
#define	  WATCHDOG_TC_TCNT	__BITS(15,0)
#define	 MESON_WATCHDOG_RESET	0x04
#define	  WATCHDOG_RESET_COUNT	__BITS(15,0)

#define MESON8B_ARM_VBASE	(MESON_CORE_APB3_VBASE + MESON_CORE_APB3_SIZE)
#define	MESON8B_ARM_PBASE	0xc4200000
#define MESON8B_ARM_SIZE	0x00200000
#define MESON8B_ARM_PL310_BASE	0x00000000
#define MESON8B_ARM_SCU_BASE	0x00100000

#define MESON8B_AOBUS_VBASE	(MESON8B_ARM_VBASE + MESON8B_ARM_SIZE)
#define	MESON8B_AOBUS_PBASE	0xc8000000
#define MESON8B_AOBUS_SIZE	0x00200000

#define MESON_AOBUS_PWR_CTRL0_REG	0xe0
#define MESON_AOBUS_PWR_CTRL1_REG	0xe4
#define MESON_AOBUS_PWR_MEM_PD0_REG	0xf4

#define MESON_CBUS_CPU_CLK_CNTL_REG	0x419c


#define MESON8B_SRAM_VBASE	(MESON8B_AOBUS_VBASE + MESON8B_AOBUS_SIZE)
#define MESON8B_SRAM_PBASE	0xd9000000
#define MESON8B_SRAM_SIZE	0x00200000	/* 0x10000 rounded up */

#define MESON8B_SRAM_CPUCONF_OFFSET		0x1ff80
#define MESON8B_SRAM_CPUCONF_CTRL_REG		0x00
#define MESON8B_SRAM_CPUCONF_CPU_ADDR_REG(n)	(0x04 * (n))


extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern struct bus_space arm_generic_bs_tag;
extern struct bus_space arm_generic_a4x_bs_tag;

#define	meson_dma_tag		arm_generic_dma_tag
#define	meson_bs_tag		arm_generic_bs_tag
#define	meson_a4x_bs_tag	arm_generic_a4x_bs_tag

static const struct pmap_devmap *
meson_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(MESON_CORE_APB3_VBASE,
			     MESON_CORE_APB3_PBASE,
			     MESON_CORE_APB3_SIZE),
		DEVMAP_ENTRY(MESON8B_ARM_VBASE,
			     MESON8B_ARM_PBASE,
			     MESON8B_ARM_SIZE),
		DEVMAP_ENTRY(MESON8B_AOBUS_VBASE,
			     MESON8B_AOBUS_PBASE,
			     MESON8B_AOBUS_SIZE),
		DEVMAP_ENTRY(MESON8B_SRAM_VBASE,
			     MESON8B_SRAM_PBASE,
			     MESON8B_SRAM_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
meson_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &meson_bs_tag;
	faa->faa_a4x_bst = &meson_a4x_bs_tag;
	faa->faa_dmat = &meson_dma_tag;
}

void meson_platform_early_putchar(char);

void
meson_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	((CONSADDR - MESON8B_AOBUS_PBASE) + MESON8B_AOBUS_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;
	int timo = 150000;

	while ((uartaddr[UART_STATUS_REG/4] & UART_STATUS_TX_EMPTY) == 0) {
		if (--timo == 0)
			break;
	}

	uartaddr[UART_WFIFO_REG/4] = c;

	while ((uartaddr[UART_STATUS_REG/4] & UART_STATUS_TX_EMPTY) == 0) {
		if (--timo == 0)
			break;
	}
#endif
}

static void
meson_platform_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	if (device_is_a(self, "awge") && device_unit(self) == 0) {
		uint8_t enaddr[ETHER_ADDR_LEN];
		if (get_bootconf_option(boot_args, "awge0.mac-address",
		    BOOTOPT_TYPE_MACADDR, enaddr)) {
			prop_data_t pd = prop_data_create_data(enaddr,
			    sizeof(enaddr));
			prop_dictionary_set(dict, "mac-address", pd);
			prop_object_release(pd);
		}
	}

	if (device_is_a(self, "mesonfb")) {
		int scale, depth;

		if (get_bootconf_option(boot_args, "fb.scale",
		    BOOTOPT_TYPE_INT, &scale) && scale > 0) {
			prop_dictionary_set_uint32(dict, "scale", scale);
		}
		if (get_bootconf_option(boot_args, "fb.depth",
		    BOOTOPT_TYPE_INT, &depth)) {
			prop_dictionary_set_uint32(dict, "depth", depth);
		}
	}
}

#if defined(SOC_MESON8B)
#define	MESON8B_BOOTINFO_REG	0xd901ff04
static int
meson8b_get_boot_id(void)
{
	static int boot_id = -1;
	bus_space_tag_t bst = &arm_generic_bs_tag;
	bus_space_handle_t bsh;

	if (boot_id == -1) {
		if (bus_space_map(bst, MESON8B_BOOTINFO_REG, 4, 0, &bsh) != 0)
			return -1;

		boot_id = (int)bus_space_read_4(bst, bsh, 0);

		bus_space_unmap(bst, bsh, 4);
	}

	return boot_id;
}

static void
meson8b_platform_device_register(device_t self, void *aux)
{
	device_t parent = device_parent(self);
	char *ptr;

	if (device_is_a(self, "ld") &&
	    device_is_a(parent, "sdmmc") &&
	    (device_is_a(device_parent(parent), "mesonsdhc") ||
	     device_is_a(device_parent(parent), "mesonsdio"))) {

		const int boot_id = meson8b_get_boot_id();
		const bool has_rootdev = get_bootconf_option(boot_args, "root", BOOTOPT_TYPE_STRING, &ptr) != 0;

		if (!has_rootdev) {
			char rootarg[64];
			snprintf(rootarg, sizeof(rootarg), " root=%sa", device_xname(self));

			/* Assume that SDIO is used for SD cards and SDHC is used for eMMC */
			if (device_is_a(device_parent(parent), "mesonsdhc") && boot_id == 0)
				strcat(boot_args, rootarg);
			else if (device_is_a(device_parent(parent), "mesonsdio") && boot_id != 0)
				strcat(boot_args, rootarg);
		}
	}
			
	meson_platform_device_register(self, aux);
}
#endif

static u_int
meson_platform_uart_freq(void)
{
	return 0;
}

static void
meson_platform_bootstrap(void)
{
	arm_fdt_cpu_bootstrap();

	void *fdt_data = __UNCONST(fdtbus_get_data());
	const int chosen_off = fdt_path_offset(fdt_data, "/chosen");
	if (chosen_off < 0)
		return;

	if (match_bootconf_option(boot_args, "console", "fb")) {
		const int framebuffer_off =
		    fdt_path_offset(fdt_data, "/chosen/framebuffer");
		if (framebuffer_off >= 0) {
			const char *status = fdt_getprop(fdt_data,
			    framebuffer_off, "status", NULL);
			if (status == NULL || strncmp(status, "ok", 2) == 0) {
				fdt_setprop_string(fdt_data, chosen_off,
				    "stdout-path", "/chosen/framebuffer");
			}
		}
	} else if (match_bootconf_option(boot_args, "console", "serial")) {
		fdt_setprop_string(fdt_data, chosen_off,
		    "stdout-path", "serial0:115200n8");
	}
}

#if defined(SOC_MESON8B)
static void
meson8b_platform_bootstrap(void)
{

#if NARML2CC > 0
	const bus_space_handle_t pl310_bh = MESON8B_ARM_VBASE + MESON8B_ARM_PL310_BASE;
	arml2cc_init(&arm_generic_bs_tag, pl310_bh, 0);
#endif

	meson_platform_bootstrap();
}
#endif

static void
meson_platform_reset(void)
{
	bus_space_tag_t bst = &meson_bs_tag;
	bus_space_handle_t bsh;

	bus_space_map(bst, MESON_WATCHDOG_BASE, MESON_WATCHDOG_SIZE, 0, &bsh);

	bus_space_write_4(bst, bsh, MESON_WATCHDOG_TC,
	    WATCHDOG_TC_CPUS | WATCHDOG_TC_ENABLE | __SHIFTIN(0xfff, WATCHDOG_TC_TCNT));
	bus_space_write_4(bst, bsh, MESON_WATCHDOG_RESET, 0);

	for (;;) {
		__asm("wfi");
	}
}

#if defined(SOC_MESON8B)
static void
meson8b_mpinit_delay(u_int n)
{
	for (volatile int i = 0; i < n; i++)
		;
}

static int
cpu_enable_meson8b(int phandle)
{
	const bus_addr_t cbar = armreg_cbar_read();
	bus_space_tag_t bst = &arm_generic_bs_tag;

	const bus_space_handle_t scu_bsh =
	    cbar - MESON8B_ARM_PBASE + MESON8B_ARM_VBASE;
	const bus_space_handle_t cpuconf_bsh =
	    MESON8B_SRAM_VBASE + MESON8B_SRAM_CPUCONF_OFFSET;
	const bus_space_handle_t ao_bsh =
	    MESON8B_AOBUS_VBASE;
	const bus_space_handle_t cbus_bsh =
	    MESON_CORE_APB3_VBASE + MESON_CBUS_OFFSET;
	uint32_t pwr_sts, pwr_cntl0, pwr_cntl1, cpuclk, mempd0;
	uint64_t mpidr;

	fdtbus_get_reg64(phandle, 0, &mpidr, NULL);

	const u_int cpuno = __SHIFTOUT(mpidr, MPIDR_AFF0);

	bus_space_write_4(bst, cpuconf_bsh, MESON8B_SRAM_CPUCONF_CPU_ADDR_REG(cpuno),
	    KERN_VTOPHYS((vaddr_t)cpu_mpstart));

	pwr_sts = bus_space_read_4(bst, scu_bsh, SCU_CPU_PWR_STS);
	pwr_sts &= ~(3 << (8 * cpuno));
	bus_space_write_4(bst, scu_bsh, SCU_CPU_PWR_STS, pwr_sts);

	pwr_cntl0 = bus_space_read_4(bst, ao_bsh, MESON_AOBUS_PWR_CTRL0_REG);
	pwr_cntl0 &= ~((3 << 18) << ((cpuno - 1) * 2));
	bus_space_write_4(bst, ao_bsh, MESON_AOBUS_PWR_CTRL0_REG, pwr_cntl0);

	meson8b_mpinit_delay(5000);

	cpuclk = bus_space_read_4(bst, cbus_bsh, MESON_CBUS_CPU_CLK_CNTL_REG);
	cpuclk |= (1 << (24 + cpuno));
	bus_space_write_4(bst, cbus_bsh, MESON_CBUS_CPU_CLK_CNTL_REG, cpuclk);

	mempd0 = bus_space_read_4(bst, ao_bsh, MESON_AOBUS_PWR_MEM_PD0_REG);
	mempd0 &= ~((uint32_t)(0xf << 28) >> ((cpuno - 1) * 4));
	bus_space_write_4(bst, ao_bsh, MESON_AOBUS_PWR_MEM_PD0_REG, mempd0);

	pwr_cntl1 = bus_space_read_4(bst, ao_bsh, MESON_AOBUS_PWR_CTRL1_REG);
	pwr_cntl1 &= ~((3 << 4) << ((cpuno - 1) * 2));
	bus_space_write_4(bst, ao_bsh, MESON_AOBUS_PWR_CTRL1_REG, pwr_cntl1);

	meson8b_mpinit_delay(10000);

	for (;;) {
		pwr_cntl1 = bus_space_read_4(bst, ao_bsh,
		    MESON_AOBUS_PWR_CTRL1_REG) & ((1 << 17) << (cpuno - 1));
		if (pwr_cntl1)
			break;
		meson8b_mpinit_delay(10000);
	}

	pwr_cntl0 = bus_space_read_4(bst, ao_bsh, MESON_AOBUS_PWR_CTRL0_REG);
	pwr_cntl0 &= ~(1 << cpuno);
	bus_space_write_4(bst, ao_bsh, MESON_AOBUS_PWR_CTRL0_REG, pwr_cntl0);

	cpuclk = bus_space_read_4(bst, cbus_bsh, MESON_CBUS_CPU_CLK_CNTL_REG);
	cpuclk &= ~(1 << (24 + cpuno));
	bus_space_write_4(bst, cbus_bsh, MESON_CBUS_CPU_CLK_CNTL_REG, cpuclk);

	bus_space_write_4(bst, cpuconf_bsh, MESON8B_SRAM_CPUCONF_CPU_ADDR_REG(cpuno),
	    KERN_VTOPHYS((vaddr_t)cpu_mpstart));

	uint32_t ctrl = bus_space_read_4(bst, cpuconf_bsh, MESON8B_SRAM_CPUCONF_CTRL_REG);
	ctrl |= __BITS(cpuno,0);
	bus_space_write_4(bst, cpuconf_bsh, MESON8B_SRAM_CPUCONF_CTRL_REG, ctrl);

	return 0;
}

ARM_CPU_METHOD(meson8b, "amlogic,meson8b-smp", cpu_enable_meson8b);

static int
meson8b_mpstart(void)
{
	int ret = 0;
	const bus_addr_t cbar = armreg_cbar_read();
	bus_space_tag_t bst = &arm_generic_bs_tag;

	if (cbar == 0)
		return ret;

	const bus_space_handle_t scu_bsh =
	    cbar - MESON8B_ARM_PBASE + MESON8B_ARM_VBASE;

	const uint32_t scu_cfg = bus_space_read_4(bst, scu_bsh, SCU_CFG);
	const u_int ncpus = (scu_cfg & SCU_CFG_CPUMAX) + 1;

	if (ncpus < 2)
		return ret;

	/*
	 * Invalidate all SCU cache tags. That is, for all cores (0-3)
	 */
	bus_space_write_4(bst, scu_bsh, SCU_INV_ALL_REG, 0xffff);

	uint32_t scu_ctl = bus_space_read_4(bst, scu_bsh, SCU_CTL);
	scu_ctl |= SCU_CTL_SCU_ENA;
	bus_space_write_4(bst, scu_bsh, SCU_CTL, scu_ctl);

	armv7_dcache_wbinv_all();

	ret = arm_fdt_cpu_mpstart();
	return ret;
}

static const struct arm_platform meson8b_platform = {
	.ap_devmap = meson_platform_devmap,
	.ap_bootstrap = meson8b_platform_bootstrap,
	.ap_init_attach_args = meson_platform_init_attach_args,
	.ap_device_register = meson8b_platform_device_register,
	.ap_reset = meson_platform_reset,
	.ap_delay = a9tmr_delay,
	.ap_uart_freq = meson_platform_uart_freq,
	.ap_mpstart = meson8b_mpstart,
};

ARM_PLATFORM(meson8b, "amlogic,meson8b", &meson8b_platform);
#endif	/* SOC_MESON8B */

#if defined(SOC_MESONGXBB)
static const struct arm_platform mesongxbb_platform = {
	.ap_devmap = meson_platform_devmap,
	.ap_bootstrap = meson_platform_bootstrap,
	.ap_init_attach_args = meson_platform_init_attach_args,
	.ap_device_register = meson_platform_device_register,
	.ap_reset = meson_platform_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = meson_platform_uart_freq,
	.ap_mpstart = arm_fdt_cpu_mpstart,
};

ARM_PLATFORM(mesongxbb, "amlogic,meson-gxbb", &mesongxbb_platform);
#endif
