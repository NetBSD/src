/*	$NetBSD: atari_init.c,v 1.129 2026/11/08 08:39:10 tsutsui Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman
 * Copyright (c) 1994 Michael L. Hitch
 * Copyright (c) 1993 Markus Wild
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
 *      This product includes software developed by Markus Wild.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: atari_init.c,v 1.129 2026/11/08 08:39:10 tsutsui Exp $");

#include "opt_ddb.h"
#include "opt_mbtype.h"
#include "opt_m060sp.h"
#include "opt_m68k_arch.h"
#include "opt_st_pool_size.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/buf.h>
#include <sys/msgbuf.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/reboot.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <machine/vmparam.h>
#include <machine/pmap.h>
#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <machine/scu.h>
#include <machine/acia.h>
#include <machine/intr.h>
#include <machine/vectors.h>

#include <m68k/cpu.h>
#include <m68k/cacheops.h>
#include <m68k/pcr.h>

#include <atari/atari/stalloc.h>
#include <atari/dev/clockvar.h>
#include <atari/dev/ym2149reg.h>

#include "pci.h"

/* Temporary cross-file declarations for the NEW_PMAP transition. */
void cpu_init_kcorehdr(paddr_t, paddr_t);

void *start_c(int, u_int, u_int, u_int, char *);
static void atari_hwinit(void);
static void initcpu(void);
static void set_machtype(void);

#if defined(_MILANHW_)
static u_int milan_probe_bank_1(paddr_t paddr);
static u_int milan_probe_bank(paddr_t paddr);

#define NBANK	2
#define NSLOT	4

#define MB(n)		((n) * 1024 * 1024)
#define MB_END(n)	(MB(n) - 1)
#define MAGIC_4M	(4 - 1)
#define MAGIC_4M_INV	((uint8_t)~MAGIC_4M)
#define MAGIC_8M	(8 - 1)
#define MAGIC_16M	(16 - 1)
#define MAGIC_32M	(32 - 1)
#define MAGIC_64M	(64 - 1)
#endif

struct memseg	boot_segs[NMEM_SEGS];
struct memseg	usable_segs[NMEM_SEGS];

extern u_int	lowram;
int		machineid, mmutype, cputype;

extern char		*esym;
extern struct pcb	*curpcb;

/*
 * This is the virtual address of physical page 0. Used by 'do_boot()'.
 */
vaddr_t	page_zero;

/*
 * Simple support for allocation in ST-ram.
 * Currently 16 bit ST-ram is required to allocate DMA buffers for SCSI and
 * FDC transfers, and video memory for the XFree68 based Xservers.
 * The physical address is also returned because the video init needs it to
 * setup the controller at the time the vm-system is not yet operational so
 * 'kvtop()' cannot be used.
 */
#define	ST_POOL_SIZE_MIN	24	/* for DMA bounce buffers */
#ifndef ST_POOL_SIZE
#define	ST_POOL_SIZE		56	/* Xserver requires 320KB (40 pages) */
#endif

psize_t	st_pool_size = ST_POOL_SIZE * PAGE_SIZE; /* Patchable	*/
vaddr_t	st_pool_virt;
paddr_t	st_pool_phys;

/*
 * Thresholds to restrict size of reserved ST memory to make sure
 * the kernel at least boot even on lower memory machines.
 * Nowadays we could assume most users have 4MB ST-RAM and 16MB TT-RAM.
 */
#define	STRAM_MINTHRESH		(2 * 1024 * 1024)
#define	TTRAM_MINTHRESH		(4 * 1024 * 1024)

/* I/O address space variables */
vaddr_t	stio_addr;		/* Where the st io-area is mapped	*/
vaddr_t	pci_conf_addr;		/* KVA base of PCI config space		*/
vaddr_t	pci_io_addr;		/* KVA base of PCI io-space		*/
vaddr_t	pci_mem_addr;		/* KVA base of PCI mem-space		*/
vaddr_t	pci_mem_uncached;	/* KVA base of an uncached PCI mem-page	*/

/*
 * Are we relocating the kernel to TT-Ram if possible? It is faster, but
 * it is also reported not to work on all TT's. So the default is NO.
 */
#ifndef	RELOC_KERNEL
#define	RELOC_KERNEL	0
#endif
int	reloc_kernel = RELOC_KERNEL;		/* Patchable	*/

