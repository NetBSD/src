/*	$NetBSD: armadaxp_machdep.c,v 1.2.2.3 2014/08/20 00:02:52 tls Exp $	*/
/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

Developed by Semihalf

********************************************************************************
Marvell BSD License

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
            this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: armadaxp_machdep.c,v 1.2.2.3 2014/08/20 00:02:52 tls Exp $");

#include "opt_machdep.h"
#include "opt_mvsoc.h"
#include "opt_evbarm_boardtype.h"
#include "opt_com.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_pci.h"
#include "opt_ipkdb.h"

#include <sys/bus.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <sys/conf.h>
#include <dev/cons.h>
#include <dev/md.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <machine/pci_machdep.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <machine/bootconfig.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/armreg.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/armadaxpreg.h>

#include <evbarm/marvell/marvellreg.h>
#include <evbarm/marvell/marvellvar.h>

#include "mvpex.h"
#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependent as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */

BootConfig bootconfig;		/* Boot config storage */
char *boot_args = NULL;
char *boot_file = NULL;

extern int KERNEL_BASE_phys[];

/*
 * Put some bogus settings of the MEMSTART and MEMSIZE
 * if they are not defined in kernel configuration file.
 */
#ifndef MEMSTART
#define MEMSTART 0x00000000UL
#endif
#ifndef MEMSIZE
#define MEMSIZE 0x40000000UL
#endif

#ifndef STARTUP_PAGETABLE_ADDR
#define	STARTUP_PAGETABLE_ADDR 0x00000000UL
#endif

/* Physical offset of the kernel from MEMSTART */
#define KERNEL_OFFSET		(paddr_t)&KERNEL_BASE_phys
/* Kernel base virtual address */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + KERNEL_OFFSET)

#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x40000000)
#define KERNEL_VM_SIZE		0x14000000

/* Prototypes */
extern int armadaxp_l2_init(bus_addr_t);
extern void armadaxp_io_coherency_init(void);

void consinit(void);
#ifdef KGDB
static void kgdb_port_init(void);
#endif

static void axp_device_register(device_t dev, void *aux);

