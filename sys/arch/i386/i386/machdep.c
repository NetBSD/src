/*	$NetBSD: machdep.c,v 1.337 1999/02/06 18:46:21 drochner Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include "opt_cputype.h"
#include "opt_ddb.h"
#include "opt_vm86.h"
#include "opt_user_ldt.h"
#include "opt_uvm.h"
#include "opt_pmap_new.h"
#include "opt_compat_netbsd.h"
#include "opt_cpureset_delay.h"
#include "opt_compat_svr4.h"
#include "opt_sysv.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/syscallargs.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <machine/kcore.h>
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVSHM
#include <sys/shm.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <dev/cons.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#if defined(UVM)
#include <uvm/uvm_extern.h>
#endif

#include <sys/sysctl.h>

#define _I386_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/gdt.h>
#include <machine/pio.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/specialreg.h>
#include <machine/bootinfo.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/mc146818reg.h>
#include <i386/isa/isa_machdep.h>
#include <i386/isa/nvram.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#ifdef VM86
#include <machine/vm86.h>
#endif

#include "apm.h"
#include "bioscall.h"

#if NBIOSCALL > 0
#include <machine/bioscall.h>
#endif

#if NAPM > 0
#include <machine/apmvar.h>
#endif

#include "isa.h"
#include "isadma.h"
#include "npx.h"
#if NNPX > 0
extern struct proc *npxproc;
#endif

#include "vga.h"
#include "pcdisplay.h"
#if (NVGA > 0) || (NPCDISPLAY > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#if (NVGA > 0)
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif
#if (NPCDISPLAY > 0)
#include <dev/isa/pcdisplayvar.h>
#endif
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/isa/pckbcvar.h>
#endif

#include "pc.h"
#if (NPC > 0)
#include <machine/pccons.h>
#endif

#include "vt.h"
#if (NVT > 0)
#include <i386/isa/pcvt/pcvt_cons.h>
#endif

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

/* the following is used externally (sysctl_hw) */
char machine[] = "i386";		/* cpu "architecture" */
char machine_arch[] = "i386";		/* machine == machine_arch */

char bootinfo[BOOTINFO_MAXSIZE];

/*
 * Declare these as initialized data so we can patch them.
 */
int	nswbuf = 0;
#ifdef	NBUF
int	nbuf = NBUF;
#else
int	nbuf = 0;
#endif
#ifdef	BUFPAGES
int	bufpages = BUFPAGES;
#else
int	bufpages = 0;
#endif
#ifdef BUFCACHE
int	bufcache = BUFCACHE;	/* % of RAM to use for buffer cache */
#else
int	bufcache = 0;		/* fallback to old algorithm */
#endif

#ifdef CPURESET_DELAY
int	cpureset_delay = CPURESET_DELAY;
#else
int     cpureset_delay = 2000; /* default to 2s */
#endif


int	physmem;
int	dumpmem_low;
int	dumpmem_high;
int	boothowto;
int	cpu_class;

vaddr_t	msgbuf_vaddr;
paddr_t msgbuf_paddr;

vaddr_t	idt_vaddr;
paddr_t	idt_paddr;

#ifdef I586_CPU
vaddr_t	pentium_idt_vaddr;
#endif

#if defined(UVM)
vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;
#else
vm_map_t buffer_map;
#endif

extern	int biosbasemem, biosextmem;
extern	paddr_t avail_start, avail_end;
extern	paddr_t hole_start, hole_end;

/*
 * Extent maps to manage I/O and ISA memory hole space.  Allocate
 * storage for 8 regions in each, initially.  Later, ioport_malloc_safe
 * will indicate that it's safe to use malloc() to dynamically allocate
 * region descriptors.
 *
 * N.B. At least two regions are _always_ allocated from the iomem
 * extent map; (0 -> ISA hole) and (end of ISA hole -> end of RAM).
 *
 * The extent maps are not static!  Machine-dependent ISA and EISA
 * routines need access to them for bus address space allocation.
 */
static	long ioport_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
static	long iomem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
struct	extent *ioport_ex;
struct	extent *iomem_ex;
static	int ioport_malloc_safe;

/*
 * Size of memory segments, before any memory is stolen.
 */
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int	mem_cluster_cnt;

caddr_t	allocsys __P((caddr_t));
int	cpu_dump __P((void));
int	cpu_dumpsize __P((void));
u_long	cpu_dump_mempagecnt __P((void));
void	dumpsys __P((void));
void	identifycpu __P((void));
void	init386 __P((paddr_t));

#ifndef CONSDEVNAME
#define CONSDEVNAME "pc"
#endif
#if (NCOM > 0)
#ifndef CONADDR
#define CONADDR 0x3f8
#endif
#ifndef CONSPEED
#define CONSPEED TTYDEF_SPEED
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comcnmode = CONMODE;
#endif /* NCOM */
struct btinfo_console default_consinfo = {
	{0, 0},
	CONSDEVNAME,
#if (NCOM > 0)
	CONADDR, CONSPEED
#else
	0, 0
#endif
};
void	consinit __P((void));

#ifdef KGDB
#ifndef KGDB_DEVNAME
#define KGDB_DEVNAME "com"
#endif
char kgdb_devname[] = KGDB_DEVNAME;
#if (NCOM > 0)
#ifndef KGDBADDR
#define KGDBADDR 0x3f8
#endif
int comkgdbaddr = KGDBADDR;
#ifndef KGDBRATE
#define KGDBRATE TTYDEF_SPEED
#endif
int comkgdbrate = KGDBRATE;
#ifndef KGDBMODE
#define KGDBMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comkgdbmode = KGDBMODE;
#endif /* NCOM */
void kgdb_port_init __P((void));
#endif /* KGDB */

#ifdef COMPAT_NOMID
static int exec_nomid	__P((struct proc *, struct exec_package *));
#endif

int	i386_mem_add_mapping __P((bus_addr_t, bus_size_t,
	    int, bus_space_handle_t *));

int	_bus_dmamap_load_buffer __P((bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int, paddr_t *, int *, int));

void cyrix6x86_cpu_setup __P((void));

static __inline u_char
cyrix_read_reg(u_char reg)
{
	outb(0x22, reg);
	return inb(0x23);
}

static __inline void
cyrix_write_reg(u_char reg, u_char data)
{
	outb(0x22, reg);
	outb(0x23, data);
}

/*
 * Machine-dependent startup code
 */
void
cpu_startup()
{
	unsigned i;
	caddr_t v;
	int sz;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	struct pcb *pcb;
	int x;
#if NBIOSCALL > 0
	extern int biostramp_image_size;
	extern u_char biostramp_image[];
#endif

	/*
	 * Initialize error message buffer (et end of core).
	 */
#if defined(UVM) && defined(PMAP_NEW)
	msgbuf_vaddr =  uvm_km_valloc(kernel_map, i386_round_page(MSGBUFSIZE));
	if (msgbuf_vaddr == NULL)
		panic("failed to valloc msgbuf_vaddr");
#endif
	/* msgbuf_paddr was init'd in pmap */
#if defined(PMAP_NEW)
	for (x = 0; x < btoc(MSGBUFSIZE); x++)
		pmap_kenter_pa((vaddr_t)msgbuf_vaddr + x * NBPG,
		    msgbuf_paddr + x * NBPG, VM_PROT_ALL);
#else
	for (x = 0; x < btoc(MSGBUFSIZE); x++)
		pmap_enter(pmap_kernel(), (vaddr_t)msgbuf_vaddr + x * NBPG,
		    msgbuf_paddr + x * NBPG, VM_PROT_ALL, TRUE);
#endif
	initmsgbuf((caddr_t)msgbuf_vaddr, round_page(MSGBUFSIZE));

	printf(version);
	identifycpu();

	printf("real mem  = %d\n", ctob(physmem));

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	sz = (int)allocsys((caddr_t)0);
#if defined(UVM)
	if ((v = (caddr_t)uvm_km_zalloc(kernel_map, round_page(sz))) == 0)
		panic("startup: no room for tables");
#else
	if ((v = (caddr_t)kmem_alloc(kernel_map, round_page(sz))) == 0)
		panic("startup: no room for tables");
#endif
	if (allocsys(v) - v != sz)
		panic("startup: table size inconsistency");

	/*
	 * Allocate virtual address space for the buffers.  The area
	 * is not managed by the VM system.
	 */
	size = MAXBSIZE * nbuf;
#if defined(UVM)
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != KERN_SUCCESS)
		panic("cpu_startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
#else
	buffer_map = kmem_suballoc(kernel_map, (vaddr_t *)&buffers,
				   &maxaddr, size, TRUE);
	minaddr = (vaddr_t)buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(size), (vaddr_t)0,
			&minaddr, size, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");
#endif
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}

	/*
	 * XXX We defer allocation of physical pages for buffers until
	 * XXX after autoconfiguration has run.  We must do this because
	 * XXX on system with large amounts of memory or with large
	 * XXX user-configured buffer caches, the buffer cache will eat
	 * XXX up all of the lower 16M of RAM.  This prevents ISA DMA
	 * XXX maps from allocating bounce pages.
	 *
	 * XXX Note that nothing can use buffer cache buffers until after
	 * XXX autoconfiguration completes!!
	 *
	 * XXX This is a hack, and needs to be replaced with a better
	 * XXX solution!  --thorpej@netbsd.org, December 6, 1997
	 */

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
#if defined(UVM)
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   16*NCARGS, TRUE, FALSE, NULL);
#else
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, TRUE);
#endif

	/*
	 * Allocate a submap for physio
	 */
