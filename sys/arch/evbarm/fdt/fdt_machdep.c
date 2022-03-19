/* $NetBSD: fdt_machdep.c,v 1.89 2022/03/19 09:55:30 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_machdep.c,v 1.89 2022/03/19 09:55:30 skrll Exp $");

#include "opt_arm_debug.h"
#include "opt_bootconfig.h"
#include "opt_cpuoptions.h"
#include "opt_ddb.h"
#include "opt_efi.h"
#include "opt_machdep.h"
#include "opt_md.h"
#include "opt_multiprocessor.h"

#include "genfb.h"
#include "ukbd.h"
#include "wsdisplay.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/ksyms.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/bootblock.h>
#include <sys/disklabel.h>
#include <sys/vnode.h>
#include <sys/kauth.h>
#include <sys/fcntl.h>
#include <sys/uuid.h>
#include <sys/disk.h>
#include <sys/md5.h>
#include <sys/pserialize.h>
#include <sys/rnd.h>
#include <sys/rndsource.h>

#include <net/if.h>
#include <net/if_dl.h>

#include <dev/cons.h>
#include <uvm/uvm_extern.h>

#include <sys/conf.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/bootconfig.h>
#include <arm/armreg.h>

#include <arm/cpufunc.h>

#include <evbarm/include/autoconf.h>
#include <evbarm/fdt/machdep.h>
#include <evbarm/fdt/platform.h>

#include <arm/fdt/arm_fdtvar.h>
#include <dev/fdt/fdt_private.h>
#include <dev/fdt/fdt_memory.h>

#ifdef EFI_RUNTIME
#include <arm/arm/efi_runtime.h>
#endif

#if NWSDISPLAY > 0 && NGENFB > 0
#include <arm/fdt/arm_simplefb.h>
#endif

#if NUKBD > 0
#include <dev/usb/ukbdvar.h>
#endif
#if NWSDISPLAY > 0
#include <dev/wscons/wsdisplayvar.h>
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

/* filled in before cleaning bss. keep in .data */
u_long uboot_args[4] __attribute__((__section__(".data")));
const uint8_t *fdt_addr_r __attribute__((__section__(".data")));

static uint64_t initrd_start, initrd_end;
static uint64_t rndseed_start, rndseed_end; /* our on-disk seed */
static uint64_t efirng_start, efirng_end;   /* firmware's EFI RNG output */

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>
#define FDT_BUF_SIZE	(512*1024)
static uint8_t fdt_data[FDT_BUF_SIZE];

extern char KERNEL_BASE_phys[];
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)

static void fdt_update_stdout_path(void);
static void fdt_device_register(device_t, void *);
static void fdt_device_register_post_config(device_t, void *);
static void fdt_cpu_rootconf(void);
static void fdt_reset(void);
static void fdt_powerdown(void);

#if BYTE_ORDER == BIG_ENDIAN
static void fdt_update_fb_format(void);
#endif

static void
earlyconsputc(dev_t dev, int c)
{
	uartputc(c);
}

static int
earlyconsgetc(dev_t dev)
{
	return -1;
}

static struct consdev earlycons = {
	.cn_putc = earlyconsputc,
	.cn_getc = earlyconsgetc,
	.cn_pollc = nullcnpollc,
};

#ifdef VERBOSE_INIT_ARM
#define VPRINTF(...)	printf(__VA_ARGS__)
#else
#define VPRINTF(...)	__nothing
#endif

static void
fdt_add_dram_blocks(const struct fdt_memory *m, void *arg)
{
	BootConfig *bc = arg;

	VPRINTF("  %" PRIx64 " - %" PRIx64 "\n", m->start, m->end - 1);
	bc->dram[bc->dramblocks].address = m->start;
	bc->dram[bc->dramblocks].pages =
	    (m->end - m->start) / PAGE_SIZE;
	bc->dramblocks++;
}

#define MAX_PHYSMEM 64
static int nfdt_physmem = 0;
static struct boot_physmem fdt_physmem[MAX_PHYSMEM];