static void
axp_system_reset(void)
{
	extern vaddr_t misc_base;

#define write_miscreg(r, v)	(*(volatile uint32_t *)(misc_base + (r)) = (v))

	cpu_reset_address = 0;

	/* Unmask soft reset */
	write_miscreg(ARMADAXP_MISC_RSTOUTNMASKR,
	    ARMADAXP_MISC_RSTOUTNMASKR_GLOBALSOFTRSTOUTEN);
	/* Assert soft reset */
	write_miscreg(ARMADAXP_MISC_SSRR, ARMADAXP_MISC_SSRR_GLOBALSOFTRST);

	while (1);
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

#define	_A(a)	((a) & ~L1_S_OFFSET)
#define	_S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap devmap[] = {
	{
		/* Internal registers */
		.pd_va = _A(MARVELL_INTERREGS_VBASE),
		.pd_pa = _A(MARVELL_INTERREGS_PBASE),
		.pd_size = _S(MARVELL_INTERREGS_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{0, 0, 0, 0, 0}
};

#undef	_A
#undef	_S

static inline pd_entry_t *
read_ttb(void)
{
	return (pd_entry_t *)(armreg_ttbr_read() & ~((1<<14)-1));
}

static int
axp_pcie_free_win(void)
{
	/* Find first disabled window */
	for (size_t i = 0; i < ARMADAXP_MLMB_NWINDOW; i++) {
		if ((read_mlmbreg(MVSOC_MLMB_WCR(i)) &
		    MVSOC_MLMB_WCR_WINEN) == 0) {
			return i;
		}
	}
	/* If there is no free window, return erroneous value */
	return (-1);
}

static void
reset_axp_pcie_win(void)
{
	uint32_t target, attr;
	int memtag = 0, iotag = 0, window, i;
	uint32_t membase;
	uint32_t iobase;
	uint32_t tags[] = { ARMADAXP_TAG_PEX00_MEM, ARMADAXP_TAG_PEX00_IO,
			    ARMADAXP_TAG_PEX01_MEM, ARMADAXP_TAG_PEX01_IO,
			    ARMADAXP_TAG_PEX02_MEM, ARMADAXP_TAG_PEX02_IO,
			    ARMADAXP_TAG_PEX03_MEM, ARMADAXP_TAG_PEX03_IO,
			    ARMADAXP_TAG_PEX2_MEM, ARMADAXP_TAG_PEX2_IO,
			    ARMADAXP_TAG_PEX3_MEM, ARMADAXP_TAG_PEX3_IO};

	nwindow = ARMADAXP_MLMB_NWINDOW;
	nremap = ARMADAXP_MLMB_NREMAP;
	membase = MARVELL_PEXMEM_PBASE;
	iobase = MARVELL_PEXIO_PBASE;
	for (i = 0; i < __arraycount(tags) / 2; i++) {
		memtag = tags[2 * i];
		iotag = tags[(2 * i) + 1];

		/* Reset PCI-Express space to window register. */
		window = mvsoc_target(memtag, &target, &attr, NULL, NULL);

		/* Find free window if we've got spurious one */
		if (window >= nwindow) {
			window = axp_pcie_free_win();
			/* Just break if there is no free windows left */
			if (window < 0) {
				aprint_error(": no free windows for PEX MEM\n");
				break;
			}
		}
		write_mlmbreg(MVSOC_MLMB_WCR(window),
		    MVSOC_MLMB_WCR_WINEN |
		    MVSOC_MLMB_WCR_TARGET(target) |
		    MVSOC_MLMB_WCR_ATTR(attr) |
		    MVSOC_MLMB_WCR_SIZE(MARVELL_PEXMEM_SIZE));
		write_mlmbreg(MVSOC_MLMB_WBR(window),
		    membase & MVSOC_MLMB_WBR_BASE_MASK);
#ifdef PCI_NETBSD_CONFIGURE
		if (window < nremap) {
			write_mlmbreg(MVSOC_MLMB_WRLR(window),
			    membase & MVSOC_MLMB_WRLR_REMAP_MASK);
			write_mlmbreg(MVSOC_MLMB_WRHR(window), 0);
		}
#endif
		window = mvsoc_target(iotag, &target, &attr, NULL, NULL);

		/* Find free window if we've got spurious one */
		if (window >= nwindow) {
			window = axp_pcie_free_win();
			/* Just break if there is no free windows left */
			if (window < 0) {
				aprint_error(": no free windows for PEX I/O\n");
				break;
			}
		}
		write_mlmbreg(MVSOC_MLMB_WCR(window),
		    MVSOC_MLMB_WCR_WINEN |
		    MVSOC_MLMB_WCR_TARGET(target) |
		    MVSOC_MLMB_WCR_ATTR(attr) |
		    MVSOC_MLMB_WCR_SIZE(MARVELL_PEXIO_SIZE));
		write_mlmbreg(MVSOC_MLMB_WBR(window),
		    iobase & MVSOC_MLMB_WBR_BASE_MASK);
#ifdef PCI_NETBSD_CONFIGURE
		if (window < nremap) {
			write_mlmbreg(MVSOC_MLMB_WRLR(window),
			    iobase & MVSOC_MLMB_WRLR_REMAP_MASK);
			write_mlmbreg(MVSOC_MLMB_WRHR(window), 0);
		}
#endif
		membase += MARVELL_PEXMEM_SIZE;
		iobase += MARVELL_PEXIO_SIZE;
	}
}

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
	cpu_reset_address = axp_system_reset;

	mvsoc_bootstrap(MARVELL_INTERREGS_VBASE);

	/* Set CPU functions */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/*
	 * Map devices into the initial page table
	 * in order to use early console during initialization process.
	 * consinit is going to use this mapping.
	 */
	pmap_devmap_bootstrap((vaddr_t)read_ttb(), devmap);

	/* Initialize system console */
	consinit();

	/* Reset PCI-Express space to window register. */
	reset_axp_pcie_win();

	/* Get CPU, system and timebase frequencies */
	extern vaddr_t misc_base;
	misc_base = MARVELL_INTERREGS_VBASE + ARMADAXP_MISC_BASE;
	armadaxp_getclks();
	mvsoc_clkgating = armadaxp_clkgating;

	/* Preconfigure interrupts */
	armadaxp_intr_bootstrap(MARVELL_INTERREGS_PBASE);

#ifdef L2CACHE_ENABLE
	/* Initialize L2 Cache */
	(void)armadaxp_l2_init(MARVELL_INTERREGS_PBASE);
#endif

#ifdef AURORA_IO_CACHE_COHERENCY
	/* Initialize cache coherency */
	armadaxp_io_coherency_init();
#endif

#ifdef KGDB
	kgdb_port_init();
#endif

#ifdef VERBOSE_INIT_ARM
	/* Talk to the user */
#define	BDSTR(s)	_BDSTR(s)
#define	_BDSTR(s)	#s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");
#endif

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	const bool mapallmem_p = true;
#else
	const bool mapallmem_p = false;
#endif

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif
	psize_t memsize = MEMSIZE;
	if (mapallmem_p && memsize > KERNEL_VM_BASE - KERNEL_BASE) {
		printf("%s: dropping RAM size from %luMB to %uMB\n",
		    __func__, (unsigned long) (memsize >> 20),
		    (KERNEL_VM_BASE - KERNEL_BASE) >> 20);
		memsize = KERNEL_VM_BASE - KERNEL_BASE;
	}
	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = MEMSTART;
	bootconfig.dram[0].pages = memsize / PAGE_SIZE;

        physical_start = bootconfig.dram[0].address;
        physical_end = physical_start + (bootconfig.dram[0].pages * PAGE_SIZE);

	arm32_bootmem_init(0, physical_end, (uintptr_t) KERNEL_BASE_phys);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_LOW, 0,
	    devmap, mapallmem_p);

	/* we've a specific device_register routine */
	evbarm_device_register = axp_device_register;

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
}

