/* $NetBSD: fdt_machdep.c,v 1.20.2.1 2018/04/07 04:12:12 pgoyette Exp $ */

/*-
 * Copyright (c) 2015-2017 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: fdt_machdep.c,v 1.20.2.1 2018/04/07 04:12:12 pgoyette Exp $");

#include "opt_machdep.h"
#include "opt_bootconfig.h"
#include "opt_ddb.h"
#include "opt_md.h"
#include "opt_arm_debug.h"
#include "opt_multiprocessor.h"
#include "opt_cpuoptions.h"

#include "ukbd.h"

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
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#include <sys/conf.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/bootconfig.h>
#include <arm/armreg.h>

#include <arm/cpufunc.h>
#ifdef __aarch64__
#include <aarch64/machdep.h>
#else
#include <arm/arm32/machdep.h>
#endif


#include <evbarm/include/autoconf.h>
#include <evbarm/fdt/platform.h>

#include <arm/fdt/arm_fdtvar.h>

#if NUKBD > 0
#include <dev/usb/ukbdvar.h>
#endif

#ifdef MEMORY_DISK_DYNAMIC
#include <dev/md.h>
#endif

#ifndef FDT_MAX_BOOT_STRING
#define FDT_MAX_BOOT_STRING 1024
#endif

BootConfig bootconfig;
char bootargs[FDT_MAX_BOOT_STRING] = "";
char *boot_args = NULL;
/*
 * filled in by xxx_start.S (must not be in bss)
 */
unsigned long  uboot_args[4] = { 0 };
const uint8_t *fdt_addr_r = (const uint8_t *)0xdeadc0de;

static char fdt_memory_ext_storage[EXTENT_FIXED_STORAGE_SIZE(DRAM_BLOCKS)];
static struct extent *fdt_memory_ext;

static uint64_t initrd_start, initrd_end;

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>
#define FDT_BUF_SIZE	(128*1024)
static uint8_t fdt_data[FDT_BUF_SIZE];

extern char KERNEL_BASE_phys[];
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)

static void fdt_update_stdout_path(void);
static void fdt_device_register(device_t, void *);
static void fdt_reset(void);
static void fdt_powerdown(void);

#ifdef VERBOSE_INIT_ARM
static void
fdt_putchar(char c)
{
	const struct arm_platform *plat = arm_fdt_platform();
	if (plat && plat->early_putchar)
		plat->early_putchar(c);
}

static void
fdt_putstr(const char *s)
{
	for (const char *p = s; *p; p++)
		fdt_putchar(*p);
}

static void
fdt_printn(unsigned long n, int base)
{
	char *p, buf[(sizeof(unsigned long) * NBBY / 3) + 1 + 2 /* ALT + SIGN */];

	p = buf;
	do {
		*p++ = hexdigits[n % base];
	} while (n /= base);

	do {
		fdt_putchar(*--p);
	} while (p > buf);
}
#define DPRINTF(...)		printf(__VA_ARGS__)
#define DPRINT(x)		fdt_putstr(x)
#define DPRINTN(x,b)		fdt_printn((x), (b))
#else
#define DPRINTF(...)
#define DPRINT(x)
#define DPRINTN(x,b)
#endif

/*
 * ARM: Get the first physically contiguous region of memory.
 * ARM64: Get all of physical memory, including holes.
 */
static void
fdt_get_memory(uint64_t *pstart, uint64_t *pend)
{
	const int memory = OF_finddevice("/memory");
	uint64_t cur_addr, cur_size;
	int index;

	/* Assume the first entry is the start of memory */
	if (fdtbus_get_reg64(memory, 0, &cur_addr, &cur_size) != 0)
		panic("Cannot determine memory size");

	*pstart = cur_addr;
	*pend = cur_addr + cur_size;

	DPRINTF("FDT /memory [%d] @ 0x%" PRIx64 " size 0x%" PRIx64 "\n",
	    0, *pstart, *pend - *pstart);

	for (index = 1;
	     fdtbus_get_reg64(memory, index, &cur_addr, &cur_size) == 0;
	     index++) {
		DPRINTF("FDT /memory [%d] @ 0x%" PRIx64 " size 0x%" PRIx64 "\n",
		    index, cur_addr, cur_size);

#ifdef __aarch64__
		if (cur_addr + cur_size > *pend)
			*pend = cur_addr + cur_size;
#else
		/* If subsequent entries follow the previous, append them. */
		if (*pend == cur_addr)
			*pend = cur_addr + cur_size;
#endif
	}
}

