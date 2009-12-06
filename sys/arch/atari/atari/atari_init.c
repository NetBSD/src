/*	$NetBSD: atari_init.c,v 1.90 2009/12/06 00:33:59 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: atari_init.c,v 1.90 2009/12/06 00:33:59 tsutsui Exp $");

#include "opt_ddb.h"
#include "opt_mbtype.h"
#include "opt_m060sp.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/buf.h>
#include <sys/msgbuf.h>
#include <sys/mbuf.h>
#include <sys/extent.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/dkbad.h>
#include <sys/reboot.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>
#include <sys/core.h>
#include <sys/kcore.h>

#include <uvm/uvm_extern.h>

#include <machine/vmparam.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <machine/scu.h>
#include <machine/acia.h>
#include <machine/kcore.h>
#include <machine/intr.h>

#include <m68k/cpu.h>
#include <m68k/cacheops.h>

#include <atari/atari/stalloc.h>
#include <atari/dev/clockvar.h>
#include <atari/dev/ym2149reg.h>

#include "pci.h"

void start_c(int, u_int, u_int, u_int, char *);
static void atari_hwinit(void);
static void cpu_init_kcorehdr(paddr_t, paddr_t);
static void initcpu(void);
static void mmu030_setup(paddr_t, u_int, paddr_t, psize_t, paddr_t, paddr_t);
static void map_io_areas(paddr_t, psize_t, u_int);
static void set_machtype(void);

#if defined(M68040) || defined(M68060)
static void mmu040_setup(paddr_t, u_int, paddr_t, psize_t, paddr_t, paddr_t);
#endif

/*
 * Extent maps to manage all memory space, including I/O ranges.  Allocate
 * storage for 8 regions in each, initially.  Later, iomem_malloc_safe
 * will indicate that it's safe to use malloc() to dynamically allocate
 * region descriptors.
 * This means that the fixed static storage is only used for registrating
 * the found memory regions and the bus-mapping of the console.
 *
 * The extent maps are not static!  They are used for bus address space
 * allocation.
 */
static long iomem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
struct extent *iomem_ex;
int iomem_malloc_safe;

/*
 * All info needed to generate a panic dump. All fields are setup by
 * start_c().
 * XXX: Should sheck usage of phys_segs. There is some unwanted overlap
 *      here.... Also, the name is badly choosen. Phys_segs contains the
 *      segment descriptions _after_ reservations are made.
 * XXX: 'lowram' is obsoleted by the new panicdump format
 */
static cpu_kcore_hdr_t cpu_kcore_hdr;

extern u_int 	lowram;
int		machineid, mmutype, cputype, astpending;
#if defined(M68040) || defined(M68060)
extern u_int	protostfree;
#endif

extern char		*esym;
extern struct pcb	*curpcb;

/*
 * This is the virtual address of physical page 0. Used by 'do_boot()'.
 */
vaddr_t	page_zero;

/*
 * Crude support for allocation in ST-ram. Currently only used to allocate
 * video ram.
 * The physical address is also returned because the video init needs it to
 * setup the controller at the time the vm-system is not yet operational so
 * 'kvtop()' cannot be used.
 */
#ifndef ST_POOL_SIZE
#define	ST_POOL_SIZE	40			/* XXX: enough? */
#endif

u_long	st_pool_size = ST_POOL_SIZE * PAGE_SIZE; /* Patchable	*/
u_long	st_pool_virt, st_pool_phys;

/*
 * Are we relocating the kernel to TT-Ram if possible? It is faster, but
 * it is also reported not to work on all TT's. So the default is NO.
 */
#ifndef	RELOC_KERNEL
#define	RELOC_KERNEL	0
#endif
int	reloc_kernel = RELOC_KERNEL;		/* Patchable	*/

#define	RELOC_PA(base, pa)	((base) + (pa))	/* used to set up PTE etc. */

