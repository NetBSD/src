/*	$NetBSD: arm32_machdep.c,v 1.2.2.6 2002/02/11 20:07:17 jdolecek Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 * Machine dependant functions for kernel setup
 *
 * Created      : 17/09/94
 * Updated	: 18/04/01 updated for new wscons
 */

#include "opt_md.h"
#include "opt_pmap_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/msgbuf.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

#include <dev/cons.h>

#include <arm/arm32/katelib.h>
#include <arm/arm32/machdep.h>
#include <machine/bootconfig.h>

#include "opt_ipkdb.h"
#include "opt_mdsize.h"
#include "md.h"

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

extern int physmem;

#ifndef PMAP_STATIC_L1S
extern int max_processes;
#endif	/* !PMAP_STATIC_L1S */
#if NMD > 0 && defined(MEMORY_DISK_HOOKS) && !defined(MINIROOTSIZE)
extern u_int memory_disc_size;		/* Memory disc size */
#endif	/* NMD && MEMORY_DISK_HOOKS && !MINIROOTSIZE */

pv_addr_t systempage;
pv_addr_t kernelstack;

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

extern pt_entry_t msgbufpte;
caddr_t	msgbufaddr;
extern paddr_t msgbufphys;

int kernel_debug = 0;

struct user *proc0paddr;

char *booted_kernel;


/* Prototypes */

u_long strtoul			__P((const char *s, char **ptr, int base));
void data_abort_handler		__P((trapframe_t *frame));
void prefetch_abort_handler	__P((trapframe_t *frame));
void zero_page_readonly		__P((void));
void zero_page_readwrite	__P((void));
extern void configure		__P((void));

/*
 * Debug function just to park the CPU
 */

void
halt()
{
	while (1)
		cpu_sleep(0);
}


/* Sync the discs and unmount the filesystems */

void
bootsync(void)
{
	static int bootsyncdone = 0;

	if (bootsyncdone) return;

	bootsyncdone = 1;

	/* Make sure we can still manage to do things */
	if (GetCPSR() & I32_bit) {
		/*
		 * If we get here then boot has been called without RB_NOSYNC
		 * and interrupts were disabled. This means the boot() call
		 * did not come from a user process e.g. shutdown, but must
		 * have come from somewhere in the kernel.
		 */
		IRQenable;
		printf("Warning IRQ's disabled during boot()\n");
	}

	vfs_shutdown();
}

/*
 * A few functions that are used to help construct the page tables
 * during the bootstrap process.
 */

void
map_section(pagetable, va, pa, cacheable)
	vaddr_t pagetable;
	vaddr_t va;
	paddr_t pa;
	int cacheable;
{
#ifdef	DIAGNOSTIC
	if (((va | pa) & (L1_SEC_SIZE - 1)) != 0)
		panic("initarm: Cannot allocate 1MB section on non 1MB boundry\n");
#endif	/* DIAGNOSTIC */

	if (cacheable)
		((u_int *)pagetable)[(va >> PDSHIFT)] =
		    L1_SEC((pa & PD_MASK), pte_cache_mode);
	else
		((u_int *)pagetable)[(va >> PDSHIFT)] =
		    L1_SEC((pa & PD_MASK), 0);
}


void
map_pagetable(pagetable, va, pa)
	vaddr_t pagetable;
	vaddr_t va;
	paddr_t pa;
{
#ifdef	DIAGNOSTIC
	if ((pa & 0xc00) != 0)
		panic("pagetables should be group allocated on pageboundry");
#endif	/* DIAGNOSTIC */

	((u_int *)pagetable)[(va >> PDSHIFT) + 0] =
	     L1_PTE((pa & PG_FRAME) + 0x000);
	((u_int *)pagetable)[(va >> PDSHIFT) + 1] =
	     L1_PTE((pa & PG_FRAME) + 0x400);
	((u_int *)pagetable)[(va >> PDSHIFT) + 2] =
	     L1_PTE((pa & PG_FRAME) + 0x800);
	((u_int *)pagetable)[(va >> PDSHIFT) + 3] =
	     L1_PTE((pa & PG_FRAME) + 0xc00);
}