void
fdt_add_reserved_memory_range(uint64_t addr, uint64_t size)
{
	uint64_t start = trunc_page(addr);
	uint64_t end = round_page(addr + size);

	int error = extent_free(fdt_memory_ext, start,
	     end - start, EX_NOWAIT);
	if (error != 0)
		printf("MEM ERROR: res %llx-%llx failed: %d\n",
		    start, end, error);
	else
		DPRINTF("MEM: res %llx-%llx\n", start, end);
}

/*
 * Exclude memory ranges from memory config from the device tree
 */
static void
fdt_add_reserved_memory(uint64_t max_addr)
{
	uint64_t addr, size;
	int index, error;

	const int num = fdt_num_mem_rsv(fdtbus_get_data());
	for (index = 0; index <= num; index++) {
		error = fdt_get_mem_rsv(fdtbus_get_data(), index,
		    &addr, &size);
		if (error != 0 || size == 0)
			continue;
		if (addr >= max_addr)
			continue;
		if (addr + size > max_addr)
			size = max_addr - addr;
		fdt_add_reserved_memory_range(addr, size);
	}
}

/*
 * Define usable memory regions.
 */
static void
fdt_build_bootconfig(uint64_t mem_start, uint64_t mem_end)
{
	const int memory = OF_finddevice("/memory");
	BootConfig *bc = &bootconfig;
	struct extent_region *er;
	uint64_t addr, size;
	int index, error;

	fdt_memory_ext = extent_create("FDT Memory", mem_start, mem_end,
	    fdt_memory_ext_storage, sizeof(fdt_memory_ext_storage), EX_EARLY);

	for (index = 0;
	     fdtbus_get_reg64(memory, index, &addr, &size) == 0;
	     index++) {
		if (addr >= mem_end || size == 0)
			continue;
		if (addr + size > mem_end)
			size = mem_end - addr;

		error = extent_alloc_region(fdt_memory_ext, addr, size,
		    EX_NOWAIT);
		if (error != 0)
			printf("MEM ERROR: add %llx-%llx failed: %d\n",
			    addr, addr + size, error);
		DPRINTF("MEM: add %llx-%llx\n", addr, addr + size);
	}

	fdt_add_reserved_memory(mem_end);

	const uint64_t initrd_size = initrd_end - initrd_start;
	if (initrd_size > 0)
		fdt_add_reserved_memory_range(initrd_start, initrd_size);

	DPRINTF("Usable memory:\n");
	bc->dramblocks = 0;
	LIST_FOREACH(er, &fdt_memory_ext->ex_regions, er_link) {
		DPRINTF("  %lx - %lx\n", er->er_start, er->er_end);
		bc->dram[bc->dramblocks].address = er->er_start;
		bc->dram[bc->dramblocks].pages =
		    (er->er_end - er->er_start) / PAGE_SIZE;
		bc->dramblocks++;
	}
}

static void
fdt_probe_initrd(uint64_t *pstart, uint64_t *pend)
{
	*pstart = *pend = 0;

#ifdef MEMORY_DISK_DYNAMIC
	const int chosen = OF_finddevice("/chosen");
	if (chosen < 0)
		return;

	int len;
	const void *start_data = fdtbus_get_prop(chosen,
	    "linux,initrd-start", &len);
	const void *end_data = fdtbus_get_prop(chosen,
	    "linux,initrd-end", NULL);
	if (start_data == NULL || end_data == NULL)
		return;

	switch (len) {
	case 4:
		*pstart = be32dec(start_data);
		*pend = be32dec(end_data);
		break;
	case 8:
		*pstart = be64dec(start_data);
		*pend = be64dec(end_data);
		break;
	default:
		printf("Unsupported len %d for /chosen/initrd-start\n", len);
		return;
	}
#endif
}