/*
 * this is the C-level entry function, it's called from locore.s.
 * Preconditions:
 *	Interrupts are disabled
 *	PA == VA, we don't have to relocate addresses before enabling
 *		the MMU
 * 	Exec is no longer available (because we're loaded all over 
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
int kernel_copyback = 1;

void
start_c(int id, u_int ttphystart, u_int ttphysize, u_int stphysize,
    char *esym_addr)
	/* id:			 Machine id			*/
	/* ttphystart, ttphysize: Start address and size of TT-ram */
	/* stphysize:		 Size of ST-ram 		*/
	/* esym_addr:		 Address of kernel '_esym' symbol */
{
	extern char	end[];
	extern void	etext(void);
	extern u_long	protorp[2];
	paddr_t		pstart;		/* Next available physical address */
	vaddr_t		vstart;		/* Next available virtual address */
	vsize_t		avail;
	paddr_t		ptpa;
	psize_t		ptsize;
	u_int		ptextra;
	vaddr_t		kva;
	u_int		tc, i;
	pt_entry_t	*pg, *epg;
	pt_entry_t	pg_proto;
	vaddr_t		end_loaded;
	paddr_t		kbase;
	u_int		kstsize;
	paddr_t		Sysseg_pa;
	paddr_t		Sysptmap_pa;

#if defined(_MILANHW_)
	/* XXX
	 * XXX The right place todo this is probably the booter (Leo)
	 * XXX More than 16MB memory is not yet supported on the Milan!
	 * The Milan Lies about the presence of TT-RAM. If you insert
	 * 16MB it is split in 14MB ST starting at address 0 and 2MB TT RAM,
	 * starting at address 16MB. 
	 */
	stphysize += ttphysize;
	ttphysize  = ttphystart = 0;
#endif
	boot_segs[0].start       = 0;
	boot_segs[0].end         = stphysize;
	boot_segs[1].start       = ttphystart;
	boot_segs[1].end         = ttphystart + ttphysize;
	boot_segs[2].start = boot_segs[2].end = 0; /* End of segments! */

	/*
	 * The following is a hack. We do not know how much ST memory we
	 * really need until after configuration has finished. At this
	 * time I have no idea how to grab ST memory at that time.
	 * The round_page() call is ment to correct errors made by
	 * binpatching!
	 */
	st_pool_size   = m68k_round_page(st_pool_size);
	st_pool_phys   = stphysize - st_pool_size;
	stphysize      = st_pool_phys;

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

	/*
	 * We run the kernel from ST memory at the moment.
	 * The kernel segment table is put just behind the loaded image.
	 * pstart: start of usable ST memory
	 * avail : size of ST memory available.
	 */
	vstart = (vaddr_t)end_loaded;
	vstart = m68k_round_page(vstart);
	pstart = (paddr_t)vstart;	/* pre-reloc PA == kernel VA here */
	avail  = stphysize - pstart;

	/*
	 * Save KVA of lwp0 uarea and allocate it.
	 */
	lwp0uarea  = vstart;
	pstart    += USPACE;
	vstart    += USPACE;
	avail     -= USPACE;

	/*
	 * Calculate the number of pages needed for Sysseg.
	 * For the 68030, we need 256 descriptors (segment-table-entries).
	 * This easily fits into one page.
	 * For the 68040, both the level-1 and level-2 descriptors are
	 * stored into Sysseg. We currently handle a maximum sum of MAXKL2SIZE
	 * level-1 & level-2 tables.
	 */
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040)
		kstsize = MAXKL2SIZE / (NPTEPG/SG4_LEV2SIZE);
	else
#endif
		kstsize = 1;
	/*
	 * allocate the kernel segment table
	 */
	Sysseg_pa  = pstart;			/* pre-reloc PA to init STEs */
	Sysseg     = (st_entry_t *)vstart;
	pstart    += kstsize * PAGE_SIZE;
	vstart    += kstsize * PAGE_SIZE;
	avail     -= kstsize * PAGE_SIZE;

	/*
	 * allocate kernel page table map
	 */
	Sysptmap_pa = pstart;			/* pre-reloc PA to init PTEs */
	Sysptmap = (pt_entry_t *)vstart;
	pstart  += PAGE_SIZE;
	vstart  += PAGE_SIZE;
	avail   -= PAGE_SIZE;

	/*
	 * Determine the number of pte's we need for extra's like
	 * ST I/O map's.
	 */
	ptextra = btoc(STIO_SIZE);

	/*
	 * If present, add pci areas
	 */
	if (machineid & ATARI_HADES)
		ptextra += btoc(PCI_CONFIG_SIZE + PCI_IO_SIZE + PCI_MEM_SIZE);
	if (machineid & ATARI_MILAN)
		ptextra += btoc(PCI_IO_SIZE + PCI_MEM_SIZE);
	ptextra += btoc(BOOTM_VA_POOL);

	/*
	 * The 'pt' (the initial kernel pagetable) has to map the kernel and
	 * the I/O areas. The various I/O areas are mapped (virtually) at
	 * the top of the address space mapped by 'pt' (ie. just below Sysmap).
	 */
	ptpa	= pstart;			/* pre-reloc PA to init PTEs */
	ptsize  = (Sysptsize + howmany(ptextra, NPTEPG)) << PGSHIFT;
	pstart += ptsize;
	vstart += ptsize;
	avail  -= ptsize;

	/*
	 * Sysmap is now placed at the end of Supervisor virtual address space.
	 */
	Sysmap = (pt_entry_t *)-(NPTEPG * PAGE_SIZE);

	/*
	 * Initialize segment tables
	 */
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040)
		mmu040_setup(Sysseg_pa, kstsize, ptpa, ptsize, Sysptmap_pa,
		    kbase);
	else
