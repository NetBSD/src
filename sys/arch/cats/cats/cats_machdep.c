/*	$NetBSD: cats_machdep.c,v 1.74.2.2 2014/08/20 00:02:50 tls Exp $	*/

/*
 * Copyright (c) 1997,1998 Mark Brinicombe.
 * Copyright (c) 1997,1998 Causality Limited.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Machine dependent functions for kernel setup for EBSA285 core architecture
 * using cyclone firmware
 *
 * Created      : 24/11/97
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cats_machdep.c,v 1.74.2.2 2014/08/20 00:02:50 tls Exp $");

#include "opt_ddb.h"
#include "opt_modular.h"
#include "opt_pmap_debug.h"

#include "isadma.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/ksyms.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/bootconfig.h>
#define	_ARM32_BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <arm/locore.h>
#include <arm/undefined.h>
#include <arm/arm32/machdep.h>

#include <machine/cyclone_boot.h>
#include <arm/footbridge/dc21285mem.h>
#include <arm/footbridge/dc21285reg.h>

#include "ksyms.h"
#include "opt_ableelf.h"

#include "isa.h"
#if NISA > 0
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#endif

/* Kernel text starts at the base of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00000000)
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)

/*
 * The range 0xf1000000 - 0xfcffffff is available for kernel VM space
 * Footbridge registers and I/O mappings occupy 0xfd000000 - 0xffffffff
 */

/*
 * Size of available KVM space, note that growkernel will grow into this.
 */
#define KERNEL_VM_SIZE	0x0C000000

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependent as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */

u_int dc21285_fclk = FCLK;

struct ebsaboot ebsabootinfo;
BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING + 1];
char *boot_args = NULL;
char *boot_file = NULL;


#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif


/* Prototypes */

void consinit(void);

int fcomcnattach(u_int iobase, int rate, tcflag_t cflag);
int fcomcndetach(void);

static void process_kernel_args(const char *);
extern void configure(void);

/* A load of console goo. */
#include "vga.h"
#if (NVGA > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#endif

#include "com.h"
#if (NCOM > 0)
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#ifndef CONCOMADDR
#define CONCOMADDR 0x3f8
#endif
#endif

#ifndef CONSDEVNAME
#define CONSDEVNAME "vga"
#endif

#define CONSPEED B38400
#ifndef CONSPEED
#define CONSPEED B9600	/* TTYDEF_SPEED */
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;