/*
 * this is the C-level entry function, it's called from locore.s.
 * Preconditions:
 *	Interrupts are disabled
 *	PA == VA, we don't have to relocate addresses before enabling
 *		the MMU
 *	Exec is no longer available (because we're loaded all over
 *		low memory, no ExecBase is available anymore)
 *
 * It's purpose is:
 *	Do the things that are done in locore.s in the hp300 version,
 *		this includes allocation of kernel maps and enabling the MMU.
 *
 * Some of the code in here is `stolen' from Amiga MACH, and was
 * written by Bryan Ford and Niklas Hallqvist.
 *
 * Very crude 68040 support by Michael L. Hitch.
 */

void * __attribute__((no_instrument_function))
start_c(int id, u_int ttphystart, u_int ttphysize, u_int stphysize,
    char *esym_addr)
	/* id:			 Machine id			*/
	/* ttphystart, ttphysize: Start address and size of TT-ram */
	/* stphysize:		 Size of ST-ram			*/
	/* esym_addr:		 Address of kernel '_esym' symbol */
{
	extern char	end[];
	paddr_t		pstart;		/* Next available physical address */
	u_int		i;
	vaddr_t		end_loaded;
	paddr_t		kbase;
#if defined(_MILANHW_)
	/*
	 * The Milan Lies about the presence of TT-RAM. If you insert
	 * 16MB it is split in 14MB ST starting at address 0 and 2MB TT RAM,
	 * starting at address 16MB as the BIOS remapping memory using MMU.
	 *
	 * Milan actually has four SIMM slots and each slot has two banks,
	 * so it could have up to 8 memory segment regions.
	 */
	const paddr_t simm_base[NBANK][NSLOT] = {
		/* slot 0-3, bank 0 */
		{ 0x00000000, 0x04000000, 0x08000000, 0x0c000000 },
		/* slot 0-3, bank 1 */
		{ 0x10000000, 0x14000000, 0x18000000, 0x1c000000 }
	};
	int slot, bank, seg;
	u_int mb;

	/* On Milan, all RAMs are fast 32 bit so no need to reloc kernel */
	reloc_kernel = 0;

	/* probe memory region in all SIMM slots and banks */
	seg = 0;
	ttphysize = 0;
	for (bank = 0; bank < 2; bank++) {
		for (slot = 0; slot < 4; slot++) {
			if (bank == 0 && slot == 0) {
				/*
				 * The first bank has at least 16MB because
				 * the Milan's ROM bootloader requires it
				 * to allocate ST RAM.
				 */
				mb = milan_probe_bank_1(simm_base[bank][slot]);
				boot_segs[0].start = 0;
				boot_segs[0].end   = MB(mb);
				stphysize          = MB(mb);
				seg++;
			} else {
				/*
				 * The rest banks could be empty or
				 * have 4, 8, 16, 32, or 64MB.
				 */
				mb = milan_probe_bank(simm_base[bank][slot]);
				if (mb > 0) {
					boot_segs[seg].start =
					    simm_base[bank][slot];
					boot_segs[seg].end   =
					    simm_base[bank][slot] + MB(mb);
					ttphysize += MB(mb);
					seg++;
				}
			}
		}
	}
#else /* _MILANHW_ */
	boot_segs[0].start       = 0;
	boot_segs[0].end         = stphysize;
	boot_segs[1].start       = ttphystart;
	boot_segs[1].end         = ttphystart + ttphysize;
	boot_segs[2].start = boot_segs[2].end = 0; /* End of segments! */
#endif

	/*
	 * We do not know how much ST memory we really need until after
	 * configuration has finished, but typical users of ST memory
	 * are bounce buffers DMA against TT-RAM for SCSI and FDC,
	 * and video memory for the Xserver.
	 * If we have enough RAMs reserve ST memory including for the Xserver.
	 * Otherwise just allocate minimum one for SCSI and FDC.
	 *
	 * The round_page() call is meant to correct errors made by
	 * binpatching!
	 */
	if (st_pool_size > ST_POOL_SIZE_MIN * PAGE_SIZE &&
	    (stphysize <= STRAM_MINTHRESH || ttphysize <= TTRAM_MINTHRESH)) {
		st_pool_size = ST_POOL_SIZE_MIN * PAGE_SIZE;
	}
	st_pool_size   = m68k_round_page(st_pool_size);
	st_pool_phys   = stphysize - st_pool_size;
	stphysize      = st_pool_phys;

	physmem        = btoc(stphysize) + btoc(ttphysize);
	machineid      = id;
	esym           = esym_addr;

	/*
	 * the kernel ends at end() or esym.
	 */
	if (esym == NULL)
		end_loaded = (vaddr_t)&end;
	else
		end_loaded = (vaddr_t)esym;

	/*
	 * If we have enough fast-memory to put the kernel in and the
	 * RELOC_KERNEL option is set, do it!
	 */
	if ((reloc_kernel != 0) && (ttphysize >= end_loaded))
		kbase = ttphystart;
	else
		kbase = 0;

	/*
	 * Determine the type of machine we are running on. This needs
	 * to be done early (and before initcpu())!
	 */
	set_machtype();

	/*
	 * Initialize CPU specific stuff
	 */
	initcpu();

	pstart = pmap_bootstrap1(end_loaded, kbase);

	lowram  = 0 >> PGSHIFT; /* XXX */

	/*
	 * Fill in usable segments. The page indexes will be initialized
	 * later when all reservations are made.
	 */
	usable_segs[0].start = 0;
	usable_segs[0].end   = stphysize;
	usable_segs[0].free_list = VM_FREELIST_STRAM;
#if defined(_MILANHW_)
	for (i = 1; i < seg; i++) {
		usable_segs[i].start = boot_segs[i].start;
		usable_segs[i].end   = boot_segs[i].end;
		usable_segs[i].free_list = VM_FREELIST_TTRAM;
	}
	for (; i < NMEM_SEGS; i++) {
		usable_segs[i].start = usable_segs[i].end = 0;
	}
#else
	usable_segs[1].start = ttphystart;
	usable_segs[1].end   = ttphystart + ttphysize;
	usable_segs[1].free_list = VM_FREELIST_TTRAM;
	usable_segs[2].start = usable_segs[2].end = 0; /* End of segments! */
#endif

	if (kbase) {
		/*
		 * First page of ST-ram is unusable, reserve the space
		 * for the kernel in the TT-ram segment.
		 * Note: Because physical page-zero is partially mapped to ROM
		 *       by hardware, it is unusable.
		 */
		usable_segs[0].start  = PAGE_SIZE;
		usable_segs[1].start += pstart;
	} else
		usable_segs[0].start += pstart;

	/*
	 * As all segment sizes are now valid, calculate page indexes and
	 * available physical memory.
	 */
	usable_segs[0].first_page = 0;
	for (i = 1; i < NMEM_SEGS && usable_segs[i].start; i++) {
		usable_segs[i].first_page  = usable_segs[i-1].first_page;
		usable_segs[i].first_page +=
		    (usable_segs[i-1].end - usable_segs[i-1].start) / PAGE_SIZE;
	}
	for (i = 0, physmem = 0; i < NMEM_SEGS && usable_segs[i].start; i++)
		physmem += usable_segs[i].end - usable_segs[i].start;
	physmem >>= PGSHIFT;

	/*
	 * get the pmap module in sync with reality.
	 */
	pmap_bootstrap();

#if defined(M68030)
	/*
	 * Prepare the SRP prototype before relocating the kernel.
	 * The relocated copy must contain the root table address
	 * when the MMU is enabled.
	 */
	protorp[1] = Sysseg_pa;
#endif

	cpu_init_kcorehdr(kbase, Sysseg_pa);

	/*
	 * copy over the kernel (and all now initialized variables)
	 * to fastram.  DONT use bcopy(), this beast is much larger
	 * than 128k !
	 */
	if (kbase) {
		register paddr_t *lp, *le, *fp;

		lp = (paddr_t *)0;
		le = (paddr_t *)pstart;
		fp = (paddr_t *)kbase;
		while (lp < le)
			*fp++ = *lp++;
	}
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040) {
		/*
		 * movel Sysseg_pa,a0;
		 * movec a0,SRP;
		 * pflusha;
		 * movel #$0xc000,d0;
		 * movec d0,TC
		 */
		if (cputype == CPU_68060) {
			/* XXX: Need the branch cache be cleared? */
			__asm volatile (".word 0x4e7a,0x0002;"
				      "orl #0x400000,%%d0;"
				      ".word 0x4e7b,0x0002" : : : "d0");
		}
		__asm volatile ("movel %0,%%a0;"
			      ".word 0x4e7b,0x8807" : : "a" (Sysseg_pa) : "a0");
		__asm volatile (".word 0xf518" : : );
		__asm volatile ("movel #0xc000,%%d0;"
			      ".word 0x4e7b,0x0003" : : : "d0" );
	} else