#endif /* defined(M68040) || defined(M68060) */
		mmu030_setup(Sysseg_pa, kstsize, ptpa, ptsize, Sysptmap_pa,
		    kbase);

	/*
	 * initialize kernel page table page(s).
	 * Assume load at VA 0.
	 * - Text pages are RO
	 * - Page zero is invalid
	 */
	pg_proto = RELOC_PA(kbase, 0) | PG_RO | PG_V;
	pg       = (pt_entry_t *)ptpa;
	*pg++    = PG_NV;

	pg_proto += PAGE_SIZE;
	for (kva = PAGE_SIZE; kva < (vaddr_t)etext; kva += PAGE_SIZE) {
		*pg++ = pg_proto;
		pg_proto += PAGE_SIZE;
	}

	/* 
	 * data, bss and dynamic tables are read/write
	 */
	pg_proto = (pg_proto & PG_FRAME) | PG_RW | PG_V;

#if defined(M68040) || defined(M68060)
	/*
	 * Map the kernel segment table cache invalidated for 
	 * these machines (for the 68040 not strictly necessary, but
	 * recommended by Motorola; for the 68060 mandatory)
	 */
	if (mmutype == MMU_68040) {

		if (kernel_copyback)
			pg_proto |= PG_CCB;

		for (; kva < (vaddr_t)Sysseg; kva += PAGE_SIZE) {
			*pg++ = pg_proto;
			pg_proto += PAGE_SIZE;
		}

		pg_proto = (pg_proto & ~PG_CCB) | PG_CI;
		for (; kva < (vaddr_t)Sysptmap; kva += PAGE_SIZE) {
			*pg++ = pg_proto;
			pg_proto += PAGE_SIZE;
		}

		pg_proto = (pg_proto & ~PG_CI);
		if (kernel_copyback)
			pg_proto |= PG_CCB;
	}
