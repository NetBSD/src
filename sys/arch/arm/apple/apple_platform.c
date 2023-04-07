/* $NetBSD: apple_platform.c,v 1.6 2023/04/07 08:55:29 skrll Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apple_platform.c,v 1.6 2023/04/07 08:55:29 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>

#include <arm/fdt/arm_fdtvar.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>

#include <net/if_ether.h>

#include <dev/pci/pcivar.h>
#include <machine/pci_machdep.h>

#include <arm/cpufunc.h>

#include <arm/cortex/gtmr_var.h>

#include <arm/arm/psci.h>
#include <arm/fdt/psci_fdtvar.h>

#include <libfdt.h>

#include <arch/evbarm/fdt/platform.h>

extern struct bus_space arm_generic_bs_tag;

static struct bus_space apple_nonposted_bs_tag;

struct arm32_bus_dma_tag apple_coherent_dma_tag;
static struct arm32_dma_range apple_coherent_ranges[] = {
	[0] = {
		.dr_sysbase = 0,
		.dr_busbase = 0,
		.dr_len = UINTPTR_MAX,
		.dr_flags = _BUS_DMAMAP_COHERENT,
	}
};

static int
apple_nonposted_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int flag,
    bus_space_handle_t *bshp)
{
	if (flag == 0) {
		flag |= BUS_SPACE_MAP_NONPOSTED;
	}

	return bus_space_map(&arm_generic_bs_tag, bpa, size, flag, bshp);
}

static void
apple_platform_bootstrap(void)
{
	extern struct arm32_bus_dma_tag arm_generic_dma_tag;

	apple_nonposted_bs_tag = arm_generic_bs_tag;
	apple_nonposted_bs_tag.bs_map = apple_nonposted_bs_map;

	apple_coherent_dma_tag = arm_generic_dma_tag;
	apple_coherent_dma_tag._ranges = apple_coherent_ranges;
	apple_coherent_dma_tag._nranges = __arraycount(apple_coherent_ranges);

	arm_fdt_cpu_bootstrap();
}

static void
apple_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &apple_nonposted_bs_tag;
	faa->faa_dmat = &apple_coherent_dma_tag;
}

static const struct pmap_devmap *
apple_platform_devmap(void)
{
	/* Size this to hold possible entries for the UART and SMP spin-table */
	static struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY_END,
		DEVMAP_ENTRY_END,
		DEVMAP_ENTRY_END
	};
	bus_addr_t uart_base;
	vaddr_t devmap_va = KERNEL_IO_VBASE;
	u_int devmap_index = 0;
	uint64_t release_addr;
	int phandle;

	phandle = fdtbus_get_stdout_phandle();
	if (phandle > 0 && fdtbus_get_reg(phandle, 0, &uart_base, NULL) == 0) {
		devmap[devmap_index].pd_pa = DEVMAP_ALIGN(uart_base);
		devmap[devmap_index].pd_va = DEVMAP_ALIGN(devmap_va);
		devmap[devmap_index].pd_size = DEVMAP_SIZE(L3_SIZE);
		devmap[devmap_index].pd_prot = VM_PROT_READ | VM_PROT_WRITE;
		devmap[devmap_index].pd_flags = PMAP_DEV_NP;
		devmap_va = DEVMAP_SIZE(devmap[devmap_index].pd_va +
		    devmap[devmap_index].pd_size);
		devmap_index++;
	}

	/* XXX hopefully all release addresses are in the same 2M */
	phandle = OF_finddevice("/cpus/cpu@1");
	if (phandle > 0 &&
	    of_getprop_uint64(phandle, "cpu-release-addr", &release_addr) == 0) {
		devmap[devmap_index].pd_pa = DEVMAP_ALIGN(release_addr);
		devmap[devmap_index].pd_va = DEVMAP_ALIGN(devmap_va);
		devmap[devmap_index].pd_size = DEVMAP_SIZE(L2_SIZE);
		devmap[devmap_index].pd_prot = VM_PROT_READ | VM_PROT_WRITE;
		devmap[devmap_index].pd_flags = PMAP_WRITE_BACK;
		devmap_va = DEVMAP_SIZE(devmap[devmap_index].pd_va +
		    devmap[devmap_index].pd_size);
		devmap_index++;
	}

	return devmap;
}

static u_int
apple_platform_uart_freq(void)
{
	return 0;
}

static int
apple_platform_get_mac_address(pci_chipset_tag_t pc, pcitag_t tag,
    uint8_t *eaddr)
{
	int b, d, f;
	int bridge, len;
	u_int bdf;

	const int pcie = of_find_bycompat(OF_finddevice("/"), "apple,pcie");
	if (pcie == -1) {
		return -1;
	}

	/* Convert PCI tag to encoding of phys.hi for PCI-PCI bridge regs */
	pci_decompose_tag(pc, tag, &b, &d, &f);
	bdf = (b << 16) | (d << 11) | (f << 8);

	for (bridge = OF_child(pcie); bridge; bridge = OF_peer(bridge)) {
		const int ethernet =
		    of_find_firstchild_byname(bridge, "ethernet");
		if (ethernet == -1) {
			continue;
		}
		const u_int *data = fdtbus_get_prop(ethernet, "reg", &len);
		if (data == NULL || len < 4) {
			continue;
		}
		if (bdf != be32toh(data[0])) {
			continue;
		}

		return OF_getprop(ethernet, "local-mac-address",
		    eaddr, ETHER_ADDR_LEN);
	}

	return -1;
}

static void
apple_platform_device_register(device_t self, void *aux)
{
	prop_dictionary_t prop = device_properties(self);
	uint8_t eaddr[ETHER_ADDR_LEN];
	int len;

	if (device_is_a(self, "cpu")) {
		struct fdt_attach_args * const faa = aux;
		bus_addr_t cpuid;

		if (fdtbus_get_reg(faa->faa_phandle, 0, &cpuid, NULL) != 0) {
			cpuid = 0;
		}

		/*
		 * On Apple M1 (and hopefully later models), AFF2 is 0 for
		 * efficiency and 1 for performance cores. Use this value
		 * to provide a fake DMIPS/MHz value -- the actual number
		 * only matters in relation to the value presented by other
		 * cores.
		 */
		const u_int aff2 = __SHIFTOUT(cpuid, MPIDR_AFF2);
		prop_dictionary_set_uint32(prop, "capacity_dmips_mhz", aff2);
		return;
	}

	if (device_is_a(self, "bge") &&
	    device_is_a(device_parent(self), "pci")) {
		struct pci_attach_args * const pa = aux;

		len = apple_platform_get_mac_address(pa->pa_pc, pa->pa_tag,
		    eaddr);
		if (len == ETHER_ADDR_LEN) {
			prop_dictionary_set_bool(prop, "without-seeprom", true);
			prop_dictionary_set_data(prop, "mac-address", eaddr,
			    sizeof(eaddr));
		}
		return;
	}
}

static const struct fdt_platform apple_fdt_platform = {
	.fp_devmap = apple_platform_devmap,
	.fp_bootstrap = apple_platform_bootstrap,
	.fp_init_attach_args = apple_platform_init_attach_args,
	.fp_reset = psci_fdt_reset,
	.fp_delay = gtmr_delay,
	.fp_uart_freq = apple_platform_uart_freq,
	.fp_device_register = apple_platform_device_register,
	.fp_mpstart = arm_fdt_cpu_mpstart,
};

FDT_PLATFORM(apple_arm, "apple,arm-platform", &apple_fdt_platform);