static void
fdt_add_boot_physmem(const struct fdt_memory *m, void *arg)
{
	const paddr_t saddr = round_page(m->start);
	const paddr_t eaddr = trunc_page(m->end);

	VPRINTF("  %" PRIx64 " - %" PRIx64, m->start, m->end - 1);
	if (saddr >= eaddr) {
		VPRINTF(" skipped\n");
		return;
	}
	VPRINTF("\n");

	struct boot_physmem *bp = &fdt_physmem[nfdt_physmem++];

	KASSERT(nfdt_physmem <= MAX_PHYSMEM);

	bp->bp_start = atop(saddr);
	bp->bp_pages = atop(eaddr) - bp->bp_start;
	bp->bp_freelist = VM_FREELIST_DEFAULT;

#ifdef PMAP_NEED_ALLOC_POOLPAGE
	const uint64_t memory_size = *(uint64_t *)arg;
	if (atop(memory_size) > bp->bp_pages) {
		arm_poolpage_vmfreelist = VM_FREELIST_DIRECTMAP;
		bp->bp_freelist = VM_FREELIST_DIRECTMAP;
	}
#endif
}


static void
fdt_print_memory(const struct fdt_memory *m, void *arg)
{

	VPRINTF("FDT /memory @ 0x%" PRIx64 " size 0x%" PRIx64 "\n",
	    m->start, m->end - m->start);
}


/*
 * Define usable memory regions.
 */
static void
fdt_build_bootconfig(uint64_t mem_start, uint64_t mem_end)
{
	BootConfig *bc = &bootconfig;

	uint64_t addr, size;
	int index;

	fdt_memory_remove_reserved(mem_start, mem_end);

	const uint64_t initrd_size =
	    round_page(initrd_end) - trunc_page(initrd_start);
	if (initrd_size > 0)
		fdt_memory_remove_range(trunc_page(initrd_start), initrd_size);

	const uint64_t rndseed_size =
	    round_page(rndseed_end) - trunc_page(rndseed_start);
	if (rndseed_size > 0)
		fdt_memory_remove_range(trunc_page(rndseed_start),
		    rndseed_size);

	const uint64_t efirng_size =
	    round_page(efirng_end) - trunc_page(efirng_start);
	if (efirng_size > 0)
		fdt_memory_remove_range(trunc_page(efirng_start), efirng_size);

	const int framebuffer = OF_finddevice("/chosen/framebuffer");
	if (framebuffer >= 0) {
		for (index = 0;
		     fdtbus_get_reg64(framebuffer, index, &addr, &size) == 0;
		     index++) {
			fdt_memory_remove_range(addr, size);
		}
	}

	VPRINTF("Usable memory:\n");
	bc->dramblocks = 0;
	fdt_memory_foreach(fdt_add_dram_blocks, bc);
}

static void
fdt_probe_range(const char *startname, const char *endname,
    uint64_t *pstart, uint64_t *pend)
{
	int chosen, len;
	const void *start_data, *end_data;

	*pstart = *pend = 0;

	chosen = OF_finddevice("/chosen");
	if (chosen < 0)
		return;

	start_data = fdtbus_get_prop(chosen, startname, &len);
	end_data = fdtbus_get_prop(chosen, endname, NULL);
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
		printf("Unsupported len %d for /chosen `%s'\n",
		    len, startname);
		return;
	}
}

static void *
fdt_map_range(uint64_t start, uint64_t end, uint64_t *psize,
    const char *purpose)
{
	const paddr_t startpa = trunc_page(start);
	const paddr_t endpa = round_page(end);
	paddr_t pa;
	vaddr_t va;
	void *ptr;

	*psize = end - start;
	if (*psize == 0)
		return NULL;

	const vaddr_t voff = start & PAGE_MASK;

	va = uvm_km_alloc(kernel_map, *psize, 0, UVM_KMF_VAONLY|UVM_KMF_NOWAIT);
	if (va == 0) {
		printf("Failed to allocate VA for %s\n", purpose);
		return NULL;
	}
	ptr = (void *)(va + voff);

	for (pa = startpa; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE)
		pmap_kenter_pa(va, pa, VM_PROT_READ|VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());

	return ptr;
}

static void
fdt_unmap_range(void *ptr, uint64_t size)
{
	const char *start = ptr, *end = start + size;
	const vaddr_t startva = trunc_page((vaddr_t)(uintptr_t)start);
	const vaddr_t endva = round_page((vaddr_t)(uintptr_t)end);

	pmap_kremove(startva, endva - startva);
	pmap_update(pmap_kernel());
}

static void
fdt_probe_initrd(uint64_t *pstart, uint64_t *pend)
{
	*pstart = *pend = 0;

#ifdef MEMORY_DISK_DYNAMIC
	fdt_probe_range("linux,initrd-start", "linux,initrd-end", pstart, pend);
#endif
}