#endif /* defined(M68040) || defined(M68060) */

	/*
	 * go till end of data allocated so far
	 * plus lwp0 u-area (to be allocated)
	 */
	for (; kva < vstart; kva += PAGE_SIZE) {
		*pg++ = pg_proto;
		pg_proto += PAGE_SIZE;
	}

	/*
	 * invalidate remainder of kernel PT
	 */
	epg = (pt_entry_t *)ptpa;
	epg = &epg[ptsize / sizeof(pt_entry_t)];
	while (pg < epg)
		*pg++ = PG_NV;

	/*
	 * Map various I/O areas
	 */
	map_io_areas(ptpa, ptsize, ptextra);

	/*
	 * Map the allocated space in ST-ram now. In the contig-case, there
	 * is no need to make a distinction between virtual and physical
	 * addresses. But I make it anyway to be prepared.
	 * Physcal space is already reserved!
	 */
	st_pool_virt = vstart;
	pg           = (pt_entry_t *)ptpa;
	pg           = &pg[vstart / PAGE_SIZE];
	pg_proto     = st_pool_phys | PG_RW | PG_CI | PG_V;
	vstart      += st_pool_size;
	while (pg_proto < (st_pool_phys + st_pool_size)) {
		*pg++     = pg_proto;
		pg_proto += PAGE_SIZE;
	}

	/*
	 * Map physical page_zero and page-zero+1 (First ST-ram page). We need
	 * to reference it in the reboot code. Two pages are mapped, because
	 * we must make sure 'doboot()' is contained in it (see the tricky
	 * copying there....).
	 */
	page_zero  = vstart;
	pg         = (pt_entry_t *)ptpa;
	pg         = &pg[vstart / PAGE_SIZE];
	*pg++      = PG_RW | PG_CI | PG_V;
	vstart    += PAGE_SIZE;
	*pg        = PG_RW | PG_CI | PG_V | PAGE_SIZE;
	vstart    += PAGE_SIZE;

	/*
	 * All necessary STEs and PTEs have been initialized.
	 * Update Sysseg_pa and Sysptmap_pa to point relocated PA.
	 */
	if (kbase) {
		Sysseg_pa   += kbase;
		Sysptmap_pa += kbase;
	}

	lowram  = 0 >> PGSHIFT; /* XXX */

	/*
	 * Fill in usable segments. The page indexes will be initialized
	 * later when all reservations are made.
	 */
	usable_segs[0].start = 0;
	usable_segs[0].end   = stphysize;
	usable_segs[0].free_list = VM_FREELIST_STRAM;
	usable_segs[1].start = ttphystart;
	usable_segs[1].end   = ttphystart + ttphysize;
	usable_segs[1].free_list = VM_FREELIST_TTRAM;
	usable_segs[2].start = usable_segs[2].end = 0; /* End of segments! */

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
	for (i = 1; usable_segs[i].start; i++) {
		usable_segs[i].first_page  = usable_segs[i-1].first_page;
		usable_segs[i].first_page +=
		    (usable_segs[i-1].end - usable_segs[i-1].start) / PAGE_SIZE;
	}
	for (i = 0, physmem = 0; usable_segs[i].start; i++)
		physmem += usable_segs[i].end - usable_segs[i].start;
	physmem >>= PGSHIFT;

	/*
	 * get the pmap module in sync with reality.
	 */
	pmap_bootstrap(vstart, Sysseg_pa);

	/*
	 * Prepare to enable the MMU.
	 * Setup and load SRP nolimit, share global, 4 byte PTE's
	 */
	protorp[0] = 0x80000202;
	protorp[1] = Sysseg_pa;			/* + segtable address */

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
		while(lp < le)
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
		__asm volatile ("pmove %0@,%%srp" : : "a" (&protorp[0]));
		/*
		 * setup and load TC register.
		 * enable_cpr, enable_srp, pagesize=8k,
		 * A = 8 bits, B = 11 bits
		 */
		tc = 0x82d08b00;
		__asm volatile ("pmove %0@,%%tc" : : "a" (&tc));
	}

	/*
	 * Initialize the "u-area" pages etc.
	 */
	pmap_bootstrap_finalize();

	/*
	 * Get the hardware into a defined state
	 */
	atari_hwinit();

	/*
	 * Initialize stmem allocator
	 */
	init_stmem();

	/*
	 * Initialize the I/O mem extent map.
	 * Note: we don't have to check the return value since
	 * creation of a fixed extent map will never fail (since
	 * descriptor storage has already been allocated).
	 *
	 * N.B. The iomem extent manages _all_ physical addresses
	 * on the machine.  When the amount of RAM is found, all
	 * extents of RAM are allocated from the map.
	 */
	iomem_ex = extent_create("iomem", 0x0, 0xffffffff, M_DEVBUF,
	    (void *)iomem_ex_storage, sizeof(iomem_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);

	/*
	 * Allocate the physical RAM from the extent map
	 */
	for (i = 0; boot_segs[i].end != 0; i++) {
		if (extent_alloc_region(iomem_ex, boot_segs[i].start,
		    boot_segs[i].end - boot_segs[i].start, EX_NOWAIT)) {
			/* XXX: Ahum, should not happen ;-) */
			printf("Warning: Cannot allocate boot memory from"
			    " extent map!?\n");
		}
	}

	/*
	 * Initialize interrupt mapping.
	 */
	intr_init();
}

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
	if (badbaddr((void *)__UNVOLATILE(&MFP2->mf_gpip), sizeof(char))) {
		/*
		 * Watch out! We can also have a Hades with < 16Mb
		 * RAM here...
		 */
		if (!badbaddr((void *)__UNVOLATILE(&MFP->mf_gpip),
		    sizeof(char))) {
			machineid |= ATARI_FALCON;
			return;
		}
	}
	if (!badbaddr((void *)(PCI_CONFB_PHYS + PCI_CONFM_PHYS), sizeof(char)))
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

/*
 * Do the dull work of mapping the various I/O areas. They MUST be Cache
 * inhibited!
 * All I/O areas are virtually mapped at the end of the pt-table.
 */