static void
fdt_setup_initrd(void)
{
#ifdef MEMORY_DISK_DYNAMIC
	const uint64_t initrd_size = initrd_end - initrd_start;
	paddr_t startpa = trunc_page(initrd_start);
	paddr_t endpa = round_page(initrd_end);
	paddr_t pa;
	vaddr_t va;
	void *md_start;

	if (initrd_size == 0)
		return;

	va = uvm_km_alloc(kernel_map, initrd_size, 0,
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va == 0) {
		printf("Failed to allocate VA for initrd\n");
		return;
	}

	md_start = (void *)va;

	for (pa = startpa; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE)
		pmap_kenter_pa(va, pa, VM_PROT_READ|VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());

	md_root_setconf(md_start, initrd_size);
#endif
}

u_int initarm(void *arg);

u_int
initarm(void *arg)
{
	const struct arm_platform *plat;
	uint64_t memory_start, memory_end;

	/* Load FDT */
	int error = fdt_check_header(fdt_addr_r);
	if (error == 0) {
		error = fdt_move(fdt_addr_r, fdt_data, sizeof(fdt_data));
		if (error != 0)
			panic("fdt_move failed: %s", fdt_strerror(error));
		fdtbus_set_data(fdt_data);
	} else {
		panic("fdt_check_header failed: %s", fdt_strerror(error));
	}

	/* Lookup platform specific backend */
	plat = arm_fdt_platform();
	if (plat == NULL)
		panic("Kernel does not support this device");

	/* Early console may be available, announce ourselves. */
	DPRINT("FDT<");
	DPRINTN((uintptr_t)fdt_addr_r, 16);
	DPRINT(">");

	const int chosen = OF_finddevice("/chosen");
	if (chosen >= 0)
		OF_getprop(chosen, "bootargs", bootargs, sizeof(bootargs));
	boot_args = bootargs;

	DPRINT(" devmap");
	pmap_devmap_register(plat->devmap());
#ifdef __aarch64__
	pmap_devmap_bootstrap(plat->devmap());
#endif

	/* Heads up ... Setup the CPU / MMU / TLB functions. */
	DPRINT(" cpufunc");
	if (set_cpufuncs())
		panic("cpu not recognized!");

	DPRINT(" bootstrap");
	plat->bootstrap();

	/*
	 * If stdout-path is specified on the command line, override the
	 * value in /chosen/stdout-path before initializing console.
	 */
	fdt_update_stdout_path();

	DPRINT(" consinit");
	consinit();

	DPRINTF(" ok\n");

	DPRINTF("uboot: args %#lx, %#lx, %#lx, %#lx\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);

	cpu_reset_address = fdt_reset;
	cpu_powerdown_address = fdt_powerdown;
	evbarm_device_register = fdt_device_register;

	/* Talk to the user */
	DPRINTF("\nNetBSD/evbarm (fdt) booting ...\n");

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

#ifndef __aarch64__
	DPRINTF("KERNEL_BASE=0x%x, "
		"KERNEL_VM_BASE=0x%x, "
		"KERNEL_VM_BASE - KERNEL_BASE=0x%x, "
		"KERNEL_BASE_VOFFSET=0x%x\n",
		KERNEL_BASE,
		KERNEL_VM_BASE,
		KERNEL_VM_BASE - KERNEL_BASE,
		KERNEL_BASE_VOFFSET);
#endif

	fdt_get_memory(&memory_start, &memory_end);

#if !defined(_LP64)
	/* Cannot map memory above 4GB */
	if (memory_end >= 0x100000000ULL)
		memory_end = 0x100000000ULL - PAGE_SIZE;

	uint64_t memory_size = memory_end - memory_start;
#endif

#ifndef __aarch64__
#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	const bool mapallmem_p = true;
#ifndef PMAP_NEED_ALLOC_POOLPAGE
	if (memory_size > KERNEL_VM_BASE - KERNEL_BASE) {
		DPRINTF("%s: dropping RAM size from %luMB to %uMB\n",
		    __func__, (unsigned long) (memory_size >> 20),
		    (KERNEL_VM_BASE - KERNEL_BASE) >> 20);
		memory_size = KERNEL_VM_BASE - KERNEL_BASE;
	}
#endif
#else
	const bool mapallmem_p = false;
#endif
#endif

	/* Parse ramdisk info */
	fdt_probe_initrd(&initrd_start, &initrd_end);

	/*
	 * Populate bootconfig structure for the benefit of
	 * dodumpsys
	 */
	fdt_build_bootconfig(memory_start, memory_end);

#ifdef __aarch64__
	extern char __kernel_text[];
	extern char _end[];

	vaddr_t kernstart = trunc_page((vaddr_t)__kernel_text);
	vaddr_t kernend = round_page((vaddr_t)_end);

	paddr_t	kernstart_phys = KERN_VTOPHYS(kernstart);
	paddr_t kernend_phys = KERN_VTOPHYS(kernend);

	DPRINTF("%s: kernel phys start %lx end %lx\n", __func__, kernstart_phys, kernend_phys);

        fdt_add_reserved_memory_range(kernstart_phys,
	     kernend_phys - kernstart_phys);
#else
	arm32_bootmem_init(memory_start, memory_size, KERNEL_BASE_PHYS);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0,
	    plat->devmap(), mapallmem_p);
#endif

	DPRINTF("bootargs: %s\n", bootargs);

	parse_mi_bootargs(boot_args);

	#define MAX_PHYSMEM 16
	static struct boot_physmem fdt_physmem[MAX_PHYSMEM];
	int nfdt_physmem = 0;
	struct extent_region *er;

	LIST_FOREACH(er, &fdt_memory_ext->ex_regions, er_link) {
		DPRINTF("  %lx - %lx\n", er->er_start, er->er_end);
		struct boot_physmem *bp = &fdt_physmem[nfdt_physmem++];

		KASSERT(nfdt_physmem <= MAX_PHYSMEM);
		bp->bp_start = atop(er->er_start);
		bp->bp_pages = atop(er->er_end - er->er_start);
		bp->bp_freelist = VM_FREELIST_DEFAULT;

#ifdef _LP64
		if (er->er_end > 0x100000000)
			bp->bp_freelist = VM_FREELIST_HIGHMEM;
#endif

#ifdef PMAP_NEED_ALLOC_POOLPAGE
		if (atop(memory_size) > bp->bp_pages) {
			arm_poolpage_vmfreelist = VM_FREELIST_DIRECTMAP;
			bp->bp_freelist = VM_FREELIST_DIRECTMAP;
		}
#endif
	}

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, fdt_physmem,
	     nfdt_physmem);
}