static void
fdt_setup_initrd(void)
{
#ifdef MEMORY_DISK_DYNAMIC
	void *md_start;
	uint64_t initrd_size;

	md_start = fdt_map_range(initrd_start, initrd_end, &initrd_size,
	    "initrd");
	if (md_start == NULL)
		return;
	md_root_setconf(md_start, initrd_size);
#endif
}

static void
fdt_probe_rndseed(uint64_t *pstart, uint64_t *pend)
{

	fdt_probe_range("netbsd,rndseed-start", "netbsd,rndseed-end",
	    pstart, pend);
}

static void
fdt_setup_rndseed(void)
{
	uint64_t rndseed_size;
	void *rndseed;

	rndseed = fdt_map_range(rndseed_start, rndseed_end, &rndseed_size,
	    "rndseed");
	if (rndseed == NULL)
		return;
	rnd_seed(rndseed, rndseed_size);
	fdt_unmap_range(rndseed, rndseed_size);
}

static void
fdt_probe_efirng(uint64_t *pstart, uint64_t *pend)
{

	fdt_probe_range("netbsd,efirng-start", "netbsd,efirng-end",
	    pstart, pend);
}

static struct krndsource efirng_source;

static void
fdt_setup_efirng(void)
{
	uint64_t efirng_size;
	void *efirng;

	efirng = fdt_map_range(efirng_start, efirng_end, &efirng_size,
	    "efirng");
	if (efirng == NULL)
		return;

	rnd_attach_source(&efirng_source, "efirng", RND_TYPE_RNG,
	    RND_FLAG_DEFAULT);

	/*
	 * We don't really have specific information about the physical
	 * process underlying the data provided by the firmware via the
	 * EFI RNG API, so the entropy estimate here is heuristic.
	 * What efiboot provides us is up to 4096 bytes of data from
	 * the EFI RNG API, although in principle it may return short.
	 *
	 * The UEFI Specification (2.8 Errata A, February 2020[1]) says
	 *
	 *	When a Deterministic Random Bit Generator (DRBG) is
	 *	used on the output of a (raw) entropy source, its
	 *	security level must be at least 256 bits.
	 *
	 * It's not entirely clear whether `it' refers to the DRBG or
	 * the entropy source; if it refers to the DRBG, it's not
	 * entirely clear how ANSI X9.31 3DES, one of the options for
	 * DRBG in the UEFI spec, can provide a `256-bit security
	 * level' because it has only 232 bits of inputs (three 56-bit
	 * keys and one 64-bit block).  That said, even if it provides
	 * only 232 bits of entropy, that's enough to prevent all
	 * attacks and we probably get a few more bits from sampling
	 * the clock anyway.
	 *
	 * In the event we get raw samples, e.g. the bits sampled by a
	 * ring oscillator, we hope that the samples have at least half
	 * a bit of entropy per bit of data -- and efiboot tries to
	 * draw 4096 bytes to provide plenty of slop.  Hence we divide
	 * the total number of bits by two and clamp at 256.  There are
	 * ways this could go wrong, but on most machines it should
	 * behave reasonably.
	 *
	 * [1] https://uefi.org/sites/default/files/resources/UEFI_Spec_2_8_A_Feb14.pdf
	 */
	rnd_add_data(&efirng_source, efirng, efirng_size,
	    MIN(256, efirng_size*NBBY/2));

	explicit_memset(efirng, 0, efirng_size);
	fdt_unmap_range(efirng, efirng_size);
}

#ifdef EFI_RUNTIME
static void
fdt_map_efi_runtime(const char *prop, enum arm_efirt_mem_type type)
{
	int len;

	const int chosen_off = fdt_path_offset(fdt_data, "/chosen");
	if (chosen_off < 0)
		return;

	const uint64_t *map = fdt_getprop(fdt_data, chosen_off, prop, &len);
	if (map == NULL)
		return;

	while (len >= 24) {
		const paddr_t pa = be64toh(map[0]);
		const vaddr_t va = be64toh(map[1]);
		const size_t sz = be64toh(map[2]);
		VPRINTF("%s: %s %#" PRIxPADDR "-%#" PRIxVADDR " (%#" PRIxVADDR
		    "-%#" PRIxVSIZE ")\n", __func__, prop, pa, pa + sz - 1,
		    va, va + sz - 1);
		arm_efirt_md_map_range(va, pa, sz, type);
		map += 3;
		len -= 24;
	}
}
#endif