static void
map_io_areas(paddr_t ptpa, psize_t ptsize, u_int ptextra)
	/* ptsize:	 Size of 'pt' in bytes		*/
	/* ptextra:	 #of additional I/O pte's	*/
{
	extern void	bootm_init(vaddr_t, pt_entry_t *, u_long);
	vaddr_t		ioaddr;
	pt_entry_t	*pt, *pg, *epg;
	pt_entry_t	pg_proto;
	u_long		mask;

	pt = (pt_entry_t *)ptpa;
	ioaddr = ((ptsize / sizeof(pt_entry_t)) - ptextra) * PAGE_SIZE;

	/*
	 * Map ST-IO area
	 */
	stio_addr = ioaddr;
	ioaddr   += STIO_SIZE;
	pg        = &pt[stio_addr / PAGE_SIZE];
	epg       = &pg[btoc(STIO_SIZE)];
#ifdef _MILANHW_
	/*
	 * Turn on byte swaps in the ST I/O area. On the Milan, the
	 * U0 signal of the MMU controls the BigEndian signal
	 * of the PLX9080. We use this setting so we can read/write the
	 * PLX registers (and PCI-config space) in big-endian mode.
	 */
	pg_proto  = STIO_PHYS | PG_RW | PG_CI | PG_V | 0x100;
#else
	pg_proto  = STIO_PHYS | PG_RW | PG_CI | PG_V;
#endif
	while(pg < epg) {
		*pg++     = pg_proto;
		pg_proto += PAGE_SIZE;
	}

	/*
	 * Map PCI areas
	 */
	if (machineid & ATARI_HADES) {
		/*
		 * Only Hades maps the PCI-config space!
		 */
		pci_conf_addr = ioaddr;
		ioaddr       += PCI_CONFIG_SIZE;
		pg            = &pt[pci_conf_addr / PAGE_SIZE];
		epg           = &pg[btoc(PCI_CONFIG_SIZE)];
		mask          = PCI_CONFM_PHYS;
		pg_proto      = PCI_CONFB_PHYS | PG_RW | PG_CI | PG_V;
		for (; pg < epg; mask <<= 1)
			*pg++ = pg_proto | mask;
	} else
		pci_conf_addr = 0; /* XXX: should crash */

	if (machineid & (ATARI_HADES|ATARI_MILAN)) {
		pci_io_addr   = ioaddr;
		ioaddr       += PCI_IO_SIZE;
		pg	      = &pt[pci_io_addr / PAGE_SIZE];
		epg           = &pg[btoc(PCI_IO_SIZE)];
		pg_proto      = PCI_IO_PHYS | PG_RW | PG_CI | PG_V;
		while (pg < epg) {
			*pg++     = pg_proto;
			pg_proto += PAGE_SIZE;
		}

		pci_mem_addr  = ioaddr;
		/* Provide an uncached PCI address for the MILAN */
		pci_mem_uncached = ioaddr;
		ioaddr       += PCI_MEM_SIZE;
		epg           = &pg[btoc(PCI_MEM_SIZE)];
		pg_proto      = PCI_VGA_PHYS | PG_RW | PG_CI | PG_V;
		while (pg < epg) {
			*pg++     = pg_proto;
			pg_proto += PAGE_SIZE;
		}
	}

	bootm_init(ioaddr, pg, BOOTM_VA_POOL);
	/*
	 * ioaddr += BOOTM_VA_POOL;
	 * pg = &pg[btoc(BOOTM_VA_POOL)];
	 */
}

/*
 * Used by dumpconf() to get the size of the machine-dependent panic-dump
 * header in disk blocks.
 */

#define CHDRSIZE (ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)))
#define MDHDRSIZE roundup(CHDRSIZE, dbtob(1))

int
cpu_dumpsize(void)
{

	return btodb(MDHDRSIZE);
}

/*
 * Called by dumpsys() to dump the machine-dependent header.
 * XXX: Assumes that it will all fit in one diskblock.
 */
int
cpu_dump(int (*dump)(dev_t, daddr_t, void *, size_t), daddr_t *p_blkno)
{
	int		buf[MDHDRSIZE/sizeof(int)];
	int		error;
	kcore_seg_t	*kseg_p;
	cpu_kcore_hdr_t	*chdr_p;

	kseg_p = (kcore_seg_t *)buf;
	chdr_p = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(*kseg_p)) / sizeof(int)];

	/*
	 * Generate a segment header
	 */
	CORE_SETMAGIC(*kseg_p, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg_p->c_size = MDHDRSIZE - ALIGN(sizeof(*kseg_p));

	/*
	 * Add the md header
	 */
	*chdr_p = cpu_kcore_hdr;
	error = dump(dumpdev, *p_blkno, (void *)buf, sizeof(buf));
	*p_blkno += btodb(sizeof(buf));
	return (error);
}

