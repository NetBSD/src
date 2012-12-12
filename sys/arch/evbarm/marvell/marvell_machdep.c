/*	$NetBSD: marvell_machdep.c,v 1.21 2012/12/12 00:03:11 matt Exp $ */
/*
 * Copyright (c) 2007, 2008, 2010 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: marvell_machdep.c,v 1.21 2012/12/12 00:03:11 matt Exp $");

#include "opt_evbarm_boardtype.h"
#include "opt_ddb.h"
#include "opt_pci.h"
#include "opt_mvsoc.h"
#include "com.h"
#include "gtpci.h"
#include "mvpex.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <prop/proplib.h>

#include <dev/cons.h>
#include <dev/md.h>

#include <dev/marvell/marvellreg.h>
#include <dev/marvell/marvellvar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <machine/autoconf.h>
#include <machine/bootconfig.h>
#include <machine/pci_machdep.h>

#include <uvm/uvm_extern.h>

#include <arm/db_machdep.h>
#include <arm/undefined.h>
#include <arm/arm32/machdep.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/orionreg.h>
#include <arm/marvell/kirkwoodreg.h>
#include <arm/marvell/mvsocgppvar.h>

#include <evbarm/marvell/marvellreg.h>
#include <evbarm/marvell/marvellvar.h>

#include <ddb/db_extern.h>
#include <ddb/db_sym.h>

#include "ksyms.h"


/* Kernel text starts 2MB in from the bottom of the kernel address space. */
#define KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00000000)
#define KERNEL_VM_BASE		(KERNEL_BASE + 0x02000000)

/*
 * The range 0xc2000000 - 0xdfffffff is available for kernel VM space
 * Core-logic registers and I/O mappings occupy 0xfe000000 - 0xffffffff
 */
#define KERNEL_VM_SIZE		0x1e000000

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;

extern int KERNEL_BASE_phys[];
extern char _end[];

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERNEL_BASE_PHYS	physical_start
#define KERN_VTOPHYS(va) \
	((paddr_t)((vaddr_t)va - KERNEL_BASE + KERNEL_BASE_PHYS))
#define KERN_PHYSTOV(pa) \
	((vaddr_t)((paddr_t)pa - KERNEL_BASE_PHYS + KERNEL_BASE))


#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#ifndef CONSPEED
#define CONSPEED	B115200	/* It's a setting of the default of u-boot */
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;
#endif

#include "opt_kgdb.h"
#ifdef KGDB
#include <sys/kgdb.h>
#endif

static void marvell_device_register(device_t, void *);
#if NGTPCI > 0 || NMVPEX > 0
static void marvell_startend_by_tag(int, uint64_t *, uint64_t *);
#endif

static void
marvell_system_reset(void)
{
	/* unmask soft reset */
	write_mlmbreg(MVSOC_MLMB_RSTOUTNMASKR,
	    MVSOC_MLMB_RSTOUTNMASKR_SOFTRSTOUTEN);
	/* assert soft reset */
	write_mlmbreg(MVSOC_MLMB_SSRR, MVSOC_MLMB_SSRR_SYSTEMSOFTRST);
	/* if we're still running, jump to the reset address */
	cpu_reset_address = 0;
	cpu_reset_address_paddr = 0xffff0000;
	cpu_reset();
	/*NOTREACHED*/
}

static inline
pd_entry_t *
read_ttb(void)
{
	long ttb;

	__asm volatile("mrc	p15, 0, %0, c2, c0, 0" : "=r" (ttb));

	return (pd_entry_t *)(ttb & ~((1<<14)-1));
}

/*
 * Static device mappings. These peripheral registers are mapped at
 * fixed virtual addresses very early in initarm() so that we can use
 * them while booting the kernel, and stay at the same address
 * throughout whole kernel's life time.
 *
 * We use this table twice; once with bootstrap page table, and once
 * with kernel's page table which we build up in initarm().
 *
 * Since we map these registers into the bootstrap page table using
 * pmap_devmap_bootstrap() which calls pmap_map_chunk(), we map
 * registers segment-aligned and segment-rounded in order to avoid
 * using the 2nd page tables.
 */
#define _A(a)	((a) & ~L1_S_OFFSET)
#define _S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap marvell_devmap[] = {
	{
		MARVELL_INTERREGS_VBASE,
		_A(MARVELL_INTERREGS_PBASE),
		_S(MARVELL_INTERREGS_SIZE),
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},

	{ 0, 0, 0, 0, 0 }
};