/* cats kernels have a 2nd l2 pt, so the range is bigger hence the 0x7ff etc */
vsize_t
map_chunk(pd, pt, va, pa, size, acc, flg)
	vaddr_t pd;
	vaddr_t pt;
	vaddr_t va;
	paddr_t pa;
	vsize_t size;
	u_int acc;
	u_int flg;
{
	pd_entry_t *l1pt = (pd_entry_t *)pd;
	pt_entry_t *l2pt = (pt_entry_t *)pt;
	vsize_t remain;
	u_int loop;

	remain = (size + (NBPG - 1)) & ~(NBPG - 1);
#ifdef VERBOSE_INIT_ARM
	printf("map_chunk: pa=%lx va=%lx sz=%lx rem=%lx acc=%x flg=%x\n",
	    pa, va, size, remain, acc, flg);
	printf("map_chunk: ");
#endif
	size = remain;

	while (remain > 0) {
		/* Can we do a section mapping ? */
		if (l1pt && !((pa | va) & (L1_SEC_SIZE - 1))
		    && remain >= L1_SEC_SIZE) {
#ifdef VERBOSE_INIT_ARM
			printf("S");
#endif
			l1pt[(va >> PDSHIFT)] = L1_SECPTE(pa, acc, flg);
			va += L1_SEC_SIZE;
			pa += L1_SEC_SIZE;
			remain -= L1_SEC_SIZE;
		} else
		/* Can we do a large page mapping ? */
		if (!((pa | va) & (L2_LPAGE_SIZE - 1))
		    && (remain >= L2_LPAGE_SIZE)) {
#ifdef VERBOSE_INIT_ARM
			printf("L");
#endif
			for (loop = 0; loop < 16; ++loop)
#ifndef cats
				l2pt[((va >> PGSHIFT) & 0x3f0) + loop] =
				    L2_LPTE(pa, acc, flg);
#else
				l2pt[((va >> PGSHIFT) & 0x7f0) + loop] =
				    L2_LPTE(pa, acc, flg);
#endif	
			va += L2_LPAGE_SIZE;
			pa += L2_LPAGE_SIZE;
			remain -= L2_LPAGE_SIZE;
		} else
		/* All we can do is a small page mapping */
		{
#ifdef VERBOSE_INIT_ARM
			printf("P");
#endif
#ifndef cats			
			l2pt[((va >> PGSHIFT) & 0x3ff)] = L2_SPTE(pa, acc, flg);
#else
			l2pt[((va >> PGSHIFT) & 0x7ff)] = L2_SPTE(pa, acc, flg);
#endif
			va += NBPG;
			pa += NBPG;
			remain -= NBPG;
		}
	}
#ifdef VERBOSE_INIT_ARM
	printf("\n");
#endif
	return(size);
}

/* cats versions have larger 2 l2pt's next to each other */
void
map_entry(pagetable, va, pa)
	vaddr_t pagetable;
	vaddr_t va;
	paddr_t pa;
{
#ifndef cats
	((pt_entry_t *)pagetable)[((va >> PGSHIFT) & 0x000003ff)] =
	    L2_PTE((pa & PG_FRAME), AP_KRW);
#else
	((pt_entry_t *)pagetable)[((va >> PGSHIFT) & 0x000007ff)] =
	    L2_PTE((pa & PG_FRAME), AP_KRW);
#endif	
}


void
map_entry_nc(pagetable, va, pa)
	vaddr_t pagetable;
	vaddr_t va;
	paddr_t pa;
{
#ifndef cats
	((pt_entry_t *)pagetable)[((va >> PGSHIFT) & 0x000003ff)] =
	    L2_PTE_NC_NB((pa & PG_FRAME), AP_KRW);
#else
	((pt_entry_t *)pagetable)[((va >> PGSHIFT) & 0x000007ff)] =
	    L2_PTE_NC_NB((pa & PG_FRAME), AP_KRW);
#endif
}


void
map_entry_ro(pagetable, va, pa)
	vaddr_t pagetable;
	vaddr_t va;
	paddr_t pa;
{
#ifndef cats
	((pt_entry_t *)pagetable)[((va >> PGSHIFT) & 0x000003ff)] =
	    L2_PTE((pa & PG_FRAME), AP_KR);
#else
	((pt_entry_t *)pagetable)[((va >> PGSHIFT) & 0x000007ff)] =
	    L2_PTE((pa & PG_FRAME), AP_KR);
#endif
}


/*
 * void cpu_startup(void)
 *
 * Machine dependant startup code. 
 *
 */