#if (M68K_NPHYS_RAM_SEGS < NMEM_SEGS)
#error "Configuration error: M68K_NPHYS_RAM_SEGS < NMEM_SEGS"
#endif
/*
 * Initialize the cpu_kcore_header.
 */
static void
cpu_init_kcorehdr(paddr_t kbase, paddr_t sysseg_pa)
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	extern char end[];
	int i;

	memset(&cpu_kcore_hdr, 0, sizeof(cpu_kcore_hdr));

	/*
	 * Initialize the `dispatcher' portion of the header.
	 */
	strcpy(h->name, machine);
	h->page_size = PAGE_SIZE;
	h->kernbase = KERNBASE;

	/*
	 * Fill in information about our MMU configuration.
	 */
	m->mmutype	= mmutype;
	m->sg_v		= SG_V;
	m->sg_frame	= SG_FRAME;
	m->sg_ishift	= SG_ISHIFT;
	m->sg_pmask	= SG_PMASK; 
	m->sg40_shift1	= SG4_SHIFT1;
	m->sg40_mask2	= SG4_MASK2;
	m->sg40_shift2	= SG4_SHIFT2;
	m->sg40_mask3	= SG4_MASK3;
	m->sg40_shift3	= SG4_SHIFT3;
	m->sg40_addr1	= SG4_ADDR1;
	m->sg40_addr2	= SG4_ADDR2;
	m->pg_v		= PG_V;
	m->pg_frame	= PG_FRAME;

	/*
	 * Initialize pointer to kernel segment table.
	 */
	m->sysseg_pa = sysseg_pa;		/* PA after relocation */

	/*
	 * Initialize relocation value such that:
	 *
	 *	pa = (va - KERNBASE) + reloc
	 */
	m->reloc = kbase;

	/*
	 * Define the end of the relocatable range.
	 */
	m->relocend = (vaddr_t)end;

	for (i = 0; i < NMEM_SEGS; i++) {
		m->ram_segs[i].start = boot_segs[i].start;
		m->ram_segs[i].size  = boot_segs[i].end -
		    boot_segs[i].start;
	}
}

void
mmu030_setup(paddr_t sysseg_pa, u_int kstsize, paddr_t ptpa, psize_t ptsize,
    paddr_t sysptmap_pa, paddr_t kbase)
	/* sysseg_pa:	 System segment table		*/
	/* kstsize:	 size of 'sysseg' in pages	*/
	/* ptpa:	 Kernel page table		*/
	/* ptsize:	 size	of 'pt' in bytes	*/
	/* sysptmap_pa:	 System page table		*/
{
	st_entry_t	sg_proto, *sg, *esg;
	pt_entry_t	pg_proto, *pg, *epg;

	/*
	 * Map the page table pages in both the HW segment table
	 * and the software Sysptmap.
	 */
	sg  = (st_entry_t *)sysseg_pa;
	pg  = (pt_entry_t *)sysptmap_pa;
	epg = &pg[ptsize >> PGSHIFT];
	sg_proto = RELOC_PA(kbase, ptpa) | SG_RW | SG_V;
	pg_proto = RELOC_PA(kbase, ptpa) | PG_RW | PG_CI | PG_V;
	while (pg < epg) {
		*sg++ = sg_proto;
		*pg++ = pg_proto;
		sg_proto += PAGE_SIZE;
		pg_proto += PAGE_SIZE;
	}

	/* 
	 * Invalidate the remainder of the tables.
	 */
	esg = (st_entry_t *)sysseg_pa;
	esg = &esg[256];			/* XXX should be TIA_SIZE */
	while (sg < esg)
		*sg++ = SG_NV;
	epg = (pt_entry_t *)sysptmap_pa;
	epg = &epg[NPTEPG];			/* XXX should be TIB_SIZE */
	while (pg < epg)
		*pg++ = PG_NV;

	/*
	 * Initialize the PTE for the last one to point Sysptmap.
	 */
	sg = (st_entry_t *)sysseg_pa;
	sg = &sg[256 - 1];			/* XXX should be TIA_SIZE */
	pg = (pt_entry_t *)sysptmap_pa;
	pg = &pg[256 - 1];			/* XXX should be TIA_SIZE */
	*sg = RELOC_PA(kbase, sysptmap_pa) | SG_RW | SG_V;
	*pg = RELOC_PA(kbase, sysptmap_pa) | PG_RW | PG_CI | PG_V;
}