#if defined(UVM)
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, TRUE, FALSE, NULL);
#else
	phys_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, TRUE);
#endif

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
#if defined(UVM)
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_MBUF_SIZE, FALSE, FALSE, NULL);
#else
	mb_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_MBUF_SIZE, FALSE);
#endif

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	/*
	 * XXX Buffer cache pages haven't yet been allocated, so
	 * XXX we need to account for those pages when printing
	 * XXX the amount of free memory.
	 */
#if defined(UVM)
	printf("avail mem = %ld\n", ptoa(uvmexp.free - bufpages));
#else
	printf("avail mem = %ld\n", ptoa(cnt.v_free_count - bufpages));
#endif
	printf("using %d buffers containing %d bytes of memory\n",
		nbuf, bufpages * CLBYTES);

#if NBIOSCALL > 0
	/*
	 * this should be caught at kernel build time, but put it here
	 * in case someone tries to fake it out...
	 */
#ifdef DIAGNOSTIC
	if (biostramp_image_size > NBPG)
	    panic("biostramp_image_size too big: %x vs. %x\n",
		  biostramp_image_size, NBPG);
#endif
#if defined(PMAP_NEW)
	pmap_kenter_pa((vaddr_t)BIOSTRAMP_BASE,	/* virtual */
		       (paddr_t)BIOSTRAMP_BASE,	/* physical */
		       VM_PROT_ALL);		/* protection */
#else
	pmap_enter(pmap_kernel(),
		   (vaddr_t)BIOSTRAMP_BASE,	/* virtual */
		   (paddr_t)BIOSTRAMP_BASE,	/* physical */
		   VM_PROT_ALL,			/* protection */
		   TRUE);			/* wired down */
#endif
	memcpy((caddr_t)BIOSTRAMP_BASE, biostramp_image, biostramp_image_size);
#ifdef DEBUG
	printf("biostramp installed @ %x\n", BIOSTRAMP_BASE);
#endif
#endif

	/*
	 * Configure the system.
	 */
	ioport_malloc_safe = 1;
	configure();

	/*
	 * Set up proc0's TSS and LDT.
	 */
	gdt_init();
	curpcb = pcb = &proc0.p_addr->u_pcb;
	pcb->pcb_flags = 0;
	pcb->pcb_tss.tss_ioopt =
	    ((caddr_t)pcb->pcb_iomap - (caddr_t)&pcb->pcb_tss) << 16;
	for (x = 0; x < sizeof(pcb->pcb_iomap) / 4; x++)
		pcb->pcb_iomap[x] = 0xffffffff;

	pcb->pcb_ldt_sel = GSEL(GLDT_SEL, SEL_KPL);
	pcb->pcb_cr0 = rcr0();
	pcb->pcb_tss.tss_ss0 = GSEL(GDATA_SEL, SEL_KPL);
	pcb->pcb_tss.tss_esp0 = (int)proc0.p_addr + USPACE - 16;
	tss_alloc(pcb);

	ltr(pcb->pcb_tss_sel);
	lldt(pcb->pcb_ldt_sel);

	proc0.p_md.md_regs = (struct trapframe *)pcb->pcb_tss.tss_esp0 - 1;

}

/*
 * XXX Finish up the deferred buffer cache allocation and initialization.
 */
void
i386_bufinit()
{
	int i, base, residual;

	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
#if defined(UVM)
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
		curbufsize = CLBYTES * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			/*
			 * Attempt to allocate buffers from the first
			 * 16M of RAM to avoid bouncing file system
			 * transfers.
			 */
			pg = uvm_pagealloc_strat(NULL, 0, NULL,
			    UVM_PGA_STRAT_FALLBACK, VM_FREELIST_FIRST16);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
#if defined(PMAP_NEW)
			pmap_kenter_pgs(curbuf, &pg, 1);
#else
			pmap_enter(kernel_map->pmap, curbuf,
				   VM_PAGE_TO_PHYS(pg), VM_PROT_ALL, TRUE);
#endif
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
#else
		vsize_t curbufsize;
		vaddr_t curbuf;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */
		curbuf = (vaddr_t)buffers + i * MAXBSIZE;
		curbufsize = CLBYTES * (i < residual ? base+1 : base);
		vm_map_pageable(buffer_map, curbuf, curbuf+curbufsize, FALSE);
		vm_map_simplify(buffer_map, curbuf);
#endif
	}

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
}

/*
 * Allocate space for system data structures.  We are given
 * a starting virtual address and we return a final virtual
 * address; along the way we set each data structure pointer.
 *
 * We call allocsys() with 0 to find out how much space we want,
 * allocate that much and fill it with zeroes, and then call
 * allocsys() again with the correct base virtual address.
 */
caddr_t
allocsys(v)
	caddr_t v;
{

#define	valloc(name, type, num) \
	    v = (caddr_t)(((name) = (type *)v) + (num))
#ifdef REAL_CLISTS
	valloc(cfree, struct cblock, nclist);
#endif
	valloc(callout, struct callout, ncallout);
#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
#ifdef SYSVSEM
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(sem, struct sem, seminfo.semmns);
	/* This is pretty disgusting! */
	valloc(semu, int, (seminfo.semmnu * seminfo.semusz) / sizeof(int));
#endif
#ifdef SYSVMSG
	valloc(msgpool, char, msginfo.msgmax);
	valloc(msgmaps, struct msgmap, msginfo.msgseg);
	valloc(msghdrs, struct msg, msginfo.msgtql);
	valloc(msqids, struct msqid_ds, msginfo.msgmni);
#endif

	/*
	 * If necessary, determine the number of pages to use for the
	 * buffer cache.  We allocate 1/2 as many swap buffer headers
	 * as file I/O buffers.
	 */
	if (bufpages == 0) {
		if (bufcache == 0) {		/* use old algorithm */
			/*
			 * Determine how many buffers to allocate. We use 10%
			 * of the first 2MB of memory, and 5% of the rest, with
			 * a minimum of 16 buffers.
			 */
			if (physmem < btoc(2 * 1024 * 1024))
				bufpages = physmem / (10 * CLSIZE);
			else
				bufpages = (btoc(2 * 1024 * 1024) + physmem) /
				    (20 * CLSIZE);
		} else {
			/*
			 * Set size of buffer cache to physmem/bufcache * 100
			 * (i.e., bufcache % of physmem).
			 */
			if (bufcache < 5 || bufcache > 95) {
				printf(
		"warning: unable to set bufcache to %d%% of RAM, using 10%%",
				    bufcache);
				bufcache = 10;
			}
			bufpages= physmem / (CLSIZE * 100) * bufcache;
		}
	}
	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}

	/*
	 * XXX stopgap measure to prevent wasting too much KVM on
	 * the sparsely filled buffer cache.
	 */
	if (nbuf * MAXBSIZE > VM_MAX_KERNEL_BUF)
		nbuf = VM_MAX_KERNEL_BUF / MAXBSIZE;

	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}
#if !defined(UVM)
	valloc(swbuf, struct buf, nswbuf);
#endif
	valloc(buf, struct buf, nbuf);
	return v;
}

/*  
 * Info for CTL_HW
 */
char	cpu_model[120];
extern	char version[];

/*
 * Note: these are just the ones that may not have a cpuid instruction.
 * We deal with the rest in a different way.
 */
struct cpu_nocpuid_nameclass i386_nocpuid_cpus[] = {
	{ CPUVENDOR_INTEL, "Intel", "386SX",	CPUCLASS_386,
		NULL},				/* CPU_386SX */
	{ CPUVENDOR_INTEL, "Intel", "386DX",	CPUCLASS_386,
		NULL},				/* CPU_386   */
	{ CPUVENDOR_INTEL, "Intel", "486SX",	CPUCLASS_486,
		NULL},				/* CPU_486SX */
	{ CPUVENDOR_INTEL, "Intel", "486DX",	CPUCLASS_486,
		NULL},				/* CPU_486   */
	{ CPUVENDOR_CYRIX, "Cyrix", "486DLC",	CPUCLASS_486,
		NULL},				/* CPU_486DLC */
	{ CPUVENDOR_CYRIX, "Cyrix", "6x86",		CPUCLASS_486,
		cyrix6x86_cpu_setup},	/* CPU_6x86 */
	{ CPUVENDOR_NEXGEN,"NexGen","586",      CPUCLASS_386,
		NULL},				/* CPU_NX586 */
};

const char *classnames[] = {
	"386",
	"486",
	"586",
	"686"
};

const char *modifiers[] = {
	"",
	"OverDrive ",
	"Dual ",
	""
};