#endif
	{
#if defined(M68030)
		__asm volatile ("pmove %0@,%%srp"
		    :
		    : "a" (&protorp[0]), "m" (protorp[0]), "m" (protorp[1]));
		/*
		 * setup and load TC register.
		 * enable_cpr, enable_srp, pagesize=8k,
		 * A = 8 bits, B = 11 bits
		 */
		u_int tc = MMU51_TCR_BITS;
		__asm volatile ("pflusha" : : );
		__asm volatile ("pmove %0@,%%tc" : : "a" (&tc), "m" (tc));
#endif /* M68030 */
	}

	/*
	 * Initialize the "u-area" pages etc.
	 */
	void *ksp = pmap_bootstrap2();

	/*
	 * Get the hardware into a defined state
	 */
	atari_hwinit();

	/*
	 * Initialize stmem allocator
	 */
	init_stmem();

	/*
	 * Initialize the iomem arena for bus_space(9) to manage address
	 * spaces and allocate the physical RAM from the extent map.
	 */
	atari_bus_space_arena_init(0x0, 0xffffffff);
	for (i = 0; i < NMEM_SEGS && boot_segs[i].end != 0; i++) {
		if (atari_bus_space_alloc_physmem(boot_segs[i].start,
		    boot_segs[i].end)) {
			/* XXX: Ahum, should not happen ;-) */
			printf("Warning: Cannot allocate boot memory from"
			    " extent map!?\n");
		}
	}

	/*
	 * Initialize interrupt mapping.
	 */
	intr_init();

	return ksp;
}