static const struct pmap_devmap cats_devmap[] = {
	/* Map 1MB for CSR space */
	{ DC21285_ARMCSR_VBASE,			DC21285_ARMCSR_BASE,
	    DC21285_ARMCSR_VSIZE,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	/* Map 1MB for fast cache cleaning space */
	{ DC21285_CACHE_FLUSH_VBASE,		DC21285_SA_CACHE_FLUSH_BASE,
	    DC21285_CACHE_FLUSH_VSIZE,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_CACHE },

	/* Map 1MB for PCI IO space */
	{ DC21285_PCI_IO_VBASE,			DC21285_PCI_IO_BASE,
	    DC21285_PCI_IO_VSIZE,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	/* Map 1MB for PCI IACK space */
	{ DC21285_PCI_IACK_VBASE,		DC21285_PCI_IACK_SPECIAL,
	    DC21285_PCI_IACK_VSIZE,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	/* Map 16MB of type 1 PCI config access */
	{ DC21285_PCI_TYPE_1_CONFIG_VBASE,	DC21285_PCI_TYPE_1_CONFIG,
	    DC21285_PCI_TYPE_1_CONFIG_VSIZE,	VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	/* Map 16MB of type 0 PCI config access */
	{ DC21285_PCI_TYPE_0_CONFIG_VBASE,	DC21285_PCI_TYPE_0_CONFIG,
	    DC21285_PCI_TYPE_0_CONFIG_VSIZE,	VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	/* Map 1MB of 32 bit PCI address space for ISA MEM accesses via PCI */
	{ DC21285_PCI_ISA_MEM_VBASE,		DC21285_PCI_MEM_BASE,
	    DC21285_PCI_ISA_MEM_VSIZE,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	{ 0, 0, 0, 0, 0 }
};

#define MAX_PHYSMEM 4
static struct boot_physmem cats_physmem[MAX_PHYSMEM];
int ncats_physmem = 0;

extern struct bus_space footbridge_pci_io_bs_tag;
extern struct bus_space footbridge_pci_mem_bs_tag;
void footbridge_pci_bs_tag_init(void);

/*
 * u_int initarm(struct ebsaboot *bootinfo)
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
initarm(void *arm_bootargs)
{
	struct ebsaboot *bootinfo = arm_bootargs;
	extern u_int cpu_get_control(void);

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	set_cpufuncs();

	/* Copy the boot configuration structure */
	ebsabootinfo = *bootinfo;

	if (ebsabootinfo.bt_fclk >= 50000000
	    && ebsabootinfo.bt_fclk <= 66000000)
		dc21285_fclk = ebsabootinfo.bt_fclk;

	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independent */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = ebsabootinfo.bt_memstart;
	bootconfig.dram[0].pages = (ebsabootinfo.bt_memend
	    - ebsabootinfo.bt_memstart) / PAGE_SIZE;

	/*
	 * Initialise the diagnostic serial console
	 * This allows a means of generating output during initarm().
	 * Once all the memory map changes are complete we can call consinit()
	 * and not have to worry about things moving.
	 */
	pmap_devmap_bootstrap((vaddr_t)ebsabootinfo.bt_l1, cats_devmap);

#ifdef FCOM_INIT_ARM
	fcomcnattach(DC21285_ARMCSR_BASE, comcnspeed, comcnmode);
#endif

	/* Talk to the user */
	printf("NetBSD/cats booting ...\n");

	if (ebsabootinfo.bt_magic != BT_MAGIC_NUMBER_EBSA
	    && ebsabootinfo.bt_magic != BT_MAGIC_NUMBER_CATS)
		panic("Incompatible magic number %#x passed in boot args",
		    ebsabootinfo.bt_magic);

#ifdef VERBOSE_INIT_ARM
	/* output the incoming bootinfo */
	printf("bootinfo @ %p\n", arm_bootargs);
	printf("bt_magic    = 0x%08x\n", ebsabootinfo.bt_magic);
	printf("bt_vargp    = 0x%08x\n", ebsabootinfo.bt_vargp);
	printf("bt_pargp    = 0x%08x\n", ebsabootinfo.bt_pargp);
	printf("bt_args @ %p, contents = \"%s\"\n", ebsabootinfo.bt_args, ebsabootinfo.bt_args);
	printf("bt_l1       = %p\n", ebsabootinfo.bt_l1);

	printf("bt_memstart = 0x%08x\n", ebsabootinfo.bt_memstart);
	printf("bt_memend   = 0x%08x\n", ebsabootinfo.bt_memend);
	printf("bt_memavail = 0x%08x\n", ebsabootinfo.bt_memavail);
	printf("bt_fclk     = 0x%08x\n", ebsabootinfo.bt_fclk);
	printf("bt_pciclk   = 0x%08x\n", ebsabootinfo.bt_pciclk);
	printf("bt_vers     = 0x%08x\n", ebsabootinfo.bt_vers);
	printf("bt_features = 0x%08x\n", ebsabootinfo.bt_features);
#endif

	/*
	 * Examine the boot args string for options we need to know about
	 * now.
	 */
	process_kernel_args(ebsabootinfo.bt_args);

	arm32_bootmem_init(ebsabootinfo.bt_memstart,
	    ebsabootinfo.bt_memend - ebsabootinfo.bt_memstart,
	    ebsabootinfo.bt_memstart);

	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_LOW, 0, cats_devmap,
	    false);

	printf("init subsystems: patch ");

	/*
	 * PATCH PATCH ...
	 *
	 * Fixup the first word of the kernel to be the instruction
	 * add pc, pc, #0x41000000
	 *
	 * This traps the case where the CPU core resets due to bus contention
	 * on a prototype CATS system and will reboot into the firmware.
	 */
	*((u_int *)KERNEL_TEXT_BASE) = 0xe28ff441;

#if NISA > 0
	/* Initialise the ISA subsystem early ... */
	isa_footbridge_init(DC21285_PCI_IO_VBASE, DC21285_PCI_ISA_MEM_VBASE);
#endif

	footbridge_pci_bs_tag_init();

	printf("busses done.\n");

	/* Map the boot arguments page */
	if (ebsabootinfo.bt_vargp != vector_page) {
		pmap_map_entry(kernel_l1pt.pv_va, ebsabootinfo.bt_vargp,
		    ebsabootinfo.bt_pargp, VM_PROT_READ, PTE_CACHE);
	}

	extern struct bootmem_info bootmem_info;
	struct bootmem_info * const bmi = &bootmem_info;

	printf("Doing freeblocks: %d\n", bmi->bmi_nfreeblocks);

	for (size_t i = 0; i < bmi->bmi_nfreeblocks; i++) {
		pv_addr_t * const pv = &bmi->bmi_freeblocks[i];
		paddr_t start = pv->pv_pa;
		paddr_t end = pv->pv_pa + pv->pv_size;
		struct boot_physmem *bp;
#if NISADMA > 0
		paddr_t istart, isize;
		extern struct arm32_dma_range *footbridge_isa_dma_ranges;
		extern int footbridge_isa_dma_nranges;
#endif

#if 0
		printf("%zu: %lx -> %lx\n", i, start, end - 1);
#endif

#if NISADMA > 0
		if (arm32_dma_range_intersect(footbridge_isa_dma_ranges,
		   footbridge_isa_dma_nranges, start, end - start,
		   &istart, &isize)) {
			/*
			 * Place the pages that intersect with the
			 * ISA DMA range onto the ISA DMA free list.
			 */
#if 0
			printf("    ISADMA 0x%lx -> 0x%lx\n", istart,
			    istart + isize - 1);
#endif
			bp = &cats_physmem[ncats_physmem++];
			KASSERT(ncats_physmem < MAX_PHYSMEM);
			bp->bp_start = atop(istart);
			bp->bp_pages = atop(isize);
			bp->bp_freelist = VM_FREELIST_ISADMA;

			/*
			 * Load the pieces that come before the
			 * intersection onto the default free list.
			 */
			if (start < istart) {
#if 0
				printf("    BEFORE 0x%lx -> 0x%lx\n",
				    start, istart - 1);
#endif
				bp = &cats_physmem[ncats_physmem++];
				KASSERT(ncats_physmem < MAX_PHYSMEM);
				bp->bp_start = atop(start);
				bp->bp_pages = atop(istart - start);
				bp->bp_freelist = VM_FREELIST_DEFAULT;
			}

			/*
			 * Load the pieces that come after the
			 * intersection onto the default free list.
			 */
			if ((istart + isize) < end) {
#if 0
				printf("     AFTER 0x%lx -> 0x%lx\n",
				    (istart + isize), end - 1);
#endif
				bp = &cats_physmem[ncats_physmem++];
				KASSERT(ncats_physmem < MAX_PHYSMEM);
				bp->bp_start = atop(istart + isize);
				bp->bp_pages = atop(end - (istart + isize));
				bp->bp_freelist = VM_FREELIST_DEFAULT;
			}
		} else {
			bp = &cats_physmem[ncats_physmem++];
			KASSERT(ncats_physmem < MAX_PHYSMEM);
			bp->bp_start = atop(start);
			bp->bp_pages = atop(end - start);
			bp->bp_freelist = VM_FREELIST_DEFAULT;
		}
#else /* NISADMA > 0 */
		bp = &cats_physmem[ncats_physmem++];
		KASSERT(ncats_physmem < MAX_PHYSMEM);
		bp->bp_start = atop(start);
		bp->bp_pages = atop(end - start);
		bp->bp_freelist = VM_FREELIST_DEFAULT;
#endif /* NISADMA > 0 */
	}

	cpu_reset_address_paddr = DC21285_ROM_BASE;

	/* initarm_common returns the new stack pointer address */
	u_int sp;
	sp = initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, cats_physmem,
	    ncats_physmem);

	/* Setup the IRQ system */
	printf("init subsystems: irq ");
	footbridge_intr_init();

	printf("done.\n");

#ifdef FCOM_INIT_ARM
	fcomcndetach();
#endif


#if NKSYMS || defined(DDB) || defined(MODULAR)
#ifndef __ELF__		/* XXX */
	{
		extern int end;
		extern int *esym;

		ksyms_addsyms_elf(*(int *)&end, ((int *)&end) + 1, esym);
	}
#endif /* __ELF__ */
#endif

#ifdef DDB
	db_machine_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/*
	 * XXX this should only be done in main() but it useful to
	 * have output earlier ...
	 */
	consinit();

	return sp;
}

static void
process_kernel_args(const char *loader_args)
{
	char *args;
	boothowto = 0;

	/* Make a local copy of the bootargs */
	strncpy(bootargs, loader_args, MAX_BOOT_STRING);

	args = bootargs;
	boot_file = bootargs;

	/* Skip the kernel image filename */
	while (*args != ' ' && *args != 0)
		++args;

	if (*args != 0)
		*args++ = 0;

	while (*args == ' ')
		++args;

	boot_args = args;

	printf("bootfile: %s\n", boot_file);
	printf("bootargs: %s\n", boot_args);

	parse_mi_bootargs(boot_args);
}

void
consinit(void)
{
	static int consinit_called = 0;
	const char *console = CONSDEVNAME;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	get_bootconf_option(boot_args, "console", BOOTOPT_TYPE_STRING,
	    &console);

	if (strncmp(console, "fcom", 4) == 0
	    || strncmp(console, "diag", 4) == 0)
		fcomcnattach(DC21285_ARMCSR_VBASE, comcnspeed, comcnmode);
#if (NVGA > 0)
	else if (strncmp(console, "vga", 3) == 0) {
		vga_cnattach(&footbridge_pci_io_bs_tag,
		    &footbridge_pci_mem_bs_tag, - 1, 0);
#if (NPCKBC > 0)
		pckbc_cnattach(&isa_io_bs_tag, IO_KBD, KBCMDP, PCKBC_KBD_SLOT,
		    0);
#endif	/* NPCKBC */
	}
#endif	/* NVGA */
#if (NCOM > 0)
	else if (strncmp(console, "com", 3) == 0) {
		if (comcnattach(&isa_io_bs_tag, CONCOMADDR, comcnspeed,
		    COM_FREQ, COM_TYPE_NORMAL, comcnmode))
			panic("can't init serial console @%x", CONCOMADDR);
	}
#endif
	/* Don't know what console was requested so use the fall back. */
	else
		fcomcnattach(DC21285_ARMCSR_VBASE, comcnspeed, comcnmode);
}

/* End of cats_machdep.c */
