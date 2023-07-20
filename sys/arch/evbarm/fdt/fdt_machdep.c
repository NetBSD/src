/* $NetBSD: fdt_machdep.c,v 1.105 2023/07/10 07:01:48 rin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_machdep.c,v 1.105 2023/07/10 07:01:48 rin Exp $");

#include "opt_arm_debug.h"
#include "opt_bootconfig.h"
#include "opt_cpuoptions.h"
#include "opt_ddb.h"
#include "opt_efi.h"
#include "opt_machdep.h"
#include "opt_multiprocessor.h"

#include "genfb.h"
#include "ukbd.h"
#include "wsdisplay.h"

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/bootblock.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/endian.h>
#include <sys/exec.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/ksyms.h>
#include <sys/md5.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/pserialize.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/termios.h>
#include <sys/vnode.h>
#include <sys/uuid.h>

#include <net/if.h>
#include <net/if_dl.h>

#include <dev/cons.h>
#include <uvm/uvm_extern.h>

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

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_boot.h>
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

BootConfig bootconfig;
char *boot_args = NULL;

/* filled in before cleaning bss. keep in .data */
u_long uboot_args[4] __attribute__((__section__(".data")));
const uint8_t *fdt_addr_r __attribute__((__section__(".data")));

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>
#define FDT_BUF_SIZE	(512*1024)
static uint8_t fdt_data[FDT_BUF_SIZE];

extern char KERNEL_BASE_phys[];
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)

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

static int nfdt_physmem = 0;
static struct boot_physmem fdt_physmem[FDT_MEMORY_RANGES];

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

	KASSERT(nfdt_physmem <= FDT_MEMORY_RANGES);

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

	/* Reserve pages for ramdisk, rndseed, and firmware's RNG */
	fdt_reserve_initrd();
	fdt_reserve_rndseed();
	fdt_reserve_efirng();

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