#if defined(_MILANHW_)
/*
 * Probe and return available memory size in MB at specified address.
 * The first slot SIMM have at least 16MB, so check if it has 32 or 64 MB.
 *
 * Note it seems Milan does not generate bus errors on accesses against
 * address regions where memory doesn't exist, but it returns memory images
 * of lower address of the bank.
 */
static u_int
milan_probe_bank_1(paddr_t start_paddr)
{
	volatile uint8_t *base;
	u_int mb;
	uint8_t save_16, save_32, save_64;

	/* Assume that this bank has at least 16MB */
	mb = 16;

	base = (uint8_t *)start_paddr;

	/* save and write a MAGIC at the end of 16MB region */
	save_16 = base[MB_END(16)];
	base[MB_END(16)] = MAGIC_16M;

	/* check bus error at the end of 32MB region */
	if (badbaddr(base + MB_END(32))) {
		/* bus error; assume no memory there */
		goto out16;
	}

	/* check if the 32MB region is not image of the prior 16MB region */
	save_32 = base[MB_END(32)];
	base[MB_END(32)] = MAGIC_32M;
	if (base[MB_END(32)] != MAGIC_32M || base[MB_END(16)] != MAGIC_16M) {
		/* no memory or image at the 32MB region */
		goto out16;
	}
	/* we have at least 32MB */
	mb = 32;

	/* check bus error at the end of 64MB region */
	if (badbaddr(base + MB_END(64))) {
		/* bus error; assume no memory there */
		goto out32;
	}

	/* check if the 64MB region is not image of the prior 32MB region */
	save_64 = base[MB_END(64)];
	base[MB_END(64)] = MAGIC_64M;
	if (base[MB_END(64)] != MAGIC_64M || base[MB_END(32)] != MAGIC_32M) {
		/* no memory or image at the 64MB region */
		goto out32;
	}
	/* we have 64MB */
	mb = 64;
	base[MB_END(64)] = save_64;
 out32:
	base[MB_END(32)] = save_32;
 out16:
	base[MB_END(16)] = save_16;

	return mb;
}

/*
 * Probe and return available memory size in MB at specified address.
 * The rest slot could be empty so check all possible size.
 */
static u_int
milan_probe_bank(paddr_t start_paddr)
{
	volatile uint8_t *base;
	u_int mb;
	uint8_t save_4, save_8, save_16;

	/* The rest banks might have no memory */
	mb = 0;

	base = (uint8_t *)start_paddr;

	/* check bus error at the end of 4MB region */
	if (badbaddr(base + MB_END(4))) {
		/* bus error; assume no memory there */
		goto out;
	}

	/* check if the 4MB region has memory */
	save_4 = base[MB_END(4)];
	base[MB_END(4)] = MAGIC_4M_INV;
	if (base[MB_END(4)] != MAGIC_4M_INV) {
		/* no memory */
		goto out;
	}
	base[MB_END(4)] = MAGIC_4M;
	if (base[MB_END(4)] != MAGIC_4M) {
		/* no memory */
		goto out;
	}
	/* we have at least 4MB */
	mb = 4;

	/* check bus error at the end of 8MB region */
	if (badbaddr(base + MB_END(8))) {
		/* bus error; assume no memory there */
		goto out4;
	}

	/* check if the 8MB region is not image of the prior 4MB region */
	save_8 = base[MB_END(8)];
	base[MB_END(8)] = MAGIC_8M;
	if (base[MB_END(8)] != MAGIC_8M || base[MB_END(4)] != MAGIC_4M) {
		/* no memory or image at the 8MB region */
		goto out4;
	}
	/* we have at least 8MB */
	mb = 8;

	/* check bus error at the end of 16MB region */
	if (badbaddr(base + MB_END(16))) {
		/* bus error; assume no memory there */
		goto out8;
	}

	/* check if the 16MB region is not image of the prior 8MB region */
	save_16 = base[MB_END(16)];
	base[MB_END(16)] = MAGIC_16M;
	if (base[MB_END(16)] != MAGIC_16M || base[MB_END(8)] != MAGIC_8M) {
		/* no memory or image at the 32MB region */
		goto out8;
	}
	/* we have at least 16MB, so check more region as the first bank */
	mb = milan_probe_bank_1(start_paddr);

	base[MB_END(16)] = save_16;
 out8:
	base[MB_END(8)] = save_8;
 out4:
	base[MB_END(4)] = save_4;
 out:

	return mb;
}
#endif	/* _MILANHW_ */