#ifndef CONSADDR
#error Specify the address of the UART with the CONSADDR option.
#endif
#ifndef CONSPEED
#define	CONSPEED B115200
#endif
#ifndef CONMODE
#define	CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#ifndef CONSFREQ
#define	CONSFREQ 0
#endif
static const int	comcnspeed = CONSPEED;
static const int	comcnfreq  = CONSFREQ;
static const tcflag_t	comcnmode  = CONMODE;
static const bus_addr_t	comcnaddr  = (bus_addr_t)CONSADDR;

void
consinit(void)
{
	static bool consinit_called = false;

	if (consinit_called)
		return;
	consinit_called = true;

#if NCOM > 0
	extern int mvuart_cnattach(bus_space_tag_t, bus_addr_t, int,
	    uint32_t, int);

	if (mvuart_cnattach(&mvsoc_bs_tag, comcnaddr, comcnspeed,
			comcnfreq ? comcnfreq : mvTclk , comcnmode))
		panic("Serial console can not be initialized.");
#endif
}

#ifdef KGDB
#ifndef KGDB_DEVADDR
#error Specify the address of the kgdb UART with the KGDB_DEVADDR option.
#endif
#ifndef KGDB_DEVRATE
#define KGDB_DEVRATE B115200
#endif
#define MVUART_SIZE 0x20

#ifndef KGDB_DEVMODE
#define KGDB_DEVMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
static const vaddr_t comkgdbaddr = KGDB_DEVADDR;
static const int comkgdbspeed = KGDB_DEVRATE;
static const int comkgdbmode = KGDB_DEVMODE;

void
static kgdb_port_init(void)
{
	static int kgdbsinit_called = 0;

	if (kgdbsinit_called != 0)
		return;
	kgdbsinit_called = 1;

	if (com_kgdb_attach(&mvsoc_bs_tag, comkgdbaddr, comkgdbspeed,
			MVUART_SIZE, COM_TYPE_16550_NOERS, comkgdbmode))
		panic("KGDB uart can not be initialized.");
}
#endif

#if NMVPEX > 0
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
	} else
		*start = *end = 0;
}
#endif