#if defined(M68040) || defined(M68060)
void
mmu040_setup(paddr_t sysseg_pa, u_int kstsize, paddr_t ptpa, psize_t ptsize,
    paddr_t sysptmap_pa, paddr_t kbase)
	/* sysseg_pa:	 System segment table		*/
	/* kstsize:	 size of 'sysseg' in pages	*/
	/* ptpa:	 Kernel page table		*/
	/* ptsize:	 size	of 'pt' in bytes	*/
	/* sysptmap_pa:	 System page table		*/
{
	int		nl1desc, nl2desc, i;
	st_entry_t	sg_proto, *sg, *esg;
	pt_entry_t	pg_proto, *pg, *epg;

	/*
	 * First invalidate the entire "segment table" pages
	 * (levels 1 and 2 have the same "invalid" values).
	 */
	sg  = (st_entry_t *)sysseg_pa;
	esg = &sg[kstsize * NPTEPG];
	while (sg < esg)
		*sg++ = SG_NV;

	/*
	 * Initialize level 2 descriptors (which immediately
	 * follow the level 1 table).
	 * We need:
	 *	NPTEPG / SG4_LEV3SIZE
	 * level 2 descriptors to map each of the nptpages
	 * pages of PTEs.  Note that we set the "used" bit
	 * now to save the HW the expense of doing it.
	 */
	nl2desc = (ptsize >> PGSHIFT) * (NPTEPG / SG4_LEV3SIZE);
	sg  = (st_entry_t *)sysseg_pa;
	sg  = &sg[SG4_LEV1SIZE];
	esg = &sg[nl2desc];
	sg_proto = RELOC_PA(kbase, ptpa) | SG_U | SG_RW | SG_V;
	while (sg < esg) {
		*sg++     = sg_proto;
		sg_proto += (SG4_LEV3SIZE * sizeof(st_entry_t));
	}

	/*
	 * Initialize level 1 descriptors.  We need:
	 *	howmany(nl2desc, SG4_LEV2SIZE)
	 * level 1 descriptors to map the 'nl2desc' level 2's.
	 */
	nl1desc = howmany(nl2desc, SG4_LEV2SIZE);
	sg  = (st_entry_t *)sysseg_pa;
	esg = &sg[nl1desc];
	sg_proto = RELOC_PA(kbase, (paddr_t)&sg[SG4_LEV1SIZE])
	    | SG_U | SG_RW | SG_V;
	while (sg < esg) {
		*sg++     = sg_proto;
		sg_proto += (SG4_LEV2SIZE * sizeof(st_entry_t));
	}

	/* Sysmap is last entry in level 1 */
	sg  = (st_entry_t *)sysseg_pa;
	sg  = &sg[SG4_LEV1SIZE - 1];
	*sg = sg_proto;

	/*
	 * Kernel segment table at end of next level 2 table
	 */
	i = SG4_LEV1SIZE + (nl1desc * SG4_LEV2SIZE);
	sg  = (st_entry_t *)sysseg_pa;
	sg  = &sg[i + SG4_LEV2SIZE - (NPTEPG / SG4_LEV3SIZE)];
	esg = &sg[NPTEPG / SG4_LEV3SIZE];
	sg_proto = RELOC_PA(kbase, sysptmap_pa) | SG_U | SG_RW | SG_V;
	while (sg < esg) {
		*sg++ = sg_proto;
		sg_proto += (SG4_LEV3SIZE * sizeof(st_entry_t));
	}

	/* Include additional level 2 table for Sysmap in protostfree */
	protostfree = (~0 << (1 + nl1desc + 1)) /* & ~(~0 << MAXKL2SIZE) */;

	/*
	 * Initialize Sysptmap
	 */
	pg  = (pt_entry_t *)sysptmap_pa;
	epg = &pg[ptsize >> PGSHIFT];
	pg_proto = RELOC_PA(kbase, ptpa) | PG_RW | PG_CI | PG_V;
	while (pg < epg) {
		*pg++ = pg_proto;
		pg_proto += PAGE_SIZE;
	}

	/*
	 * Invalidate rest of Sysptmap page.
	 */
	epg = (pt_entry_t *)sysptmap_pa;
	epg = &epg[NPTEPG];		/* XXX: should be TIB_SIZE */
	while (pg < epg)
		*pg++ = PG_NV;

	/*
	 * Initialize the PTE for the last one to point Sysptmap.
	 */
	pg = (pt_entry_t *)sysptmap_pa;
	pg = &pg[256 - 1];		/* XXX: should be TIA_SIZE */
	*pg = RELOC_PA(kbase, sysptmap_pa) | PG_RW | PG_CI | PG_V;
}
#endif /* M68040 */