#undef  _A
#undef  _S

extern uint32_t *u_boot_args[];

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
	uint32_t target, attr, base, size;
	int cs, memtag = 0, iotag = 0, window;

	/* Use the mapped reset routine! */
	cpu_reset_address = marvell_system_reset;

	mvsoc_bootstrap(MARVELL_INTERREGS_VBASE);

	/* map some peripheral registers */
	pmap_devmap_bootstrap((vaddr_t)read_ttb(), marvell_devmap);

	/* Get ready for splfoo() */
	switch (mvsoc_model()) {
#ifdef ORION
	case MARVELL_ORION_1_88F1181:
	case MARVELL_ORION_1_88F5082:
	case MARVELL_ORION_1_88F5180N:
	case MARVELL_ORION_1_88F5181:
	case MARVELL_ORION_1_88F5182:
	case MARVELL_ORION_1_88F6082:
	case MARVELL_ORION_1_88F6183:
	case MARVELL_ORION_1_88W8660:
	case MARVELL_ORION_2_88F1281:
	case MARVELL_ORION_2_88F5281:
		orion_intr_bootstrap();

		memtag = ORION_TAG_PEX0_MEM;
		iotag = ORION_TAG_PEX0_IO;
		nwindow = ORION_MLMB_NWINDOW;
		nremap = ORION_MLMB_NREMAP;

		orion_getclks(MARVELL_INTERREGS_VBASE);
		break;
#endif	/* ORION */

#ifdef KIRKWOOD
	case MARVELL_KIRKWOOD_88F6180:
	case MARVELL_KIRKWOOD_88F6192:
	case MARVELL_KIRKWOOD_88F6281:
	case MARVELL_KIRKWOOD_88F6282:
		kirkwood_intr_bootstrap();

		memtag = KIRKWOOD_TAG_PEX_MEM;
		iotag = KIRKWOOD_TAG_PEX_IO;
		nwindow = KIRKWOOD_MLMB_NWINDOW;
		nremap = KIRKWOOD_MLMB_NREMAP;

		kirkwood_getclks(MARVELL_INTERREGS_VBASE);
		break;
#endif	/* KIRKWOOD */

#ifdef MV78XX0
	case MARVELL_MV78XX0_MV78100:
	case MARVELL_MV78XX0_MV78200:
		mv78xx0_intr_bootstrap();

		memtag = MV78XX0_TAG_PEX_MEM;
		iotag = MV78XX0_TAG_PEX_IO;
		nwindow = MV78XX0_MLMB_NWINDOW;
		nremap = MV78XX0_MLMB_NREMAP;

		mv78xx0_getclks(MARVELL_INTERREGS_VBASE);
		break;
#endif	/* MV78XX0 */

	default:
		/* We can't output console here yet... */
		panic("unknown model...\n");

		/* NOTREACHED */
	}

	/* Reset PCI-Express space to window register. */
	window = mvsoc_target(memtag, &target, &attr, NULL, NULL);
	write_mlmbreg(MVSOC_MLMB_WCR(window),
	    MVSOC_MLMB_WCR_WINEN |
	    MVSOC_MLMB_WCR_TARGET(target) |
	    MVSOC_MLMB_WCR_ATTR(attr) |
	    MVSOC_MLMB_WCR_SIZE(MARVELL_PEXMEM_SIZE));
	write_mlmbreg(MVSOC_MLMB_WBR(window),
	    MARVELL_PEXMEM_PBASE & MVSOC_MLMB_WBR_BASE_MASK);
#ifdef PCI_NETBSD_CONFIGURE
	if (window < nremap) {
		write_mlmbreg(MVSOC_MLMB_WRLR(window),
		    MARVELL_PEXMEM_PBASE & MVSOC_MLMB_WRLR_REMAP_MASK);
		write_mlmbreg(MVSOC_MLMB_WRHR(window), 0);
	}