vaddr_t
initarm(void *arg)
{
	const struct arm_platform *plat;
	uint64_t memory_start, memory_end;

	/* set temporally to work printf()/panic() even before consinit() */
	cn_tab = &earlycons;

	/* Load FDT */
	int error = fdt_check_header(fdt_addr_r);
	if (error != 0)
		panic("fdt_check_header failed: %s", fdt_strerror(error));

	/* If the DTB is too big, try to pack it in place first. */
	if (fdt_totalsize(fdt_addr_r) > sizeof(fdt_data))
		(void)fdt_pack(__UNCONST(fdt_addr_r));

	error = fdt_open_into(fdt_addr_r, fdt_data, sizeof(fdt_data));
	if (error != 0)
		panic("fdt_move failed: %s", fdt_strerror(error));

	fdtbus_init(fdt_data);

	/* Lookup platform specific backend */
	plat = arm_fdt_platform();
	if (plat == NULL)
		panic("Kernel does not support this device");

	/* Early console may be available, announce ourselves. */
	VPRINTF("FDT<%p>\n", fdt_addr_r);

	const int chosen = OF_finddevice("/chosen");
	if (chosen >= 0)
		OF_getprop(chosen, "bootargs", bootargs, sizeof(bootargs));
	boot_args = bootargs;

	/* Heads up ... Setup the CPU / MMU / TLB functions. */
	VPRINTF("cpufunc\n");
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/*
	 * Memory is still identity/flat mapped this point so using ttbr for
	 * l1pt VA is fine
	 */

	VPRINTF("devmap %p\n", plat->ap_devmap());
	extern char ARM_BOOTSTRAP_LxPT[];
	pmap_devmap_bootstrap((vaddr_t)ARM_BOOTSTRAP_LxPT, plat->ap_devmap());

	VPRINTF("bootstrap\n");
	plat->ap_bootstrap();

	/*
	 * If stdout-path is specified on the command line, override the
	 * value in /chosen/stdout-path before initializing console.
	 */
	VPRINTF("stdout\n");
	fdt_update_stdout_path();

#if BYTE_ORDER == BIG_ENDIAN
	/*
	 * Most boards are configured to little-endian mode in initial, and
	 * switched to big-endian mode after kernel is loaded. In this case,
	 * framebuffer seems byte-swapped to CPU. Override FDT to let
	 * drivers know.
	 */
	VPRINTF("fb_format\n");
	fdt_update_fb_format();
#endif

	/*
	 * Done making changes to the FDT.
	 */
	fdt_pack(fdt_data);

	VPRINTF("consinit ");
	consinit();
	VPRINTF("ok\n");

	VPRINTF("uboot: args %#lx, %#lx, %#lx, %#lx\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);

	cpu_reset_address = fdt_reset;
	cpu_powerdown_address = fdt_powerdown;
	evbarm_device_register = fdt_device_register;
	evbarm_device_register_post_config = fdt_device_register_post_config;
	evbarm_cpu_rootconf = fdt_cpu_rootconf;

	/* Talk to the user */
	printf("NetBSD/evbarm (fdt) booting ...\n");

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

	fdt_memory_get(&memory_start, &memory_end);

	fdt_memory_foreach(fdt_print_memory, NULL);

#if !defined(_LP64)
	/* Cannot map memory above 4GB (remove last page as well) */
	const uint64_t memory_limit = 0x100000000ULL - PAGE_SIZE;
	if (memory_end > memory_limit) {
		fdt_memory_remove_range(memory_limit , memory_end);
		memory_end = memory_limit;
	}
#endif
	uint64_t memory_size = memory_end - memory_start;

	VPRINTF("%s: memory start %" PRIx64 " end %" PRIx64 " (len %"
	    PRIx64 ")\n", __func__, memory_start, memory_end, memory_size);

	/* Parse ramdisk info */
	fdt_probe_initrd(&initrd_start, &initrd_end);

	/* Parse our on-disk rndseed and the firmware's RNG from EFI */
	fdt_probe_rndseed(&rndseed_start, &rndseed_end);
	fdt_probe_efirng(&efirng_start, &efirng_end);

	/*
	 * Populate bootconfig structure for the benefit of dodumpsys
	 */
	VPRINTF("%s: fdt_build_bootconfig\n", __func__);
	fdt_build_bootconfig(memory_start, memory_end);

#ifdef EFI_RUNTIME
	fdt_map_efi_runtime("netbsd,uefi-runtime-code", ARM_EFIRT_MEM_CODE);
	fdt_map_efi_runtime("netbsd,uefi-runtime-data", ARM_EFIRT_MEM_DATA);
	fdt_map_efi_runtime("netbsd,uefi-runtime-mmio", ARM_EFIRT_MEM_MMIO);
#endif

	/* Perform PT build and VM init */
	cpu_kernel_vm_init(memory_start, memory_size);

	VPRINTF("bootargs: %s\n", bootargs);

	parse_mi_bootargs(boot_args);

	VPRINTF("Memory regions:\n");

	/* Populate fdt_physmem / nfdt_physmem for initarm_common */
	fdt_memory_foreach(fdt_add_boot_physmem, &memory_size);

	vaddr_t sp = initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, fdt_physmem,
	     nfdt_physmem);

	/*
	 * initarm_common flushes cache if required before AP start
	 */
	error = 0;
	if ((boothowto & RB_MD1) == 0) {
		VPRINTF("mpstart\n");
		if (plat->ap_mpstart)
			error = plat->ap_mpstart();
	}

	if (error)
		return sp;

	/*
	 * Now we have APs started the pages used for stacks and L1PT can
	 * be given to uvm
	 */
	extern char const __start__init_memory[];
	extern char const __stop__init_memory[] __weak;

	if (__start__init_memory != __stop__init_memory) {
		const paddr_t spa = KERN_VTOPHYS((vaddr_t)__start__init_memory);
		const paddr_t epa = KERN_VTOPHYS((vaddr_t)__stop__init_memory);
		const paddr_t spg = atop(spa);
		const paddr_t epg = atop(epa);

		VPRINTF("         start %08lx  end %08lx... "
		    "loading in freelist %d\n", spa, epa, VM_FREELIST_DEFAULT);

		uvm_page_physload(spg, epg, spg, epg, VM_FREELIST_DEFAULT);

	}

	return sp;
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

	plat->ap_init_attach_args(&faa);
	faa.faa_phandle = fdtbus_get_stdout_phandle();

	if (plat->ap_uart_freq != NULL)
		uart_freq = plat->ap_uart_freq();

	cons->consinit(&faa, uart_freq);

	initialized = true;
}