#if defined(M68060)
int m68060_pcr_init = 0x21;	/* make this patchable */
#endif

static void
initcpu(void)
{
	typedef void trapfun(void);

	switch (cputype) {

#if defined(M68060)
	case CPU_68060:
		{
			extern trapfun	*vectab[256];
			extern trapfun	buserr60, addrerr4060, fpfault;
#if defined(M060SP)
			extern u_int8_t FP_CALL_TOP[], I_CALL_TOP[];
#else
			extern trapfun illinst;
#endif

			__asm volatile ("movl %0,%%d0; .word 0x4e7b,0x0808" : : 
					"d"(m68060_pcr_init):"d0" );

			/* bus/addrerr vectors */
			vectab[2] = buserr60;
			vectab[3] = addrerr4060;

#if defined(M060SP)
			/* integer support */
			vectab[61] = (trapfun *)&I_CALL_TOP[128 + 0x00];

			/* floating point support */
			/*
			 * XXX maybe we really should run-time check for the
			 * stack frame format here:
			 */
			vectab[11] = (trapfun *)&FP_CALL_TOP[128 + 0x30];

			vectab[55] = (trapfun *)&FP_CALL_TOP[128 + 0x38];
			vectab[60] = (trapfun *)&FP_CALL_TOP[128 + 0x40];

			vectab[54] = (trapfun *)&FP_CALL_TOP[128 + 0x00];
			vectab[52] = (trapfun *)&FP_CALL_TOP[128 + 0x08];
			vectab[53] = (trapfun *)&FP_CALL_TOP[128 + 0x10];
			vectab[51] = (trapfun *)&FP_CALL_TOP[128 + 0x18];
			vectab[50] = (trapfun *)&FP_CALL_TOP[128 + 0x20];
			vectab[49] = (trapfun *)&FP_CALL_TOP[128 + 0x28];
#else
			vectab[61] = illinst;
#endif
			vectab[48] = fpfault;
		}
		break;
#endif /* defined(M68060) */
#if defined(M68040)
	case CPU_68040:
		{
			extern trapfun	*vectab[256];
			extern trapfun	buserr40, addrerr4060;

			/* bus/addrerr vectors */
			vectab[2] = buserr40;
			vectab[3] = addrerr4060;
		}
		break;
#endif /* defined(M68040) */
#if defined(M68030) || defined(M68020)
	case CPU_68030:
	case CPU_68020:
		{
			extern trapfun	*vectab[256];
			extern trapfun	buserr2030, addrerr2030;

			/* bus/addrerr vectors */
			vectab[2] = buserr2030;
			vectab[3] = addrerr2030;
		}
		break;
#endif /* defined(M68030) || defined(M68020) */
	}

	DCIS();
}

#ifdef DEBUG
void dump_segtable(u_int *);
void dump_pagetable(u_int *, u_int, u_int);
u_int vmtophys(u_int *, u_int);

void
dump_segtable(u_int *stp)
{
	u_int *s, *es;
	int shift, i;

	s = stp;
	{
		es = s + (M68K_STSIZE >> 2);
		shift = SG_ISHIFT;
	}

	/* 
	 * XXX need changes for 68040 
	 */
	for (i = 0; s < es; s++, i++)
		if (*s & SG_V)
			printf("$%08x: $%08x\t", i << shift, *s & SG_FRAME);
	printf("\n");
}

void
dump_pagetable(u_int *ptp, u_int i, u_int n)
{
	u_int *p, *ep;

	p = ptp + i;
	ep = p + n;
	for (; p < ep; p++, i++)
		if (*p & PG_V)
			printf("$%08x -> $%08x\t", i, *p & PG_FRAME);
	printf("\n");
}

u_int
vmtophys(u_int *ste, u_int vm)
{

	ste = (u_int *)(*(ste + (vm >> SEGSHIFT)) & SG_FRAME);
	ste += (vm & SG_PMASK) >> PGSHIFT;
	return (*ste & -PAGE_SIZE) | (vm & (PAGE_SIZE - 1));
}

#endif
