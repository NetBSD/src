/* $NetBSD: cycv_platform.c,v 1.3 2018/10/18 09:01:52 skrll Exp $ */

/* This file is in the public domain. */

#include "arml2cc.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cycv_platform.c,v 1.3 2018/10/18 09:01:52 skrll Exp $");

#define	_ARM32_BUS_DMA_PRIVATE
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <arm/arm32/machdep.h>

#include <arm/altera/cycv_reg.h>
#include <arm/altera/cycv_var.h>
#include <arm/cortex/a9tmr_var.h>
#include <arm/cortex/pl310_var.h>
#include <arm/cortex/scu_reg.h>

#include <arm/bootconfig.h>
#include <arm/cpufunc.h>

#include <arm/fdt/arm_fdtvar.h>
#include <dev/fdt/fdtvar.h>

static void cycv_platform_early_putchar(char);

static const struct pmap_devmap *
cycv_platform_devmap(void) {
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(CYCV_PERIPHERAL_VBASE,
				CYCV_PERIPHERAL_BASE,
				CYCV_PERIPHERAL_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
cycv_platform_bootstrap(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh_l2c;

	bus_space_map(bst, CYCV_L2CACHE_BASE, CYCV_L2CACHE_SIZE, 0, &bsh_l2c);

#if NARML2CC > 0
	arml2cc_init(bst, bsh_l2c, 0);
#endif
}

static void
cycv_mpstart(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh_rst;
	bus_space_handle_t bsh_scu;

	bus_space_map(bst, CYCV_RSTMGR_BASE, CYCV_RSTMGR_SIZE, 0, &bsh_rst);
	bus_space_map(bst, CYCV_SCU_BASE, CYCV_SCU_SIZE, 0, &bsh_scu);

	/* Enable Snoop Control Unit */
	bus_space_write_4(bst, bsh_rst, SCU_CTL,
		bus_space_read_4(bst, bsh_rst, SCU_CTL) | SCU_CTL_SCU_ENA);

	const uint32_t startfunc = (uint32_t) KERN_VTOPHYS((vaddr_t)cpu_mpstart);

	/*
	 * We place a "B cortex_mpstart" at address 0 in order to bootstrap
	 * CPU 1. We can't use the similar feature of the Boot ROM because
	 * it was unmapped by u-boot in favor of the SDRAM. Plus the dtb is
	 * stored very low in RAM so we can't re-map the Boot ROM easily.
	 */
	extern vaddr_t cpu_ttb;

	pmap_map_chunk(cpu_ttb, CYCV_SDRAM_VBASE, CYCV_SDRAM_BASE, L1_S_SIZE,
	    VM_PROT_READ|VM_PROT_WRITE, PMAP_NOCACHE);
	*(volatile uint32_t *) CYCV_SDRAM_VBASE =
	    htole32(0xea000000 | ((startfunc - 8 - 0x0) >> 2));
	pmap_unmap_chunk(cpu_ttb, CYCV_SDRAM_BASE, L1_S_SIZE);

	arm_cpu_max = 2;

	bus_space_write_4(bst, bsh_rst, CYCV_RSTMGR_MPUMODRST,
		bus_space_read_4(bst, bsh_rst, CYCV_RSTMGR_MPUMODRST) &
			~CYCV_RSTMGR_MPUMODRST_CPU1);
}

static void
cycv_platform_init_attach_args(struct fdt_attach_args *faa) {
	faa->faa_bst = &armv7_generic_bs_tag;
	faa->faa_a4x_bst = &armv7_generic_a4x_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

static void
cycv_platform_early_putchar(char c) {
#ifdef CONSADDR
#define CONSADDR_VA (CONSADDR - CYCV_PERIPHERAL_BASE + CYCV_PERIPHERAL_VBASE)
	volatile uint32_t *uartaddr = (volatile uint32_t *) CONSADDR_VA;

	while ((le32toh(uartaddr[com_lsr]) & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = htole32(c);
#endif
}

static void
cycv_platform_device_register(device_t dev, void *aux) {
	prop_dictionary_t dict = device_properties(dev);

	if (device_is_a(dev, "arma9tmr")) {
		prop_dictionary_set_uint32(dict, "frequency",
			cycv_clkmgr_early_get_mpu_clk() / 4);
	}
}

static void
cycv_platform_reset(void) {
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t val;

	bus_space_map(bst, CYCV_RSTMGR_BASE, CYCV_RSTMGR_SIZE, 0, &bsh);
	val = bus_space_read_4(bst, bsh, CYCV_RSTMGR_CTRL);
	bus_space_write_4(bst, bsh, CYCV_RSTMGR_CTRL,
		val | CYCV_RSTMGR_CTRL_SWCOLDRSTREQ);
}

static u_int
cycv_platform_uart_freq(void) {
	return cycv_clkmgr_early_get_l4_sp_clk();
}

static const struct arm_platform cycv_platform = {
	.ap_devmap = cycv_platform_devmap,
	.ap_bootstrap = cycv_platform_bootstrap,
	.ap_init_attach_args = cycv_platform_init_attach_args,
	.ap_early_putchar = cycv_platform_early_putchar,
	.ap_device_register = cycv_platform_device_register,
	.ap_reset = cycv_platform_reset,
	.ap_delay = a9tmr_delay,
	.ap_uart_freq = cycv_platform_uart_freq,
	.ap_mpstart = cycv_mpstart,
};

ARM_PLATFORM(cycv, "altr,socfpga-cyclone5", &cycv_platform);