#endif
	window = mvsoc_target(iotag, &target, &attr, NULL, NULL);
	write_mlmbreg(MVSOC_MLMB_WCR(window),
	    MVSOC_MLMB_WCR_WINEN |
	    MVSOC_MLMB_WCR_TARGET(target) |
	    MVSOC_MLMB_WCR_ATTR(attr) |
	    MVSOC_MLMB_WCR_SIZE(MARVELL_PEXIO_SIZE));
	write_mlmbreg(MVSOC_MLMB_WBR(window),
	    MARVELL_PEXIO_PBASE & MVSOC_MLMB_WBR_BASE_MASK);
#ifdef PCI_NETBSD_CONFIGURE
	if (window < nremap) {
		write_mlmbreg(MVSOC_MLMB_WRLR(window),
		    MARVELL_PEXIO_PBASE & MVSOC_MLMB_WRLR_REMAP_MASK);
		write_mlmbreg(MVSOC_MLMB_WRHR(window), 0);
	}
#endif

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/*
	 * U-Boot doesn't use the virtual memory.
	 *
	 * Physical Address Range     Description
	 * -----------------------    ----------------------------------
	 * 0x00000000 - 0x0fffffff    SDRAM Bank 0 (max 256MB)
	 * 0x10000000 - 0x1fffffff    SDRAM Bank 1 (max 256MB)
	 * 0x20000000 - 0x2fffffff    SDRAM Bank 2 (max 256MB)
	 * 0x30000000 - 0x3fffffff    SDRAM Bank 3 (max 256MB)
	 * 0xf1000000 - 0xf10fffff    SoC Internal Registers
	 */

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	consinit();

	/* Talk to the user */
#ifndef EVBARM_BOARDTYPE
#define EVBARM_BOARDTYPE	Marvell
#endif
#define BDSTR(s)	_BDSTR(s)
#define _BDSTR(s)	#s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");

	/* copy command line U-Boot gave us, if args is valid. */
	if (u_boot_args[3] != 0)	/* XXXXX: need more check?? */
		strncpy(bootargs, (char *)u_boot_args[3], sizeof(bootargs));

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	bootconfig.dramblocks = 0;
	paddr_t segment_end;
	segment_end = physmem = 0;
	for (cs = MARVELL_TAG_SDRAM_CS0; cs <= MARVELL_TAG_SDRAM_CS3; cs++) {
		mvsoc_target(cs, &target, &attr, &base, &size);
		if (size == 0)
			continue;

		bootconfig.dram[bootconfig.dramblocks].address = base;
		bootconfig.dram[bootconfig.dramblocks].pages = size / PAGE_SIZE;

		if (base != segment_end)
			panic("memory hole not support");

		segment_end += size;
		physmem += size / PAGE_SIZE;

		bootconfig.dramblocks++;
	}

	arm32_bootmem_init(0, segment_end, (uintptr_t) KERNEL_BASE_phys);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0,
	    marvell_devmap, false);

	/* we've a specific device_register routine */
	evbarm_device_register = marvell_device_register;

	/* parse bootargs from U-Boot */
	boot_args = bootargs;
	parse_mi_bootargs(boot_args);

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
}

void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if NCOM > 0
	{
		extern int mvuart_cnattach(bus_space_tag_t, bus_addr_t, int,
					   uint32_t, int);

		if (mvuart_cnattach(&mvsoc_bs_tag, 
		    MARVELL_INTERREGS_VBASE + MVSOC_COM0_BASE,
		    comcnspeed, mvTclk, comcnmode))
			panic("can't init serial console");
	}
#else
	panic("serial console not configured");
#endif
}