vaddr_t
initarm(void *arg)
{
	const struct fdt_platform *plat;
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
	plat = fdt_platform_find();
	if (plat == NULL)
		panic("Kernel does not support this device");

	/* Early console may be available, announce ourselves. */
	VPRINTF("FDT<%p>\n", fdt_addr_r);

	boot_args = fdt_get_bootargs();

	/* Heads up ... Setup the CPU / MMU / TLB functions. */
	VPRINTF("cpufunc\n");
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/*
	 * Memory is still identity/flat mapped this point so using ttbr for
	 * l1pt VA is fine
	 */

	VPRINTF("devmap %p\n", plat->fp_devmap());
	extern char ARM_BOOTSTRAP_LxPT[];
	pmap_devmap_bootstrap((vaddr_t)ARM_BOOTSTRAP_LxPT, plat->fp_devmap());

	VPRINTF("bootstrap\n");
	plat->fp_bootstrap();

	/*
	 * If stdout-path is specified on the command line, override the
	 * value in /chosen/stdout-path before initializing console.
	 */
	VPRINTF("stdout\n");
	fdt_update_stdout_path(fdt_data, boot_args);

#if BYTE_ORDER == BIG_ENDIAN
	/*
	 * Most boards are configured to little-endian mode initially, and
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
	fdt_probe_initrd();

	/* Parse our on-disk rndseed and the firmware's RNG from EFI */
	fdt_probe_rndseed();
	fdt_probe_efirng();

	fdt_memory_remove_reserved(memory_start, memory_end);

	/*
	 * Populate bootconfig structure for the benefit of dodumpsys
	 */
	VPRINTF("%s: fdt_build_bootconfig\n", __func__);
	fdt_build_bootconfig(memory_start, memory_end);

	/* Perform PT build and VM init */
	cpu_kernel_vm_init(memory_start, memory_size);

	VPRINTF("bootargs: %s\n", boot_args);

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
		if (plat->fp_mpstart)
			error = plat->fp_mpstart();
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

void
consinit(void)
{
	static bool initialized = false;
	const struct fdt_platform *plat = fdt_platform_find();
	const struct fdt_console *cons = fdtbus_get_console();
	struct fdt_attach_args faa;
	u_int uart_freq = 0;

	if (initialized || cons == NULL)
		return;

	plat->fp_init_attach_args(&faa);
	faa.faa_phandle = fdtbus_get_stdout_phandle();

	if (plat->fp_uart_freq != NULL)
		uart_freq = plat->fp_uart_freq();

	cons->consinit(&faa, uart_freq);

	initialized = true;
}

void
cpu_startup_hook(void)
{
#ifdef EFI_RUNTIME
	fdt_map_efi_runtime("netbsd,uefi-runtime-code", ARM_EFIRT_MEM_CODE);
	fdt_map_efi_runtime("netbsd,uefi-runtime-data", ARM_EFIRT_MEM_DATA);
	fdt_map_efi_runtime("netbsd,uefi-runtime-mmio", ARM_EFIRT_MEM_MMIO);
#endif

	fdtbus_intr_init();

	fdt_setup_rndseed();
	fdt_setup_efirng();
}

void
delay(u_int us)
{
	const struct fdt_platform *plat = fdt_platform_find();

	plat->fp_delay(us);
}

static void
fdt_detect_root_device(device_t dev)
{
	int error, len;

	const int chosen = OF_finddevice("/chosen");
	if (chosen < 0)
		return;

	if (of_hasprop(chosen, "netbsd,mbr") &&
	    of_hasprop(chosen, "netbsd,partition")) {
		struct mbr_sector mbr;
		uint8_t buf[DEV_BSIZE];
		uint8_t hash[16];
		const uint8_t *rhash;
		struct vnode *vp;
		MD5_CTX md5ctx;
		size_t resid;
		u_int part;

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
		    IO_NODELOCKED, NOCRED, &resid, NULL);
		VOP_CLOSE(vp, FREAD, NOCRED);
		vput(vp);

		if (error != 0)
			return;

		memcpy(&mbr, buf, sizeof(mbr));
		MD5Init(&md5ctx);
		MD5Update(&md5ctx, (void *)&mbr, sizeof(mbr));
		MD5Final(hash, &md5ctx);

		if (memcmp(rhash, hash, 16) == 0) {
			booted_device = dev;
			booted_partition = part;
		}

		return;
	}

	if (of_hasprop(chosen, "netbsd,gpt-guid")) {
		const struct uuid *guid =
		    fdtbus_get_prop(chosen, "netbsd,gpt-guid", &len);

		if (guid == NULL || len != 16)
			return;

		char guidstr[UUID_STR_LEN];
		uuid_snprintf(guidstr, sizeof(guidstr), guid);

		device_t dv = dkwedge_find_by_wname(guidstr);
		if (dv != NULL)
			booted_device = dv;

		return;
	}

	if (of_hasprop(chosen, "netbsd,gpt-label")) {
		const char *label = fdtbus_get_string(chosen, "netbsd,gpt-label");
		if (label == NULL || *label == '\0')
			return;

		device_t dv = dkwedge_find_by_wname(label);
		if (dv != NULL)
			booted_device = dv;

		return;
	}

	if (of_hasprop(chosen, "netbsd,booted-mac-address")) {
		const uint8_t *macaddr =
		    fdtbus_get_prop(chosen, "netbsd,booted-mac-address", &len);
		struct ifnet *ifp;

		if (macaddr == NULL || len != 6)
			return;

		int s = pserialize_read_enter();
		IFNET_READER_FOREACH(ifp) {
			if (memcmp(macaddr, CLLADDR(ifp->if_sadl), len) == 0) {
				device_t dv = device_find_by_xname(ifp->if_xname);
				if (dv != NULL)
					booted_device = dv;
				break;
			}
		}
		pserialize_read_exit(s);

		return;
	}
}

static void
fdt_device_register(device_t self, void *aux)
{
	const struct fdt_platform *plat = fdt_platform_find();

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

	if (plat && plat->fp_device_register)
		plat->fp_device_register(self, aux);
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

	if (booted_device != NULL)
		return;

	for (dev = deviter_first(&di, 0); dev; dev = deviter_next(&di)) {
		if (device_class(dev) != DV_DISK)
			continue;

		fdt_detect_root_device(dev);

		if (booted_device != NULL)
			break;
	}
	deviter_release(&di);
}

static void
fdt_reset(void)
{
	const struct fdt_platform *plat = fdt_platform_find();

	fdtbus_power_reset();

	if (plat && plat->fp_reset)
		plat->fp_reset();
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