void
cpu_startup()
{
	int loop;
	paddr_t minaddr;
	paddr_t maxaddr;
	caddr_t sysbase;
	caddr_t size;
	vsize_t bufsize;
	int base, residual;
	char pbuf[9];

	proc0paddr = (struct user *)kernelstack.pv_va;
	proc0.p_addr = proc0paddr;

	/* Set the cpu control register */
	cpu_setup(boot_args);

	/* All domains MUST be clients, permissions are VERY important */
	cpu_domains(DOMAIN_CLIENT);

	/* Lock down zero page */
	zero_page_readonly();

	/*
	 * Give pmap a chance to set up a few more things now the vm
	 * is initialised
	 */
	pmap_postinit();

	/*
	 * Initialize error message buffer (at end of core).
	 */

	/* msgbufphys was setup during the secondary boot strap */
	for (loop = 0; loop < btoc(MSGBUFSIZE); ++loop)
		pmap_kenter_pa((vaddr_t)msgbufaddr + loop * NBPG,
		    msgbufphys + loop * NBPG, VM_PROT_READ|VM_PROT_WRITE);
	pmap_update(pmap_kernel());
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));

	/*
	 * Identify ourselves for the msgbuf (everything printed earlier will
	 * not be buffered).
	 */
	printf(version);

	format_bytes(pbuf, sizeof(pbuf), arm_page_to_byte(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	size = allocsys(NULL, NULL);
	sysbase = (caddr_t)uvm_km_zalloc(kernel_map, round_page((vaddr_t)size));
	if (sysbase == 0)
		panic(
		    "cpu_startup: no room for system tables; %d bytes required",
		    (u_int)size);
	if ((caddr_t)((allocsys(sysbase, NULL) - sysbase)) != size)
		panic("cpu_startup: system table size inconsistency");

   	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	bufsize = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *)&buffers, round_page(bufsize),
	    NULL, UVM_UNKNOWN_OFFSET, 0,
	    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
	    UVM_ADV_NORMAL, 0)) != 0)
		panic("cpu_startup: cannot allocate UVM space for buffers");
	minaddr = (vaddr_t)buffers;
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}

	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (loop = 0; loop < nbuf; ++loop) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t) buffers + (loop * MAXBSIZE);
		curbufsize = NBPG * ((loop < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for buffer cache");
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
				   16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 nmbclusters * mclbytes, VM_MAP_INTRSAFE,
				 FALSE, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_flags = 0;
	curpcb->pcb_un.un_32.pcb32_und_sp = (u_int)proc0.p_addr +
	    USPACE_UNDEF_STACK_TOP;
	curpcb->pcb_un.un_32.pcb32_sp = (u_int)proc0.p_addr +
	    USPACE_SVC_STACK_TOP;
	(void) pmap_extract(pmap_kernel(), (vaddr_t)(pmap_kernel())->pm_pdir,
	    (paddr_t *)&curpcb->pcb_pagedir);

        curpcb->pcb_tf = (struct trapframe *)curpcb->pcb_un.un_32.pcb32_sp - 1;
}

/*
 * Modify the current mapping for zero page to make it read only
 *
 * This routine is only used until things start forking. Then new
 * system pages are mapped read only in pmap_enter().
 */

void
zero_page_readonly()
{
	WriteWord(PROCESS_PAGE_TBLS_BASE + 0,
	    L2_PTE((systempage.pv_pa & PG_FRAME), AP_KR));
	cpu_tlb_flushID_SE(0x00000000);
}


/*
 * Modify the current mapping for zero page to make it read/write
 *
 * This routine is only used until things start forking. Then system
 * pages belonging to user processes are never made writable.
 */

void
zero_page_readwrite()
{
	WriteWord(PROCESS_PAGE_TBLS_BASE + 0,
	    L2_PTE((systempage.pv_pa & PG_FRAME), AP_KRW));
	cpu_tlb_flushID_SE(0x00000000);
}


/*
 * machine dependent system variables.
 */

int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_DEBUG:
		return(sysctl_int(oldp, oldlenp, newp, newlen, &kernel_debug));

	case CPU_BOOTED_DEVICE:
		if (booted_device != NULL)
			return (sysctl_rdstring(oldp, oldlenp, newp,
			    booted_device->dv_xname));
		return (EOPNOTSUPP);

	case CPU_CONSDEV: {
		dev_t consdev;
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
			sizeof consdev));
	}
	case CPU_BOOTED_KERNEL: {
		if (booted_kernel != NULL && booted_kernel[0] != '\0')
			return sysctl_rdstring(oldp, oldlenp, newp,
			    booted_kernel);
		return (EOPNOTSUPP);
	}

	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

void
parse_mi_bootargs(args)
	char *args;
{
	int integer;

	if (get_bootconf_option(args, "single", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-s", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_SINGLE;
	if (get_bootconf_option(args, "kdb", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-k", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_KDB;
	if (get_bootconf_option(args, "ask", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-a", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_ASKNAME;

#ifdef PMAP_DEBUG
	if (get_bootconf_option(args, "pmapdebug", BOOTOPT_TYPE_INT, &integer)) {
		pmap_debug_level = integer;
		pmap_debug(pmap_debug_level);
	}
#endif	/* PMAP_DEBUG */

/*	if (get_bootconf_option(args, "nbuf", BOOTOPT_TYPE_INT, &integer))
		bufpages = integer;*/

#ifndef PMAP_STATIC_L1S
	if (get_bootconf_option(args, "maxproc", BOOTOPT_TYPE_INT, &integer)) {
		max_processes = integer;
		if (max_processes < 16)
			max_processes = 16;
		/* Limit is PDSIZE * (max_processes + 1) <= 4MB */
		if (max_processes > 255)
			max_processes = 255;
	}
#endif	/* !PMAP_STATUC_L1S */
#if NMD > 0 && defined(MEMORY_DISK_HOOKS) && !defined(MINIROOTSIZE)
	if (get_bootconf_option(args, "memorydisc", BOOTOPT_TYPE_INT, &integer)
	    || get_bootconf_option(args, "memorydisk", BOOTOPT_TYPE_INT, &integer)) {
		memory_disc_size = integer;
		memory_disc_size *= 1024;
		if (memory_disc_size < 32*1024)
			memory_disc_size = 32*1024;
		if (memory_disc_size > 2048*1024)
			memory_disc_size = 2048*1024;
	}
#endif	/* NMD && MEMORY_DISK_HOOKS && !MINIROOTSIZE */

	if (get_bootconf_option(args, "quiet", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-q", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= AB_QUIET;
	if (get_bootconf_option(args, "verbose", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-v", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= AB_VERBOSE;
}

/* End of machdep.c */
