/* $NetBSD: tegra_machdep.c,v 1.47 2017/05/29 23:13:03 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: tegra_machdep.c,v 1.47 2017/05/29 23:13:03 jmcneill Exp $");

#include "opt_tegra.h"
#include "opt_machdep.h"
#include "opt_ddb.h"
#include "opt_md.h"
#include "opt_arm_debug.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/ksyms.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/termios.h>

#include <uvm/uvm_extern.h>

#include <sys/conf.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/bootconfig.h>
#include <arm/armreg.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <evbarm/include/autoconf.h>
#include <evbarm/tegra/platform.h>

#include <arm/fdt/armv7_fdtvar.h>

#ifndef TEGRA_MAX_BOOT_STRING
#define TEGRA_MAX_BOOT_STRING 1024
#endif

BootConfig bootconfig;
char bootargs[TEGRA_MAX_BOOT_STRING] = "";
char *boot_args = NULL;
u_int uboot_args[4] = { 0 };	/* filled in by tegra_start.S (not in bss) */

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>
#define FDT_BUF_SIZE	(128*1024)
static uint8_t fdt_data[FDT_BUF_SIZE];

extern char KERNEL_BASE_phys[];
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)

static void tegra_device_register(device_t, void *);
static void tegra_reset(void);
static void tegra_powerdown(void);

#ifdef PMAP_NEED_ALLOC_POOLPAGE
static struct boot_physmem bp_lowgig = {
	.bp_pages = (KERNEL_VM_BASE - KERNEL_BASE) / NBPG,
	.bp_freelist = VM_FREELIST_ISADMA,
	.bp_flags = 0
};
#endif

#ifdef VERBOSE_INIT_ARM
static void
tegra_putchar(char c)
{
	const struct armv7_platform *plat = armv7_fdt_platform();
	if (plat && plat->early_purchar)
		plat->early_putchar(c);
}

static void
tegra_putstr(const char *s)
{
	for (const char *p = s; *p; p++) {
		tegra_putchar(*p);
	}
}

static void
tegra_printn(u_int n, int base)
{
	char *p, buf[(sizeof(u_int) * NBBY / 3) + 1 + 2 /* ALT + SIGN */];

	p = buf;
	do {
		*p++ = hexdigits[n % base];
	} while (n /= base);

	do {
		tegra_putchar(*--p);
	} while (p > buf);
}
#define DPRINTF(...)		printf(__VA_ARGS__)
#define DPRINT(x)		tegra_putstr(x)
#define DPRINTN(x,b)		tegra_printn((x), (b))
#else
#define DPRINTF(...)
#define DPRINT(x)
#define DPRINTN(x,b)
#endif

/*
 * u_int initarm(...)
 *
 * Initial entry point on startup. This gets called before main() is
 * entered.
 * It should be responsible for setting up everything that must be
 * in place when main is called.
 * This includes
 *   Taking a copy of the boot configuration structure.
 *   Initialising the physical console so characters can be printed.
 *   Setting up page tables for the kernel
 *   Relocating the kernel to the bottom of physical memory
 */