void
cpu_startup_hook(void)
{

	fdtbus_intr_init();

	fdt_setup_rndseed();
	fdt_setup_efirng();
}

void
delay(u_int us)
{
	const struct arm_platform *plat = arm_fdt_platform();

	plat->ap_delay(us);
}

static void
fdt_detect_root_device(device_t dev)
{
	struct mbr_sector mbr;
	uint8_t buf[DEV_BSIZE];
	uint8_t hash[16];
	const uint8_t *rhash;
	char rootarg[64];
	struct vnode *vp;
	MD5_CTX md5ctx;
	int error, len;
	size_t resid;
	u_int part;

	const int chosen = OF_finddevice("/chosen");
	if (chosen < 0)
		return;

	if (of_hasprop(chosen, "netbsd,mbr") &&
	    of_hasprop(chosen, "netbsd,partition")) {

		/*
		 * The bootloader has passed in a partition index and MD5 hash
		 * of the MBR sector. Read the MBR of this device, calculate the
		 * hash, and compare it with the value passed in.
		 */
		rhash = fdtbus_get_prop(chosen, "netbsd,mbr", &len);
		if (rhash == NULL || len != 16)
			return;
		of_getprop_uint32(chosen, "netbsd,partition", &part);
		if (part >= MAXPARTITIONS)
			return;

		vp = opendisk(dev);
		if (!vp)
			return;
		error = vn_rdwr(UIO_READ, vp, buf, sizeof(buf), 0, UIO_SYSSPACE,
		    0, NOCRED, &resid, NULL);
		VOP_CLOSE(vp, FREAD, NOCRED);
		vput(vp);

		if (error != 0)
			return;

		memcpy(&mbr, buf, sizeof(mbr));
		MD5Init(&md5ctx);
		MD5Update(&md5ctx, (void *)&mbr, sizeof(mbr));
		MD5Final(hash, &md5ctx);

		if (memcmp(rhash, hash, 16) != 0)
			return;

		snprintf(rootarg, sizeof(rootarg), " root=%s%c", device_xname(dev), part + 'a');
		strcat(boot_args, rootarg);
	}

	if (of_hasprop(chosen, "netbsd,gpt-guid")) {
		char guidbuf[UUID_STR_LEN];
		const struct uuid *guid = fdtbus_get_prop(chosen, "netbsd,gpt-guid", &len);
		if (guid == NULL || len != 16)
			return;

		uuid_snprintf(guidbuf, sizeof(guidbuf), guid);
		snprintf(rootarg, sizeof(rootarg), " root=wedge:%s", guidbuf);
		strcat(boot_args, rootarg);
	}

	if (of_hasprop(chosen, "netbsd,gpt-label")) {
		const char *label = fdtbus_get_string(chosen, "netbsd,gpt-label");
		if (label == NULL || *label == '\0')
			return;

		device_t dv = dkwedge_find_by_wname(label);
		if (dv != NULL)
			booted_device = dv;
	}

	if (of_hasprop(chosen, "netbsd,booted-mac-address")) {
		const uint8_t *macaddr = fdtbus_get_prop(chosen, "netbsd,booted-mac-address", &len);
		if (macaddr == NULL || len != 6)
			return;
		int s = pserialize_read_enter();
		struct ifnet *ifp;
		IFNET_READER_FOREACH(ifp) {
			if (memcmp(macaddr, CLLADDR(ifp->if_sadl), len) == 0) {
				device_t dv = device_find_by_xname(ifp->if_xname);
				if (dv != NULL)
					booted_device = dv;
				break;
			}
		}
		pserialize_read_exit(s);
	}
}