struct cpu_cpuid_nameclass i386_cpuid_cpus[] = {
	{
		"GenuineIntel",
		CPUVENDOR_INTEL,
		"Intel",
		/* Family 4 */
		{ {
			CPUCLASS_486, 
			{
				"486DX", "486DX", "486SX", "486DX2", "486SL",
				"486SX2", 0, "486DX2 W/B Enhanced",
				"486DX4", 0, 0, 0, 0, 0, 0, 0,
				"486"		/* Default */
			},
			NULL
		},
		/* Family 5 */
		{
			CPUCLASS_586,
			{
				0, "Pentium", "Pentium (P54C)",
				"Pentium (P24T)", "Pentium/MMX", "Pentium", 0,
				"Pentium (P54C)", 0, 0, 0, 0, 0, 0, 0, 0,
				"Pentium"	/* Default */
			},
			NULL
		},
		/* Family 6 */
		{
			CPUCLASS_686,
			{
				0, "Pentium Pro", 0, "Pentium II",
				"Pentium Pro", "Pentium II", 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0,
				"Pentium Pro"	/* Default */
			},
			NULL
		} }
	},
	{
		"AuthenticAMD",
		CPUVENDOR_AMD,
		"AMD",
		/* Family 4 */
		{ {
			CPUCLASS_486, 
			{
				0, 0, 0, "Am486DX2 W/T",
				0, 0, 0, "Am486DX2 W/B",
				"Am486DX4 W/T or Am5x86 W/T 150",
				"Am486DX4 W/B or Am5x86 W/B 150", 0, 0,
				0, 0, "Am5x86 W/T 133/160",
				"Am5x86 W/B 133/160",
				"Am486 or Am5x86"	/* Default */
			},
			NULL
		},
		/* Family 5 */
		{
			CPUCLASS_586,
			{
				"K5", "K5", "K5", "K5", 0, 0, "K6",
				"K6", "K6-3D", 0, 0, 0, 0, 0, 0, 0,
				"K5 or K6"		/* Default */
			},
			NULL
		},
		/* Family 6, not yet available from AMD */
		{
			CPUCLASS_686,
			{
				0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0,
				"Pentium Pro compatible"	/* Default */
			},
			NULL
		} }
	},
	{
		"CyrixInstead",
		CPUVENDOR_CYRIX,
		"Cyrix",
		/* Family 4 */
		{ {
			CPUCLASS_486,
			{
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				"486"		/* Default */
			},
			NULL
		},
		/* Family 5 */
		{
			CPUCLASS_586,
			{
				0, 0, "6x86", 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0,
				"6x86"		/* Default */
			},
			cyrix6x86_cpu_setup
		},
		/* Family 6 */
		{
			CPUCLASS_686,
			{
				"6x86MX", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				"6x86MX"		/* Default */
			},
			NULL
		} }
	},
	{
		"CentaurHauls",
		CPUVENDOR_IDT,
		"IDT",
		/* Family 4, IDT never had any of these */
		{ {
			CPUCLASS_486, 
			{
				0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0,
				"486 compatible"	/* Default */
			},
			NULL
		},
		/* Family 5 */
		{
			CPUCLASS_586,
			{
				0, 0, 0, 0, "WinChip C6", 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				"WinChip"		/* Default */
			},
			NULL
		},
		/* Family 6, not yet available from IDT */
		{
			CPUCLASS_686,
			{
				0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0,
				"Pentium Pro compatible"	/* Default */
			},
			NULL
		} }
	}
};

#define CPUDEBUG

void
cyrix6x86_cpu_setup()
{
	/* set up various cyrix registers */
	/* Enable suspend on halt */
	cyrix_write_reg(0xc2, cyrix_read_reg(0xc2) | 0x08);
	/* enable access to ccr4/ccr5 */
	cyrix_write_reg(0xC3, cyrix_read_reg(0xC3) | 0x10);
	/* cyrix's workaround  for the "coma bug" */
	cyrix_write_reg(0x31, cyrix_read_reg(0x31) | 0xf8);
	cyrix_write_reg(0x32, cyrix_read_reg(0x32) | 0x7f);
	cyrix_write_reg(0x33, cyrix_read_reg(0x33) & ~0xff);
	cyrix_write_reg(0x3c, cyrix_read_reg(0x3c) | 0x87);
	/* disable access to ccr4/ccr5 */
	cyrix_write_reg(0xC3, cyrix_read_reg(0xC3) & ~0x10);
}