u_int
initarm(void *arg)
{
	const struct armv7_platform *plat;
	uint64_t memory_addr, memory_size;
	psize_t ram_size = 0;

	/* Load FDT */
	const uint8_t *fdt_addr_r = (const uint8_t *)uboot_args[2];
	int error = fdt_check_header(fdt_addr_r);
	if (error == 0) {
		error = fdt_move(fdt_addr_r, fdt_data, sizeof(fdt_data));
		if (error != 0) {
			panic("fdt_move failed: %s", fdt_strerror(error));
		}
		fdtbus_set_data(fdt_data);
	} else {
		panic("fdt_check_header failed: %s", fdt_strerror(error));
	}

	/* Lookup platform specific backend */
	plat = armv7_fdt_platform();
	if (plat == NULL)
		panic("Kernel does not support this device");

	/* Early console may be available, announce ourselves. */
	DPRINT("NetBSD FDT init");

	DPRINT(" devmap");
	pmap_devmap_register(plat->devmap());

	DPRINT(" bootstrap");
	plat->bootstrap();

	/* Heads up ... Setup the CPU / MMU / TLB functions. */
	DPRINT(" cpufunc");
	if (set_cpufuncs())
		panic("cpu not recognized!");

	DPRINT(" consinit");
	consinit();

	DPRINTF(" cbar=%#x", armreg_cbar_read());

	DPRINTF(" ok\n");

	DPRINTF("uboot: args %#x, %#x, %#x, %#x\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);

	cpu_reset_address = tegra_reset;
	cpu_powerdown_address = tegra_powerdown;
	evbarm_device_register = tegra_device_register;

	/* Talk to the user */
	DPRINTF("\nNetBSD/evbarm (tegra) booting ...\n");

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

	DPRINTF("KERNEL_BASE=0x%x, KERNEL_VM_BASE=0x%x, KERNEL_VM_BASE - KERNEL_BASE=0x%x, KERNEL_BASE_VOFFSET=0x%x\n",
		KERNEL_BASE, KERNEL_VM_BASE, KERNEL_VM_BASE - KERNEL_BASE, KERNEL_BASE_VOFFSET);

	const int memory = OF_finddevice("/memory");
	if (fdtbus_get_reg64(memory, 0, &memory_addr, &memory_size) != 0)
		panic("Cannot determine memory size");

	DPRINTF("FDT memory node = %d, addr %llx, size %llu\n",
	    memory, (unsigned long long)memory_addr,
	    (unsigned long long)memory_size);

	/* Cannot map memory above 4GB */
	if (memory_addr + memory_size > 0x100000000)
		memory_size = 0x100000000 - memory_addr;

	ram_size = (bus_size_t)memory_size;

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	const bool mapallmem_p = true;
#ifndef PMAP_NEED_ALLOC_POOLPAGE
	if (ram_size > KERNEL_VM_BASE - KERNEL_BASE) {
		DPRINTF("%s: dropping RAM size from %luMB to %uMB\n",
		    __func__, (unsigned long) (ram_size >> 20),     
		    (KERNEL_VM_BASE - KERNEL_BASE) >> 20);
		ram_size = KERNEL_VM_BASE - KERNEL_BASE;
	}
#endif
#else
	const bool mapallmem_p = false;
#endif

	/*
	 * If MEMSIZE specified less than what we really have, limit ourselves
	 * to that.
	 */
#ifdef MEMSIZE
	if (ram_size == 0 || ram_size > (unsigned)MEMSIZE * 1024 * 1024)
		ram_size = (unsigned)MEMSIZE * 1024 * 1024;
	DPRINTF("ram_size = 0x%x\n", (int)ram_size);
#else
	KASSERTMSG(ram_size > 0, "RAM size unknown and MEMSIZE undefined");
#endif

	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = (bus_addr_t)memory_addr;
	bootconfig.dram[0].pages = ram_size / PAGE_SIZE;

	KASSERT((armreg_pfr1_read() & ARM_PFR1_SEC_MASK) != 0);

	arm32_bootmem_init(bootconfig.dram[0].address, ram_size,
	    KERNEL_BASE_PHYS);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0,
	    plat->devmap(), mapallmem_p);

	const int chosen = OF_finddevice("/chosen");
	if (chosen >= 0) {
		OF_getprop(chosen, "bootargs", bootargs, sizeof(bootargs));
	}

	DPRINTF("bootargs: %s\n", bootargs);

	boot_args = bootargs;
	parse_mi_bootargs(boot_args);

#ifdef PMAP_NEED_ALLOC_POOLPAGE
	bp_lowgig.bp_start = memory_addr / NBPG;
	if (atop(ram_size) > bp_lowgig.bp_pages) {
		arm_poolpage_vmfreelist = bp_lowgig.bp_freelist;
		return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE,
		    &bp_lowgig, 1);
	}
#endif

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);

}

void
consinit(void)
{
	static bool initialized = false;
	const struct armv7_platform *plat = armv7_fdt_platform();
	const struct fdt_console *cons = fdtbus_get_console();
	struct fdt_attach_args faa;

	if (initialized || cons == NULL)
		return;

	plat->init_attach_args(&faa);
	faa.faa_phandle = fdtbus_get_stdout_phandle();

	cons->consinit(&faa);

	initialized = true;
}

void
tegra_device_register(device_t self, void *aux)
{
	const struct armv7_platform *plat = armv7_fdt_platform();

	if (plat && plat->device_register)
		plat->device_register(self, aux);
}

static void
tegra_reset(void)
{
	const struct armv7_platform *plat = armv7_fdt_platform();

	fdtbus_power_reset();

	if (plat && plat->reset)
		plat->reset();
}

static void
tegra_powerdown(void)
{
	fdtbus_power_poweroff();
}