/*
 * Try to figure out on what type of machine we are running
 * Note: This module runs *before* the io-mapping is setup!
 */
static void
set_machtype(void)
{

#ifdef _MILANHW_
	machineid |= ATARI_MILAN;

#else
	stio_addr = 0xff8000;	/* XXX: For TT & Falcon only */
	if (badbaddr(&MFP2->mf_gpip)) {
		/*
		 * Watch out! We can also have a Hades with < 16Mb
		 * RAM here...
		 */
		if (!badbaddr(&MFP->mf_gpip)) {
			machineid |= ATARI_FALCON;
			return;
		}
	}
	if (!badbaddr((void *)(PCI_CONFB_PHYS + PCI_CONFM_PHYS)))
		machineid |= ATARI_HADES;
	else
		machineid |= ATARI_TT;
#endif /* _MILANHW_ */
}

static void
atari_hwinit(void)
{

#if defined(_ATARIHW_)
	/*
	 * Initialize the sound chip
	 */
	ym2149_init();

	/*
	 * Make sure that the midi acia will not generate an interrupt
	 * unless something attaches to it. We cannot do this for the
	 * keyboard acia because this breaks the '-d' option of the
	 * booter...
	 */
	MDI->ac_cs = 0;
#endif /* defined(_ATARIHW_) */

	/*
	 * Initialize both MFP chips (if both present!) to generate
	 * auto-vectored interrupts with EOI. The active-edge registers are
	 * set up. The interrupt enable registers are set to disable all
	 * interrupts.
	 */
	MFP->mf_iera  = MFP->mf_ierb = 0;
	MFP->mf_imra  = MFP->mf_imrb = 0;
	MFP->mf_aer   = MFP->mf_ddr  = 0;
	MFP->mf_vr    = 0x40;

#if defined(_ATARIHW_)
	if (machineid & (ATARI_TT|ATARI_HADES)) {
		MFP2->mf_iera = MFP2->mf_ierb = 0;
		MFP2->mf_imra = MFP2->mf_imrb = 0;
		MFP2->mf_aer  = 0x80;
		MFP2->mf_vr   = 0x50;
	}

	if (machineid & ATARI_TT) {
		/*
		 * Initialize the SCU, to enable interrupts on the SCC (ipl5),
		 * MFP (ipl6) and softints (ipl1).
		 */
		SCU->sys_mask = SCU_SYS_SOFT;
		SCU->vme_mask = SCU_MFP | SCU_SCC;
#ifdef DDB
		/*
		 * This allows people with the correct hardware modification
		 * to drop into the debugger from an NMI.
		 */
		SCU->sys_mask |= SCU_IRQ7;
#endif
	}
#endif /* defined(_ATARIHW_) */

	/*
	 * Initialize a timer for delay(9).
	 */
	init_delay();

#if NPCI > 0
	if (machineid & (ATARI_HADES|ATARI_MILAN)) {
		/*
		 * Configure PCI-bus
		 */
		init_pci_bus();
	}
#endif

}

#if defined(M68060)
int m68060_pcr_init = PCR_ESS;	/* make this patchable */
#endif

static void
initcpu(void)
{
#if defined(M68060)
	if (cputype == CPU_68060) {
		__asm volatile ("movl %0,%%d0; .word 0x4e7b,0x0808" : :
				"d"(m68060_pcr_init):"d0" );
	}
#endif
	/* Initialize the vector table. */
	vec_init();
}