void
identifycpu()
{
	extern char cpu_vendor[];
	extern int cpu_id;
	const char *name, *modifier, *vendorname;
	int class = CPUCLASS_386, vendor, i, max;
	int family, model, step, modif;
	struct cpu_cpuid_nameclass *cpup = NULL;
	void (*cpu_setup) __P((void));

	if (cpuid_level == -1) {
#ifdef DIAGNOSTIC
		if (cpu < 0 || cpu >=
		    (sizeof i386_nocpuid_cpus/sizeof(struct cpu_nocpuid_nameclass)))
			panic("unknown cpu type %d\n", cpu);
#endif
		name = i386_nocpuid_cpus[cpu].cpu_name;
		vendor = i386_nocpuid_cpus[cpu].cpu_vendor;
		vendorname = i386_nocpuid_cpus[cpu].cpu_vendorname;
		class = i386_nocpuid_cpus[cpu].cpu_class;
		cpu_setup = i386_nocpuid_cpus[cpu].cpu_setup;
		modifier = "";
	} else {
		max = sizeof (i386_cpuid_cpus) / sizeof (i386_cpuid_cpus[0]);
		modif = (cpu_id >> 12) & 3;
		family = (cpu_id >> 8) & 15;
		if (family < CPU_MINFAMILY)
			panic("identifycpu: strange family value");
		model = (cpu_id >> 4) & 15;
		step = cpu_id & 15;
#ifdef CPUDEBUG
		printf("cpu0: family %x model %x step %x\n", family, model,
			step);
#endif

		for (i = 0; i < max; i++) {
			if (!strncmp(cpu_vendor,
			    i386_cpuid_cpus[i].cpu_id, 12)) {
				cpup = &i386_cpuid_cpus[i];
				break;
			}
		}

		if (cpup == NULL) {
			vendor = CPUVENDOR_UNKNOWN;
			if (cpu_vendor[0] != '\0')
				vendorname = &cpu_vendor[0];
			else
				vendorname = "Unknown";
			if (family > CPU_MAXFAMILY)
				family = CPU_MAXFAMILY;
			class = family - 3;
			modifier = "";
			name = "";
			cpu_setup = NULL;
		} else {
			vendor = cpup->cpu_vendor;
			vendorname = cpup->cpu_vendorname;
			modifier = modifiers[modif];
			if (family > CPU_MAXFAMILY) {
				family = CPU_MAXFAMILY;
				model = CPU_DEFMODEL;
			} else if (model > CPU_MAXMODEL)
				model = CPU_DEFMODEL;
			i = family - CPU_MINFAMILY;
			name = cpup->cpu_family[i].cpu_models[model];
			if (name == NULL)
			    name = cpup->cpu_family[i].cpu_models[CPU_DEFMODEL];
			class = cpup->cpu_family[i].cpu_class;
			cpu_setup = cpup->cpu_family[i].cpu_setup;
		}
	}

	sprintf(cpu_model, "%s %s%s (%s-class)", vendorname, modifier, name,
		classnames[class]);
	printf("cpu0: %s\n", cpu_model);

	cpu_class = class;

	/*
	 * Now that we have told the user what they have,
	 * let them know if that machine type isn't configured.
	 */
	switch (cpu_class) {
#if !defined(I386_CPU) && !defined(I486_CPU) && !defined(I586_CPU) && !defined(I686_CPU)
#error No CPU classes configured.
#endif
#ifndef I686_CPU
	case CPUCLASS_686:
		printf("NOTICE: this kernel does not support Pentium Pro CPU class\n");
#ifdef I586_CPU
		printf("NOTICE: lowering CPU class to i586\n");
		cpu_class = CPUCLASS_586;
		break;
#endif
#endif
#ifndef I586_CPU
	case CPUCLASS_586:
		printf("NOTICE: this kernel does not support Pentium CPU class\n");
#ifdef I486_CPU
		printf("NOTICE: lowering CPU class to i486\n");
		cpu_class = CPUCLASS_486;
		break;
#endif
#endif
#ifndef I486_CPU
	case CPUCLASS_486:
		printf("NOTICE: this kernel does not support i486 CPU class\n");
#ifdef I386_CPU
		printf("NOTICE: lowering CPU class to i386\n");
		cpu_class = CPUCLASS_386;
		break;
#endif
#endif
#ifndef I386_CPU
	case CPUCLASS_386:
		printf("NOTICE: this kernel does not support i386 CPU class\n");
		panic("no appropriate CPU class available");
#endif
	default:
		break;
	}

	/* configure the CPU if needed */
	if (cpu_setup != NULL)
		cpu_setup();
	if (cpu == CPU_486DLC) {
#ifndef CYRIX_CACHE_WORKS
		printf("WARNING: CYRIX 486DLC CACHE UNCHANGED.\n");
#else
#ifndef CYRIX_CACHE_REALLY_WORKS
		printf("WARNING: CYRIX 486DLC CACHE ENABLED IN HOLD-FLUSH MODE.\n");
#else
		printf("WARNING: CYRIX 486DLC CACHE ENABLED.\n");
#endif
#endif
	}

#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	/*
	 * On a 486 or above, enable ring 0 write protection.
	 */
	if (cpu_class >= CPUCLASS_486)
		lcr0(rcr0() | CR0_WP);
#endif
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
	dev_t consdev;
	struct btinfo_bootpath *bibp;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof consdev));

	case CPU_BIOSBASEMEM:
		return (sysctl_rdint(oldp, oldlenp, newp, biosbasemem));

	case CPU_BIOSEXTMEM:
		return (sysctl_rdint(oldp, oldlenp, newp, biosextmem));

	case CPU_NKPDE:
		return (sysctl_rdint(oldp, oldlenp, newp, nkpde));

	case CPU_BOOTED_KERNEL:
	        bibp = lookup_bootinfo(BTINFO_BOOTPATH);
	        if(!bibp)
			return(ENOENT); /* ??? */
		return (sysctl_rdstring(oldp, oldlenp, newp, bibp->bootpath));

	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct proc *p = curproc;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	struct sigacts *psp = p->p_sigacts;
	int onstack;

	tf = p->p_md.md_regs;

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (psp->ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (psp->ps_sigact[sig].sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sigframe *)((caddr_t)psp->ps_sigstk.ss_sp +
						  psp->ps_sigstk.ss_size);
	else
		fp = (struct sigframe *)tf->tf_esp;
	fp--;

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = sig;
	frame.sf_code = code;
	frame.sf_scp = &fp->sf_sc;
	frame.sf_handler = catcher;

	/* Save register context. */
#ifdef VM86
	if (tf->tf_eflags & PSL_VM) {
		frame.sf_sc.sc_gs = tf->tf_vm86_gs;
		frame.sf_sc.sc_fs = tf->tf_vm86_fs;
		frame.sf_sc.sc_es = tf->tf_vm86_es;
		frame.sf_sc.sc_ds = tf->tf_vm86_ds;
		frame.sf_sc.sc_eflags = get_vflags(p);
	} else
#endif
	{
		__asm("movl %%gs,%w0" : "=r" (frame.sf_sc.sc_gs));
		__asm("movl %%fs,%w0" : "=r" (frame.sf_sc.sc_fs));
		frame.sf_sc.sc_es = tf->tf_es;
		frame.sf_sc.sc_ds = tf->tf_ds;
		frame.sf_sc.sc_eflags = tf->tf_eflags;
	}
	frame.sf_sc.sc_edi = tf->tf_edi;
	frame.sf_sc.sc_esi = tf->tf_esi;
	frame.sf_sc.sc_ebp = tf->tf_ebp;
	frame.sf_sc.sc_ebx = tf->tf_ebx;
	frame.sf_sc.sc_edx = tf->tf_edx;
	frame.sf_sc.sc_ecx = tf->tf_ecx;
	frame.sf_sc.sc_eax = tf->tf_eax;
	frame.sf_sc.sc_eip = tf->tf_eip;
	frame.sf_sc.sc_cs = tf->tf_cs;
	frame.sf_sc.sc_esp = tf->tf_esp;
	frame.sf_sc.sc_ss = tf->tf_ss;
	frame.sf_sc.sc_trapno = tf->tf_trapno;
	frame.sf_sc.sc_err = tf->tf_err;

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = psp->ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

#ifdef COMPAT_13
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(mask, &frame.sf_sc.__sc_mask13);
#endif

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(p, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	__asm("movl %w0,%%gs" : : "r" (GSEL(GUDATA_SEL, SEL_UPL)));
	__asm("movl %w0,%%fs" : : "r" (GSEL(GUDATA_SEL, SEL_UPL)));
	tf->tf_es = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_ds = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_eip = (int)psp->ps_sigcode;
	tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
	tf->tf_eflags &= ~(PSL_T|PSL_VM|PSL_AC);
	tf->tf_esp = (int)fp;
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
 */
int
sys___sigreturn14(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext *scp, context;
	struct trapframe *tf;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/* Restore register context. */
	tf = p->p_md.md_regs;
#ifdef VM86
	if (context.sc_eflags & PSL_VM) {
		tf->tf_vm86_gs = context.sc_gs;
		tf->tf_vm86_fs = context.sc_fs;
		tf->tf_vm86_es = context.sc_es;
		tf->tf_vm86_ds = context.sc_ds;
		set_vflags(p, context.sc_eflags);
	} else
#endif
	{
		/*
		 * Check for security violations.  If we're returning to
		 * protected mode, the CPU will validate the segment registers
		 * automatically and generate a trap on violations.  We handle
		 * the trap, rather than doing all of the checking here.
		 */
		if (((context.sc_eflags ^ tf->tf_eflags) & PSL_USERSTATIC) != 0 ||
		    !USERMODE(context.sc_cs, context.sc_eflags))
			return (EINVAL);

		/* %fs and %gs were restored by the trampoline. */
		tf->tf_es = context.sc_es;
		tf->tf_ds = context.sc_ds;
		tf->tf_eflags = context.sc_eflags;
	}
	tf->tf_edi = context.sc_edi;
	tf->tf_esi = context.sc_esi;
	tf->tf_ebp = context.sc_ebp;
	tf->tf_ebx = context.sc_ebx;
	tf->tf_edx = context.sc_edx;
	tf->tf_ecx = context.sc_ecx;
	tf->tf_eax = context.sc_eax;
	tf->tf_eip = context.sc_eip;
	tf->tf_cs = context.sc_cs;
	tf->tf_esp = context.sc_esp;
	tf->tf_ss = context.sc_ss;

	/* Restore signal stack. */
	if (context.sc_onstack & SS_ONSTACK)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &context.sc_mask, 0);

	return (EJUSTRETURN);
}

int	waittime = -1;
struct pcb dumppcb;

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
	extern int cold;

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* Disable interrupts. */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
#if NAPM > 0 && !defined(APM_NO_POWEROFF)
		/* turn off, if we can.  But try to turn disk off and
		 * wait a bit first--some disk drives are slow to clean up
		 * and users have reported disk corruption.
		 */
		delay(500000);
		apm_set_powstate(APM_DEV_DISK(0xff), APM_SYS_OFF);
		delay(500000);
		apm_set_powstate(APM_DEV_ALLDEVS, APM_SYS_OFF);
		printf("WARNING: powerdown failed!\n");
		/*
		 * RB_POWERDOWN implies RB_HALT... fall into it...
		 */
#endif
	}

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* for proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("rebooting...\n");
	if (cpureset_delay > 0)
		delay(cpureset_delay * 1000);
	cpu_reset();
	for(;;) ;
	/*NOTREACHED*/
}

/*
 * These variables are needed by /sbin/savecore
 */
u_long	dumpmag = 0x8fca0101;	/* magic number */
int 	dumpsize = 0;		/* pages */
long	dumplo = 0; 		/* blocks */

/*
 * cpu_dumpsize: calculate size of machine-dependent kernel core dump headers.
 */
int
cpu_dumpsize()
{
	int size;

	size = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)) +
	    ALIGN(mem_cluster_cnt * sizeof(phys_ram_seg_t));
	if (roundup(size, dbtob(1)) != dbtob(1))
		return (-1);

	return (1);
}

/*
 * cpu_dump_mempagecnt: calculate the size of RAM (in pages) to be dumped.
 */
u_long
cpu_dump_mempagecnt()
{
	u_long i, n;

	n = 0;
	for (i = 0; i < mem_cluster_cnt; i++)
		n += atop(mem_clusters[i].size);
	return (n);
}

/*
 * cpu_dump: dump the machine-dependent kernel core dump headers.
 */
int
cpu_dump()
{
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	char buf[dbtob(1)];
	kcore_seg_t *segp;
	cpu_kcore_hdr_t *cpuhdrp;
	phys_ram_seg_t *memsegp;
	int i;

	dump = bdevsw[major(dumpdev)].d_dump;

	memset(buf, 0, sizeof buf);
	segp = (kcore_seg_t *)buf;
	cpuhdrp = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(*segp))];
	memsegp = (phys_ram_seg_t *)&buf[ ALIGN(sizeof(*segp)) +
	    ALIGN(sizeof(*cpuhdrp))];

	/*
	 * Generate a segment header.
	 */
	CORE_SETMAGIC(*segp, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	segp->c_size = dbtob(1) - ALIGN(sizeof(*segp));

	/*
	 * Add the machine-dependent header info.
	 */
	cpuhdrp->ptdpaddr = PTDpaddr;
	cpuhdrp->nmemsegs = mem_cluster_cnt;

	/*
	 * Fill in the memory segment descriptors.
	 */
	for (i = 0; i < mem_cluster_cnt; i++) {
		memsegp[i].start = mem_clusters[i].start;
		memsegp[i].size = mem_clusters[i].size;
	}

	return (dump(dumpdev, dumplo, (caddr_t)buf, dbtob(1)));
}

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf()
{
	int nblks, dumpblks;	/* size of dump area */
	int maj;

	if (dumpdev == NODEV)
		goto bad;
	maj = major(dumpdev);
	if (maj < 0 || maj >= nblkdev)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	if (bdevsw[maj].d_psize == NULL)
		goto bad;
	nblks = (*bdevsw[maj].d_psize)(dumpdev);
	if (nblks <= ctod(1))
		goto bad;

	dumpblks = cpu_dumpsize();
	if (dumpblks < 0)
		goto bad;
	dumpblks += ctod(cpu_dump_mempagecnt());

	/* If dump won't fit (incl. room for possible label), punt. */
	if (dumpblks > (nblks - ctod(1)))
		goto bad;

	/* Put dump at end of partition */
	dumplo = nblks - dumpblks;

	/* dumpsize is in page units, and doesn't include headers. */
	dumpsize = cpu_dump_mempagecnt();
	return;

 bad:
	dumpsize = 0;
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
#define BYTES_PER_DUMP  NBPG	/* must be a multiple of pagesize XXX small */
static vaddr_t dumpspace;

vaddr_t
reserve_dumppages(p)
	vaddr_t p;
{

	dumpspace = p;
	return (p + BYTES_PER_DUMP);
}

void
dumpsys()
{
	u_long totalbytesleft, bytes, i, n, memseg;
	u_long maddr;
	int psize;
	daddr_t blkno;
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	int error;

	/* Save registers. */
	savectx(&dumppcb);

	msgbufenabled = 0;	/* don't record dump msgs in msgbuf */
	if (dumpdev == NODEV)
		return;

	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		cpu_dumpconf();
	if (dumplo <= 0 || dumpsize == 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	psize = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}

#if 0	/* XXX this doesn't work.  grr. */
        /* toss any characters present prior to dump */
	while (sget() != NULL); /*syscons and pccons differ */
#endif

	if ((error = cpu_dump()) != 0)
		goto err;

	totalbytesleft = ptoa(cpu_dump_mempagecnt());
	blkno = dumplo + cpu_dumpsize();
	dump = bdevsw[major(dumpdev)].d_dump;
	error = 0;

	for (memseg = 0; memseg < mem_cluster_cnt; memseg++) {
		maddr = mem_clusters[memseg].start;
		bytes = mem_clusters[memseg].size;

		for (i = 0; i < bytes; i += n, totalbytesleft -= n) {
			/* Print out how many MBs we have left to go. */
			if ((totalbytesleft % (1024*1024)) == 0)
				printf("%ld ", totalbytesleft / (1024 * 1024));

			/* Limit size for next transfer. */
			n = bytes - i;
			if (n > BYTES_PER_DUMP)
				n = BYTES_PER_DUMP;

			(void) pmap_map(dumpspace, maddr, maddr + n,
			    VM_PROT_READ);

			error = (*dump)(dumpdev, blkno, (caddr_t)dumpspace, n);
			if (error)
				goto err;
			maddr += n;
			blkno += btodb(n);		/* XXX? */

#if 0	/* XXX this doesn't work.  grr. */
			/* operator aborting dump? */
			if (sget() != NULL) {
				error = EINTR;
				break;
			}
#endif
		}
	}

 err:
	switch (error) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	case EINTR:
		printf("aborted from console\n");
		break;

	case 0:
		printf("succeeded\n");
		break;

	default:
		printf("error %d\n", error);
		break;
	}
	printf("\n\n");
	delay(5000000);		/* 5 seconds */
}