static void
fdt_device_register(device_t self, void *aux)
{
	const struct arm_platform *plat = arm_fdt_platform();

	if (device_is_a(self, "armfdt")) {
		fdt_setup_initrd();

#if NWSDISPLAY > 0 && NGENFB > 0
		/*
		 * Setup framebuffer console, if present.
		 */
		arm_simplefb_preattach();
#endif
	}

#if NWSDISPLAY > 0 && NGENFB > 0
	if (device_is_a(self, "genfb")) {
		prop_dictionary_t dict = device_properties(self);
		prop_dictionary_set_uint64(dict,
		    "simplefb-physaddr", arm_simplefb_physaddr());
	}
#endif

	if (plat && plat->ap_device_register)
		plat->ap_device_register(self, aux);
}

static void
fdt_device_register_post_config(device_t self, void *aux)
{
#if NUKBD > 0 && NWSDISPLAY > 0
	if (device_is_a(self, "wsdisplay")) {
		struct wsdisplay_softc *sc = device_private(self);
		if (wsdisplay_isconsole(sc))
			ukbd_cnattach();
	}
#endif
}

static void
fdt_cpu_rootconf(void)
{
	device_t dev;
	deviter_t di;
	char *ptr;

	if (booted_device != NULL)
		return;

	for (dev = deviter_first(&di, 0); dev; dev = deviter_next(&di)) {
		if (device_class(dev) != DV_DISK)
			continue;

		if (get_bootconf_option(boot_args, "root", BOOTOPT_TYPE_STRING, &ptr) != 0)
			break;

		if (device_is_a(dev, "ld") || device_is_a(dev, "sd") || device_is_a(dev, "wd"))
			fdt_detect_root_device(dev);
	}
	deviter_release(&di);
}

static void
fdt_reset(void)
{
	const struct arm_platform *plat = arm_fdt_platform();

	fdtbus_power_reset();

	if (plat && plat->ap_reset)
		plat->ap_reset();
}

static void
fdt_powerdown(void)
{
	fdtbus_power_poweroff();
}

#if BYTE_ORDER == BIG_ENDIAN
static void
fdt_update_fb_format(void)
{
	int off, len;
	const char *format, *replace;

	off = fdt_path_offset(fdt_data, "/chosen");
	if (off < 0)
		return;

	for (;;) {
		off = fdt_node_offset_by_compatible(fdt_data, off,
		    "simple-framebuffer");
		if (off < 0)
			return;

		format = fdt_getprop(fdt_data, off, "format", &len);
		if (format == NULL)
			continue;

		replace = NULL;
		if (strcmp(format, "a8b8g8r8") == 0)
			replace = "r8g8b8a8";
		else if (strcmp(format, "x8r8g8b8") == 0)
			replace = "b8g8r8x8";
		if (replace != NULL)
			fdt_setprop(fdt_data, off, "format", replace,
			    strlen(replace) + 1);
	}
}
#endif