static void
marvell_device_register(device_t dev, void *aux)
{
	prop_dictionary_t dict = device_properties(dev);

#if NCOM > 0
	if (device_is_a(dev, "com") &&
	    device_is_a(device_parent(dev), "mvsoc"))
		prop_dictionary_set_uint32(dict, "frequency", mvTclk);
#endif
	if (device_is_a(dev, "gtidmac"))
		prop_dictionary_set_uint32(dict,
		    "dmb_speed", mvTclk * sizeof(uint32_t));	/* XXXXXX */
#if NGTPCI > 0 && defined(ORION)
	if (device_is_a(dev, "gtpci")) {
		extern struct bus_space
		    orion_pci_io_bs_tag, orion_pci_mem_bs_tag;
		extern struct arm32_pci_chipset arm32_gtpci_chipset;

		prop_data_t io_bs_tag, mem_bs_tag, pc;
		prop_array_t int2gpp;
		prop_number_t gpp;
		uint64_t start, end;
		int i, j;
		static struct {
			const char *boardtype;
			int pin[PCI_INTERRUPT_PIN_MAX];
		} hints[] = {
			{ "kuronas_x4",
			    { 11, PCI_INTERRUPT_PIN_NONE } },

			{ NULL,
			    { PCI_INTERRUPT_PIN_NONE } },
		};

		arm32_gtpci_chipset.pc_conf_v = device_private(dev);
		arm32_gtpci_chipset.pc_intr_v = device_private(dev);

		io_bs_tag = prop_data_create_data_nocopy(
		    &orion_pci_io_bs_tag, sizeof(struct bus_space));
		KASSERT(io_bs_tag != NULL);
		prop_dictionary_set(dict, "io-bus-tag", io_bs_tag);
		prop_object_release(io_bs_tag);
		mem_bs_tag = prop_data_create_data_nocopy(
		    &orion_pci_mem_bs_tag, sizeof(struct bus_space));
		KASSERT(mem_bs_tag != NULL);
		prop_dictionary_set(dict, "mem-bus-tag", mem_bs_tag);
		prop_object_release(mem_bs_tag);

		pc = prop_data_create_data_nocopy(&arm32_gtpci_chipset,
		    sizeof(struct arm32_pci_chipset));
		KASSERT(pc != NULL);
		prop_dictionary_set(dict, "pci-chipset", pc);
		prop_object_release(pc);

		marvell_startend_by_tag(ORION_TAG_PCI_IO, &start, &end);
		prop_dictionary_set_uint64(dict, "iostart", start);
		prop_dictionary_set_uint64(dict, "ioend", end);
		marvell_startend_by_tag(ORION_TAG_PCI_MEM, &start, &end);
		prop_dictionary_set_uint64(dict, "memstart", start);
		prop_dictionary_set_uint64(dict, "memend", end);
		prop_dictionary_set_uint32(dict,
		    "cache-line-size", arm_dcache_align);

		/* Setup the hint for interrupt-pin. */
#define BDSTR(s)		_BDSTR(s)
#define _BDSTR(s)		#s
#define THIS_BOARD(str)		(strcmp(str, BDSTR(EVBARM_BOARDTYPE)) == 0)
		for (i = 0; hints[i].boardtype != NULL; i++)
			if (THIS_BOARD(hints[i].boardtype))
				break;
		if (hints[i].boardtype == NULL)
			return;

		int2gpp =
		    prop_array_create_with_capacity(PCI_INTERRUPT_PIN_MAX + 1);

		/* first set dummy */
		gpp = prop_number_create_integer(0);
		prop_array_add(int2gpp, gpp);
		prop_object_release(gpp);

		for (j = 0; hints[i].pin[j] != PCI_INTERRUPT_PIN_NONE; j++) {
			gpp = prop_number_create_integer(hints[i].pin[j]);
			prop_array_add(int2gpp, gpp);
			prop_object_release(gpp);
		}
		prop_dictionary_set(dict, "int2gpp", int2gpp);
	}
#endif	/* NGTPCI > 0 && defined(ORION) */
#if NMVPEX > 0
	if (device_is_a(dev, "mvpex")) {
#ifdef ORION
		extern struct bus_space
		    orion_pex0_io_bs_tag, orion_pex0_mem_bs_tag,
		    orion_pex1_io_bs_tag, orion_pex1_mem_bs_tag;
#endif
#ifdef KIRKWOOD
		extern struct bus_space
		    kirkwood_pex_io_bs_tag, kirkwood_pex_mem_bs_tag,
		    kirkwood_pex1_io_bs_tag, kirkwood_pex1_mem_bs_tag;
#endif
		extern struct arm32_pci_chipset arm32_mvpex0_chipset;
#if defined(ORION) || defined(KIRKWOOD)
		extern struct arm32_pci_chipset arm32_mvpex1_chipset;

		struct marvell_attach_args *mva = aux;
#endif
		struct bus_space *mvpex_io_bs_tag, *mvpex_mem_bs_tag;
		struct arm32_pci_chipset *arm32_mvpex_chipset;
		prop_data_t io_bs_tag, mem_bs_tag, pc;
		uint64_t start, end;
		int iotag, memtag;

		switch (mvsoc_model()) {
#ifdef ORION
		case MARVELL_ORION_1_88F5180N:
		case MARVELL_ORION_1_88F5181:
		case MARVELL_ORION_1_88F5182:
		case MARVELL_ORION_1_88W8660:
		case MARVELL_ORION_2_88F5281:
			if (mva->mva_offset == MVSOC_PEX_BASE) {
				mvpex_io_bs_tag = &orion_pex0_io_bs_tag;
				mvpex_mem_bs_tag = &orion_pex0_mem_bs_tag;
				arm32_mvpex_chipset = &arm32_mvpex0_chipset;
				iotag = ORION_TAG_PEX0_IO;
				memtag = ORION_TAG_PEX0_MEM;
			} else {
				mvpex_io_bs_tag = &orion_pex1_io_bs_tag;
				mvpex_mem_bs_tag = &orion_pex1_mem_bs_tag;
				arm32_mvpex_chipset = &arm32_mvpex1_chipset;
				iotag = ORION_TAG_PEX1_IO;
				memtag = ORION_TAG_PEX1_MEM;
			}
			break;
#endif

#ifdef KIRKWOOD
		case MARVELL_KIRKWOOD_88F6282:
			if (mva->mva_offset != MVSOC_PEX_BASE) {
				mvpex_io_bs_tag = &kirkwood_pex1_io_bs_tag;
				mvpex_mem_bs_tag = &kirkwood_pex1_mem_bs_tag;
				arm32_mvpex_chipset = &arm32_mvpex1_chipset;
				iotag = KIRKWOOD_TAG_PEX1_IO;
				memtag = KIRKWOOD_TAG_PEX1_MEM;
				break;
			}

			/* FALLTHROUGH */

		case MARVELL_KIRKWOOD_88F6180:
		case MARVELL_KIRKWOOD_88F6192:
		case MARVELL_KIRKWOOD_88F6281:
			mvpex_io_bs_tag = &kirkwood_pex_io_bs_tag;
			mvpex_mem_bs_tag = &kirkwood_pex_mem_bs_tag;
			arm32_mvpex_chipset = &arm32_mvpex0_chipset;
			iotag = KIRKWOOD_TAG_PEX_IO;
			memtag = KIRKWOOD_TAG_PEX_MEM;
			break;
#endif

		default:
			return;
		}

		arm32_mvpex_chipset->pc_conf_v = device_private(dev);
		arm32_mvpex_chipset->pc_intr_v = device_private(dev);

		io_bs_tag = prop_data_create_data_nocopy(
		    mvpex_io_bs_tag, sizeof(struct bus_space));
		KASSERT(io_bs_tag != NULL);
		prop_dictionary_set(dict, "io-bus-tag", io_bs_tag);
		prop_object_release(io_bs_tag);
		mem_bs_tag = prop_data_create_data_nocopy(
		    mvpex_mem_bs_tag, sizeof(struct bus_space));
		KASSERT(mem_bs_tag != NULL);
		prop_dictionary_set(dict, "mem-bus-tag", mem_bs_tag);
		prop_object_release(mem_bs_tag);

		pc = prop_data_create_data_nocopy(arm32_mvpex_chipset,
		    sizeof(struct arm32_pci_chipset));
		KASSERT(pc != NULL);
		prop_dictionary_set(dict, "pci-chipset", pc);
		prop_object_release(pc);

		marvell_startend_by_tag(iotag, &start, &end);
		prop_dictionary_set_uint64(dict, "iostart", start);
		prop_dictionary_set_uint64(dict, "ioend", end);
		marvell_startend_by_tag(memtag, &start, &end);
		prop_dictionary_set_uint64(dict, "memstart", start);
		prop_dictionary_set_uint64(dict, "memend", end);
		prop_dictionary_set_uint32(dict,
		    "cache-line-size", arm_dcache_align);
	}
#endif
}

#if NGTPCI > 0 || NMVPEX > 0
static void
marvell_startend_by_tag(int tag, uint64_t *start, uint64_t *end)
{
	uint32_t base, size;
	int win;

	win = mvsoc_target(tag, NULL, NULL, &base, &size);
	if (size != 0) {
		if (win < nremap)
			*start = read_mlmbreg(MVSOC_MLMB_WRLR(win)) |
			    ((read_mlmbreg(MVSOC_MLMB_WRHR(win)) << 16) << 16);
		else
			*start = base;
		*end = *start + size - 1;
	}
}
#endif