/*
 * Clear registers on exec
 */
void
setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	struct pcb *pcb = &p->p_addr->u_pcb;
	struct trapframe *tf;

#if NNPX > 0
	/* If we were using the FPU, forget about it. */
	if (npxproc == p)
		npxdrop();
#endif

#ifdef USER_LDT
	if (pcb->pcb_flags & PCB_USER_LDT)
		i386_user_cleanup(pcb);
#endif

	p->p_md.md_flags &= ~MDP_USEDFPU;
	pcb->pcb_flags = 0;
	pcb->pcb_savefpu.sv_env.en_cw = __NetBSD_NPXCW__;

	tf = p->p_md.md_regs;
	__asm("movl %w0,%%gs" : : "r" (LSEL(LUDATA_SEL, SEL_UPL)));
	__asm("movl %w0,%%fs" : : "r" (LSEL(LUDATA_SEL, SEL_UPL)));
	tf->tf_es = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_ds = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_edi = 0;
	tf->tf_esi = 0;
	tf->tf_ebp = 0;
	tf->tf_ebx = (int)PS_STRINGS;
	tf->tf_edx = 0;
	tf->tf_ecx = 0;
	tf->tf_eax = 0;
	tf->tf_eip = pack->ep_entry;
	tf->tf_cs = LSEL(LUCODE_SEL, SEL_UPL);
	tf->tf_eflags = PSL_USERSET;
	tf->tf_esp = stack;
	tf->tf_ss = LSEL(LUDATA_SEL, SEL_UPL);
}

/*
 * Initialize segments and descriptor tables
 */

union	descriptor *idt, *gdt, *ldt;
#ifdef I586_CPU
union	descriptor *pentium_idt;
#endif
extern  struct user *proc0paddr;

void
setgate(gd, func, args, type, dpl)
	struct gate_descriptor *gd;
	void *func;
	int args, type, dpl;
{

	gd->gd_looffset = (int)func;
	gd->gd_selector = GSEL(GCODE_SEL, SEL_KPL);
	gd->gd_stkcpy = args;
	gd->gd_xx = 0;
	gd->gd_type = type;
	gd->gd_dpl = dpl;
	gd->gd_p = 1;
	gd->gd_hioffset = (int)func >> 16;
}

void
setregion(rd, base, limit)
	struct region_descriptor *rd;
	void *base;
	size_t limit;
{

	rd->rd_limit = (int)limit;
	rd->rd_base = (int)base;
}

void
setsegment(sd, base, limit, type, dpl, def32, gran)
	struct segment_descriptor *sd;
	void *base;
	size_t limit;
	int type, dpl, def32, gran;
{

	sd->sd_lolimit = (int)limit;
	sd->sd_lobase = (int)base;
	sd->sd_type = type;
	sd->sd_dpl = dpl;
	sd->sd_p = 1;
	sd->sd_hilimit = (int)limit >> 16;
	sd->sd_xx = 0;
	sd->sd_def32 = def32;
	sd->sd_gran = gran;
	sd->sd_hibase = (int)base >> 24;
}

#define	IDTVEC(name)	__CONCAT(X, name)
typedef void (vector) __P((void));
extern vector IDTVEC(syscall);
extern vector IDTVEC(osyscall);
extern vector *IDTVEC(exceptions)[];
#ifdef COMPAT_SVR4
extern vector IDTVEC(svr4_fasttrap);
#endif /* COMPAT_SVR4 */