static void
axp_device_register(device_t dev, void *aux)
{
	prop_dictionary_t dict = device_properties(dev);

#if NCOM > 0
	if (device_is_a(dev, "com") &&
	    device_is_a(device_parent(dev), "mvsoc"))
		prop_dictionary_set_uint32(dict, "frequency", mvTclk);
#endif

#if NMVPEX > 0
	extern struct bus_space
	    armadaxp_pex00_io_bs_tag, armadaxp_pex00_mem_bs_tag,
	    armadaxp_pex01_io_bs_tag, armadaxp_pex01_mem_bs_tag,
	    armadaxp_pex02_io_bs_tag, armadaxp_pex02_mem_bs_tag,
	    armadaxp_pex03_io_bs_tag, armadaxp_pex03_mem_bs_tag,
	    armadaxp_pex2_io_bs_tag, armadaxp_pex2_mem_bs_tag,
	    armadaxp_pex3_io_bs_tag, armadaxp_pex3_mem_bs_tag;
	extern struct arm32_pci_chipset arm32_mvpex0_chipset,
	    arm32_mvpex1_chipset, arm32_mvpex2_chipset,
	    arm32_mvpex3_chipset, arm32_mvpex4_chipset,
	    arm32_mvpex5_chipset;

	struct marvell_attach_args *mva = aux;

	if (device_is_a(dev, "mvpex")) {
		struct bus_space *mvpex_io_bs_tag, *mvpex_mem_bs_tag;
		struct arm32_pci_chipset *arm32_mvpex_chipset;
		prop_data_t io_bs_tag, mem_bs_tag, pc;
		uint64_t start, end;
		int iotag, memtag;

		if (mva->mva_offset == MVSOC_PEX_BASE) {
			mvpex_io_bs_tag = &armadaxp_pex00_io_bs_tag;
			mvpex_mem_bs_tag = &armadaxp_pex00_mem_bs_tag;
			arm32_mvpex_chipset = &arm32_mvpex0_chipset;
			iotag = ARMADAXP_TAG_PEX00_IO;
			memtag = ARMADAXP_TAG_PEX00_MEM;
		} else if (mva->mva_offset == MVSOC_PEX_BASE + 0x4000) {
			mvpex_io_bs_tag = &armadaxp_pex01_io_bs_tag;
			mvpex_mem_bs_tag = &armadaxp_pex01_mem_bs_tag;
			arm32_mvpex_chipset = &arm32_mvpex1_chipset;
			iotag = ARMADAXP_TAG_PEX01_IO;
			memtag = ARMADAXP_TAG_PEX01_MEM;
		} else if (mva->mva_offset == MVSOC_PEX_BASE + 0x8000) {
			mvpex_io_bs_tag = &armadaxp_pex02_io_bs_tag;
			mvpex_mem_bs_tag = &armadaxp_pex02_mem_bs_tag;
			arm32_mvpex_chipset = &arm32_mvpex2_chipset;
			iotag = ARMADAXP_TAG_PEX02_IO;
			memtag = ARMADAXP_TAG_PEX02_MEM;
		} else if (mva->mva_offset == MVSOC_PEX_BASE + 0xc000) {
			mvpex_io_bs_tag = &armadaxp_pex03_io_bs_tag;
			mvpex_mem_bs_tag = &armadaxp_pex03_mem_bs_tag;
			arm32_mvpex_chipset = &arm32_mvpex3_chipset;
			iotag = ARMADAXP_TAG_PEX03_IO;
			memtag = ARMADAXP_TAG_PEX03_MEM;
		} else if (mva->mva_offset == MVSOC_PEX_BASE + 0x2000) {
			mvpex_io_bs_tag = &armadaxp_pex2_io_bs_tag;
			mvpex_mem_bs_tag = &armadaxp_pex2_mem_bs_tag;
			arm32_mvpex_chipset = &arm32_mvpex4_chipset;
			iotag = ARMADAXP_TAG_PEX2_IO;
			memtag = ARMADAXP_TAG_PEX2_MEM;
		} else {
			mvpex_io_bs_tag = &armadaxp_pex3_io_bs_tag;
			mvpex_mem_bs_tag = &armadaxp_pex3_mem_bs_tag;
			arm32_mvpex_chipset = &arm32_mvpex5_chipset;
			iotag = ARMADAXP_TAG_PEX3_IO;
			memtag = ARMADAXP_TAG_PEX3_MEM;
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