static void
fdt_update_stdout_path(void)
{
	char *stdout_path, *ep;
	int stdout_path_len;
	char buf[256];

	const int chosen_off = fdt_path_offset(fdt_data, "/chosen");
	if (chosen_off == -1)
		return;

	if (get_bootconf_option(boot_args, "stdout-path",
	    BOOTOPT_TYPE_STRING, &stdout_path) == 0)
		return;

	ep = strchr(stdout_path, ' ');
	stdout_path_len = ep ? (ep - stdout_path) : strlen(stdout_path);
	if (stdout_path_len >= sizeof(buf))
		return;

	strncpy(buf, stdout_path, stdout_path_len);
	buf[stdout_path_len] = '\0';
	fdt_setprop(fdt_data, chosen_off, "stdout-path",
	    buf, stdout_path_len + 1);
}

void
consinit(void)
{
	static bool initialized = false;
	const struct arm_platform *plat = arm_fdt_platform();
	const struct fdt_console *cons = fdtbus_get_console();
	struct fdt_attach_args faa;
	u_int uart_freq = 0;

	if (initialized || cons == NULL)
		return;

	plat->init_attach_args(&faa);
	faa.faa_phandle = fdtbus_get_stdout_phandle();

	if (plat->uart_freq != NULL)
		uart_freq = plat->uart_freq();

	cons->consinit(&faa, uart_freq);

#if NUKBD > 0
	ukbd_cnattach();	/* allow USB keyboard to become console */
#endif

	initialized = true;
}

void
delay(u_int us)
{
	const struct arm_platform *plat = arm_fdt_platform();

	plat->delay(us);
}

static void
fdt_device_register(device_t self, void *aux)
{
	const struct arm_platform *plat = arm_fdt_platform();

	if (device_is_a(self, "armfdt"))
		fdt_setup_initrd();

	if (plat && plat->device_register)
		plat->device_register(self, aux);
}

static void
fdt_reset(void)
{
	const struct arm_platform *plat = arm_fdt_platform();

	fdtbus_power_reset();

	if (plat && plat->reset)
		plat->reset();
}

static void
fdt_powerdown(void)
{
	fdtbus_power_poweroff();
}