void
init386(first_avail)
	vaddr_t first_avail;
{
	int x;
	struct region_descriptor region;
	extern void consinit __P((void));

	proc0.p_addr = proc0paddr;
#if defined(PMAP_NEW)
	/* XXX: PMAP_NEW requires valid curpcb.   also init'd in cpu_startup */
	curpcb = &proc0.p_addr->u_pcb;
#endif


	/*
	 * Initialize the I/O port and I/O mem extent maps.
	 * Note: we don't have to check the return value since
	 * creation of a fixed extent map will never fail (since
	 * descriptor storage has already been allocated).
	 *
	 * N.B. The iomem extent manages _all_ physical addresses
	 * on the machine.  When the amount of RAM is found, the two
	 * extents of RAM are allocated from the map (0 -> ISA hole
	 * and end of ISA hole -> end of RAM).
	 */
	ioport_ex = extent_create("ioport", 0x0, 0xffff, M_DEVBUF,
	    (caddr_t)ioport_ex_storage, sizeof(ioport_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);
	iomem_ex = extent_create("iomem", 0x0, 0xffffffff, M_DEVBUF,
	    (caddr_t)iomem_ex_storage, sizeof(iomem_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);

	consinit();	/* XXX SHOULD NOT BE DONE HERE */

	/*
	 * Allocate the physical addresses used by RAM from the iomem
	 * extent map.  This is done before the addresses are
	 * page rounded just to make sure we get them all.
	 */
	if (extent_alloc_region(iomem_ex, 0, biosbasemem * 1024, EX_NOWAIT)) {
		/* XXX What should we do? */
		printf("WARNING: CAN'T ALLOCATE BASE MEMORY FROM IOMEM EXTENT MAP!\n");
	}
	if (extent_alloc_region(iomem_ex, IOM_END, biosextmem * 1024,
	    EX_NOWAIT)) {
		/* XXX What should we do? */
		printf("WARNING: CAN'T ALLOCATE EXTENDED MEMORY FROM IOMEM EXTENT MAP!\n");
	}

#if NISADMA > 0
	/*
	 * Some motherboards/BIOSes remap the 384K of RAM that would
	 * normally be covered by the ISA hole to the end of memory
	 * so that it can be used.  However, on a 16M system, this
	 * would cause bounce buffers to be allocated and used.
	 * This is not desirable behaviour, as more than 384K of
	 * bounce buffers might be allocated.  As a work-around,
	 * we round memory down to the nearest 1M boundary if
	 * we're using any isadma devices and the remapped memory
	 * is what puts us over 16M.
	 */
	if (biosextmem > (15*1024) && biosextmem < (16*1024)) {
		printf("Warning: ignoring %dk of remapped memory\n",
		    biosextmem - (15*1024));
		biosextmem = (15*1024);
	}
#endif

#if NBIOSCALL > 0
	avail_start = 3*NBPG;	/* save us a page for trampoline code and
				 one additional PT page! */
#else
	avail_start = NBPG;	/* BIOS leaves data in low memory */
				/* and VM system doesn't work with phys 0 */
#endif
	avail_end = IOM_END + trunc_page(biosextmem * 1024);

	hole_start = trunc_page(biosbasemem * 1024);
	/* we load right after the I/O hole; adjust hole_end to compensate */
	hole_end = round_page(first_avail);

	/* Call pmap initialization to make new kernel address space. */
	pmap_bootstrap((vaddr_t)atdevbase + IOM_SIZE);

#if NBIOSCALL > 0
	/* install page 2 (reserved above) as PT page for first 4M */
	pmap_enter(pmap_kernel(), (u_long)vtopte(0), 2*NBPG, VM_PROT_ALL, TRUE);
	memset(vtopte(0), 0, NBPG);  /* make sure it is clean before using */
#endif

	pmap_enter(pmap_kernel(), idt_vaddr, idt_paddr, VM_PROT_ALL, TRUE);
	idt = (union descriptor *)idt_vaddr;
#ifdef I586_CPU
	pmap_enter(pmap_kernel(), pentium_idt_vaddr, idt_paddr, VM_PROT_READ,
	    TRUE);
	pentium_idt = (union descriptor *)pentium_idt_vaddr;
#endif
	gdt = idt + NIDT;
	ldt = gdt + NGDT;


	/* make gdt gates and memory segments */
	setsegment(&gdt[GCODE_SEL].sd, 0, 0xfffff, SDT_MEMERA, SEL_KPL, 1, 1);
	setsegment(&gdt[GDATA_SEL].sd, 0, 0xfffff, SDT_MEMRWA, SEL_KPL, 1, 1);
	setsegment(&gdt[GLDT_SEL].sd, ldt, NLDT * sizeof(ldt[0]) - 1,
	    SDT_SYSLDT, SEL_KPL, 0, 0);
	setsegment(&gdt[GUCODE_SEL].sd, 0, i386_btop(VM_MAXUSER_ADDRESS) - 1,
	    SDT_MEMERA, SEL_UPL, 1, 1);
	setsegment(&gdt[GUDATA_SEL].sd, 0, i386_btop(VM_MAXUSER_ADDRESS) - 1,
	    SDT_MEMRWA, SEL_UPL, 1, 1);
#if NBIOSCALL > 0
	/* bios trampoline GDT entries */
	setsegment(&gdt[GBIOSCODE_SEL].sd, 0, 0xfffff, SDT_MEMERA, SEL_KPL, 0,
	    0);
	setsegment(&gdt[GBIOSDATA_SEL].sd, 0, 0xfffff, SDT_MEMRWA, SEL_KPL, 0,
	    0);
#endif

	/* make ldt gates and memory segments */
	setgate(&ldt[LSYS5CALLS_SEL].gd, &IDTVEC(osyscall), 1,
	    SDT_SYS386CGT, SEL_UPL);
	ldt[LUCODE_SEL] = gdt[GUCODE_SEL];
	ldt[LUDATA_SEL] = gdt[GUDATA_SEL];
	ldt[LSOL26CALLS_SEL] = ldt[LBSDICALLS_SEL] = ldt[LSYS5CALLS_SEL];

	/* exceptions */
	for (x = 0; x < 32; x++)
		setgate(&idt[x].gd, IDTVEC(exceptions)[x], 0, SDT_SYS386TGT,
		    (x == 3 || x == 4) ? SEL_UPL : SEL_KPL);

	/* new-style interrupt gate for syscalls */
	setgate(&idt[128].gd, &IDTVEC(syscall), 0, SDT_SYS386TGT, SEL_UPL);
#ifdef COMPAT_SVR4
	setgate(&idt[0xd2].gd, &IDTVEC(svr4_fasttrap), 0, SDT_SYS386TGT,
	    SEL_UPL);
#endif /* COMPAT_SVR4 */

	setregion(&region, gdt, NGDT * sizeof(gdt[0]) - 1);
	lgdt(&region);
#ifdef I586_CPU
	setregion(&region, pentium_idt, NIDT * sizeof(idt[0]) - 1);
#else
	setregion(&region, idt, NIDT * sizeof(idt[0]) - 1);
#endif
	lidt(&region);


#ifdef DDB
	{
		extern int end;
		extern int *esym;
		struct btinfo_symtab *symtab;

		symtab = lookup_bootinfo(BTINFO_SYMTAB);
		if (symtab) {
			symtab->ssym += KERNBASE;
			symtab->esym += KERNBASE;
			ddb_init(symtab->nsym, (int *)symtab->ssym,
			    (int *)symtab->esym);
		}
		else
			ddb_init(*(int *)&end, ((int *)&end) + 1, esym);
	}
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef KGDB
	kgdb_port_init();
	if (boothowto & RB_KDB) {
		kgdb_debug_init = 1;
		kgdb_connect(1);
	}
#endif

#if NISA > 0
	isa_defaultirq();
#endif

	splraise(-1);
	enable_intr();

	/* number of pages of physmem addr space */
	physmem = btoc(biosbasemem * 1024) + btoc(biosextmem * 1024);

	mem_clusters[0].start = 0;
	mem_clusters[0].size  = trunc_page(biosbasemem * 1024);

	mem_clusters[1].start = IOM_END;
	mem_clusters[1].size  = trunc_page(biosextmem * 1024);

	mem_cluster_cnt = 2;

	if (physmem < btoc(2 * 1024 * 1024)) {
		printf("warning: too little memory available; "
		       "have %d bytes, want %d bytes\n"
		       "running in degraded mode\n"
		       "press a key to confirm\n\n",
		       ctob(physmem), 2*1024*1024);
		cngetc();
	}
}

struct queue {
	struct queue *q_next, *q_prev;
};

/*
 * insert an element into a queue
 */
void
_insque(v1, v2)
	void *v1;
	void *v2;
{
	struct queue *elem = v1, *head = v2;
	struct queue *next;

	next = head->q_next;
	elem->q_next = next;
	head->q_next = elem;
	elem->q_prev = head;
	next->q_prev = elem;
}

/*
 * remove an element from a queue
 */
void
_remque(v)
	void *v;
{
	struct queue *elem = v;
	struct queue *next, *prev;

	next = elem->q_next;
	prev = elem->q_prev;
	next->q_prev = prev;
	prev->q_next = next;
	elem->q_prev = 0;
}

#ifdef COMPAT_NOMID
static int
exec_nomid(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	int error;
	u_long midmag, magic;
	u_short mid;
	struct exec *execp = epp->ep_hdr;

	/* check on validity of epp->ep_hdr performed by exec_out_makecmds */

	midmag = ntohl(execp->a_midmag);
	mid = (midmag >> 16) & 0xffff;
	magic = midmag & 0xffff;

	if (magic == 0) {
		magic = (execp->a_midmag & 0xffff);
		mid = MID_ZERO;
	}

	midmag = mid << 16 | magic;

	switch (midmag) {
	case (MID_ZERO << 16) | ZMAGIC:
		/*
		 * 386BSD's ZMAGIC format:
		 */
		error = exec_aout_prep_oldzmagic(p, epp);
		break;

	case (MID_ZERO << 16) | QMAGIC:
		/*
		 * BSDI's QMAGIC format:
		 * same as new ZMAGIC format, but with different magic number
		 */
		error = exec_aout_prep_zmagic(p, epp);
		break;

	case (MID_ZERO << 16) | NMAGIC:
		/*
		 * BSDI's NMAGIC format:
		 * same as NMAGIC format, but with different magic number
		 * and with text starting at 0.
		 */
		error = exec_aout_prep_oldnmagic(p, epp);
		break;

	case (MID_ZERO << 16) | OMAGIC:
		/*
		 * BSDI's OMAGIC format:
		 * same as OMAGIC format, but with different magic number
		 * and with text starting at 0.
		 */
		error = exec_aout_prep_oldomagic(p, epp);
		break;

	default:
		error = ENOEXEC;
	}

	return error;
}
#endif

/*
 * cpu_exec_aout_makecmds():
 *	cpu-dependent a.out format hook for execve().
 *
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 *
 * On the i386, old (386bsd) ZMAGIC binaries and BSDI QMAGIC binaries
 * if COMPAT_NOMID is given as a kernel option.
 */
int
cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	int error = ENOEXEC;

#ifdef COMPAT_NOMID
	if ((error = exec_nomid(p, epp)) == 0)
		return error;
#endif /* ! COMPAT_NOMID */

	return error;
}

void *
lookup_bootinfo(type)
int type;
{
	struct btinfo_common *help;
	int n = *(int*)bootinfo;
	help = (struct btinfo_common *)(bootinfo + sizeof(int));
	while(n--) {
		if(help->type == type)
			return(help);
		help = (struct btinfo_common *)((char*)help + help->len);
	}
	return(0);
}

/*
 * consinit:
 * initialize the system console.
 * XXX - shouldn't deal with this initted thing, but then,
 * it shouldn't be called from init386 either.
 */
void
consinit()
{
	struct btinfo_console *consinfo;
	static int initted;

	if (initted)
		return;
	initted = 1;

#ifndef CONS_OVERRIDE
	consinfo = lookup_bootinfo(BTINFO_CONSOLE);
	if (!consinfo)
#endif
		consinfo = &default_consinfo;

#if (NPC > 0) || (NVT > 0) || (NVGA > 0) || (NPCDISPLAY > 0)
	if (!strcmp(consinfo->devname, "pc")) {
#if (NVGA > 0)
		if (!vga_cnattach(I386_BUS_SPACE_IO, I386_BUS_SPACE_MEM,
				  -1, 1))
			goto dokbd;
#endif
#if (NPCDISPLAY > 0)
		if (!pcdisplay_cnattach(I386_BUS_SPACE_IO, I386_BUS_SPACE_MEM))
			goto dokbd;
#endif
#if (NPC > 0) || (NVT > 0)
		pccnattach();
#endif
		if (0) goto dokbd; /* XXX stupid gcc */
dokbd:
#if (NPCKBC > 0)
		pckbc_cnattach(I386_BUS_SPACE_IO, PCKBC_KBD_SLOT);
#endif
		return;
	}
#endif /* PC | VT | VGA | PCDISPLAY */
#if (NCOM > 0)
	if (!strcmp(consinfo->devname, "com")) {
		bus_space_tag_t tag = I386_BUS_SPACE_IO;

		if (comcnattach(tag, consinfo->addr, consinfo->speed,
				COM_FREQ, comcnmode))
			panic("can't init serial console @%x", consinfo->addr);

		return;
	}
#endif
	panic("invalid console device %s", consinfo->devname);
}

#if (NPCKBC > 0) && (NPCKBD == 0)
/*
 * glue code to support old console code with the
 * mi keyboard controller driver
 */
int
pckbc_machdep_cnattach(kbctag, kbcslot)
	pckbc_tag_t kbctag;
	pckbc_slot_t kbcslot;
{
#if (NPC > 0) && (NPCCONSKBD > 0)
	return (pcconskbd_cnattach(kbctag, kbcslot));
#else
	return (ENXIO);
#endif
}
#endif

#ifdef KGDB
void
kgdb_port_init()
{
#if (NCOM > 0)
	if(!strcmp(kgdb_devname, "com")) {
		bus_space_tag_t tag = I386_BUS_SPACE_IO;

		com_kgdb_attach(tag, comkgdbaddr, comkgdbrate, COM_FREQ, 
		    comkgdbmode);
	}
#endif
}
#endif

void
cpu_reset()
{

	disable_intr();

	/*
	 * The keyboard controller has 4 random output pins, one of which is
	 * connected to the RESET pin on the CPU in many PCs.  We tell the
	 * keyboard controller to pulse this line a couple of times.
	 */
	outb(IO_KBD + KBCMDP, KBC_PULSE0);
	delay(100000);
	outb(IO_KBD + KBCMDP, KBC_PULSE0);
	delay(100000);

	/*
	 * Try to cause a triple fault and watchdog reset by making the IDT
	 * invalid and causing a fault.
	 */
	memset((caddr_t)idt, 0, NIDT * sizeof(idt[0]));
	__asm __volatile("divl %0,%1" : : "q" (0), "a" (0)); 

#if 0
	/*
	 * Try to cause a triple fault and watchdog reset by unmapping the
	 * entire address space and doing a TLB flush.
	 */
	memset((caddr_t)PTD, 0, NBPG);
	pmap_update(); 
#endif

	for (;;);
}

int
i386_memio_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	int error;
	struct extent *ex;

	/*
	 * Pick the appropriate extent map.
	 */
	if (t == I386_BUS_SPACE_IO) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		ex = ioport_ex;
	} else if (t == I386_BUS_SPACE_MEM)
		ex = iomem_ex;
	else
		panic("i386_memio_map: bad bus space tag");

	/*
	 * Before we go any further, let's make sure that this
	 * region is available.
	 */
	error = extent_alloc_region(ex, bpa, size,
	    EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0));
	if (error)
		return (error);

	/*
	 * For I/O space, that's all she wrote.
	 */
	if (t == I386_BUS_SPACE_IO) {
		*bshp = bpa;
		return (0);
	}

	if (bpa >= IOM_BEGIN && (bpa + size) <= IOM_END) {
		*bshp = (bus_space_handle_t)ISA_HOLE_VADDR(bpa);
		return(0);
	}

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	error = i386_mem_add_mapping(bpa, size,
		(flags & BUS_SPACE_MAP_CACHEABLE) != 0, bshp);
	if (error) {
		if (extent_free(ex, bpa, size, EX_NOWAIT |
		    (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
			printf("i386_memio_map: pa 0x%lx, size 0x%lx\n",
			    bpa, size);
			printf("i386_memio_map: can't free region\n");
		}
	}

	return (error);
}

int
_i386_memio_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{

	/*
	 * For I/O space, just fill in the handle.
	 */
	if (t == I386_BUS_SPACE_IO) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		*bshp = bpa;
		return (0);
	}

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	return (i386_mem_add_mapping(bpa, size,
	    (flags & BUS_SPACE_MAP_CACHEABLE) != 0, bshp));
}

