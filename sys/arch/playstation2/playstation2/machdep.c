/*	$NetBSD: machdep.c,v 1.1.4.4 2002/06/24 22:06:49 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ddb.h"
#include "opt_kloader_kernel_path.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifndef DB_ELFSIZE
#error Must define DB_ELFSIZE!
#endif
#define ELFSIZE		DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

#include <dev/cons.h>	/* cntab access (cpu_reboot) */
#include <machine/bootinfo.h>
#include <machine/psl.h>
#include <machine/intr.h>/* hardintr_init */
#include <playstation2/playstation2/sifbios.h>
#include <playstation2/playstation2/interrupt.h>
#ifdef KLOADER_KERNEL_PATH
#include <playstation2/playstation2/kloader.h>
#endif

/* For sysctl. */
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;
char cpu_model[] = "SONY PlayStation 2";

struct cpu_info cpu_info_store;

struct vm_map *exec_map;
struct vm_map *mb_map;
struct vm_map *phys_map;
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;
int physmem;	/* for buffer cache, vnode cache estimation */

#ifdef DEBUG
static void bootinfo_dump(void);
#endif

void mach_init(void);
/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init()
{
	extern char kernel_text[], edata[], end[];
	extern struct user *proc0paddr;
	caddr_t kernend, v;
	paddr_t start;
	size_t size;

	/*
	 * Clear the BSS segment.
	 */
	kernend = (caddr_t)mips_round_page(end);
	memset(edata, 0, kernend - edata);

	/* disable all interrupt */
	interrupt_init_bootstrap();

	/* enable SIF BIOS */
	sifbios_init();

	consinit();

	printf("kernel_text=%p edata=%p end=%p\n", kernel_text, edata, end);

#ifdef DEBUG
	bootinfo_dump();
#endif
	uvm_setpagesize();
	physmem = atop(PS2_MEMORY_SIZE);

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init();

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	start = (paddr_t)round_page(MIPS_KSEG0_TO_PHYS(kernend));
	size = PS2_MEMORY_SIZE - start - BOOTINFO_BLOCK_SIZE;
	memset((void *)MIPS_PHYS_TO_KSEG1(start), 0, size);
	    
	/* kernel itself */
	mem_clusters[0].start = trunc_page(MIPS_KSEG0_TO_PHYS(kernel_text));
	mem_clusters[0].size = start - mem_clusters[0].start;
	/* heap */
	mem_clusters[1].start = start;
	mem_clusters[1].size = size;
	/* load */
	printf("load memory %#lx, %#x\n", start, size);
	uvm_page_physload(atop(start), atop(start + size),
	    atop(start), atop(start + size), VM_FREELIST_DEFAULT);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Compute the size of system data structures.  pmap_bootstrap()
	 * needs some of this information.
	 */
	size = (vsize_t)allocsys(NULL, NULL);

	pmap_bootstrap();

	/*
	 * Allocate space for proc0's USPACE.
	 */
	v = (caddr_t)uvm_pageboot_alloc(USPACE); 
	proc0.p_addr = proc0paddr = (struct user *)v;
	proc0.p_md.md_regs = (struct frame *)(v + USPACE) - 1;
	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_context[11] = PSL_LOWIPL;	/* SR */
#ifdef IPL_ICU_MASK
	curpcb->pcb_ppl = 0;
#endif

	/*
	 * Allocate space for system data structures.  These data structures
	 * are allocated here instead of cpu_startup() because physical
	 * memory is directly addressable.  We don't have to map these into
	 * virtual address space.
	 */
	v = (caddr_t)uvm_pageboot_alloc(size); 
	if ((allocsys(v, NULL) - v) != size)
		panic("mach_init: table size inconsistency");
}

/*
 * Allocate memory for variable-sized tables,
 */
void
cpu_startup()
{
	unsigned i;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	printf("%s\n", cpu_model);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("%s memory", pbuf);

	/*
	 * Allocate virtual address space for file I/O buffers.
	 * Note they are different than the array of headers, 'buf',
	 * and usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *)&buffers, round_page(size),
	    NULL, UVM_UNKNOWN_OFFSET, 0, UVM_MAPFLAG(UVM_PROT_NONE,
		UVM_PROT_NONE, UVM_INH_NONE, UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t) buffers + (i * MAXBSIZE);
		curbufsize = NBPG * ((i < residual) ? (base + 1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
			    VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}
	pmap_update(pmap_kernel());

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    16 * NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);
	/*
	 * Allocate a submap for physio.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * (No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG to
	 * map those pages.)
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf(", %s free", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf(", %s in %d buffers\n", pbuf, nbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
}

/*
 * Machine dependent system variables.
 */
int
cpu_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp, void *newp,
    size_t newlen, struct proc *p)
{
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return (ENOTDIR);

	switch (name[0]) {
	case CPU_CONSDEV:
		return (sysctl_rdstruct(oldp, oldlenp, newp, &cn_tab->cn_dev,
		    sizeof cn_tab->cn_dev));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

void
cpu_reboot(int howto, char *bootstr)
{
	static int waittime = -1;

	/* Take a snapshot before clobbering any registers. */
	if (curlwp)
		savectx((struct user *)curpcb);

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	/* If "always halt" was specified as a boot flag, obey. */
	if (boothowto & RB_HALT) {
		howto |= RB_HALT;
	}

#ifdef KLOADER_KERNEL_PATH
	if ((howto & RB_HALT) == 0)
		kloader_reboot_setup(KLOADER_KERNEL_PATH);
#endif

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && (waittime < 0)) {
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	splhigh();

	if (howto & RB_DUMP)
		dumpsys();

 haltsys:
	doshutdownhooks();

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN)
		sifbios_halt(0); /* power down */
	else if (howto & RB_HALT)
		sifbios_halt(1); /* halt */
	else {
#ifdef KLOADER_KERNEL_PATH
		kloader_reboot();
		/* NOTREACHED */
#endif
		sifbios_halt(2); /* reset */
	}

	while (1)
		;
	/* NOTREACHED */
}

#ifdef DEBUG
void
bootinfo_dump()
{
	printf("devconf=%#x, option=%#x, rtc=%#x, pcmcia_type=%#x,"
	    "sysconf=%#x\n",
	    BOOTINFO_REF(BOOTINFO_DEVCONF),
	    BOOTINFO_REF(BOOTINFO_OPTION_PTR),
	    BOOTINFO_REF(BOOTINFO_RTC),
	    BOOTINFO_REF(BOOTINFO_PCMCIA_TYPE),	
	    BOOTINFO_REF(BOOTINFO_SYSCONF));
}
#endif /* DEBUG */