int
i386_memio_alloc(t, rstart, rend, size, alignment, boundary, flags,
    bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	struct extent *ex;
	u_long bpa;
	int error;

	/*
	 * Pick the appropriate extent map.
	 */
	if (t == I386_BUS_SPACE_IO) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		ex = ioport_ex;
	} else if (t == I386_BUS_SPACE_MEM)
		ex = iomem_ex;
	else
		panic("i386_memio_alloc: bad bus space tag");

	/*
	 * Sanity check the allocation against the extent's boundaries.
	 */
	if (rstart < ex->ex_start || rend > ex->ex_end)
		panic("i386_memio_alloc: bad region start/end");

	/*
	 * Do the requested allocation.
	 */
	error = extent_alloc_subregion(ex, rstart, rend, size, alignment,
	    boundary,
	    EX_FAST | EX_NOWAIT | (ioport_malloc_safe ?  EX_MALLOCOK : 0),
	    &bpa);

	if (error)
		return (error);

	/*
	 * For I/O space, that's all she wrote.
	 */
	if (t == I386_BUS_SPACE_IO) {
		*bshp = *bpap = bpa;
		return (0);
	}

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	error = i386_mem_add_mapping(bpa, size,
	    (flags & BUS_SPACE_MAP_CACHEABLE) != 0, bshp);
	if (error) {
		if (extent_free(iomem_ex, bpa, size, EX_NOWAIT |
		    (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
			printf("i386_memio_alloc: pa 0x%lx, size 0x%lx\n",
			    bpa, size);
			printf("i386_memio_alloc: can't free region\n");
		}
	}

	*bpap = bpa;

	return (error);
}

int
i386_mem_add_mapping(bpa, size, cacheable, bshp)
	bus_addr_t bpa;
	bus_size_t size;
	int cacheable;
	bus_space_handle_t *bshp;
{
	u_long pa, endpa;
	vaddr_t va;
	pt_entry_t *pte;

	pa = i386_trunc_page(bpa);
	endpa = i386_round_page(bpa + size);

#ifdef DIAGNOSTIC
	if (endpa <= pa)
		panic("i386_mem_add_mapping: overflow");
#endif

#if defined(UVM)
	va = uvm_km_valloc(kernel_map, endpa - pa);
#else
	va = kmem_alloc_pageable(kernel_map, endpa - pa);
#endif
	if (va == 0)
		return (ENOMEM);

	*bshp = (bus_space_handle_t)(va + (bpa & PGOFSET));

	for (; pa < endpa; pa += NBPG, va += NBPG) {
		pmap_enter(pmap_kernel(), va, pa,
		    VM_PROT_READ | VM_PROT_WRITE, TRUE);

		/*
		 * PG_N doesn't exist on 386's, so we assume that
		 * the mainboard has wired up device space non-cacheable
		 * on those machines.
		 */
		if (cpu_class != CPUCLASS_386) {
			pte = kvtopte(va);
			if (cacheable)
				*pte &= ~PG_N;
			else
				*pte |= PG_N;
#if defined(PMAP_NEW)
			pmap_update_pg(va);
#else
			pmap_update();
#endif
		}
	}
 
	return 0;
}

void
i386_memio_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	struct extent *ex;
	u_long va, endva;
	bus_addr_t bpa;

	/*
	 * Find the correct extent and bus physical address.
	 */
	if (t == I386_BUS_SPACE_IO) {
		ex = ioport_ex;
		bpa = bsh;
	} else if (t == I386_BUS_SPACE_MEM) {
		ex = iomem_ex;

		if (bsh >= atdevbase &&
		    (bsh + size) <= (atdevbase + IOM_SIZE)) {
			bpa = (bus_addr_t)ISA_PHYSADDR(bsh);
			goto ok;
		}

		va = i386_trunc_page(bsh);
		endva = i386_round_page(bsh + size);

#ifdef DIAGNOSTIC
		if (endva <= va)
			panic("i386_memio_unmap: overflow");
#endif

		bpa = pmap_extract(pmap_kernel(), va) + (bsh & PGOFSET);

		/*
		 * Free the kernel virtual mapping.
		 */
#if defined(UVM)
		uvm_km_free(kernel_map, va, endva - va);
#else
		kmem_free(kernel_map, va, endva - va);
#endif
	} else
		panic("i386_memio_unmap: bad bus space tag");

ok:
	if (extent_free(ex, bpa, size,
	    EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
		printf("i386_memio_unmap: %s 0x%lx, size 0x%lx\n",
		    (t == I386_BUS_SPACE_IO) ? "port" : "pa", bpa, size);
		printf("i386_memio_unmap: can't free region\n");
	}
}

void    
i386_memio_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{

	/* i386_memio_unmap() does all that we need to do. */
	i386_memio_unmap(t, bsh, size);
}

int
i386_memio_subregion(t, bsh, offset, size, nbshp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + offset;
	return (0);
}

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct i386_bus_dmamap *map;
	void *mapstore;
	size_t mapsize;

	/*
	 * Allocate and initialize the DMA map.  The end of the map
	 * is a variable-sized array of segments, so we allocate enough
	 * room for them in one shot.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifies others that we've reserved these resources,
	 * and they are not to be freed.
	 *
	 * The bus_dmamap_t includes one bus_dma_segment_t, hence
	 * the (nsegments - 1).
	 */
	mapsize = sizeof(struct i386_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	memset(mapstore, 0, mapsize);
	map = (struct i386_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_bounce_thresh = t->_bounce_thresh;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->dm_mapsize = 0;		/* no valid mappings */
	map->dm_nsegs = 0;

	*dmamp = map;
	return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
_bus_dmamap_destroy(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{

	free(map, M_DMAMAP);
}

/*
 * Common function for loading a DMA map with a linear buffer.  May
 * be called by bus-specific DMA map load functions.
 */
int
_bus_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	paddr_t lastaddr;
	int seg, error;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	seg = 0;
	error = _bus_dmamap_load_buffer(t, map, buf, buflen, p, flags,
	    &lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
_bus_dmamap_load_mbuf(t, map, m0, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m0;
	int flags;
{
	paddr_t lastaddr;
	int seg, error, first;
	struct mbuf *m;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("_bus_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		error = _bus_dmamap_load_buffer(t, map, m->m_data, m->m_len,
		    NULL, flags, &lastaddr, &seg, first);
		first = 0;
	}
	if (error == 0) {
		map->dm_mapsize = m0->m_pkthdr.len;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
_bus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{
	paddr_t lastaddr;
	int seg, i, error, first;
	bus_size_t minlen, resid;
	struct proc *p = NULL;
	struct iovec *iov;
	caddr_t addr;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	resid = uio->uio_resid;
	iov = uio->uio_iov;

	if (uio->uio_segflg == UIO_USERSPACE) {
		p = uio->uio_procp;
#ifdef DIAGNOSTIC
		if (p == NULL)
			panic("_bus_dmamap_load_uio: USERSPACE but no proc");
#endif
	}

	first = 1;
	seg = 0;
	error = 0;
	for (i = 0; i < uio->uio_iovcnt && resid != 0 && error == 0; i++) {
		/*
		 * Now at the first iovec to load.  Load each iovec
		 * until we have exhausted the residual count.
		 */
		minlen = resid < iov[i].iov_len ? resid : iov[i].iov_len;
		addr = (caddr_t)iov[i].iov_base;

		error = _bus_dmamap_load_buffer(t, map, addr, minlen,
		    p, flags, &lastaddr, &seg, first);
		first = 0;

		resid -= minlen;
	}
	if (error == 0) {
		map->dm_mapsize = uio->uio_resid;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
_bus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{

	panic("_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * bus-specific DMA map unload functions.
 */
void
_bus_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{

	/*
	 * No resources to free; just mark the mappings as
	 * invalid.
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}

/*
 * Common function for DMA map synchronization.  May be called
 * by bus-specific DMA map synchronization functions.
 */
void
_bus_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{

	/* Nothing to do here. */
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{

	return (_bus_dmamem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, 0, trunc_page(avail_end)));
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_bus_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	vm_page_t m;
	bus_addr_t addr;
	struct pglist mlist;
	int curseg;

	/*
	 * Build a list of pages to free back to the VM system.
	 */
	TAILQ_INIT(&mlist);
	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += PAGE_SIZE) {
			m = PHYS_TO_VM_PAGE(addr);
			TAILQ_INSERT_TAIL(&mlist, m, pageq);
		}
	}

#if defined(UVM)
	uvm_pglistfree(&mlist);
#else
	vm_page_free_memory(&mlist);
#endif
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
_bus_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	vaddr_t va;
	bus_addr_t addr;
	int curseg;

	size = round_page(size);

#if defined(UVM)
	va = uvm_km_valloc(kernel_map, size);
#else
	va = kmem_alloc_pageable(kernel_map, size);
#endif

	if (va == 0)
		return (ENOMEM);

	*kvap = (caddr_t)va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += NBPG, va += NBPG, size -= NBPG) {
			if (size == 0)
				panic("_bus_dmamem_map: size botch");
#if defined(PMAP_NEW)
			pmap_kenter_pa(va, addr, VM_PROT_READ | VM_PROT_WRITE);
#else
			pmap_enter(pmap_kernel(), va, addr,
			    VM_PROT_READ | VM_PROT_WRITE, TRUE);
#endif
		}
	}

	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(t, kva, size)
	bus_dma_tag_t t;
	caddr_t kva;
	size_t size;
{

#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
		panic("_bus_dmamem_unmap");
#endif

	size = round_page(size);

#if defined(UVM)
	uvm_km_free(kernel_map, (vaddr_t)kva, size);
#else
	kmem_free(kernel_map, (vaddr_t)kva, size);
#endif
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
int
_bus_dmamem_mmap(t, segs, nsegs, off, prot, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs, off, prot, flags;
{
	int i;

	for (i = 0; i < nsegs; i++) {
#ifdef DIAGNOSTIC
		if (off & PGOFSET)
			panic("_bus_dmamem_mmap: offset unaligned");
		if (segs[i].ds_addr & PGOFSET)
			panic("_bus_dmamem_mmap: segment unaligned");
		if (segs[i].ds_len & PGOFSET)
			panic("_bus_dmamem_mmap: segment size not multiple"
			    " of page size");
#endif
		if (off >= segs[i].ds_len) {
			off -= segs[i].ds_len;
			continue;
		}

		return (i386_btop((caddr_t)segs[i].ds_addr + off));
	}

	/* Page not found. */
	return (-1);
}

/**********************************************************************
 * DMA utility functions
 **********************************************************************/

/*
 * Utility function to load a linear buffer.  lastaddrp holds state
 * between invocations (for multiple-buffer loads).  segp contains
 * the starting segment on entrace, and the ending segment on exit.
 * first indicates if this is the first invocation of this function.
 */
int
_bus_dmamap_load_buffer(t, map, buf, buflen, p, flags, lastaddrp, segp, first)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
	paddr_t *lastaddrp;
	int *segp;
	int first;
{
	bus_size_t sgsize;
	bus_addr_t curaddr, lastaddr, baddr, bmask;
	vaddr_t vaddr = (vaddr_t)buf;
	int seg;
	pmap_t pmap;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	lastaddr = *lastaddrp;
	bmask  = ~(map->_dm_boundary - 1);

	for (seg = *segp; buflen > 0 ; ) {
		/*
		 * Get the physical address for this segment.
		 */
		curaddr = pmap_extract(pmap, vaddr);

		/*
		 * If we're beyond the bounce threshold, notify
		 * the caller.
		 */
		if (map->_dm_bounce_thresh != 0 &&
		    curaddr >= map->_dm_bounce_thresh)
			return (EINVAL);

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = NBPG - ((u_long)vaddr & PGOFSET);
		if (buflen < sgsize)
			sgsize = buflen;

		/*
		 * Make sure we don't cross any boundaries.
		 */
		if (map->_dm_boundary > 0) {
			baddr = (curaddr + map->_dm_boundary) & bmask;
			if (sgsize > (baddr - curaddr))
				sgsize = (baddr - curaddr);
		}

		/*
		 * Insert chunk into a segment, coalescing with
		 * previous segment if possible.
		 */
		if (first) {
			map->dm_segs[seg].ds_addr = curaddr;
			map->dm_segs[seg].ds_len = sgsize;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->_dm_maxsegsz &&
			    (map->_dm_boundary == 0 ||
			     (map->dm_segs[seg].ds_addr & bmask) ==
			     (curaddr & bmask)))
				map->dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->_dm_segcnt)
					break;
				map->dm_segs[seg].ds_addr = curaddr;
				map->dm_segs[seg].ds_len = sgsize;
			}
		}

		lastaddr = curaddr + sgsize;
		vaddr += sgsize;
		buflen -= sgsize;
	}

	*segp = seg;
	*lastaddrp = lastaddr;

	/*
	 * Did we fit?
	 */
	if (buflen != 0)
		return (EFBIG);		/* XXX better return value here? */
	return (0);
}

/*
 * Allocate physical memory from the given physical address range.
 * Called by DMA-safe memory allocation methods.
 */
int
_bus_dmamem_alloc_range(t, size, alignment, boundary, segs, nsegs, rsegs,
    flags, low, high)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
	paddr_t low;
	paddr_t high;
{
	paddr_t curaddr, lastaddr;
	vm_page_t m;
	struct pglist mlist;
	int curseg, error;

	/* Always round the size. */
	size = round_page(size);

	/*
	 * Allocate pages from the VM system.
	 */
	TAILQ_INIT(&mlist);
#if defined(UVM)
	error = uvm_pglistalloc(size, low, high, alignment, boundary,
	    &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
#else
	error = vm_page_alloc_memory(size, low, high,
	    alignment, boundary, &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
#endif
	if (error)
		return (error);

	/*
	 * Compute the location, size, and number of segments actually
	 * returned by the VM code.
	 */
	m = mlist.tqh_first;
	curseg = 0;
	lastaddr = segs[curseg].ds_addr = VM_PAGE_TO_PHYS(m);
	segs[curseg].ds_len = PAGE_SIZE;
	m = m->pageq.tqe_next;

	for (; m != NULL; m = m->pageq.tqe_next) {
		curaddr = VM_PAGE_TO_PHYS(m);
#ifdef DIAGNOSTIC
		if (curaddr < low || curaddr >= high) {
			printf("vm_page_alloc_memory returned non-sensical"
			    " address 0x%lx\n", curaddr);
			panic("_bus_dmamem_alloc_range");
		}
#endif
		if (curaddr == (lastaddr + PAGE_SIZE))
			segs[curseg].ds_len += PAGE_SIZE;
		else {
			curseg++;
			segs[curseg].ds_addr = curaddr;
			segs[curseg].ds_len = PAGE_SIZE;
		}
		lastaddr = curaddr;
	}

	*rsegs = curseg + 1;

	return (0);
}
